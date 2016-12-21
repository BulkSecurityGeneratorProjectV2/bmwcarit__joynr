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
#include "JoynrClusterControllerRuntime.h"

#include <cassert>
#include <cstdint>
#include <chrono>
#include <functional>

#include <boost/algorithm/string/predicate.hpp>
#include <mosquittopp.h>
#include "websocket/WebSocketCcMessagingSkeleton.h"

#include "cluster-controller/capabilities-client/CapabilitiesClient.h"
#include "cluster-controller/http-communication-manager/HttpMessagingSkeleton.h"
#include "cluster-controller/http-communication-manager/HttpReceiver.h"
#include "cluster-controller/http-communication-manager/HttpSender.h"
#include "cluster-controller/messaging/joynr-messaging/HttpMessagingStubFactory.h"
#include "cluster-controller/messaging/joynr-messaging/MqttMessagingStubFactory.h"
#include "cluster-controller/messaging/MessagingPropertiesPersistence.h"
#include "cluster-controller/mqtt/MqttMessagingSkeleton.h"
#include "cluster-controller/mqtt/MqttReceiver.h"
#include "cluster-controller/mqtt/MqttSender.h"

#include "joynr/BrokerUrl.h"
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/ConnectorFactory.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/Dispatcher.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/HttpMulticastAddressCalculator.h"
#include "joynr/IDispatcher.h"
#include "joynr/IMessageReceiver.h"
#include "joynr/IMessageSender.h"
#include "joynr/IMulticastAddressCalculator.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/InProcessAddress.h"
#include "joynr/InProcessConnectorFactory.h"
#include "joynr/InProcessDispatcher.h"
#include "joynr/InProcessMessagingAddress.h"
#include "joynr/InProcessPublicationSender.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/MqttMulticastAddressCalculator.h"
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "joynr/LocalDiscoveryAggregator.h"
#include "joynr/MessageRouter.h"
#include "joynr/MessagingQos.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/MulticastMessagingSkeletonDirectory.h"
#include "joynr/ParticipantIdStorage.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/ProxyFactory.h"
#include "joynr/PublicationManager.h"
#include "joynr/Settings.h"
#include "joynr/SubscriptionManager.h"
#include "joynr/system/DiscoveryInProcessConnector.h"
#include "joynr/system/DiscoveryProvider.h"
#include "joynr/system/DiscoveryRequestCaller.h"
#include "joynr/system/RoutingProvider.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/MqttProtocol.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/SystemServicesSettings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/serializer/Serializer.h"

#include "libjoynr/in-process/InProcessLibJoynrMessagingSkeleton.h"
#include "libjoynr/in-process/InProcessMessagingStubFactory.h"
#include "libjoynr/joynr-messaging/DummyPlatformSecurityManager.h"
#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
#include "libjoynr/dbus/DbusMessagingStubFactory.h"
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

