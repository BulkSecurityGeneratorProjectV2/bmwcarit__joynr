package io.joynr.pubsub.publication;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.dispatcher.RequestCaller;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.dispatcher.rpc.ReflectionUtils;
import io.joynr.exceptions.JoynrException;
import io.joynr.messaging.MessagingQos;
import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.PubSubState;
import io.joynr.pubsub.SubscriptionQos;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import joynr.BroadcastSubscriptionRequest;
import joynr.OnChangeSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Maps;
import com.google.common.collect.Multimap;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class PublicationManagerImpl implements PublicationManager {
    private static final Logger logger = LoggerFactory.getLogger(PublicationManagerImpl.class);
    // Map ProviderId -> SubscriptionRequest
    private final Multimap<String, PublicationInformation> queuedSubscriptionRequests;
    // Map SubscriptionId -> SubscriptionRequest
    private final ConcurrentMap<String, PublicationInformation> subscriptionId2PublicationInformation;
    // Map SubscriptionId -> PubSubState
    private final ConcurrentMap<String, PubSubState> publicationStates;
    // Map SubscriptionId -> PublicationTimer
    private final ConcurrentMap<String, PublicationTimer> publicationTimers;
    // Map SubscriptionId -> ScheduledFuture
    private final ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures;
    // Map SubscriptionId -> UnregisterOnChange
    private final ConcurrentMap<String, UnregisterOnChange> unregisterOnChange;

    private AttributePollInterpreter attributePollInterpreter;
    private ScheduledExecutorService cleanupScheduler;
    private RequestReplySender requestReplySender;

    static class PublicationInformation {
        private String providerParticipantId;
        private String proxyParticipantId;
        private SubscriptionRequest subscriptionRequest;
        public PubSubState pubState;

        PublicationInformation(String providerParticipantId,
                               String proxyParticipantId,
                               SubscriptionRequest subscriptionRequest) {
            pubState = new PubSubState();
            this.setProviderParticipantId(providerParticipantId);
            this.subscriptionRequest = subscriptionRequest;
            this.setProxyParticipantId(proxyParticipantId);
        }

        public String getProviderParticipantId() {
            return providerParticipantId;
        }

        public void setProviderParticipantId(String providerParticipantId) {
            this.providerParticipantId = providerParticipantId;
        }

        public String getProxyParticipantId() {
            return proxyParticipantId;
        }

        public void setProxyParticipantId(String proxyParticipantId) {
            this.proxyParticipantId = proxyParticipantId;
        }

        public String getSubscriptionId() {
            return subscriptionRequest.getSubscriptionId();
        }

        @Override
        public boolean equals(Object arg0) {
            if (!(arg0 instanceof PublicationInformation)) {
                return false;
            }
            PublicationInformation pi = (PublicationInformation) arg0;
            return proxyParticipantId.equals(pi.proxyParticipantId)
                    && providerParticipantId.equals(pi.providerParticipantId)
                    && subscriptionRequest.equals(pi.subscriptionRequest);
        }

        @Override
        public int hashCode() {
            final int prime = 31;
            int result = 1;
            result = prime * result + ((proxyParticipantId == null) ? 0 : proxyParticipantId.hashCode());
            result = prime * result + ((providerParticipantId == null) ? 0 : providerParticipantId.hashCode());
            result = prime * result + ((subscriptionRequest == null) ? 0 : subscriptionRequest.hashCode());
            return result;
        }
    }

    @Inject
    public PublicationManagerImpl(AttributePollInterpreter attributePollInterpreter,
                                  RequestReplySender requestReplySender,
                                  @Named("joynr.scheduler.cleanup") ScheduledExecutorService cleanupScheduler) {
        super();
        this.requestReplySender = requestReplySender;
        this.cleanupScheduler = cleanupScheduler;
        this.queuedSubscriptionRequests = HashMultimap.create();
        this.subscriptionId2PublicationInformation = Maps.newConcurrentMap();
        this.publicationStates = Maps.newConcurrentMap();
        this.publicationTimers = Maps.newConcurrentMap();
        this.subscriptionEndFutures = Maps.newConcurrentMap();
        this.unregisterOnChange = Maps.newConcurrentMap();
        this.attributePollInterpreter = attributePollInterpreter;

    }

    PublicationManagerImpl(Multimap<String, PublicationInformation> queuedSubscriptionRequests,
                           ConcurrentMap<String, PublicationInformation> subscriptionId2SubscriptionRequest,
                           ConcurrentMap<String, PubSubState> publicationStates,
                           ConcurrentMap<String, PublicationTimer> publicationTimers,
                           ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures,
                           AttributePollInterpreter attributePollInterpreter,
                           RequestReplySender requestReplySender,
                           ScheduledExecutorService cleanupScheduler) {
        super();
        this.queuedSubscriptionRequests = queuedSubscriptionRequests;
        this.subscriptionId2PublicationInformation = subscriptionId2SubscriptionRequest;
        this.publicationStates = publicationStates;
        this.publicationTimers = publicationTimers;
        this.subscriptionEndFutures = subscriptionEndFutures;
        this.attributePollInterpreter = attributePollInterpreter;
        this.requestReplySender = requestReplySender;
        this.cleanupScheduler = cleanupScheduler;

        this.unregisterOnChange = Maps.newConcurrentMap();
    }

    private void handleSubscriptionRequest(PublicationInformation publicationInformation,
                                           SubscriptionRequest subscriptionRequest,
                                           RequestCaller requestCaller) {

        final String subscriptionId = subscriptionRequest.getSubscriptionId();
        SubscriptionQos subscriptionQos = subscriptionRequest.getQos();

        try {
            Method method = findGetterForAttributeName(requestCaller.getClass(),
                                                       subscriptionRequest.getSubscribedToName());

            // Send initial publication
            triggerPublication(publicationInformation, requestCaller, method);

            boolean hasSubscriptionHeartBeat = subscriptionQos instanceof HeartbeatSubscriptionInformation;
            boolean isKeepAliveSubscription = subscriptionQos instanceof OnChangeWithKeepAliveSubscriptionQos;

            if (hasSubscriptionHeartBeat || isKeepAliveSubscription) {
                final PublicationTimer timer = new PublicationTimer(publicationInformation.providerParticipantId,
                                                                    publicationInformation.proxyParticipantId,
                                                                    method,
                                                                    publicationInformation.pubState,
                                                                    subscriptionRequest,
                                                                    requestCaller,
                                                                    requestReplySender,
                                                                    attributePollInterpreter);

                timer.startTimer();
                publicationTimers.putIfAbsent(subscriptionId, timer);
            }

            // Handle onChange subscriptions
            if (subscriptionQos instanceof OnChangeSubscriptionQos) {
                AttributeListener attributeListener = new AttributeListenerImpl(subscriptionId, this);
                String attributeName = subscriptionRequest.getSubscribedToName();
                requestCaller.registerAttributeListener(attributeName, attributeListener);
                unregisterOnChange.putIfAbsent(subscriptionId, new UnregisterOnChange(requestCaller,
                                                                                      attributeName,
                                                                                      attributeListener));
            }
        } catch (NoSuchMethodException e) {
            cancelPublicationCreation(subscriptionId);
            logger.error("Error subscribing: {}. The provider does not have the requested attribute",
                         subscriptionRequest);
        } catch (IllegalArgumentException e) {
            cancelPublicationCreation(subscriptionId);
            logger.error("Error subscribing: " + subscriptionRequest, e);

        }

    }

    private void handleBroadcastSubscriptionRequest(String proxyParticipantId,
                                                    String providerParticipantId,
                                                    BroadcastSubscriptionRequest subscriptionRequest,
                                                    RequestCaller requestCaller) {
        logger.info("adding broadcast publication: " + subscriptionRequest.toString());

    }

    @Override
    public void addSubscriptionRequest(String proxyParticipantId,
                                       String providerParticipantId,
                                       SubscriptionRequest subscriptionRequest,
                                       RequestCaller requestCaller) {

        // Check that this is a valid subscription
        SubscriptionQos subscriptionQos = subscriptionRequest.getQos();
        long subscriptionEndDelay = subscriptionQos.getExpiryDate() == SubscriptionQos.NO_EXPIRY_DATE ? SubscriptionQos.NO_EXPIRY_DATE
                : subscriptionQos.getExpiryDate() - System.currentTimeMillis();

        if (subscriptionEndDelay < 0) {
            logger.error("Not adding subscription which ends in {} ms", subscriptionEndDelay);
            return;
        }

        // See if the publications for this subscription are already handled
        final String subscriptionId = subscriptionRequest.getSubscriptionId();
        if (publicationExists(subscriptionId)) {
            logger.info("Publication with id: " + subscriptionId + " already exists.");
            // TODO update subscription
            return;
        }

        logger.info("adding publication: " + subscriptionRequest.toString());
        PublicationInformation publicationInformation = new PublicationInformation(providerParticipantId,
                                                                                   proxyParticipantId,
                                                                                   subscriptionRequest);
        PublicationInformation existingSubscriptionRequest = subscriptionId2PublicationInformation.putIfAbsent(subscriptionId,
                                                                                                               publicationInformation);
        if (existingSubscriptionRequest != null) {
            // we only use putIfAbsent instead of .put, because putIfAbsent is threadsafe
            logger.debug("there already was a SubscriptionRequest with that subscriptionId in the map");
        }
        PubSubState existingPubSubState = publicationStates.putIfAbsent(subscriptionId, publicationInformation.pubState);
        if (existingPubSubState != null) {
            // we only use putIfAbsent instead of .put, because putIfAbsent is threadsafe
            logger.debug("there already was a pubState with that subscriptionId in the map");
        }

        if (subscriptionRequest instanceof BroadcastSubscriptionRequest) {
            handleBroadcastSubscriptionRequest(proxyParticipantId,
                                               providerParticipantId,
                                               (BroadcastSubscriptionRequest) subscriptionRequest,
                                               requestCaller);
        } else {
            handleSubscriptionRequest(publicationInformation, subscriptionRequest, requestCaller);
        }

        if (subscriptionQos.getExpiryDate() != SubscriptionQos.NO_EXPIRY_DATE) {
            // Create a runnable to remove the publication when the subscription expires
            ScheduledFuture<?> subscriptionEndFuture = cleanupScheduler.schedule(new Runnable() {

                @Override
                public void run() {
                    logger.info("Publication expired...");
                    removePublication(subscriptionId);
                }

            }, subscriptionEndDelay, TimeUnit.MILLISECONDS);
            subscriptionEndFutures.putIfAbsent(subscriptionId, subscriptionEndFuture);
        }
        logger.info("publication added: " + subscriptionRequest.toString());
    }

    private void cancelPublicationCreation(String subscriptionId) {
        subscriptionId2PublicationInformation.remove(subscriptionId);
        publicationStates.remove(subscriptionId);
        logger.error("Subscription request rejected. Removing publication.");
    }

    private boolean publicationExists(String subscriptionId) {
        return publicationStates.containsKey(subscriptionId);
    }

    @Override
    public void addSubscriptionRequest(String proxyParticipantId,
                                       String providerParticipantId,
                                       SubscriptionRequest subscriptionRequest) {
        logger.info("Adding subscription request for non existing provider to queue.");
        PublicationInformation publicationInformation = new PublicationInformation(providerParticipantId,
                                                                                   proxyParticipantId,
                                                                                   subscriptionRequest);
        queuedSubscriptionRequests.put(providerParticipantId, publicationInformation);
        subscriptionId2PublicationInformation.putIfAbsent(subscriptionRequest.getSubscriptionId(),
                                                          publicationInformation);
    }

    protected void removePublication(String subscriptionId) {

        if (subscriptionId2PublicationInformation.containsKey(subscriptionId)) {
            PublicationInformation publicationInformation = subscriptionId2PublicationInformation.get(subscriptionId);
            String providerParticipantId = publicationInformation.getProviderParticipantId();
            if (providerParticipantId != null && queuedSubscriptionRequests.containsKey(providerParticipantId)) {
                queuedSubscriptionRequests.removeAll(providerParticipantId);
            }
        }
        subscriptionId2PublicationInformation.remove(subscriptionId);
        if (publicationTimers.containsKey(subscriptionId)) {
            publicationTimers.get(subscriptionId).cancel();
            publicationTimers.remove(subscriptionId);
        }
        publicationStates.remove(subscriptionId);

        ScheduledFuture<?> future = subscriptionEndFutures.remove(subscriptionId);
        if (future != null) {
            future.cancel(true);
        }

        UnregisterOnChange onChange = unregisterOnChange.remove(subscriptionId);
        if (onChange != null) {
            onChange.unregister();
        }
    }

    // Class that holds information needed to unregister an onChange subscription
    static class UnregisterOnChange {
        final RequestCaller requestCaller;
        final String attributeName;
        final AttributeListener attributeListener;

        public UnregisterOnChange(RequestCaller requestCaller, String attributeName, AttributeListener attributeListener) {
            this.requestCaller = requestCaller;
            this.attributeName = attributeName;
            this.attributeListener = attributeListener;
        }

        public void unregister() {
            requestCaller.unregisterAttributeListener(attributeName, attributeListener);
        }
    }

    @Override
    public void stopPublication(String subscriptionId) {
        removePublication(subscriptionId);
    }

    @Override
    public void stopPublicationByProviderId(String providerId) {
        for (PublicationInformation publcationInformation : subscriptionId2PublicationInformation.values()) {
            if (publcationInformation.getProviderParticipantId().equals(providerId)) {
                removePublication(publcationInformation.getSubscriptionId());
            }
        }
    }

    @Override
    public void restoreQueuedSubscription(String providerId, RequestCaller requestCaller) {
        Collection<PublicationInformation> queuedRequests = queuedSubscriptionRequests.get(providerId);
        for (PublicationInformation publicInformation : queuedRequests) {
            if (System.currentTimeMillis() < publicInformation.subscriptionRequest.getQos().getExpiryDate()) {
                addSubscriptionRequest(publicInformation.getProxyParticipantId(),
                                       publicInformation.getProviderParticipantId(),
                                       publicInformation.subscriptionRequest,
                                       requestCaller);
            }
            queuedSubscriptionRequests.remove(providerId, publicInformation);
        }
    }

    @Override
    public void attributeValueChanged(String subscriptionId, Object value) {

        if (subscriptionId2PublicationInformation.containsKey(subscriptionId)) {
            PublicationInformation publicationInformation = subscriptionId2PublicationInformation.get(subscriptionId);

            PublicationTimer publicationTimer = publicationTimers.get(subscriptionId);
            if (publicationTimer != null) {
                // used by OnChangedWithKeepAlive
                publicationTimer.sendPublicationNow(value);
            } else {
                sendPublication(value, publicationInformation);
            }

            logger.info("attribute changed for subscription id: {} sending publication if delay > minInterval.",
                        subscriptionId);

        } else {
            logger.error("subscription {} has expired but attributeValueChanged has been called", subscriptionId);
            return;
        }

    }

    @Override
    public void eventOccurred(String subscriptionId, List<BroadcastFilter> filters, Object... values) {
        if (subscriptionId2PublicationInformation.containsKey(subscriptionId)) {
            PublicationInformation publicationInformation = subscriptionId2PublicationInformation.get(subscriptionId);

            if (processFilterChain(publicationInformation, filters, values)) {
                sendPublication(values, publicationInformation);
                logger.info("attribute changed for subscription id: {} sending publication if delay > minInterval.",
                            subscriptionId);
            }

        } else {
            logger.error("subscription {} has expired but eventOccurred has been called", subscriptionId);
            return;
        }

    }

    private boolean processFilterChain(PublicationInformation publicationInformation,
                                       List<BroadcastFilter> filters,
                                       Object[] values) {

        if (filters != null && filters.size() > 0) {
            boolean filterResult = true;
            BroadcastSubscriptionRequest subscriptionRequest = (BroadcastSubscriptionRequest) publicationInformation.subscriptionRequest;
            Map<String, Object> filterParameters = subscriptionRequest.getFilterParameters();

            for (BroadcastFilter filter : filters) {
                filterResult &= filter.filter(values, filterParameters);
            }
            return filterResult;
        } else {
            return true;
        }
    }

    private void sendPublication(Object value, PublicationInformation publicationInformation) {
        SubscriptionPublication publication = new SubscriptionPublication(value,
                                                                          publicationInformation.getSubscriptionId());
        try {
            MessagingQos messagingQos = new MessagingQos();
            messagingQos.setTtl_ms(publicationInformation.subscriptionRequest.getQos().getPublicationTtl());
            requestReplySender.sendSubscriptionPublication(publicationInformation.providerParticipantId,
                                                           publicationInformation.proxyParticipantId,
                                                           publication,
                                                           messagingQos);
            // TODO handle exceptions during publication. See JOYNR-2113
        } catch (JoynrException e) {
            logger.error("sendPublication error: {}", e.getMessage());
        } catch (JsonGenerationException e) {
            logger.error("sendPublication error: {}", e.getMessage());
        } catch (JsonMappingException e) {
            logger.error("sendPublication error: {}", e.getMessage());
        } catch (IOException e) {
            logger.error("sendPublication error: {}", e.getMessage());
        }
    }

    private void triggerPublication(PublicationInformation publicationInformation,
                                    RequestCaller requestCaller,
                                    Method method) {
        sendPublication(attributePollInterpreter.execute(requestCaller, method), publicationInformation);
    }

    private Method findGetterForAttributeName(Class<?> clazz, String attributeName) throws NoSuchMethodException {
        String attributeGetterName = "get" + attributeName.toUpperCase().charAt(0)
                + attributeName.subSequence(1, attributeName.length());
        return ReflectionUtils.findMethodByParamTypes(clazz, attributeGetterName, new Class[]{});

    }

    @Override
    public void shutdown() {

    }
}
