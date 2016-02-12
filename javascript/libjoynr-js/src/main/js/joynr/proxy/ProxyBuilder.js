/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

define("joynr/proxy/ProxyBuilder", [
    "global/Promise",
    "joynr/proxy/ProxyAttributeNotifyReadWrite",
    "joynr/proxy/ProxyAttributeNotifyRead",
    "joynr/proxy/ProxyAttributeNotifyWrite",
    "joynr/proxy/ProxyAttributeNotify",
    "joynr/proxy/ProxyAttributeReadWrite",
    "joynr/proxy/ProxyAttributeRead",
    "joynr/proxy/ProxyAttributeWrite",
    "joynr/proxy/ProxyOperation",
    "joynr/proxy/ProxyEvent",
    "uuid",
    "joynr/proxy/DiscoveryQos",
    "joynr/messaging/MessagingQos",
    "joynr/types/TypeRegistrySingleton",
    "joynr/util/Util",
    "joynr/system/LoggerFactory"
], function(
        Promise,
        ProxyAttributeNotifyReadWrite,
        ProxyAttributeNotifyRead,
        ProxyAttributeNotifyWrite,
        ProxyAttributeNotify,
        ProxyAttributeReadWrite,
        ProxyAttributeRead,
        ProxyAttributeWrite,
        ProxyOperation,
        ProxyEvent,
        uuid,
        DiscoveryQos,
        MessagingQos,
        TypeRegistrySingleton,
        Util,
        LoggerFactory) {

    var proxyElementTypes = {
        ProxyAttributeNotifyReadWrite : ProxyAttributeNotifyReadWrite,
        ProxyAttributeNotifyRead : ProxyAttributeNotifyRead,
        ProxyAttributeNotifyWrite : ProxyAttributeNotifyWrite,
        ProxyAttributeNotify : ProxyAttributeNotify,
        ProxyAttributeReadWrite : ProxyAttributeReadWrite,
        ProxyAttributeRead : ProxyAttributeRead,
        ProxyAttributeWrite : ProxyAttributeWrite,
        ProxyOperation : ProxyOperation,
        ProxyEvent : ProxyEvent
    };
    var typeRegistry = TypeRegistrySingleton.getInstance();

    /**
     * @name ProxyBuilder
     * @constructor
     *
     * @param {Object}
     *            proxyDependencies injected for the children of the proxyBuilder and its children
     * @param {Arbitrator}
     *            proxyDependencies.arbitrator used by the proxyBuilder to find the correct provider
     * @param {RequestReplyManager}
     *            proxyDependencies.requestReplyManager passed on to proxyAttribute and
     *            proxyOperation
     * @param {SubscriptionManager}
     *            proxyDependencies.subscriptionManager passed on to proxyAttribute and
     *            proxyOperation
     * @param {PublicationManager}
     *            proxyDependencies.publicationManager passed on to proxyAttribute and
     *            proxyOperation
     * @param {Object}
     *            dependencies injected for the proxyBuilder
     * @param {MessageRouter}
     *            dependencies.messageRouter the message router
     * @param {LoggingManager}
     *            dependencies.loggingManager provides the logging context
     * @param {Address}
     *            dependencies.libjoynrMessagingAddress address to this libjoynr's message receiver
     * @param {TypeRegistry}
     *            dependencies.typeRegistry the typeRegistry being able to augment raw objects with
     *            type information
     */
    function ProxyBuilder(proxyDependencies, dependencies) {
        var arbitrator = proxyDependencies.arbitrator;
        var log = LoggerFactory.getLogger("joynr.proxy.ProxyBuilder");
        var typeRegisteredTimeout_ms = 3000; //3 secs

        /**
         * A function that constructs, arbitrates a object and provides the result and error using
         * an A+ promise
         *
         * @name ProxyBuilder#build
         * @function
         *
         * @param {Function}
         *            ProxyConstructor - the constructor function of the generated Proxy that
         *            creates a new proxy instance
         * @param {Object}
         *            settings - the settings object that is passed to the constructor when building
         *            the object
         * @param {String}
         *            settings.domain - the domain on which the provider should be looked for
         * @param {DiscoveryQos}
         *            settings.discoveryQos - jsthe settings object determining arbitration
         *            parameters
         * @param {MessagingQos}
         *            settings.messagingQos - the settings object determining arbitration parameters
         * @param {Boolean}
         *            settings.freeze - define if the returned proxy object should be frozen
         * @param {Object}
         *            settings.loggingContext - optional logging context will be appended to logging
         *            messages created in the name of
         *            this proxy
         * @returns {Object} an A Promise object, that will provide the proxy object upon completed
         *            arbitration, callback signatures: then({*Proxy} proxy), {Error} error)
         * @throws {Error}
         *             if arbitrator was not provided
         */
        this.build =
                function build(ProxyConstructor, settings) {
                    // augment Qos objects if they're missing
                    settings.discoveryQos = new DiscoveryQos(settings.discoveryQos);
                    settings.messagingQos = new MessagingQos(settings.messagingQos);

                    // check if objects are there and of correct type
                    Util.checkProperty(settings, "Object", "settings");
                    Util.checkProperty(settings.domain, "String", "settings.domain");
                    Util
                            .checkProperty(
                                    settings.discoveryQos,
                                    DiscoveryQos,
                                    "settings.discoveryQos");
                    Util
                            .checkProperty(
                                    settings.messagingQos,
                                    MessagingQos,
                                    "settings.messagingQos");

                    if (!arbitrator) {
                        throw new Error("value 'arbitrator' is undefined");
                    }

                    settings.dependencies = proxyDependencies;
                    settings.proxyElementTypes = proxyElementTypes;
                    var proxy = new ProxyConstructor(settings);
                    proxy.domain = settings.domain;
                    proxy.proxyParticipantId =
                            "proxy"
                                + "."
                                + settings.domain
                                + "."
                                + proxy.interfaceName
                                + "."
                                + uuid();
                    proxy.messagingQos = settings.messagingQos;

                    var datatypePromises =
                            ProxyConstructor.getUsedDatatypes().map(
                                    function(datatype) {
                                        return typeRegistry.getTypeRegisteredPromise(
                                                datatype,
                                                typeRegisteredTimeout_ms);
                                    });

                    return Promise.all(datatypePromises).then(
                            function() {
                                return arbitrator.startArbitration({
                                    domain : proxy.domain,
                                    interfaceName : proxy.interfaceName,
                                    discoveryQos : settings.discoveryQos,
                                    staticArbitration : settings.staticArbitration
                                }).then(
                                        function(arbitratedCaps) {
                                            if (settings.loggingContext !== undefined) {
                                                dependencies.loggingManager.setLoggingContext(
                                                        proxy.proxyParticipantId,
                                                        settings.loggingContext);
                                            }
                                            if (arbitratedCaps && arbitratedCaps.length > 0) {
                                                proxy.providerParticipantId =
                                                        arbitratedCaps[0].participantId;
                                            }
                                            dependencies.messageRouter.addNextHop(
                                                    proxy.proxyParticipantId,
                                                    dependencies.libjoynrMessagingAddress);

                                            var freeze =
                                                    settings.freeze === undefined
                                                        || settings.freeze;
                                            if (freeze) {
                                                // make proxy object immutable and return asynchronously
                                                proxy = Object.freeze(proxy);
                                            }

                                            return proxy;
                                        });
                            });
                };
    }

    return ProxyBuilder;
});
