/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

#include "ShortCircuitRuntime.h"

#include <chrono>

#include "joynr/Util.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/Dispatcher.h"
#include "joynr/InProcessMessagingAddress.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "libjoynr/in-process/InProcessLibJoynrMessagingSkeleton.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/Settings.h"
#include "joynr/MessagingSettings.h"

namespace joynr
{

ShortCircuitRuntime::ShortCircuitRuntime()
{
    std::string libjoynrMessagingId = "libjoynr.messaging.participantid_short-circuit-uuid";
    auto libjoynrMessagingAddress =
            std::make_shared<joynr::system::RoutingTypes::WebSocketClientAddress>(
                    libjoynrMessagingId);

    auto messagingStubFactory = std::make_unique<MessagingStubFactory>();

    messagingStubFactory->registerStubFactory(std::make_unique<InProcessMessagingStubFactory>());

    messageRouter = std::make_shared<MessageRouter>(
            std::move(messagingStubFactory), libjoynrMessagingAddress);

    joynrMessageSender = std::make_unique<JoynrMessageSender>(messageRouter);
    joynrDispatcher = new Dispatcher(joynrMessageSender.get());
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    dispatcherMessagingSkeleton =
            std::make_shared<InProcessLibJoynrMessagingSkeleton>(joynrDispatcher);
    dispatcherAddress = std::make_shared<InProcessMessagingAddress>(dispatcherMessagingSkeleton);

    publicationManager = new PublicationManager();
    subscriptionManager = new SubscriptionManager();
    inProcessDispatcher = new InProcessDispatcher();

    inProcessPublicationSender = std::make_unique<InProcessPublicationSender>(subscriptionManager);
    inProcessConnectorFactory = new InProcessConnectorFactory(
            subscriptionManager,
            publicationManager,
            inProcessPublicationSender.get(),
            dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher));
    joynrMessagingConnectorFactory =
            new JoynrMessagingConnectorFactory(joynrMessageSender.get(), subscriptionManager);

    auto connectorFactory = std::make_unique<ConnectorFactory>(
            inProcessConnectorFactory, joynrMessagingConnectorFactory);
    proxyFactory = std::make_unique<ProxyFactory>(
            libjoynrMessagingAddress, std::move(connectorFactory), nullptr);

    std::string persistenceFilename = "dummy.txt";
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    std::vector<IDispatcher*> dispatcherList;
    dispatcherList.push_back(inProcessDispatcher);
    dispatcherList.push_back(joynrDispatcher);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    discoveryProxy = std::make_unique<DummyDiscovery>();
    capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(dispatcherList,
                                                                    *discoveryProxy,
                                                                    participantIdStorage,
                                                                    dispatcherAddress,
                                                                    messageRouter);

    maximumTtlMs = std::chrono::milliseconds(std::chrono::hours(24) * 30).count();
}

} // namespace joynr
