package io.joynr.runtime;

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

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.ProviderDirectory;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessLibjoynrMessagingSkeleton;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.subtypes.JoynrType;

import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;

import joynr.BroadcastSubscriptionRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.system.DiscoveryProxy;
import joynr.system.RoutingTypes.Address;
import joynr.types.ProviderQos;

import org.reflections.Reflections;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

abstract public class JoynrRuntimeImpl implements JoynrRuntime {

    private static final Logger logger = LoggerFactory.getLogger(JoynrRuntimeImpl.class);

    @Inject
    private CapabilitiesRegistrar capabilitiesRegistrar;
    @Inject
    private RequestReplyManager requestReplyManager;
    @Inject
    private PublicationManager publicationManager;
    private Dispatcher dispatcher;

    @Inject
    public ObjectMapper objectMapper;

    @Inject
    @Named(JOYNR_SCHEDULER_CLEANUP)
    ScheduledExecutorService cleanupScheduler;

    private final ProxyBuilderFactory proxyBuilderFactory;

    protected final ProviderDirectory requestCallerDirectory;
    protected final ReplyCallerDirectory replyCallerDirectory;
    protected final String discoveryProxyParticipantId;

    private MessagingStubFactory messagingStubFactory;
    private MessagingSkeletonFactory messagingSkeletonFactory;

    // CHECKSTYLE:OFF
    @Inject
    public JoynrRuntimeImpl(ObjectMapper objectMapper,
                            ProxyBuilderFactory proxyBuilderFactory,
                            ProviderDirectory requestCallerDirectory,
                            ReplyCallerDirectory replyCallerDirectory,
                            Dispatcher dispatcher,
                            MessagingStubFactory messagingStubFactory,
                            MessagingSkeletonFactory messagingSkeletonFactory,
                            LocalDiscoveryAggregator localDiscoveryAggregator,
                            @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                            @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                            @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS) Address capabilitiesDirectoryAddress,
                            @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_ADDRESS) Address channelUrlDirectoryAddress,
                            @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_ADDRESS) Address domainAccessControllerAddress,
                            @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address discoveryProviderAddress) {
        // CHECKSTYLE:ON
        this.requestCallerDirectory = requestCallerDirectory;
        this.replyCallerDirectory = replyCallerDirectory;
        this.dispatcher = dispatcher;
        this.objectMapper = objectMapper;
        this.messagingStubFactory = messagingStubFactory;
        this.messagingSkeletonFactory = messagingSkeletonFactory;

        Reflections reflections = new Reflections("joynr");
        Set<Class<? extends JoynrType>> subClasses = reflections.getSubTypesOf(JoynrType.class);
        objectMapper.registerSubtypes(subClasses.toArray(new Class<?>[subClasses.size()]));

        Class<?>[] messageTypes = new Class[]{ Request.class, Reply.class, SubscriptionRequest.class,
                SubscriptionStop.class, SubscriptionPublication.class, BroadcastSubscriptionRequest.class };
        objectMapper.registerSubtypes(messageTypes);
        this.proxyBuilderFactory = proxyBuilderFactory;
        if (dispatcherAddress instanceof InProcessAddress) {
            ((InProcessAddress) dispatcherAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }
        if (channelUrlDirectoryAddress instanceof InProcessAddress) {
            ((InProcessAddress) channelUrlDirectoryAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }
        if (capabilitiesDirectoryAddress instanceof InProcessAddress) {
            ((InProcessAddress) capabilitiesDirectoryAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }
        if (domainAccessControllerAddress instanceof InProcessAddress) {
            ((InProcessAddress) domainAccessControllerAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }

        if (discoveryProviderAddress instanceof InProcessAddress) {
            ((InProcessAddress) discoveryProviderAddress).setSkeleton(new InProcessLibjoynrMessagingSkeleton(dispatcher));
        }

        ProxyBuilder<DiscoveryProxy> discoveryProxyBuilder = getProxyBuilder(systemServicesDomain, DiscoveryProxy.class);
        discoveryProxyParticipantId = discoveryProxyBuilder.getParticipantId();
        localDiscoveryAggregator.setDiscoveryProxy(discoveryProxyBuilder.build());

        messagingSkeletonFactory.start();
    }

    @Override
    public <T extends JoynrInterface> ProxyBuilder<T> getProxyBuilder(final String domain, final Class<T> interfaceClass) {

        if (domain == null || domain.isEmpty()) {
            throw new IllegalArgumentException("Cannot create ProxyBuilder: domain was not set");

        }

        if (interfaceClass == null) {
            throw new IllegalArgumentException("Cannot create ProxyBuilder: interfaceClass may not be NULL");
        }

        return proxyBuilderFactory.get(domain, interfaceClass);
    }

    /**
     * Registers a provider in the joynr framework
     *
     * @deprecated Will be removed by end of the year 2016. Use {@link io.joynr.runtime.JoynrRuntimeImpl#registerProvider(String, Object, ProviderQos)} instead.
     * @param domain
     *            The domain the provider should be registered for. Has to be identical at the client to be able to find
     *            the provider.
     * @param provider
     *            Instance of the provider implementation (has to extend a generated ...AbstractProvider).
     * @return Returns a Future which can be used to check the registration status.
     */
    @Deprecated
    @Override
    public Future<Void> registerProvider(String domain, AbstractJoynrProvider provider) {
        return capabilitiesRegistrar.registerProvider(domain, provider, provider.getProviderQos());
    }

    @Override
    public Future<Void> registerProvider(String domain, Object provider, ProviderQos providerQos) {
        return capabilitiesRegistrar.registerProvider(domain, provider, providerQos);
    }

    @Override
    public void unregisterProvider(String domain, Object provider) {
        capabilitiesRegistrar.unregisterProvider(domain, provider);

    }

    @Override
    public void shutdown(boolean clear) {
        logger.info("SHUTTING DOWN runtime");
        //TODO: this will be inverted, with elements needing shutdown registering themselves
        try {
            messagingSkeletonFactory.shutdown();
        } catch (Exception e) {
            logger.error("error shutting down skeletons: {}", e.getMessage());
        }
        try {
            capabilitiesRegistrar.shutdown(clear);
        } catch (Exception e) {
            logger.error("error clearing capabiltities while shutting down: {}", e.getMessage());
        }
        try {
            requestReplyManager.shutdown();
        } catch (Exception e) {
            logger.error("error shutting down requestReplyManager: {}", e.getMessage());
        }
        try {
            publicationManager.shutdown();
        } catch (Exception e) {
            logger.error("error shutting down publicationManager: {}", e.getMessage());
        }
        try {
            dispatcher.shutdown(clear);
        } catch (Exception e) {
            logger.error("error shutting down dispatcher: {}", e.getMessage());
        }
        try {
            messagingStubFactory.shutdown();
        } catch (Exception e) {
            logger.error("error shutting down messagingStubFactory: {}", e.getMessage());
        }
        try {
            cleanupScheduler.shutdownNow();
        } catch (Exception e) {
            logger.error("error shutting down queue cleanup scheduler: {}", e.getMessage());
        }
    }
}
