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
#ifndef MESSAGEROUTER_H
#define MESSAGEROUTER_H
#include <chrono>
#include <unordered_set>
#include <mutex>
#include <string>
#include <memory>
#include <boost/asio/steady_timer.hpp>

#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include "joynr/ObjectWithDecayTime.h"
#include "joynr/JoynrMessage.h"
#include "joynr/IMessaging.h"
#include "joynr/MessagingSettings.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/system/RoutingAbstractProvider.h"
#include "joynr/RoutingTable.h"
#include "joynr/MessageQueue.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/Runnable.h"
#include "joynr/Semaphore.h"
#include "joynr/Logger.h"

namespace boost
{
namespace asio
{
class io_service;
} // namespace asio
namespace system
{
class error_code;
} // namespace system
} // namespace boost

namespace joynr
{

class IMessagingStubFactory;
class JoynrMessagingEndpointAddress;
class IAccessController;
class IPlatformSecurityManager;

namespace system
{
class Address;
} // namespace system

/**
  * Class MessageRouter receives incoming JoynrMessages on the ClusterController
  * and forwards them either to a remote ClusterController or to a LibJoynr on the machine.
  *
  *  1 extracts the destination participant ID and looks up the EndpointAddress in the
  *MessagingEndpointDirectory
  *  2 creates a <Middleware>MessagingStub by calling MessagingStubFactory.create(EndpointAddress
  *addr)
  *  3 forwards the message using the <Middleware>MessagingStub.send(JoynrMessage msg)
  *
  *  In sending, a ThreadPool of default size 6 is used with a 500ms default retry interval.
  */

class JOYNR_EXPORT MessageRouter : public joynr::system::RoutingAbstractProvider
{
public:
    // TODO: change shared_ptr to unique_ptr once JoynrClusterControllerRuntime is refactored
    MessageRouter(std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
                  std::unique_ptr<IPlatformSecurityManager> securityManager,
                  boost::asio::io_service& ioService,
                  int maxThreads = 6,
                  std::unique_ptr<MessageQueue> messageQueue = std::make_unique<MessageQueue>());

    MessageRouter(std::shared_ptr<IMessagingStubFactory> messagingStubFactory,
                  std::shared_ptr<const joynr::system::RoutingTypes::Address> incomingAddress,
                  boost::asio::io_service& ioService,
                  int maxThreads = 6,
                  std::unique_ptr<MessageQueue> messageQueue = std::make_unique<MessageQueue>());

    ~MessageRouter() override;

    /**
     * @brief Forwards the message towards its destination (determined by inspecting the message
     * header). NOTE: the init method must be called before the first message is routed.
     *
     * @param message the message to route.
     * @param qos the QoS used to route the message.
     */
    virtual void route(const JoynrMessage& message, std::uint32_t tryCount = 0);

    void addNextHop(const std::string& participantId,
                    const joynr::system::RoutingTypes::ChannelAddress& channelAddress,
                    std::function<void()> onSuccess,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    void addNextHop(const std::string& participantId,
                    const joynr::system::RoutingTypes::MqttAddress& mqttAddress,
                    std::function<void()> onSuccess,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    void addNextHop(const std::string& participantId,
                    const joynr::system::RoutingTypes::CommonApiDbusAddress& commonApiDbusAddress,
                    std::function<void()> onSuccess,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    void addNextHop(const std::string& participantId,
                    const joynr::system::RoutingTypes::BrowserAddress& browserAddress,
                    std::function<void()> onSuccess,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    void addNextHop(const std::string& participantId,
                    const joynr::system::RoutingTypes::WebSocketAddress& webSocketAddress,
                    std::function<void()> onSuccess,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    void addNextHop(
            const std::string& participantId,
            const joynr::system::RoutingTypes::WebSocketClientAddress& webSocketClientAddress,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
    void removeNextHop(const std::string& participantId,
                       std::function<void()> onSuccess = nullptr,
                       std::function<void(const joynr::exceptions::ProviderRuntimeException&)>
                               onError = nullptr) override;
    void resolveNextHop(const std::string& participantId,
                        std::function<void(const bool& resolved)> onSuccess,
                        std::function<void(const joynr::exceptions::ProviderRuntimeException&)>
                                onError) override;

    void addProvisionedNextHop(std::string participantId,
                               std::shared_ptr<const joynr::system::RoutingTypes::Address> address);

    void setAccessController(std::shared_ptr<IAccessController> accessController);

    void setParentRouter(std::unique_ptr<joynr::system::RoutingProxy> parentRouter,
                         std::shared_ptr<const joynr::system::RoutingTypes::Address> parentAddress,
                         std::string parentParticipantId);

    virtual void addNextHop(
            const std::string& participantId,
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& inprocessAddress,
            std::function<void()> onSuccess = nullptr);

    void saveRoutingTable();
    void loadRoutingTable(std::string fileName);

    friend class MessageRunnable;
    friend class ConsumerPermissionCallback;

private:
    DISALLOW_COPY_AND_ASSIGN(MessageRouter);
    std::shared_ptr<IMessagingStubFactory> messagingStubFactory;
    RoutingTable routingTable;
    ReadWriteLock routingTableLock;
    ThreadPoolDelayedScheduler messageScheduler;
    std::unique_ptr<joynr::system::RoutingProxy> parentRouter;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> parentAddress;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> incomingAddress;
    ADD_LOGGER(MessageRouter);

    std::unique_ptr<MessageQueue> messageQueue;
    std::unordered_set<std::string> runningParentResolves;
    std::shared_ptr<IAccessController> accessController;
    std::unique_ptr<IPlatformSecurityManager> securityManager;
    mutable std::mutex parentResolveMutex;
    std::string routingTableFileName;

    boost::asio::steady_timer messageQueueCleanerTimer;
    const std::chrono::milliseconds messageQueueCleanerTimerPeriodMs;

    void addNextHopToParent(std::string participantId,
                            std::function<void(void)> callbackFct = nullptr,
                            std::function<void(const joynr::exceptions::ProviderRuntimeException&)>
                                    onError = nullptr);

    void sendMessage(const JoynrMessage& message,
                     std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
                     std::uint32_t tryCount = 0);

    void sendMessages(const std::string& destinationPartId,
                      std::shared_ptr<const joynr::system::RoutingTypes::Address> address);

    bool isChildMessageRouter();

    void addToRoutingTable(std::string participantId,
                           std::shared_ptr<const joynr::system::RoutingTypes::Address> address);

    void removeRunningParentResolvers(const std::string& destinationPartId);

    void scheduleMessage(const JoynrMessage& message,
                         std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
                         std::uint32_t tryCount,
                         std::chrono::milliseconds delay = std::chrono::milliseconds(0));

    void activateMessageCleanerTimer();
    void onMessageCleanerTimerExpired(const boost::system::error_code& errorCode);
};

/**
 * Class to send message
 */
class MessageRunnable : public Runnable, public ObjectWithDecayTime
{
public:
    MessageRunnable(const JoynrMessage& message,
                    std::shared_ptr<IMessaging> messagingStub,
                    std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress,
                    MessageRouter& messageRouter,
                    std::uint32_t tryCount);
    void shutdown() override;
    void run() override;

private:
    JoynrMessage message;
    std::shared_ptr<IMessaging> messagingStub;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> destAddress;
    MessageRouter& messageRouter;
    std::uint32_t tryCount;
    ADD_LOGGER(MessageRunnable);
};

} // namespace joynr
#endif // MESSAGEROUTER_H