namespace joynr
{

INIT_LOGGER(JoynrClusterControllerRuntime);

JoynrClusterControllerRuntime::JoynrClusterControllerRuntime(
        std::unique_ptr<Settings> settings,
        std::shared_ptr<IMessageReceiver> httpMessageReceiver,
        std::shared_ptr<IMessageSender> httpMessageSender,
        std::shared_ptr<IMessageReceiver> mqttMessageReceiver,
        std::shared_ptr<IMessageSender> mqttMessageSender)
        : JoynrRuntime(*settings),
          joynrDispatcher(nullptr),
          inProcessDispatcher(nullptr),
          ccDispatcher(nullptr),
          subscriptionManager(nullptr),
          joynrMessagingSendSkeleton(nullptr),
          joynrMessageSender(nullptr),
          localCapabilitiesDirectory(nullptr),
          cache(),
          libJoynrMessagingSkeleton(nullptr),
          httpMessagingSkeleton(nullptr),
          mqttMessagingSkeleton(nullptr),
          httpMessageReceiver(httpMessageReceiver),
          httpMessageSender(httpMessageSender),
          mqttMessageReceiver(mqttMessageReceiver),
          mqttMessageSender(mqttMessageSender),
          dispatcherList(),
          inProcessConnectorFactory(nullptr),
          inProcessPublicationSender(nullptr),
          joynrMessagingConnectorFactory(nullptr),
          connectorFactory(nullptr),
          settings(std::move(settings)),
          libjoynrSettings(*(this->settings)),
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
          dbusSettings(nullptr),
          ccDbusMessageRouterAdapter(nullptr),
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
          wsSettings(*(this->settings)),
          wsCcMessagingSkeleton(nullptr),
          httpMessagingIsRunning(false),
          mqttMessagingIsRunning(false),
          doMqttMessaging(false),
          doHttpMessaging(false),
          mqttSettings(),
          multicastMessagingSkeletonDirectory(
                  std::make_shared<MulticastMessagingSkeletonDirectory>())
{
    initializeAllDependencies();
}

void JoynrClusterControllerRuntime::initializeAllDependencies()
{
    /**
      * libjoynr side skeleton & dispatcher
      * This needs some preparation of libjoynr and clustercontroller parts.
      */
    messagingSettings.printSettings();
    libjoynrSettings.printSettings();
    wsSettings.printSettings();

    const BrokerUrl brokerUrl = messagingSettings.getBrokerUrl();
    assert(brokerUrl.getBrokerChannelsBaseUrl().isValid());
    const BrokerUrl bounceProxyUrl = messagingSettings.getBounceProxyUrl();
    assert(bounceProxyUrl.getBrokerChannelsBaseUrl().isValid());

    // If the BrokerUrl is a mqtt url, MQTT is used instead of HTTP
    const Url url = brokerUrl.getBrokerChannelsBaseUrl();
    std::string brokerProtocol = url.getProtocol();
    std::string bounceproxyProtocol = bounceProxyUrl.getBrokerChannelsBaseUrl().getProtocol();

    std::transform(brokerProtocol.begin(), brokerProtocol.end(), brokerProtocol.begin(), ::toupper);
    std::transform(bounceproxyProtocol.begin(),
                   bounceproxyProtocol.end(),
                   bounceproxyProtocol.begin(),
                   ::toupper);

    std::unique_ptr<IMulticastAddressCalculator> addressCalculator;

    if (brokerProtocol == joynr::system::RoutingTypes::MqttProtocol::getLiteral(
                                  joynr::system::RoutingTypes::MqttProtocol::Enum::MQTT)) {
        JOYNR_LOG_INFO(logger, "MQTT-Messaging");
        auto globalAddress = std::make_shared<const joynr::system::RoutingTypes::MqttAddress>(
                brokerUrl.toString(), "");
        addressCalculator = std::make_unique<joynr::MqttMulticastAddressCalculator>(globalAddress);
        doMqttMessaging = true;
    } else {
        JOYNR_LOG_INFO(logger, "HTTP-Messaging");
        auto globalAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
                brokerUrl.toString(), "");
        addressCalculator = std::make_unique<joynr::HttpMulticastAddressCalculator>(globalAddress);
        doHttpMessaging = true;
    }

    if (!doHttpMessaging && boost::starts_with(bounceproxyProtocol, "HTTP")) {
        JOYNR_LOG_INFO(logger, "HTTP-Messaging");
        doHttpMessaging = true;
    }

    std::string capabilitiesDirectoryChannelId =
            messagingSettings.getCapabilitiesDirectoryChannelId();
    std::string capabilitiesDirectoryParticipantId =
            messagingSettings.getCapabilitiesDirectoryParticipantId();

    // Initialise security manager
    std::unique_ptr<IPlatformSecurityManager> securityManager =
            std::make_unique<DummyPlatformSecurityManager>();

    // CAREFUL: the factory creates an old style dispatcher, not the new one!
    inProcessDispatcher = new InProcessDispatcher(singleThreadIOService->getIOService());
    /* CC */
    // create the messaging stub factory
    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    messagingStubFactory->registerStubFactory(std::make_shared<DbusMessagingStubFactory>());
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
    messagingStubFactory->registerStubFactory(std::make_shared<InProcessMessagingStubFactory>());

    // init message router
    messageRouter = std::make_shared<MessageRouter>(messagingStubFactory,
                                                    multicastMessagingSkeletonDirectory,
                                                    std::move(securityManager),
                                                    singleThreadIOService->getIOService(),
                                                    std::move(addressCalculator));
    messageRouter->loadRoutingTable(libjoynrSettings.getMessageRouterPersistenceFilename());

