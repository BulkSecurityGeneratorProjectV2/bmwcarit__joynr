/*jslint es5: true */

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

define(
        "joynr/start/InterTabClusterControllerRuntime",
        [
            "global/Promise",
            "joynr/capabilities/arbitration/Arbitrator",
            "joynr/provider/ProviderBuilder",
            "joynr/proxy/ProxyBuilder",
            "joynr/types/GlobalDiscoveryEntry",
            "joynr/capabilities/CapabilitiesRegistrar",
            "joynr/capabilities/ParticipantIdStorage",
            "joynr/capabilities/discovery/CapabilityDiscovery",
            "joynr/capabilities/CapabilitiesStore",
            "joynr/dispatching/RequestReplyManager",
            "joynr/dispatching/subscription/PublicationManager",
            "joynr/dispatching/subscription/SubscriptionManager",
            "joynr/dispatching/Dispatcher",
            "joynr/security/PlatformSecurityManager",
            "joynr/messaging/channel/ChannelMessagingSender",
            "joynr/messaging/channel/ChannelMessagingStubFactory",
            "joynr/messaging/channel/ChannelMessagingSkeleton",
            "joynr/system/RoutingTypes/MqttAddress",
            "joynr/messaging/MessagingStubFactory",
            "joynr/messaging/routing/MessageRouter",
            "joynr/messaging/routing/MessageQueue",
            "joynr/messaging/CommunicationModule",
            "joynr/util/InProcessSkeleton",
            "joynr/util/InProcessStub",
            "joynr/system/RoutingTypes/ChannelAddress",
            "joynr/messaging/inprocess/InProcessMessagingStubFactory",
            "joynr/messaging/inprocess/InProcessMessagingSkeleton",
            "joynr/messaging/inprocess/InProcessMessagingStub",
            "joynr/messaging/inprocess/InProcessAddress",
            "joynr/messaging/browser/BrowserMessagingStubFactory",
            "joynr/messaging/browser/BrowserMessagingSkeleton",
            "joynr/messaging/webmessaging/WebMessagingStub",
            "joynr/messaging/webmessaging/WebMessagingSkeleton",
            "joynr/messaging/channel/LongPollingChannelMessageReceiver",
            "joynr/messaging/MessagingQos",
            "joynr/proxy/DiscoveryQos",
            "joynr/types/ProviderQos",
            "joynr/types/ProviderScope",
            "joynr/types/DiscoveryScope",
            "joynr/system/DiscoveryProvider",
            "joynr/system/RoutingProvider",
            "joynr/types/TypeRegistrySingleton",
            "joynr/util/UtilInternal",
            "joynr/system/DistributedLoggingAppenderConstructorFactory",
            "joynr/system/DistributedLoggingAppender",
            "joynr/system/WebWorkerMessagingAppender",
            "uuid",
            "joynr/system/LoggingManager",
            "joynr/system/LoggerFactory",
            "joynr/start/settings/defaultSettings",
            "joynr/start/settings/defaultInterTabSettings",
            "joynr/start/settings/defaultClusterControllerSettings",
            "joynr/util/Typing",
            "joynr/util/LongTimer",
            "global/LocalStorage"
        ],
        function(
                Promise,
                Arbitrator,
                ProviderBuilder,
                ProxyBuilder,
                GlobalDiscoveryEntry,
                CapabilitiesRegistrar,
                ParticipantIdStorage,
                CapabilityDiscovery,
                CapabilitiesStore,
                RequestReplyManager,
                PublicationManager,
                SubscriptionManager,
                Dispatcher,
                PlatformSecurityManager,
                ChannelMessagingSender,
                ChannelMessagingStubFactory,
                ChannelMessagingSkeleton,
                MqttAddress,
                MessagingStubFactory,
                MessageRouter,
                MessageQueue,
                CommunicationModule,
                InProcessSkeleton,
                InProcessStub,
                ChannelAddress,
                InProcessMessagingStubFactory,
                InProcessMessagingSkeleton,
                InProcessMessagingStub,
                InProcessAddress,
                BrowserMessagingStubFactory,
                BrowserMessagingSkeleton,
                WebMessagingStub,
                WebMessagingSkeleton,
                LongPollingChannelMessageReceiver,
                MessagingQos,
                DiscoveryQos,
                ProviderQos,
                ProviderScope,
                DiscoveryScope,
                DiscoveryProvider,
                RoutingProvider,
                TypeRegistrySingleton,
                Util,
                DistributedLoggingAppenderConstructorFactory,
                DistributedLoggingAppender,
                WebWorkerMessagingAppender,
                uuid,
                LoggingManager,
                LoggerFactory,
                defaultSettings,
                defaultInterTabSettings,
                defaultClusterControllerSettings,
                Typing,
                LongTimer,
                LocalStorage) {
            var JoynrStates = {
                SHUTDOWN : "shut down",
                STARTING : "starting",
                STARTED : "started",
                SHUTTINGDOWN : "shutting down"
            };

            var TWO_DAYS_IN_MS = 172800000;
            var clusterControllerSettings;

            /**
             * The InterTabClusterControllerRuntime is the version of the libjoynr-js runtime that
             * hosts its own cluster controller
             *
             * @name InterTabClusterControllerRuntime
             * @constructor
             *
             * @param provisioning
             */
            function InterTabClusterControllerRuntime(provisioning) {
                var loggingManager;
                var initialRoutingTable;
                var untypedCapabilities;
                var typedCapabilities;
                var channelMessagingSender;
                var channelMessagingStubFactory;
                var messagingStubFactory;
                var webMessagingStub;
                var webMessagingSkeleton;
                var browserMessagingSkeleton;
                var messageRouter;
                var communicationModule;
                var longPollingMessageReceiver;
                var libjoynrMessagingSkeleton;
                var clusterControllerMessagingSkeleton;
                var clusterControllerChannelMessagingSkeleton;
                var clusterControllerMessagingStub;
                var dispatcher;
                var typeRegistry;
                var requestReplyManager;
                var subscriptionManager;
                var publicationManager;
                var participantIdStorage;
                var capabilityDiscovery;
                var arbitrator;
                var channelId;
                var bounceProxyBaseUrl;
                var providerBuilder;
                var proxyBuilder;
                var capabilitiesRegistrar;
                var localCapabilitiesStore;
                var globalCapabilitiesCache;
                var discoveryStub;
                var messageQueueSettings;
                var providerQos;
                var discoveryProvider;
                var routingProvider;
                var registerDiscoveryProviderPromise;
                var registerRoutingProviderPromise;
                var persistency;
                var longPollingCreatePromise;
                var freshnessIntervalId;

                // this is required at load time of libjoynr
                typeRegistry = Object.freeze(TypeRegistrySingleton.getInstance());

                /**
                 * @name InterTabClusterControllerRuntime#typeRegistry
                 * @type TypeRegistry
                 */
                Object.defineProperty(this, "typeRegistry", {
                    get : function() {
                        return typeRegistry;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#registration
                 * @type CapabilitiesRegistrar
                 */
                Object.defineProperty(this, "registration", {
                    get : function() {
                        return capabilitiesRegistrar;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#capabilities
                 * @type CapabilitiesRegistrar
                 * @deprecated capabilities will be removed by 01.01.2017. please use registration instead
                 */
                Object.defineProperty(this, "capabilities", {
                    get : function() {
                        return capabilitiesRegistrar;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#participantIdStorage
                 * @type ParticipantIdStorage
                 */
                Object.defineProperty(this, "participantIdStorage", {
                    get : function() {
                        return participantIdStorage;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#providerBuilder
                 * @type ProviderBuilder
                 */
                Object.defineProperty(this, "providerBuilder", {
                    get : function() {
                        return providerBuilder;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#proxyBuilder
                 * @type ProxyBuilder
                 */
                Object.defineProperty(this, "proxyBuilder", {
                    get : function() {
                        return proxyBuilder;
                    },
                    enumerable : true
                });

                /**
                 * @name InterTabClusterControllerRuntime#logging
                 * @type LoggingManager
                 */
                Object.defineProperty(this, "logging", {
                    get : function() {
                        return loggingManager;
                    },
                    enumerable : true
                });

                var log, relativeTtl;

                if (provisioning.logging && provisioning.logging.ttl) {
                    relativeTtl = provisioning.logging.ttl;
                } else {
                    relativeTtl = TWO_DAYS_IN_MS;
                }

                var loggingMessagingQos = new MessagingQos({
                    ttl : relativeTtl
                });
                loggingManager = Object.freeze(new LoggingManager());
                LoggerFactory.init(loggingManager);

                var joynrState = JoynrStates.SHUTDOWN;

                /**
                 * Starts up the libjoynr instance
                 *
                 * @name InterTabClusterControllerRuntime#start
                 * @function
                 * @returns {Object} an A+ promise object, reporting when libjoynr startup is completed or failed
                 * @throws {Error}
                 *             if libjoynr is not in SHUTDOWN state
                 */
                this.start =
                        function start() {
                            var i,j;

                            if (joynrState !== JoynrStates.SHUTDOWN) {
                                throw new Error("Cannot start libjoynr because it's currently \""
                                    + joynrState
                                    + "\"");
                            }
                            joynrState = JoynrStates.STARTING;

                            if (!provisioning) {
                                throw new Error("Constructor has been invoked without provisioning");
                            }

                            // initialize Logger with external logging configuration or default
                            // values
                            loggingManager.registerAppenderClass(
                                    "WebWorker",
                                    WebWorkerMessagingAppender);
                            loggingManager.registerAppenderClass(
                                    "Distributed",
                                    DistributedLoggingAppenderConstructorFactory.build(
                                            proxyBuilder,
                                            loggingMessagingQos));

                            if (provisioning.logging) {
                                loggingManager.configure(provisioning.logging);
                            }

                            log =
                                    LoggerFactory
                                            .getLogger("joynr.start.InterTabClusterControllerRuntime");

                            var persistencyProvisioning = provisioning.persistency || {};
                            persistency = new LocalStorage({
                                clearPersistency : persistencyProvisioning.clearPersistency,
                                location : persistencyProvisioning.location
                            });

                            if (Util.checkNullUndefined(provisioning.bounceProxyUrl)) {
                                throw new Error(
                                        "bounce proxy URL not set in provisioning.bounceProxyUrl");
                            }
                            if (Util.checkNullUndefined(provisioning.bounceProxyBaseUrl)) {
                                throw new Error(
                                        "bounce proxy base URL not set in provisioning.bounceProxyBaseUrl");
                            }
                            if (Util.checkNullUndefined(provisioning.parentWindow)) {
                                log.debug("provisioning.parentWindow not set. Use default setting \"" + defaultInterTabSettings.parentWindow + "\" instead");
                            }

                            initialRoutingTable = {};
                            bounceProxyBaseUrl = provisioning.bounceProxyBaseUrl;

                            channelId =
                                    provisioning.channelId
                                        || persistency.getItem("joynr.channels.channelId.1")
                                        || "chjs_"
                                        + uuid();
                            persistency.setItem("joynr.channels.channelId.1", channelId);

                            clusterControllerSettings = defaultClusterControllerSettings({
                                bounceProxyBaseUrl: provisioning.bounceProxyBaseUrl
                            });
                            untypedCapabilities = provisioning.capabilities || [];
                            var defaultCapabilities = clusterControllerSettings.capabilities || [];

                            untypedCapabilities = untypedCapabilities.concat(defaultCapabilities);
                            /*jslint nomen: true */// allow use of _typeName once
                            typeRegistry.addType(new ChannelAddress()._typeName, ChannelAddress, false);
                            /*jslint nomen: false */
                            typedCapabilities = [];
                            for (i = 0; i < untypedCapabilities.length; i++) {
                                var capability =
                                        new GlobalDiscoveryEntry(untypedCapabilities[i]);
                                if (!capability.address) {
                                    throw new Error("provisioned capability is missing address: " + JSON.stringify(capability));
                                }
                                initialRoutingTable[capability.participantId] = Typing.augmentTypes(JSON.parse(capability.address), typeRegistry);
                                typedCapabilities.push(capability);
                            }

                            communicationModule = new CommunicationModule();

                            channelMessagingSender = new ChannelMessagingSender({
                                communicationModule : communicationModule,
                                channelQos : provisioning.channelQos
                            });

                            messageQueueSettings = {};
                            if (provisioning.messaging !== undefined
                                && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
                                messageQueueSettings.maxQueueSizeInKBytes =
                                        provisioning.messaging.maxQueueSizeInKBytes;
                            }

                            webMessagingStub = new WebMessagingStub({
                                // parent window variable for communication
                                window : provisioning.parentWindow || defaultInterTabSettings.parentWindow,
                                // target origin of the parent window
                                origin : provisioning.parentOrigin || defaultInterTabSettings.parentOrigin
                            });

                            webMessagingSkeleton = new WebMessagingSkeleton({
                                window : provisioning.window || defaultInterTabSettings.window
                            });

                            browserMessagingSkeleton = new BrowserMessagingSkeleton({
                                webMessagingSkeleton : webMessagingSkeleton
                            });

                            channelMessagingStubFactory = new ChannelMessagingStubFactory({
                                myChannelId : channelId,
                                channelMessagingSender : channelMessagingSender
                            });

                            var mqttAddress = new MqttAddress({
                                brokerUri : provisioning.brokerUri,
                                topic : channelId
                            });
                            messagingStubFactory = new MessagingStubFactory({
                                messagingStubFactories : {
                                    InProcessAddress : new InProcessMessagingStubFactory(),
                                    BrowserAddress : new BrowserMessagingStubFactory({
                                        webMessagingStub : webMessagingStub
                                    }),
                                    ChannelAddress : channelMessagingStubFactory
                                }
                            });
                            messageRouter = new MessageRouter({
                                initialRoutingTable : initialRoutingTable,
                                persistency : persistency,
                                typeRegistry : typeRegistry,
                                joynrInstanceId : channelId,
                                messagingStubFactory : messagingStubFactory,
                                messageQueue : new MessageQueue(messageQueueSettings)
                            });
                            browserMessagingSkeleton.registerListener(messageRouter.route);

                            longPollingMessageReceiver = new LongPollingChannelMessageReceiver({
                                persistency : persistency,
                                bounceProxyUrl : bounceProxyBaseUrl + "/bounceproxy/",
                                communicationModule : communicationModule,
                                channelQos: provisioning.channelQos
                            });

                            // link up clustercontroller messaging to channel
                            clusterControllerChannelMessagingSkeleton =
                                    new ChannelMessagingSkeleton({
                                        messageRouter : messageRouter
                                    });
                            // clusterControllerChannelMessagingSkeleton.registerListener(messageRouter.route);

                            longPollingCreatePromise = longPollingMessageReceiver.create(channelId).then(
                                    function(channelUrl) {
                                        var channelAddress = new ChannelAddress({
                                            channelId: channelId,
                                            messagingEndpointUrl: channelUrl
                                        });
                                        channelMessagingStubFactory.globalAddressReady(channelAddress);
                                        capabilityDiscovery.globalAddressReady(channelAddress);
                                        longPollingMessageReceiver
                                                .start(clusterControllerChannelMessagingSkeleton.receiveMessage);
                                        channelMessagingSender.start();
                                    });

                            // link up clustercontroller messaging to dispatcher
                            clusterControllerMessagingSkeleton = new InProcessMessagingSkeleton();
                            clusterControllerMessagingStub =
                                    new InProcessMessagingStub(clusterControllerMessagingSkeleton);

                            // clustercontroller messaging handled by the messageRouter
                            clusterControllerMessagingSkeleton
                                    .registerListener(messageRouter.route);

                            dispatcher =
                                    new Dispatcher(
                                            clusterControllerMessagingStub,
                                            new PlatformSecurityManager());

                            libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
                            libjoynrMessagingSkeleton.registerListener(dispatcher.receive);

                            requestReplyManager = new RequestReplyManager(dispatcher, typeRegistry);
                            subscriptionManager = new SubscriptionManager(dispatcher);
                            publicationManager =
                                    new PublicationManager(dispatcher, persistency, channelId);

                            dispatcher.registerRequestReplyManager(requestReplyManager);
                            dispatcher.registerSubscriptionManager(subscriptionManager);
                            dispatcher.registerPublicationManager(publicationManager);

                            localCapabilitiesStore = new CapabilitiesStore();
                            globalCapabilitiesCache = new CapabilitiesStore(typedCapabilities);

                            participantIdStorage = new ParticipantIdStorage(persistency, uuid);

                            discoveryStub =
                                new InProcessStub();

                            capabilitiesRegistrar =
                                    Object.freeze(new CapabilitiesRegistrar({
                                        discoveryStub : discoveryStub,
                                        messageRouter : messageRouter,
                                        requestReplyManager : requestReplyManager,
                                        publicationManager : publicationManager,
                                        libjoynrMessagingAddress : new InProcessAddress(
                                                libjoynrMessagingSkeleton),
                                        participantIdStorage : participantIdStorage,
                                        loggingManager : loggingManager
                                    }));

                            arbitrator = new Arbitrator(discoveryStub);

                            proxyBuilder =
                                Object.freeze(new ProxyBuilder({
                                    arbitrator : arbitrator,
                                    requestReplyManager : requestReplyManager,
                                    subscriptionManager : subscriptionManager,
                                    publicationManager : publicationManager
                                }, {
                                    messageRouter : messageRouter,
                                    libjoynrMessagingAddress : new InProcessAddress(
                                            libjoynrMessagingSkeleton),
                                            loggingManager : loggingManager
                                }));

                            var internalMessagingQos =
                                new MessagingQos(provisioning.internalMessagingQos);

                            var defaultProxyBuildSettings = {
                                domain : "io.joynr",
                                messagingQos : internalMessagingQos,
                                discoveryQos : new DiscoveryQos(
                                        {
                                            discoveryScope : DiscoveryScope.GLOBAL_ONLY,
                                            cacheMaxAgeMs : Util.getMaxLongValue()
                                        })
                            };

                            capabilityDiscovery =
                                    new CapabilityDiscovery(
                                            localCapabilitiesStore,
                                            globalCapabilitiesCache,
                                            messageRouter,
                                            proxyBuilder,
                                            defaultProxyBuildSettings.domain);

                            discoveryStub.setSkeleton(new InProcessSkeleton(capabilityDiscovery));

                            var period = provisioning.capabilitiesFreshnessUpdateIntervalMs || 3600000; // default: 1 hour
                            freshnessIntervalId = LongTimer.setInterval(function() {
                                capabilityDiscovery.touch(channelId, period).catch(function(error) {
                                    log.error("error sending freshness update: " + error);
                                });
                                return null;
                            }, period);

                            providerBuilder = Object.freeze(new ProviderBuilder());

                            providerQos = new ProviderQos({
                                customParameters : [],
                                priority : Date.now(),
                                scope : ProviderScope.LOCAL
                            });

                            discoveryProvider =
                                    providerBuilder.build(
                                            DiscoveryProvider,
                                            {
                                                add : function(opArgs) {
                                                    /*FIXME remove discoveryEntry transformation,
                                                     * once the deserialization of enums works correctly
                                                     */
                                                    if (typeof opArgs.discoveryEntry.qos.scope === "string") {
                                                        opArgs.discoveryEntry.qos.scope = ProviderScope[opArgs.discoveryEntry.qos.scope];
                                                    }
                                                    return capabilityDiscovery
                                                            .add(opArgs.discoveryEntry);
                                                },
                                                lookup : function(opArgs) {
                                                    /*FIXME remove discoveryQos transformation,
                                                     * once the deserialization of enums works correctly
                                                     */
                                                    if (typeof opArgs.discoveryQos.discoveryScope === "string") {
                                                        opArgs.discoveryQos.discoveryScope = DiscoveryScope[opArgs.discoveryQos.discoveryScope];
                                                    }
                                                    return capabilityDiscovery.lookup(
                                                            opArgs.domains,
                                                            opArgs.interfaceName,
                                                            opArgs.discoveryQos).then(function(caps){
                                                                return {
                                                                    result : caps
                                                                };
                                                            });
                                                },
                                                remove : function(opArgs) {
                                                    return capabilityDiscovery
                                                            .remove(opArgs.participantId);
                                                }
                                            });
                            registerDiscoveryProviderPromise =
                                    capabilitiesRegistrar.registerProvider(
                                            "io.joynr",
                                            discoveryProvider,
                                            providerQos);

                            routingProvider =
                                    providerBuilder.build(RoutingProvider, {
                                        addNextHop : function(opArgs) {
                                            var address;
                                            if (opArgs.channelAddress !== undefined) {
                                                address = opArgs.channelAddress; 
                                            } else if (opArgs.commonApiDbusAddress !== undefined) {
                                                address = opArgs.commonApiDbusAddress;
                                            } else if (opArgs.browserAddress !== undefined) {
                                                address = opArgs.browserAddress;
                                            } else if (opArgs.webSocketAddress !== undefined) {
                                                address = opArgs.webSocketAddress;
                                            }
                                            if (address !== undefined) {
                                                return messageRouter.addNextHop(
                                                    opArgs.participantId,
                                                    address);
                                            }
                                            return Promise.reject(new Error("RoutingProvider.addNextHop failed, because address " +
                                                    "could not be found in the operation arguments " + JSON.stringify(opArgs)));
                                        },
                                        resolveNextHop : function(opArgs) {
                                            return messageRouter.resolveNextHop(opArgs.participantId)
                                                    .then(function(address) {
                                                        var isResolved = address !== undefined;
                                                        return {
                                                            resolved: isResolved
                                                        };
                                                    }).catch(function(error) {
                                                        return false;
                                                    });
                                        },
                                        removeNextHop : function(opArgs) {
                                            return messageRouter.removeNextHop(opArgs.participantId);
                                        },
                                        addMulticastReceiver : function(opArgs) {
                                            throw new Error('not implemented');
                                        },
                                        removeMulticastReceiver : function(opArgs) {
                                            throw new Error('not implemented');
                                        }
                                    });
                            registerRoutingProviderPromise =
                                    capabilitiesRegistrar.registerProvider(
                                            "io.joynr",
                                            routingProvider,
                                            providerQos);

                            // when everything's ready we can resolve the promise
                            return Promise.all(
                                    [
                                        registerDiscoveryProviderPromise,
                                        registerRoutingProviderPromise
                                    ]).then(function() {
                                joynrState = JoynrStates.STARTED;
                                publicationManager.restore();
                                log.debug("joynr cluster controller initialized");
                                return;
                            }).catch(function(error) {
                                log.error("error starting up joynr: " + error);
                                throw error;
                            });
                        };

                /**
                 * Shuts down libjoynr
                 *
                 * @name InterTabClusterControllerRuntime#shutdown
                 * @function
                 * @throws {Error}
                 *             if libjoynr is not in the STARTED state
                 */
                this.shutdown =
                        function shutdown() {
                            if (joynrState !== JoynrStates.STARTED) {
                                throw new Error(
                                        "Cannot shutdown libjoynr because it's currently \""
                                            + joynrState
                                            + "\"");
                            }
                            joynrState = JoynrStates.SHUTTINGDOWN;

                            LongTimer.clearInterval(freshnessIntervalId);

                            longPollingCreatePromise.then(function() {
                                longPollingMessageReceiver.clear(channelId)
                                .then(function() {
                                    // stop LongPolling
                                    longPollingMessageReceiver.stop();
                                }).catch(function(error) {
                                    var errorString = "error clearing long poll channel: "
                                        + error;
                                    log.error(errorString);
                                    // stop LongPolling
                                    longPollingMessageReceiver.stop();
                                    throw new Error(errorString);
                                });
                            });

                            if (channelMessagingSender !== undefined) {
                                channelMessagingSender.shutdown();
                            }

                            if (capabilitiesRegistrar !== undefined) {
                                capabilitiesRegistrar.shutdown();
                            }

                            if (arbitrator !== undefined) {
                                arbitrator.shutdown();
                            }

                            if (messageRouter !== undefined) {
                                messageRouter.shutdown();
                            }

                            if (requestReplyManager !== undefined) {
                                requestReplyManager.shutdown();
                            }

                            if (publicationManager !== undefined) {
                                publicationManager.shutdown();
                            }

                            if (subscriptionManager !== undefined) {
                                subscriptionManager.shutdown();
                            }

                            if (dispatcher !== undefined) {
                                dispatcher.shutdown();
                            }

                            if (typeRegistry !== undefined) {
                                typeRegistry.shutdown();
                            }

                            log.debug("joynr cluster controller shut down");
                            joynrState = JoynrStates.SHUTDOWN;
                            return Promise.resolve();
                };

                // make every instance immutable
                return Object.freeze(this);
            }

            return InterTabClusterControllerRuntime;

        });
