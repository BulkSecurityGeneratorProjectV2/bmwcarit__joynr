/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#include "runtimes/libjoynr-runtime/websocket/LibJoynrWebSocketRuntime.h"

#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "libjoynr/websocket/WebSocketLibJoynrMessagingSkeleton.h"
#include "joynr/Util.h"
#include "joynr/Semaphore.h"
#include "libjoynr/websocket/WebSocketPpClient.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/SingleThreadedIOService.h"

namespace joynr
{

INIT_LOGGER(LibJoynrWebSocketRuntime);

LibJoynrWebSocketRuntime::LibJoynrWebSocketRuntime(std::unique_ptr<Settings> settings)
        : LibJoynrRuntime(std::move(settings)),
          wsSettings(*this->settings),
          websocket(new WebSocketPpClient(wsSettings, singleThreadIOService->getIOService()))
{
}

LibJoynrWebSocketRuntime::~LibJoynrWebSocketRuntime()
{
    // reset receive callback to remove last reference to MessageRouter in
    // WebSocketLibJoynrMessagingSkeleton
    websocket->registerReceiveCallback(nullptr);
    websocket->close();
    // synchronously stop the underlying boost::asio::io_service
    // this ensures all asynchronous operations are stopped now
    // which allows a safe shutdown
    singleThreadIOService->stop();
}

void LibJoynrWebSocketRuntime::connect(std::function<void()> runtimeCreatedCallback)
{
    std::string uuid = util::createUuid();
    // remove dashes
    uuid.erase(std::remove(uuid.begin(), uuid.end(), '-'), uuid.end());
    std::string libjoynrMessagingId = "libjoynr.messaging.participantid_" + uuid;
    auto libjoynrMessagingAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    libjoynrMessagingId);

    // send initialization message containing libjoynr messaging address
    std::string initializationMsg = joynr::serializer::serializeToJson(*libjoynrMessagingAddress);
    JOYNR_LOG_TRACE(logger,
                    "OUTGOING sending websocket intialization message\nmessage: {}\nto: {}",
                    initializationMsg,
                    libjoynrMessagingAddress->toString());

    // create connection to parent routing service
    auto ccMessagingAddress = std::make_shared<const joynr::system::RoutingTypes::WebSocketAddress>(
            wsSettings.createClusterControllerMessagingAddress());

    auto factory = std::make_shared<WebSocketMessagingStubFactory>();
    factory->addServer(*ccMessagingAddress, websocket->getSender());

    std::weak_ptr<WebSocketMessagingStubFactory> weakFactoryRef(factory);
    websocket->registerDisconnectCallback([weakFactoryRef, ccMessagingAddress]() {
        if (auto factory = weakFactoryRef.lock()) {
            factory->onMessagingStubClosed(*ccMessagingAddress);
        }
    });

    auto connectCallback = [
        this,
        initializationMsg,
        runtimeCreatedCallback = std::move(runtimeCreatedCallback),
        factory,
        libjoynrMessagingAddress,
        ccMessagingAddress
    ]()
    {
        auto onFailure = [this](const exceptions::JoynrRuntimeException& e) {
            // initialization message will be sent after reconnect
            JOYNR_LOG_ERROR(logger,
                            "Sending websocket initialization message failed. Error: {}",
                            e.getMessage());
        };
        websocket->sendTextMessage(initializationMsg, onFailure);

        init(factory, libjoynrMessagingAddress, ccMessagingAddress);

        runtimeCreatedCallback();
    };

    websocket->registerConnectCallback(connectCallback);
    websocket->connect(*ccMessagingAddress);
}

void LibJoynrWebSocketRuntime::startLibJoynrMessagingSkeleton(
        const std::shared_ptr<MessageRouter>& messageRouter)
{
    auto wsLibJoynrMessagingSkeleton =
            std::make_shared<WebSocketLibJoynrMessagingSkeleton>(messageRouter);
    websocket->registerReceiveCallback([wsLibJoynrMessagingSkeleton](const std::string& msg) {
        wsLibJoynrMessagingSkeleton->onTextMessageReceived(msg);
    });
}

void LibJoynrWebSocketRuntime::onWebSocketError(const std::string& errorMessage)
{
    JOYNR_LOG_ERROR(logger, "WebSocket error occurred: {}", errorMessage);
}

} // namespace joynr