    // provision global capabilities directory
    if (boost::starts_with(capabilitiesDirectoryChannelId, "{")) {
        try {
            using system::RoutingTypes::MqttAddress;
            auto globalCapabilitiesDirectoryAddress = std::make_shared<MqttAddress>();
            joynr::serializer::deserializeFromJson(
                    *globalCapabilitiesDirectoryAddress, capabilitiesDirectoryChannelId);
            messageRouter->addProvisionedNextHop(
                    capabilitiesDirectoryParticipantId, globalCapabilitiesDirectoryAddress);
        } catch (const std::invalid_argument& e) {
            JOYNR_LOG_FATAL(logger,
                            "could not deserialize MqttAddress from {} - error: {}",
                            capabilitiesDirectoryChannelId,
                            e.what());
        }
    } else {
        auto globalCapabilitiesDirectoryAddress =
                std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>(
                        messagingSettings.getCapabilitiesDirectoryUrl() +
                                capabilitiesDirectoryChannelId + "/",
                        capabilitiesDirectoryChannelId);
        messageRouter->addProvisionedNextHop(
                capabilitiesDirectoryParticipantId, globalCapabilitiesDirectoryAddress);
    }

    // setup CC WebSocket interface
    auto wsMessagingStubFactory = std::make_shared<WebSocketMessagingStubFactory>();
    wsMessagingStubFactory->registerOnMessagingStubClosedCallback([messagingStubFactory](
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress) {
        messagingStubFactory->remove(destinationAddress);
    });
    system::RoutingTypes::WebSocketAddress wsAddress =
            wsSettings.createClusterControllerMessagingAddress();
    wsCcMessagingSkeleton =
            std::make_shared<WebSocketCcMessagingSkeleton>(singleThreadIOService->getIOService(),
                                                           messageRouter,
                                                           wsMessagingStubFactory,
                                                           wsAddress);
    messagingStubFactory->registerStubFactory(wsMessagingStubFactory);

    /* LibJoynr */
    assert(messageRouter);
    joynrMessageSender = new JoynrMessageSender(messageRouter, messagingSettings.getTtlUpliftMs());
    joynrDispatcher = new Dispatcher(joynrMessageSender, singleThreadIOService->getIOService());
    joynrMessageSender->registerDispatcher(joynrDispatcher);

    /* CC */
    // TODO: libjoynrmessagingskeleton now uses the Dispatcher, should it use the
    // InprocessDispatcher?
    libJoynrMessagingSkeleton =
            std::make_shared<InProcessLibJoynrMessagingSkeleton>(joynrDispatcher);
    // EndpointAddress to messagingStub is transmitted when a provider is registered
    // messagingStubFactory->registerInProcessMessagingSkeleton(libJoynrMessagingSkeleton);

    std::string httpSerializedGlobalClusterControllerAddress;
    std::string mqttSerializedGlobalClusterControllerAddress;

    MessagingPropertiesPersistence persist(
            messagingSettings.getMessagingPropertiesPersistenceFilename());
    std::string clusterControllerId = persist.getChannelId();
    std::string receiverId = persist.getReceiverId();
    /**
      * ClusterController side HTTP
      *
      */

    if (doHttpMessaging) {

        if (!httpMessageReceiver) {
            JOYNR_LOG_INFO(logger,
                           "The http message receiver supplied is NULL, creating the default "
                           "http MessageReceiver");

            httpMessageReceiver = std::make_shared<HttpReceiver>(
                    messagingSettings, clusterControllerId, receiverId);

            assert(httpMessageReceiver != nullptr);

            httpMessagingSkeleton = std::make_shared<HttpMessagingSkeleton>(*messageRouter);
            httpMessageReceiver->registerReceiveCallback([&](const std::string& msg) {
                httpMessagingSkeleton->onTextMessageReceived(msg);
            });
        }

        httpSerializedGlobalClusterControllerAddress =
                httpMessageReceiver->getGlobalClusterControllerAddress();

        // create http message sender
        if (!httpMessageSender) {
            JOYNR_LOG_INFO(logger,
                           "The http message sender supplied is NULL, creating the default "
                           "http MessageSender");

            httpMessageSender = std::make_shared<HttpSender>(
                    messagingSettings.getBounceProxyUrl(),
                    std::chrono::milliseconds(messagingSettings.getSendMsgMaxTtl()),
                    std::chrono::milliseconds(messagingSettings.getSendMsgRetryInterval()));
        }

        messagingStubFactory->registerStubFactory(std::make_shared<HttpMessagingStubFactory>(
                httpMessageSender, httpSerializedGlobalClusterControllerAddress));
    }

