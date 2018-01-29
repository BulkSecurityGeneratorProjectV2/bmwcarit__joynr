/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#include <cassert>

#include <websocketpp/common/connection_hdl.hpp>

#include "joynr/SingleThreadedIOService.h"
#include "joynr/Util.h"
#include "joynr/WebSocketMulticastAddressCalculator.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/IKeychain.h"
#include "libjoynr/websocket/WebSocketLibJoynrMessagingSkeleton.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketPpClientNonTLS.h"
#include "libjoynr/websocket/WebSocketPpClientTLS.h"

namespace joynr
{

LibJoynrWebSocketRuntime::LibJoynrWebSocketRuntime(std::unique_ptr<Settings> settings,
                                                   std::shared_ptr<IKeychain> keyChain)
        : LibJoynrRuntime(std::move(settings), std::move(keyChain)),
          wsSettings(*this->settings),
          websocket(nullptr),
          initializationMsg(),
          isShuttingDown(false)
{
    createWebsocketClient();
}

LibJoynrWebSocketRuntime::~LibJoynrWebSocketRuntime()
{
    assert(isShuttingDown);
}

void LibJoynrWebSocketRuntime::shutdown()
{
    assert(!isShuttingDown);
    isShuttingDown = true;
    assert(websocket);
    websocket->stop();

    // synchronously stop the underlying boost::asio::io_service
    // this ensures all asynchronous operations are stopped now
    // which allows a safe shutdown
    assert(singleThreadIOService);
    singleThreadIOService->stop();
    LibJoynrRuntime::shutdown();
}

void LibJoynrWebSocketRuntime::connect(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError)
{
    std::string uuid = util::createUuid();
    // remove dashes
    uuid.erase(std::remove(uuid.begin(), uuid.end(), '-'), uuid.end());
    std::string libjoynrMessagingId = "libjoynr.messaging.participantid_" + uuid;
    auto libjoynrMessagingAddress =
            std::make_shared<const joynr::system::RoutingTypes::WebSocketClientAddress>(
                    libjoynrMessagingId);

    // send initialization message containing libjoynr messaging address
    initializationMsg = joynr::serializer::serializeToJson(*libjoynrMessagingAddress);
    JOYNR_LOG_TRACE(logger(),
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
        thisWeakPtr = joynr::util::as_weak_ptr(
                std::dynamic_pointer_cast<LibJoynrWebSocketRuntime>(this->shared_from_this())),
        onSuccess = std::move(onSuccess),
        onError = std::move(onError),
        factory,
        libjoynrMessagingAddress,
        ccMessagingAddress
    ]() mutable
    {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            thisSharedPtr->sendInitializationMsg();

            std::unique_ptr<IMulticastAddressCalculator> addressCalculator =
                    std::make_unique<joynr::WebSocketMulticastAddressCalculator>(
                            ccMessagingAddress);
            thisSharedPtr->init(factory,
                                libjoynrMessagingAddress,
                                ccMessagingAddress,
                                std::move(addressCalculator),
                                std::move(onSuccess),
                                std::move(onError));
        }
    };

    auto reconnectCallback = [thisWeakPtr = joynr::util::as_weak_ptr(std::dynamic_pointer_cast<
                                      LibJoynrWebSocketRuntime>(this->shared_from_this()))]()
    {
        if (auto thisSharedPtr = thisWeakPtr.lock()) {
            thisSharedPtr->sendInitializationMsg();
        }
    };

    websocket->registerConnectCallback(connectCallback);
    websocket->registerReconnectCallback(reconnectCallback);
    websocket->connect(*ccMessagingAddress);
}

void LibJoynrWebSocketRuntime::sendInitializationMsg()
{
    auto onFailure = [](const exceptions::JoynrRuntimeException& e) {
        // initialization message will be sent after reconnect
        JOYNR_LOG_ERROR(LibJoynrWebSocketRuntime::logger(),
                        "Sending websocket initialization message failed. Error: {}",
                        e.getMessage());
    };
    smrf::ByteVector rawMessage(initializationMsg.begin(), initializationMsg.end());
    websocket->send(smrf::ByteArrayView(rawMessage), std::move(onFailure));
}

void LibJoynrWebSocketRuntime::createWebsocketClient()
{
    system::RoutingTypes::WebSocketAddress webSocketAddress =
            wsSettings.createClusterControllerMessagingAddress();

    if (webSocketAddress.getProtocol() == system::RoutingTypes::WebSocketProtocol::WSS) {
        if (keyChain == nullptr) {
            const std::string message(
                    "TLS websocket connection was configured for but no keychain was provided");
            JOYNR_LOG_FATAL(logger(), message);
            throw exceptions::JoynrRuntimeException(message);
        }

        JOYNR_LOG_INFO(logger(), "Using TLS connection");
        websocket = std::make_shared<WebSocketPpClientTLS>(
                wsSettings, singleThreadIOService->getIOService(), keyChain);
    } else if (webSocketAddress.getProtocol() == system::RoutingTypes::WebSocketProtocol::WS) {
        JOYNR_LOG_INFO(logger(), "Using non-TLS connection");
        websocket = std::make_shared<WebSocketPpClientNonTLS>(
                wsSettings, singleThreadIOService->getIOService());
    } else {
        throw exceptions::JoynrRuntimeException(
                "Unknown protocol used for settings property 'cluster-controller-messaging-url'");
    }
}

void LibJoynrWebSocketRuntime::startLibJoynrMessagingSkeleton(
        std::shared_ptr<IMessageRouter> messageRouter)
{
    auto wsLibJoynrMessagingSkeleton =
            std::make_shared<WebSocketLibJoynrMessagingSkeleton>(util::as_weak_ptr(messageRouter));
    using ConnectionHandle = websocketpp::connection_hdl;
    websocket->registerReceiveCallback(
            [wsLibJoynrMessagingSkeleton](ConnectionHandle&& hdl, smrf::ByteVector&& msg) {
                std::ignore = hdl;
                wsLibJoynrMessagingSkeleton->onMessageReceived(std::move(msg));
            });
}

} // namespace joynr
