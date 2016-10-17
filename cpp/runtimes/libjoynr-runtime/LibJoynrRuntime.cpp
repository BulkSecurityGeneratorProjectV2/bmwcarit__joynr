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
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"

#include <cassert>
#include <memory>
#include <vector>

#include "joynr/Dispatcher.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/PublicationManager.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/MessageRouter.h"
#include "libjoynr/in-process/InProcessLibJoynrMessagingSkeleton.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/MessagingStubFactory.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "joynr/system/DiscoveryProxy.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/Util.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"

namespace joynr
{

LibJoynrRuntime::LibJoynrRuntime(std::unique_ptr<Settings> settings)
        : JoynrRuntime(*settings),
          subscriptionManager(nullptr),
          inProcessPublicationSender(nullptr),
          inProcessConnectorFactory(nullptr),
          joynrMessagingConnectorFactory(nullptr),
          joynrMessagingSendStub(nullptr),
          joynrMessageSender(nullptr),
          joynrDispatcher(nullptr),
          inProcessDispatcher(nullptr),
          settings(std::move(settings)),
          libjoynrSettings(new LibjoynrSettings(*this->settings)),
          dispatcherMessagingSkeleton(nullptr)
{
    libjoynrSettings->printSettings();
    singleThreadIOService->start();
}

LibJoynrRuntime::~LibJoynrRuntime()
{
    delete proxyFactory;
    delete inProcessDispatcher;
    delete joynrMessageSender;
    delete joynrDispatcher;
    delete libjoynrSettings;
    libjoynrSettings = nullptr;
}

void LibJoynrRuntime::init(
        std::shared_ptr<IMiddlewareMessagingStubFactory> middlewareMessagingStubFactory,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> libjoynrMessagingAddress,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> ccMessagingAddress)
{
    // create messaging stub factory
    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
    middlewareMessagingStubFactory->registerOnMessagingStubClosedCallback([messagingStubFactory](
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress) {
        messagingStubFactory->remove(destinationAddress);
    });
    messagingStubFactory->registerStubFactory(middlewareMessagingStubFactory);
    messagingStubFactory->registerStubFactory(std::make_shared<InProcessMessagingStubFactory>());

    // create message router
    messageRouter = std::make_shared<MessageRouter>(std::move(messagingStubFactory),
                                                    libjoynrMessagingAddress,
                                                    singleThreadIOService->getIOService());

    messageRouter->loadRoutingTable(libjoynrSettings->getMessageRouterPersistenceFilename());
    startLibJoynrMessagingSkeleton(messageRouter);

    joynrMessageSender = new JoynrMessageSender(messageRouter);
    joynrDispatcher = new Dispatcher(joynrMessageSender, singleThreadIOService->getIOService());
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    // create the inprocess skeleton for the dispatcher
    dispatcherMessagingSkeleton =
            std::make_shared<InProcessLibJoynrMessagingSkeleton>(joynrDispatcher);
    dispatcherAddress = std::make_shared<InProcessMessagingAddress>(dispatcherMessagingSkeleton);

    publicationManager = new PublicationManager(singleThreadIOService->getIOService());
    publicationManager->loadSavedAttributeSubscriptionRequestsMap(
            libjoynrSettings->getSubscriptionRequestPersistenceFilename());
    publicationManager->loadSavedBroadcastSubscriptionRequestsMap(
            libjoynrSettings->getBroadcastSubscriptionRequestPersistenceFilename());
    subscriptionManager =
            new SubscriptionManager(singleThreadIOService->getIOService(), messageRouter);
    inProcessDispatcher = new InProcessDispatcher(singleThreadIOService->getIOService());

    inProcessPublicationSender = new InProcessPublicationSender(subscriptionManager);
    inProcessConnectorFactory = new InProcessConnectorFactory(
            subscriptionManager,
            publicationManager,
            inProcessPublicationSender,
            dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher));
    joynrMessagingConnectorFactory =
            new JoynrMessagingConnectorFactory(joynrMessageSender, subscriptionManager);

    auto connectorFactory = std::make_unique<ConnectorFactory>(
            inProcessConnectorFactory, joynrMessagingConnectorFactory);
    proxyFactory = new ProxyFactory(libjoynrMessagingAddress, std::move(connectorFactory), nullptr);

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = libjoynrSettings->getParticipantIdsPersistenceFilename();
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    // initialize the dispatchers
    std::vector<IDispatcher*> dispatcherList;
    dispatcherList.push_back(inProcessDispatcher);
    dispatcherList.push_back(joynrDispatcher);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    discoveryProxy = std::make_unique<LocalDiscoveryAggregator>(systemServicesSettings);
    requestCallerDirectory = dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher);

    std::string systemServicesDomain = systemServicesSettings.getDomain();
    std::string routingProviderParticipantId =
            systemServicesSettings.getCcRoutingProviderParticipantId();

    DiscoveryQos routingProviderDiscoveryQos;
    routingProviderDiscoveryQos.setCacheMaxAgeMs(1000);
    routingProviderDiscoveryQos.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    routingProviderDiscoveryQos.addCustomParameter(
            "fixedParticipantId", routingProviderParticipantId);
    routingProviderDiscoveryQos.setDiscoveryTimeoutMs(50);

    std::unique_ptr<ProxyBuilder<joynr::system::RoutingProxy>> routingProxyBuilder(
            createProxyBuilder<joynr::system::RoutingProxy>(systemServicesDomain));
    auto routingProxy = routingProxyBuilder->setMessagingQos(MessagingQos(10000))
                                ->setCached(false)
                                ->setDiscoveryQos(routingProviderDiscoveryQos)
                                ->build();
    messageRouter->setParentRouter(std::unique_ptr<system::RoutingProxy>(routingProxy),
                                   ccMessagingAddress,
                                   routingProviderParticipantId);

    // setup discovery
    std::string discoveryProviderParticipantId =
            systemServicesSettings.getCcDiscoveryProviderParticipantId();
    DiscoveryQos discoveryProviderDiscoveryQos;
    discoveryProviderDiscoveryQos.setCacheMaxAgeMs(1000);
    discoveryProviderDiscoveryQos.setArbitrationStrategy(
            DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryProviderDiscoveryQos.addCustomParameter(
            "fixedParticipantId", discoveryProviderParticipantId);
    discoveryProviderDiscoveryQos.setDiscoveryTimeoutMs(1000);

    std::unique_ptr<ProxyBuilder<joynr::system::DiscoveryProxy>> discoveryProxyBuilder(
            createProxyBuilder<joynr::system::DiscoveryProxy>(systemServicesDomain));
    joynr::system::IDiscoverySync* proxy =
            discoveryProxyBuilder->setMessagingQos(MessagingQos(40000))
                    ->setCached(false)
                    ->setDiscoveryQos(discoveryProviderDiscoveryQos)
                    ->build();
    discoveryProxy->setDiscoveryProxy(std::unique_ptr<joynr::system::IDiscoverySync>(proxy));
    capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(
            dispatcherList,
            *discoveryProxy,
            participantIdStorage,
            dispatcherAddress,
            messageRouter,
            messagingSettings.getDiscoveryEntryExpiryIntervalMs());
}

void LibJoynrRuntime::unregisterProvider(const std::string& participantId)
{
    assert(capabilitiesRegistrar);
    capabilitiesRegistrar->remove(participantId);
}

} // namespace joynr