    /**
      * ClusterController side MQTT
      *
      */

    if (doMqttMessaging) {

        if (!mqttMessageReceiver && !mqttMessageSender) {
            mosqpp::lib_init();
        }

        if (!mqttMessageReceiver) {
            JOYNR_LOG_INFO(logger,
                           "The mqtt message receiver supplied is NULL, creating the default "
                           "mqtt MessageReceiver");

            mqttMessageReceiver = std::make_shared<MqttReceiver>(
                    messagingSettings, clusterControllerId, receiverId);

            assert(mqttMessageReceiver != nullptr);
        }

        if (!mqttMessagingIsRunning) {
            mqttMessagingSkeleton = std::make_shared<MqttMessagingSkeleton>(
                    *messageRouter,
                    std::static_pointer_cast<MqttReceiver>(mqttMessageReceiver),
                    messagingSettings.getTtlUpliftMs());
            mqttMessageReceiver->registerReceiveCallback([&](const std::string& msg) {
                mqttMessagingSkeleton->onTextMessageReceived(msg);
            });
            multicastMessagingSkeletonDirectory
                    ->registerSkeleton<system::RoutingTypes::MqttAddress>(mqttMessagingSkeleton);
        }

        mqttSerializedGlobalClusterControllerAddress =
                mqttMessageReceiver->getGlobalClusterControllerAddress();

        // create message sender
        if (!mqttMessageSender) {
            JOYNR_LOG_INFO(logger,
                           "The mqtt message sender supplied is NULL, creating the default "
                           "mqtt MessageSender");

            mqttMessageSender = std::make_shared<MqttSender>(messagingSettings);

            mqttMessageSender->registerReceiver(mqttMessageReceiver);
        }

        messagingStubFactory->registerStubFactory(std::make_shared<MqttMessagingStubFactory>(
                mqttMessageSender, mqttSerializedGlobalClusterControllerAddress));
    }

    const std::string channelGlobalCapabilityDir =
            doMqttMessaging ? mqttSerializedGlobalClusterControllerAddress
                            : httpSerializedGlobalClusterControllerAddress;

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    dbusSettings = new DbusSettings(*settings);
    dbusSettings->printSettings();
    // register dbus skeletons for capabilities and messaging interfaces
    std::string ccMessagingAddress(dbusSettings->createClusterControllerMessagingAddressString());
    ccDbusMessageRouterAdapter = new DBusMessageRouterAdapter(*messageRouter, ccMessagingAddress);
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

    /**
      * libJoynr side
      *
      */
    publicationManager = new PublicationManager(singleThreadIOService->getIOService(),
                                                joynrMessageSender,
                                                messagingSettings.getTtlUpliftMs());
    publicationManager->loadSavedAttributeSubscriptionRequestsMap(
            libjoynrSettings.getSubscriptionRequestPersistenceFilename());
    publicationManager->loadSavedBroadcastSubscriptionRequestsMap(
            libjoynrSettings.getBroadcastSubscriptionRequestPersistenceFilename());

