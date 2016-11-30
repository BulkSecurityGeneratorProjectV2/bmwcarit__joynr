/*global JSON: true */
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
        "joynr/messaging/routing/MessageRouter",
        [
            "global/Promise",
            "joynr/messaging/util/MulticastWildcardRegexFactory",
            "joynr/system/DiagnosticTags",
            "joynr/system/LoggerFactory",
            "joynr/messaging/inprocess/InProcessAddress",
            "joynr/messaging/JoynrMessage",
            "joynr/util/Typing",
            "joynr/util/JSONSerializer",
        ],
        function(Promise, MulticastWildcardRegexFactory, DiagnosticTags, LoggerFactory, InProcessAddress, JoynrMessage, Typing, JSONSerializer) {

            /**
             * Message Router receives a message and forwards it to the correct endpoint, as looked up in the {@link RoutingTable}
             *
             * @constructor
             * @name MessageRouter
             *
             * @param {Object}
             *            settings the settings object holding dependencies
             * @param {RoutingTable}
             *            settings.routingTable
             * @param {MessagingStubFactory}
             *            settings.messagingStubFactory
             * @param {String}
             *            settings.myChannelId
             * @param {Address} settings.incomingAddress
             * @param {Address} settings.parentMessageRouterAddress
             * @param {LocalStorage} settings.persistency - LocalStorage or another object implementing the same interface
             * @param {TypeRegistry} settings.typeRegistry
             *
             * @classdesc The <code>MessageRouter</code> is a joynr internal interface. The Message
             * Router receives messages from Message Receivers, and forwards them along using to the
             * appropriate Address, either <code>{@link ChannelAddress}</code> for messages being
             * sent to a joynr channel via HTTP, <code>{@link BrowserAddress}</code> for messages
             * going to applications running within a seperate browser tab, or
             * <code>{@link InProcessAddress}</code> for messages going to a dispatcher running
             * within the same JavaScript scope as the MessageRouter.
             *
             * MessageRouter is part of the cluster controller, and is used for
             * internal messaging only.
             */
            function MessageRouter(settings) {
                // TODO remote provider participants are registered by capabilitiesDirectory on lookup
                // TODO local providers registered (using dispatcher as local endpoint) by capabilitiesDirectory on register
                // TODO local replyCallers are registered (using dispatcher as local endpoint) when they are created

                var that = this;
                var multicastWildcardRegexFactory = new MulticastWildcardRegexFactory();
                var log = LoggerFactory.getLogger("joynr/messaging/routing/MessageRouter");
                var listener, routingProxy, messagingStub;
                var queuedAddNextHopCalls = [], queuedRemoveNextHopCalls = [], queuedAddMulticastReceiverCalls = [], queuedRemoveMulticastReceiverCalls = [];
                var routingTable = settings.initialRoutingTable || {};
                var id = settings.joynrInstanceId;
                var persistency = settings.persistency;
                var incomingAddress = settings.incomingAddress;
                var parentMessageRouterAddress = settings.parentMessageRouterAddress;
                var typeRegistry = settings.typeRegistry;
                var multicastAddressCalculator = settings.multicastAddressCalculator;
                var messagingSkeletonFactory = settings.messagingSkeletonFactory;
                var multicastReceiversRegistry = {};

                // if (settings.routingTable === undefined) {
                // throw new Error("routing table is undefined");
                // }
                if (settings.parentMessageRouterAddress !== undefined
                    && settings.incomingAddress === undefined) {
                    throw new Error("incoming address is undefined");
                }
                if (settings.messagingStubFactory === undefined) {
                    throw new Error("messaging stub factory is undefined");
                }
                if (settings.messageQueue === undefined) {
                    throw new Error("messageQueue is undefined");
                }


                var started = true;

                function isReady() {
                    return started;
                }

                /**
                 * @function MessageRouter#getStorageKey
                 *
                 * @param {String} participantId
                 *
                 * @returns {String}
                 */
                this.getStorageKey = function getStorageKey(participantId) {
                    return id + "_" + participantId.replace("/", ".");
                };

                /**
                 * @function MessageRouter#removeNextHop
                 *
                 * @param {String} participantId
                 *
                 * @returns {Promise} promise
                 */
                this.removeNextHop = function removeNextHop(participantId) {
                    if (!isReady()) {
                        log.debug("removeNextHop: ignore call as message router is already shut down");
                        return Promise.reject(new Error("message router is already shut down"));
                    }

                    routingTable[participantId] = undefined;
                    persistency.removeItem(that.getStorageKey(participantId));
                    var promise;
                    if (routingProxy !== undefined) {
                        promise = routingProxy.removeNextHop({
                            participantId : participantId
                        });
                    } else if (parentMessageRouterAddress !== undefined) {
                        promise = new Promise(function(resolve, reject) {
                            queuedRemoveNextHopCalls[queuedRemoveNextHopCalls.length] = {
                                participantId : participantId,
                                resolve : resolve,
                                reject : reject
                            };
                        });
                    } else {
                        promise = Promise.resolve();
                    }
                    return promise;
                };

                /**
                 * @function MessageRouter#addNextHopToParentRoutingTable
                 *
                 * @param {String} participantId
                 *
                 * @returns result
                 */
                this.addNextHopToParentRoutingTable =
                        function addNextHopToParentRoutingTable(participantId) {
                            var result;
                            if (Typing.getObjectType(incomingAddress) === "BrowserAddress") {
                                result = routingProxy.addNextHop({
                                    participantId : participantId,
                                    browserAddress : incomingAddress
                                });
                            } else if (Typing.getObjectType(incomingAddress) === "ChannelAddress") {
                                result = routingProxy.addNextHop({
                                    participantId : participantId,
                                    channelAddress : incomingAddress
                                });
                            } else if (Typing.getObjectType(incomingAddress) === "WebSocketAddress") {
                                result = routingProxy.addNextHop({
                                    participantId : participantId,
                                    webSocketAddress : incomingAddress
                                });
                            } else if (Typing.getObjectType(incomingAddress) === "WebSocketClientAddress") {
                                result = routingProxy.addNextHop({
                                    participantId : participantId,
                                    webSocketClientAddress : incomingAddress
                                });
                            } else if (Typing.getObjectType(incomingAddress) === "CommonApiDbusAddress") {
                                result = routingProxy.addNextHop({
                                    participantId : participantId,
                                    commonApiDbusAddress : incomingAddress
                                });
                            }
                            return result;
                        };

                /**
                 * @function MessageRouter#setRoutingProxy
                 *
                 * @param {RoutingProxy} newRoutingproxy - the routing proxy to be set
                 */
                this.setRoutingProxy =
                        function setRoutingProxy(newRoutingProxy) {
                            var hop, participantId, errorFct, receiver, queuedCall;

                            errorFct = function(error) {
                                if (!isReady()) {
                                    //in this case, the error is expected, e.g. during shut down
                                    log.debug("Adding routingProxy.proxyParticipantId " + routingProxy.proxyParticipantId
                                            + "failed while the message router is not ready. Error: " + error.message);
                                    return;
                                }
                                throw new Error(error);
                            };

                            routingProxy = newRoutingProxy;
                            if (routingProxy !== undefined) {
                                if (routingProxy.proxyParticipantId !== undefined) {
                                    that.addNextHopToParentRoutingTable(
                                            routingProxy.proxyParticipantId).catch(errorFct);
                                }
                                for (hop in queuedAddNextHopCalls) {
                                    if (queuedAddNextHopCalls.hasOwnProperty(hop)) {
                                        queuedCall = queuedAddNextHopCalls[hop];
                                        if (queuedCall.participantId !== routingProxy.proxyParticipantId) {
                                            that.addNextHopToParentRoutingTable(
                                                    queuedCall.participantId).then(
                                                    queuedCall.resolve).catch(queuedCall.reject);
                                        }
                                    }
                                }
                                for (hop in queuedRemoveNextHopCalls) {
                                    if (queuedRemoveNextHopCalls.hasOwnProperty(hop)) {
                                        queuedCall = queuedRemoveNextHopCalls[hop];
                                        that.removeNextHop(queuedCall.participantId).then(
                                                queuedCall.resolve).catch(queuedCall.reject);
                                    }
                                }
                                for (receiver in queuedAddMulticastReceiverCalls) {
                                    if (queuedAddMulticastReceiverCalls.hasOwnProperty(receiver)) {
                                        queuedCall = queuedAddMulticastReceiverCalls[receiver];
                                        routingProxy.addMulticastReceiver(queuedCall.parameters).then(
                                                queuedCall.resolve).catch(queuedCall.reject);
                                    }
                                }
                                for (receiver in queuedRemoveMulticastReceiverCalls) {
                                    if (queuedRemoveMulticastReceiverCalls.hasOwnProperty(receiver)) {
                                        queuedCall = queuedRemoveMulticastReceiverCalls[receiver];
                                        routingProxy.removeMulticastReceiver(queuedCall.parameters).then(
                                                queuedCall.resolve).catch(queuedCall.reject);
                                    }
                                }
                            }
                            queuedAddNextHopCalls = undefined;
                            queuedRemoveNextHopCalls = undefined;
                            queuedAddMulticastReceiverCalls = undefined;
                            queuedRemoveMulticastReceiverCalls = undefined;
                        };

                /*
                 * this method is called when no address can be found in the local routing table 
                 */
                function resolveNextHopInternal(participantId) {
                    var address, addressString;

                    addressString = persistency.getItem(that.getStorageKey(participantId));
                    if ((addressString === undefined
                        || addressString === null || addressString === '{}')) {
                        persistency.removeItem(that.getStorageKey(participantId));
                    } else {
                        address = Typing.augmentTypes(JSON.parse(addressString), typeRegistry);
                        routingTable[participantId] = address;
                    }
                    if (address === undefined && routingProxy !== undefined) {
                        return routingProxy.resolveNextHop({
                                participantId : participantId
                            }).then(function(opArgs) {
                                if (opArgs.resolved) {
                                    routingTable[participantId] = parentMessageRouterAddress;
                                    return parentMessageRouterAddress;
                                }
                                throw new Error("nextHop cannot be resolved, as participant with id "
                                                + participantId
                                                + " is not reachable by parent routing table");
                            });
                    }
                    return Promise.resolve(address);
                }

                /**
                 * Looks up an Address for a given participantId (next hop)
                 *
                 * @name MessageRouter#resolveNextHop
                 * @function
                 *
                 * @param {String}
                 *            participantId
                 * @returns {InProcessAddress|BrowserAddress|ChannelAddress} the address of the next hop in the direction of the given
                 *          participantId, or undefined if not found
                 */
                this.resolveNextHop = function resolveNextHop(participantId) {
                    if (!isReady()) {
                        log.debug("resolveNextHop: ignore call as message router is already shut down");
                        return Promise.reject(new Error("message router is already shut down"));
                    }

                    var address = routingTable[participantId];

                    if (address === undefined) {
                        return resolveNextHopInternal(participantId);
                    }

                    return Promise.resolve(address);
                };

                /**
                 * Get the address to which the passed in message should be sent to.
                 * This is a multicast address calculated from the header content of the message.
                 *
                 * @param message the message for which we want to find an address to send it to.
                 * @return the address to send the message to. Will not be null, because if an address can't be determined an exception is thrown.
                 */
                function getAddressesForMulticast (joynrMessage) {
                    var i, result = [], address;
                    if (!joynrMessage.isReceivedFromGlobal) {
                        address = multicastAddressCalculator.calculate(joynrMessage);
                        if (address !== undefined) {
                            result.push(address);
                        }
                    }

                    function containsAddress(array, address) {
                        //each address class provides an equals method, e.g. InProcessAddress
                        var j;
                        if (array === undefined) {
                            return false;
                        }
                        for (j=0;j<array.length;j++) {
                            if (array[j].equals(address)) {
                                return true;
                            }
                        }
                        return false;
                    }

                    var multicastIdPattern, receivers;
                    for (multicastIdPattern in multicastReceiversRegistry) {
                        if (multicastReceiversRegistry.hasOwnProperty(multicastIdPattern)) {
                            if(joynrMessage.to.match(new RegExp(multicastIdPattern)) !== null) {
                                receivers = multicastReceiversRegistry[multicastIdPattern];
                                if (receivers !== undefined) {
                                    for (i=0;i<receivers.length;i++){
                                        address = routingTable[receivers[i]];
                                        if (address !== undefined && !containsAddress(result, address)) {
                                            result.push(address);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    return result;
                }

                /**
                 * Helper function to route a message once the address is known
                 */
                function routeInternal (address, joynrMessage) {
                    var errorMsg;
                    // Error: The participant is not registered yet.
                    // remote provider participants are registered by capabilitiesDirectory on lookup
                    // local providers are registered by capabilitiesDirectory on register
                    // replyCallers are registered when they are created
                    if (address === undefined) {
                        errorMsg =
                                "No message receiver found for participantId: "
                                    + joynrMessage.to
                                    + ". Queuing request message.";
                        log.info(errorMsg, DiagnosticTags
                                .forJoynrMessage(joynrMessage));
                        settings.messageQueue.putMessage(joynrMessage);
                        throw new Error(errorMsg);
                    } else {
                        messagingStub =
                                settings.messagingStubFactory
                                        .createMessagingStub(address);
                        if (messagingStub === undefined) {
                            errorMsg =
                                    "No message receiver found for participantId: "
                                        + joynrMessage.to
                                        + " queuing message.";
                            log.info(errorMsg, DiagnosticTags
                                    .forJoynrMessage(joynrMessage));
                            throw new Error(errorMsg);
                        } else {
                            return messagingStub.transmit(joynrMessage).then(function() {
                                //succeeded, do nothing
                                return null;
                            }).catch(function(error) {
                                //error while transmitting message
                                log.debug("Error while transmitting message: " + error);
                                //TODO queue message and retry later
                                return null;
                            });
                        }
                    }
                }

                /**
                 * @name MessageRouter#route
                 * @function
                 *
                 * @param {JoynrMessage}
                 *            joynrMessage
                 * @returns {Object} A+ promise object
                 */
                this.route =
                        function route(joynrMessage) {
                    function forwardToRouteInternal(address) {
                        return routeInternal(address, joynrMessage);
                    }
                    if (joynrMessage.type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST) {
                        return Promise.all(getAddressesForMulticast(joynrMessage).map(forwardToRouteInternal));
                    }

                    var participantId = joynrMessage.to;
                    var address = routingTable[participantId];
                    if (address !== undefined) {
                        return routeInternal(address, joynrMessage);
                    }

                    return resolveNextHopInternal(participantId).then(forwardToRouteInternal);
                };

                /**
                 * Registers the next hop with this specific participant Id
                 *
                 * @name RoutingTable#addNextHop
                 * @function
                 *
                 * @param {String}
                 *            participantId
                 * @param {InProcessAddress|BrowserAddress|ChannelAddress}
                 *            address the address to register
                 */
                this.addNextHop =
                        function addNextHop(participantId, address) {
                            if (!isReady()) {
                                log.debug("addNextHop: ignore call as message router is already shut down");
                                return Promise.reject(new Error("message router is already shut down"));
                            }
                            // store the address of the participantId persistently
                            routingTable[participantId] = address;
                            var serializedAddress = JSONSerializer.stringify(address);
                            var promise;
                            if ((serializedAddress === undefined
                                || serializedAddress === null || serializedAddress === '{}')) {
                                log.info("addNextHop: HOP address "
                                    + serializedAddress
                                    + " will not be persisted for participant id: "
                                    + participantId);
                            } else {
                                persistency.setItem(
                                        that.getStorageKey(participantId),
                                        serializedAddress);
                            }

                            if (routingProxy !== undefined) {
                                // register remotely
                                promise = that.addNextHopToParentRoutingTable(participantId);
                            } else {
                                if (parentMessageRouterAddress !== undefined) {
                                    promise = new Promise(function(resolve, reject){
                                        queuedAddNextHopCalls[queuedAddNextHopCalls.length] =
                                        {
                                            participantId : participantId,
                                            resolve : resolve,
                                            reject : reject
                                        };
                                    });
                                } else {
                                    promise = Promise.resolve();
                                }
                            }
                            that.participantRegistered(participantId);
                            return promise;
                        };

                /**
                 * Adds a new receiver for the identified multicasts.
                 *
                 * @name RoutingTable#addMulticastReceiver
                 * @function
                 *
                 * @param {Object}
                 *            parameters - object containing parameters
                 * @param {String}
                 *            parameters.multicastId
                 * @param {String}
                 *            parameters.subscriberParticipantId
                 * @param {String}
                 *            parameters.providerParticipantId
                 */
                this.addMulticastReceiver = function addMulticastReceiver(parameters) {
                    //1. handle call in local router
                    //1.a store receiver in multicastReceiverRegistry
                    var multicastIdPattern = multicastWildcardRegexFactory.createIdPattern(parameters.multicastId);
                    var providerAddress = routingTable[parameters.providerParticipantId];

                    if (multicastReceiversRegistry[multicastIdPattern] === undefined) {
                        multicastReceiversRegistry[multicastIdPattern] = [];

                        //1.b the first receiver for this multicastId -> inform MessagingSkeleton about receiver
                        var skeleton = messagingSkeletonFactory.getSkeleton(providerAddress);
                        if (skeleton !== undefined && skeleton.registerMulticastSubscription !== undefined) {
                            skeleton.registerMulticastSubscription(parameters.multicastId);
                        }
                    }

                    multicastReceiversRegistry[multicastIdPattern].push(parameters.subscriberParticipantId);

                    //2. forward call to parent router (if available)
                    if (parentMessageRouterAddress === undefined || providerAddress === undefined || providerAddress instanceof InProcessAddress) {
                        return Promise.resolve();
                    }
                    if (routingProxy !== undefined) {
                        return routingProxy.addMulticastReceiver(parameters);
                    }
                    return new Promise(function(resolve, reject){
                        queuedAddMulticastReceiverCalls[queuedAddMulticastReceiverCalls.length] =
                        {
                            parameters : parameters,
                            resolve : resolve,
                            reject : reject
                        };
                    });
                };

                /**
                 * Removes a receiver for the identified multicasts.
                 *
                 * @name RoutingTable#removeMulticastReceiver
                 * @function
                 *
                 * @param {Object}
                 *            parameters - object containing parameters
                 * @param {String}
                 *            parameters.multicastId
                 * @param {String}
                 *            parameters.subscriberParticipantId
                 * @param {String}
                 *            parameters.providerParticipantId
                 */
                this.removeMulticastReceiver = function removeMulticastReceiver(parameters) {
                    //1. handle call in local router
                    //1.a remove receiver from multicastReceiverRegistry
                    var multicastIdPattern = multicastWildcardRegexFactory.createIdPattern(parameters.multicastId);
                    var providerAddress = routingTable[parameters.providerParticipantId];
                    if (multicastReceiversRegistry[multicastIdPattern] !== undefined) {
                        var i, receivers = multicastReceiversRegistry[multicastIdPattern];
                        for (i = 0; i < receivers.length; i++) {
                            if (receivers[i] === parameters.subscriberParticipantId) {
                                receivers.splice(i, 1);
                                break;
                            }
                        }
                        if (receivers.length === 0) {
                            delete multicastReceiversRegistry[multicastIdPattern];

                            //1.b no receiver anymore for this multicastId -> inform MessagingSkeleton about removed receiver
                            var skeleton = messagingSkeletonFactory.getSkeleton(providerAddress);
                            if (skeleton !== undefined && skeleton.unregisterMulticastSubscription !== undefined) {
                                skeleton.unregisterMulticastSubscription(parameters.multicastId);
                            }

                        }
                    }


                    //2. forward call to parent router (if available)
                    if (parentMessageRouterAddress === undefined || providerAddress === undefined || providerAddress instanceof InProcessAddress) {
                        return Promise.resolve();
                    }
                    if (routingProxy !== undefined) {
                        return routingProxy.removeMulticastReceiver(parameters);
                    }
                    return new Promise(function(resolve, reject){
                        queuedRemoveMulticastReceiverCalls[queuedRemoveMulticastReceiverCalls.length] =
                        {
                            parameters : parameters,
                            resolve : resolve,
                            reject : reject
                        };
                    });
                };

                /**
                 * @function MessageRouter#participantRegistered
                 *
                 * @param {String} participantId
                 *
                 * @returns {Promise} a Promise object
                 */
                this.participantRegistered =
                        function participantRegistered(participantId) {
                            var i, msgContainer, messageQueue =
                                    settings.messageQueue.getAndRemoveMessages(participantId);

                            if (messageQueue !== undefined) {
                                i = messageQueue.length;
                                while (i--) {
                                    that.route(messageQueue[i]);
                                }
                            }
                        };

                /**
                 * Tell the message router that the given participantId is known. The message router
                 * checks internally if an address is already present in the routing table. If not,
                 * it adds the parentMessageRouterAddress to the routing table for this participantId.
                 * @function MessageRouter#setToKnown
                 *
                 * @param {String} participantId
                 *
                 * @returns void
                 */
                this.setToKnown = function setToKnown(participantId) {
                    if (!isReady()) {
                        log.debug("setToKnown: ignore call as message router is already shut down");
                        return;
                    }

                    //if not already set
                    if (routingTable[participantId] === undefined) {
                        if (parentMessageRouterAddress !== undefined) {
                            routingTable[participantId] = parentMessageRouterAddress;
                        }
                    }
                };

                this.hasMulticastReceivers = function() {
                    return Object.keys(multicastReceiversRegistry).length > 0;
                };

                /**
                 * Shutdown the message router
                 *
                 * @function
                 * @name MessageRouter#shutdown
                 */
                this.shutdown = function shutdown() {
                    if (queuedAddNextHopCalls !== undefined) {
                        queuedAddNextHopCalls.forEach(function(call) {
                            call.reject("Message Router has been shut down");
                        });
                        queuedAddNextHopCalls = [];
                    }
                    if (queuedRemoveNextHopCalls !== undefined) {
                        queuedRemoveNextHopCalls.forEach(function(call) {
                            call.reject("Message Router has been shut down");
                        });
                        queuedRemoveNextHopCalls = [];
                    }
                    if (queuedAddMulticastReceiverCalls !== undefined) {
                        queuedAddMulticastReceiverCalls.forEach(function(call) {
                            call.reject("Message Router has been shut down");
                        });
                        queuedAddMulticastReceiverCalls = [];
                    }
                    if (queuedRemoveMulticastReceiverCalls !== undefined) {
                        queuedRemoveMulticastReceiverCalls.forEach(function(call) {
                            call.reject("Message Router has been shut down");
                        });
                        queuedRemoveMulticastReceiverCalls = [];
                    }
                    started = false;
                    settings.messageQueue.shutdown();
                };
            }

            return MessageRouter;
        });
