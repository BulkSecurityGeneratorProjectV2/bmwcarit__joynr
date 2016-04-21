package io.joynr.runtime;

import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.routing.ChildMessageRouter;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.Address;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

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

public class LibjoynrRuntime extends JoynrRuntimeImpl {

    public static final Logger logger = LoggerFactory.getLogger(LibjoynrRuntime.class);

    // CHECKSTYLE:OFF
    @Inject
    public LibjoynrRuntime(ObjectMapper objectMapper,
                           ProxyBuilderFactory proxyBuilderFactory,
                           RequestCallerDirectory requestCallerDirectory,
                           ReplyCallerDirectory replyCallerDirectory,
                           Dispatcher dispatcher,
                           MessagingStubFactory messagingStubFactory,
                           MessagingSkeletonFactory messagingSkeletonFactory,
                           LocalDiscoveryAggregator localDiscoveryAggregator,
                           @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                           @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                           @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS) Address capabilitiesDirectoryAddress,
                           @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_ADDRESS) Address domainAccessControllerAddress,
                           @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address discoveryProviderAddress,
                           @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address ccMessagingAddress,
                           ChildMessageRouter messageRouter,
                           @Named(SystemServicesSettings.PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID) String parentRoutingProviderParticipantId) {
        super(objectMapper,
              proxyBuilderFactory,
              requestCallerDirectory,
              replyCallerDirectory,
              dispatcher,
              messagingStubFactory,
              messagingSkeletonFactory,
              localDiscoveryAggregator,
              systemServicesDomain,
              dispatcherAddress,
              capabilitiesDirectoryAddress,
              domainAccessControllerAddress,
              discoveryProviderAddress);
        // CHECKSTYLE:ON
        ProxyBuilder<RoutingProxy> proxyBuilder = getProxyBuilder(systemServicesDomain, RoutingProxy.class);
        RoutingProxy routingProxy = proxyBuilder.build();
        messageRouter.setParentRouter(routingProxy,
                                      ccMessagingAddress,
                                      parentRoutingProviderParticipantId,
                                      proxyBuilder.getParticipantId());
        messageRouter.addNextHop(discoveryProxyParticipantId, dispatcherAddress);
    }
}