    subscriptionManager =
            new SubscriptionManager(singleThreadIOService->getIOService(), messageRouter);
    inProcessPublicationSender = new InProcessPublicationSender(subscriptionManager);
    auto libjoynrMessagingAddress =
            std::make_shared<InProcessMessagingAddress>(libJoynrMessagingSkeleton);
    // subscriptionManager = new SubscriptionManager(...)
    inProcessConnectorFactory = new InProcessConnectorFactory(
            subscriptionManager,
            publicationManager,
            inProcessPublicationSender,
            dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher));
    joynrMessagingConnectorFactory =
            new JoynrMessagingConnectorFactory(joynrMessageSender, subscriptionManager);

    auto connectorFactory = std::make_unique<ConnectorFactory>(
            inProcessConnectorFactory, joynrMessagingConnectorFactory);
    proxyFactory = new ProxyFactory(libjoynrMessagingAddress, std::move(connectorFactory), &cache);

    dispatcherList.push_back(joynrDispatcher);
    dispatcherList.push_back(inProcessDispatcher);

    // Set up the persistence file for storing provider participant ids
    std::string persistenceFilename = libjoynrSettings.getParticipantIdsPersistenceFilename();
    participantIdStorage = std::make_shared<ParticipantIdStorage>(persistenceFilename);

    dispatcherAddress = libjoynrMessagingAddress;

    const bool provisionClusterControllerDiscoveryEntries = true;
    discoveryProxy = std::make_unique<LocalDiscoveryAggregator>(
            systemServicesSettings, messagingSettings, provisionClusterControllerDiscoveryEntries);
    requestCallerDirectory = dynamic_cast<IRequestCallerDirectory*>(inProcessDispatcher);

    std::shared_ptr<ICapabilitiesClient> capabilitiesClient =
            std::make_shared<CapabilitiesClient>();
    localCapabilitiesDirectory =
            std::make_shared<LocalCapabilitiesDirectory>(messagingSettings,
                                                         capabilitiesClient,
                                                         channelGlobalCapabilityDir,
                                                         *messageRouter,
                                                         libjoynrSettings,
                                                         singleThreadIOService->getIOService(),
                                                         clusterControllerId);
    localCapabilitiesDirectory->loadPersistedFile();
    // importPersistedLocalCapabilitiesDirectory();

    std::string discoveryProviderParticipantId(
            systemServicesSettings.getCcDiscoveryProviderParticipantId());
    auto discoveryRequestCaller =
            std::make_shared<joynr::system::DiscoveryRequestCaller>(localCapabilitiesDirectory);
    auto discoveryProviderAddress = std::make_shared<InProcessAddress>(discoveryRequestCaller);

    {
        using joynr::system::DiscoveryInProcessConnector;
        auto discoveryInProcessConnector = std::make_unique<DiscoveryInProcessConnector>(
                subscriptionManager,
                publicationManager,
                inProcessPublicationSender,
                std::make_shared<DummyPlatformSecurityManager>(),
                std::string(), // can be ignored
                discoveryProviderParticipantId,
                discoveryProviderAddress);
        discoveryProxy->setDiscoveryProxy(std::move(discoveryInProcessConnector));
    }
    capabilitiesRegistrar = std::make_unique<CapabilitiesRegistrar>(
            dispatcherList,
            *discoveryProxy,
            participantIdStorage,
            dispatcherAddress,
            messageRouter,
            messagingSettings.getDiscoveryEntryExpiryIntervalMs(),
            *publicationManager);

    joynrDispatcher->registerPublicationManager(publicationManager);
    joynrDispatcher->registerSubscriptionManager(subscriptionManager);

    // ******************************************************************************************
    // WARNING: Latent dependency in place!
    //
    // ProxyBuilder performs a discovery this is why discoveryProxy->setDiscoveryProxy must be
    // called before any createProxyBuilder().
    //
    // ******************************************************************************************
    DiscoveryQos discoveryQos(10000);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.addCustomParameter(
            "fixedParticipantId", messagingSettings.getCapabilitiesDirectoryParticipantId());

    std::unique_ptr<ProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>>
            capabilitiesProxyBuilder(
                    createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                            messagingSettings.getDiscoveryDirectoriesDomain()));
    capabilitiesProxyBuilder->setDiscoveryQos(discoveryQos);

    capabilitiesClient->setProxyBuilder(std::move(capabilitiesProxyBuilder));
}

void JoynrClusterControllerRuntime::registerRoutingProvider()
{
    std::string domain(systemServicesSettings.getDomain());
    std::shared_ptr<joynr::system::RoutingProvider> routingProvider(messageRouter);
    std::string interfaceName(routingProvider->getInterfaceName());
    std::string participantId(systemServicesSettings.getCcRoutingProviderParticipantId());

    // provision the participant ID for the routing provider
    participantIdStorage->setProviderParticipantId(domain, interfaceName, participantId);

    joynr::types::ProviderQos routingProviderQos;
    routingProviderQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    routingProviderQos.setPriority(1);
    routingProviderQos.setScope(joynr::types::ProviderScope::LOCAL);
    routingProviderQos.setSupportsOnChangeSubscriptions(false);
    registerProvider<joynr::system::RoutingProvider>(domain, routingProvider, routingProviderQos);
}

void JoynrClusterControllerRuntime::registerDiscoveryProvider()
{
    std::string domain(systemServicesSettings.getDomain());
    std::shared_ptr<joynr::system::DiscoveryProvider> discoveryProvider(localCapabilitiesDirectory);
    std::string interfaceName(discoveryProvider->getInterfaceName());
    std::string participantId(systemServicesSettings.getCcDiscoveryProviderParticipantId());

    // provision the participant ID for the discovery provider
    participantIdStorage->setProviderParticipantId(domain, interfaceName, participantId);

    joynr::types::ProviderQos discoveryProviderQos;
    discoveryProviderQos.setCustomParameters(std::vector<joynr::types::CustomParameter>());
    discoveryProviderQos.setPriority(1);
    discoveryProviderQos.setScope(joynr::types::ProviderScope::LOCAL);
    discoveryProviderQos.setSupportsOnChangeSubscriptions(false);
    registerProvider<joynr::system::DiscoveryProvider>(
            domain, discoveryProvider, discoveryProviderQos);
}

JoynrClusterControllerRuntime::~JoynrClusterControllerRuntime()
{
    JOYNR_LOG_TRACE(logger, "entering ~JoynrClusterControllerRuntime");

    // synchronously stop the underlying boost::asio::io_service
    // this ensures all asynchronous operations are stopped now
    // which allows a safe shutdown
    singleThreadIOService->stop();
    stopMessaging();

    multicastMessagingSkeletonDirectory->unregisterSkeleton<system::RoutingTypes::MqttAddress>();

    if (joynrDispatcher != nullptr) {
        JOYNR_LOG_TRACE(logger, "joynrDispatcher");
        // joynrDispatcher->stopMessaging();
        delete joynrDispatcher;
    }

    delete inProcessDispatcher;
    inProcessDispatcher = nullptr;

    delete inProcessPublicationSender;
    inProcessPublicationSender = nullptr;
    delete joynrMessageSender;
    delete proxyFactory;

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    delete ccDbusMessageRouterAdapter;
    delete dbusSettings;
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

    JOYNR_LOG_TRACE(logger, "leaving ~JoynrClusterControllerRuntime");
}

void JoynrClusterControllerRuntime::startMessaging()
{
    //    assert(joynrDispatcher!=NULL);
    //    joynrDispatcher->startMessaging();
    //    joynrDispatcher->waitForMessaging();
    if (doHttpMessaging) {
        assert(httpMessageReceiver != nullptr);
        if (!httpMessagingIsRunning) {
            httpMessageReceiver->startReceiveQueue();
            httpMessagingIsRunning = true;
        }
    }
    if (doMqttMessaging) {
        assert(mqttMessageReceiver != nullptr);
        if (!mqttMessagingIsRunning) {
            mqttMessageReceiver->startReceiveQueue();
            mqttMessagingIsRunning = true;
        }
    }
}

void JoynrClusterControllerRuntime::stopMessaging()
{
    // joynrDispatcher->stopMessaging();
    if (doHttpMessaging) {
        if (httpMessagingIsRunning) {
            httpMessageReceiver->stopReceiveQueue();
            httpMessagingIsRunning = false;
        }
    }
    if (doMqttMessaging) {
        if (mqttMessagingIsRunning) {
            mqttMessageReceiver->stopReceiveQueue();
            mqttMessagingIsRunning = false;
        }
    }
}

void JoynrClusterControllerRuntime::runForever()
{
    singleThreadIOService->join();
}

JoynrClusterControllerRuntime* JoynrClusterControllerRuntime::create(
        std::unique_ptr<Settings> settings,
        const std::string& discoveryEntriesFile)
{
    JoynrClusterControllerRuntime* runtime = new JoynrClusterControllerRuntime(std::move(settings));

    assert(runtime->localCapabilitiesDirectory);
    runtime->localCapabilitiesDirectory->injectGlobalCapabilitiesFromFile(discoveryEntriesFile);
    runtime->start();

    return runtime;
}

void JoynrClusterControllerRuntime::unregisterProvider(const std::string& participantId)
{
    assert(capabilitiesRegistrar);
    capabilitiesRegistrar->remove(participantId);
}

void JoynrClusterControllerRuntime::start()
{
    startMessaging();
    registerRoutingProvider();
    registerDiscoveryProvider();
    singleThreadIOService->start();
}

void JoynrClusterControllerRuntime::stop(bool deleteChannel)
{
    if (deleteChannel) {
        this->deleteChannel();
    }
    stopMessaging();
    singleThreadIOService->stop();
}

void JoynrClusterControllerRuntime::deleteChannel()
{
    if (doHttpMessaging) {
        httpMessageReceiver->tryToDeleteChannel();
    }
    // Nothing to do for MQTT
}

} // namespace joynr
