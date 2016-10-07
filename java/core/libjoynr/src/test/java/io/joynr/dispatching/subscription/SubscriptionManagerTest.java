package io.joynr.dispatching.subscription;

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

import static org.hamcrest.Matchers.contains;
import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anySet;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.common.collect.Maps;
import com.google.common.collect.Sets;
import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.SubscriptionException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.util.MulticastWildcardRegexFactory;
import io.joynr.proxy.Future;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import joynr.MulticastPublication;
import joynr.OnChangeSubscriptionQos;
import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.tests.testBroadcastInterface.LocationUpdateBroadcastListener;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionManagerTest {

    private String attributeName;
    private AttributeSubscriptionAdapter<Integer> attributeSubscriptionCallback;

    private PeriodicSubscriptionQos qos;
    private OnChangeSubscriptionQos onChangeQos;

    private PeriodicSubscriptionQos qosWithoutExpiryDate;
    private MessagingQos qosSettings;

    @Mock
    ConcurrentMap<String, ScheduledFuture<?>> subscriptionEndFutures;

    private ConcurrentMap<String, AttributeSubscriptionListener<?>> attributeSubscriptionDirectory = spy(new ConcurrentHashMap<String, AttributeSubscriptionListener<?>>());
    private ConcurrentMap<String, BroadcastSubscriptionListener> broadcastSubscriptionDirectory = spy(new ConcurrentHashMap<String, BroadcastSubscriptionListener>());
    private ConcurrentMap<Pattern, Set<String>> multicastSubscribersDirectory = spy(new ConcurrentHashMap<Pattern, Set<String>>());
    private ConcurrentMap<String, PubSubState> subscriptionStates = spy(new ConcurrentHashMap<String, PubSubState>());
    private ConcurrentMap<String, MissedPublicationTimer> missedPublicationTimers = spy(new ConcurrentHashMap<String, MissedPublicationTimer>());
    private ConcurrentMap<String, Class<?>[]> subscriptionBroadcastTypes = spy(Maps.<String, Class<?>[]> newConcurrentMap());
    private ConcurrentMap<String, Future<String>> subscriptionFutureMap = spy(Maps.<String, Future<String>> newConcurrentMap());

    @Mock
    private PubSubState subscriptionState;

    private SubscriptionManager subscriptionManager;
    private String subscriptionId;
    private MissedPublicationTimer missedPublicationTimer;

    private String fromParticipantId;
    private String toParticipantId;
    private Future<String> future;

    @Mock
    private ConcurrentMap<String, Class<?>> subscriptionAttributeTypes;
    @Mock
    private ScheduledExecutorService cleanupScheduler;

    @Mock
    private Dispatcher dispatcher;

    @Mock
    private MulticastWildcardRegexFactory multicastWildcardRegexFactory;

    @Before
    public void setUp() {
        subscriptionManager = new SubscriptionManagerImpl(attributeSubscriptionDirectory,
                                                          broadcastSubscriptionDirectory,
                                                          multicastSubscribersDirectory,
                                                          subscriptionStates,
                                                          missedPublicationTimers,
                                                          subscriptionEndFutures,
                                                          subscriptionAttributeTypes,
                                                          subscriptionBroadcastTypes,
                                                          subscriptionFutureMap,
                                                          cleanupScheduler,
                                                          dispatcher,
                                                          multicastWildcardRegexFactory);
        subscriptionId = "testSubscription";

        attributeName = "testAttribute";
        attributeSubscriptionCallback = new AttributeSubscriptionAdapter<Integer>();
        long minInterval_ms = 100;
        long maxInterval_ms = 5000;
        long subscriptionDuration = 20000;
        long alertInterval_ms = 6000;
        long publicationTtl_ms = 1000;
        qos = new PeriodicSubscriptionQos();
        qos.setPeriodMs(maxInterval_ms);
        qos.setValidityMs(subscriptionDuration);
        qos.setAlertAfterIntervalMs(alertInterval_ms);
        qos.setPublicationTtlMs(publicationTtl_ms);

        onChangeQos = new OnChangeSubscriptionQos();
        onChangeQos.setMinIntervalMs(minInterval_ms);
        onChangeQos.setValidityMs(subscriptionDuration);
        onChangeQos.setPublicationTtlMs(publicationTtl_ms);

        qosWithoutExpiryDate = new PeriodicSubscriptionQos();
        qosWithoutExpiryDate.setPeriodMs(maxInterval_ms);
        qosWithoutExpiryDate.setValidityMs(SubscriptionQos.IGNORE_VALUE);
        qosWithoutExpiryDate.setAlertAfterIntervalMs(alertInterval_ms);
        qosWithoutExpiryDate.setPublicationTtlMs(publicationTtl_ms);

        missedPublicationTimer = new MissedPublicationTimer(System.currentTimeMillis() + subscriptionDuration,
                                                            maxInterval_ms,
                                                            alertInterval_ms,
                                                            attributeSubscriptionCallback,
                                                            subscriptionState,
                                                            subscriptionId);

        qosSettings = new MessagingQos();
        fromParticipantId = "fromParticipantId";
        toParticipantId = "toParticipantId";
        future = new Future<String>();
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                      JsonGenerationException, JsonMappingException, IOException {
        class IntegerReference extends TypeReference<Integer> {
        }

        Future<String> future = mock(Future.class);
        AttributeSubscribeInvocation subscriptionRequest = new AttributeSubscribeInvocation(attributeName,
                                                                                            IntegerReference.class,
                                                                                            attributeSubscriptionCallback,
                                                                                            qos,
                                                                                            future);
        subscriptionManager.registerAttributeSubscription(fromParticipantId,
                                                          Sets.newHashSet(toParticipantId),
                                                          subscriptionRequest);
        subscriptionId = subscriptionRequest.getSubscriptionId();

        verify(attributeSubscriptionDirectory).put(Mockito.anyString(), Mockito.eq(attributeSubscriptionCallback));
        verify(subscriptionStates).put(Mockito.anyString(), Mockito.any(PubSubState.class));

        verify(cleanupScheduler).schedule(Mockito.any(Runnable.class),
                                          Mockito.eq(qos.getExpiryDateMs()),
                                          Mockito.eq(TimeUnit.MILLISECONDS));
        verify(subscriptionEndFutures, Mockito.times(1)).put(Mockito.eq(subscriptionId),
                                                             Mockito.any(ScheduledFuture.class));

        verify(dispatcher).sendSubscriptionRequest(eq(fromParticipantId),
                                                   (Set<String>) argThat(contains(toParticipantId)),
                                                   any(SubscriptionRequest.class),
                                                   any(MessagingQos.class));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerBroadcastSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                               JsonGenerationException, JsonMappingException, IOException {
        String broadcastName = "broadcastName";
        BroadcastSubscriptionListener broadcastSubscriptionListener = mock(LocationUpdateBroadcastListener.class);
        BroadcastSubscribeInvocation subscriptionRequest = new BroadcastSubscribeInvocation(broadcastName,
                                                                                            broadcastSubscriptionListener,
                                                                                            onChangeQos,
                                                                                            future);
        subscriptionManager.registerBroadcastSubscription(fromParticipantId,
                                                          Sets.newHashSet(toParticipantId),
                                                          subscriptionRequest);
        subscriptionId = subscriptionRequest.getSubscriptionId();

        verify(broadcastSubscriptionDirectory).put(Mockito.anyString(), Mockito.eq(broadcastSubscriptionListener));
        verify(subscriptionStates).put(Mockito.anyString(), Mockito.any(PubSubState.class));

        verify(cleanupScheduler).schedule(Mockito.any(Runnable.class),
                                          Mockito.eq(qos.getExpiryDateMs()),
                                          Mockito.eq(TimeUnit.MILLISECONDS));
        verify(subscriptionEndFutures, Mockito.times(1)).put(Mockito.eq(subscriptionId),
                                                             Mockito.any(ScheduledFuture.class));

        verify(dispatcher).sendSubscriptionRequest(eq(fromParticipantId),
                                                   (Set<String>) argThat(contains(toParticipantId)),
                                                   any(SubscriptionRequest.class),
                                                   any(MessagingQos.class));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void registerSubscriptionWithoutExpiryDate() throws JoynrSendBufferFullException,
                                                       JoynrMessageNotSentException, JsonGenerationException,
                                                       JsonMappingException, IOException {
        class IntegerReference extends TypeReference<Integer> {
        }

        AttributeSubscribeInvocation request = new AttributeSubscribeInvocation(attributeName,
                                                                                IntegerReference.class,
                                                                                attributeSubscriptionCallback,
                                                                                qosWithoutExpiryDate,
                                                                                future);
        subscriptionId = request.getSubscriptionId();
        subscriptionManager.registerAttributeSubscription(fromParticipantId, Sets.newHashSet(toParticipantId), request);

        verify(attributeSubscriptionDirectory).put(Mockito.anyString(), Mockito.eq(attributeSubscriptionCallback));
        verify(subscriptionStates).put(Mockito.anyString(), Mockito.any(PubSubState.class));

        verify(cleanupScheduler, never()).schedule(Mockito.any(Runnable.class),
                                                   Mockito.anyLong(),
                                                   Mockito.any(TimeUnit.class));
        verify(subscriptionEndFutures, never()).put(Mockito.anyString(), Mockito.any(ScheduledFuture.class));

        verify(dispatcher).sendSubscriptionRequest(eq(fromParticipantId),
                                                   (Set<String>) argThat(contains(toParticipantId)),
                                                   any(SubscriptionRequest.class),
                                                   any(MessagingQos.class));
    }

    private static interface TestMulticastSubscriptionInterface {
        @JoynrMulticast(name = "myMulticast")
        void subscribeToMyMulticast();
    }

    @Test
    public void testRegisterMulticastSubscription() throws Exception {
        testRegisterMulticastSubscription(null);
    }

    @Test
    public void testRegisterMulticastSubscriptionWithExistingSubscriptionId() throws Exception {
        testRegisterMulticastSubscription(subscriptionId);
    }

    private void testRegisterMulticastSubscription(String subscriptionId) throws Exception {
        Method method = TestMulticastSubscriptionInterface.class.getMethod("subscribeToMyMulticast", new Class[0]);
        BroadcastSubscriptionListener listener = spy(new BroadcastSubscriptionListener() {
            @Override
            public void onError(SubscriptionException error) {
            }
            @Override
            public void onSubscribed(String subscriptionId) {
            }
            public void onReceive() {
            }
        });
        SubscriptionQos subscriptionQos = mock(OnChangeSubscriptionQos.class);
        Object[] args;
        if (subscriptionId == null) {
            args = new Object[] {listener, subscriptionQos };
        } else {
            args = new Object[] { subscriptionId, listener, subscriptionQos };
        }
        String multicastId = toParticipantId + "/myMulticast";
        Set<String> subscriptionIdSet = new HashSet<>();
        Pattern multicastIdPattern = Pattern.compile(multicastId);
        multicastSubscribersDirectory.put(multicastIdPattern, subscriptionIdSet);
        when(multicastWildcardRegexFactory.createIdPattern(multicastId)).thenReturn(multicastIdPattern);

        MulticastSubscribeInvocation invocation = new MulticastSubscribeInvocation(method, args, future);

        subscriptionManager.registerMulticastSubscription(fromParticipantId, Sets.newHashSet(toParticipantId), invocation);

        verify(multicastSubscribersDirectory).put(any(Pattern.class), anySet());
        assertEquals(1, subscriptionIdSet.size());
        if (subscriptionId != null) {
            assertEquals(subscriptionId, subscriptionIdSet.iterator().next());
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void unregisterSubscription() throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                        JsonGenerationException, JsonMappingException, IOException {

        when(subscriptionStates.get(subscriptionId)).thenReturn(subscriptionState);
        when(missedPublicationTimers.containsKey(subscriptionId)).thenReturn(true);
        when(missedPublicationTimers.get(subscriptionId)).thenReturn(missedPublicationTimer);
        subscriptionManager.unregisterSubscription(fromParticipantId,
                                                   Sets.newHashSet(toParticipantId),
                                                   subscriptionId,
                                                   qosSettings);

        verify(subscriptionStates).get(Mockito.eq(subscriptionId));
        verify(subscriptionState).stop();

        verify(dispatcher, times(1)).sendSubscriptionStop(Mockito.eq(fromParticipantId),
                                                          (Set<String>) argThat(contains(toParticipantId)),
                                                          Mockito.eq(new SubscriptionStop(subscriptionId)),
                                                          Mockito.any(MessagingQos.class));

    }

    @Test
    public void testHandleSubscriptionReplyWithError() {
        SubscriptionException subscriptionError = new SubscriptionException(subscriptionId);
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId, subscriptionError);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onFailure(eq(subscriptionError));
    }

    @Test
    public void testHandleSubscriptionReplyWithErrorWithSubscriptionListener() {
        SubscriptionException subscriptionError = new SubscriptionException(subscriptionId);
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId, subscriptionError);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        AttributeSubscriptionListener<?> subscriptionListener = mock(AttributeSubscriptionListener.class);
        attributeSubscriptionDirectory.put(subscriptionId, subscriptionListener);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onFailure(eq(subscriptionError));
        verify(subscriptionListener).onError(eq(subscriptionError));
    }

    @Test
    public void testHandleSubscriptionReplyWithErrorWithBroadcastListener() {
        SubscriptionException subscriptionError = new SubscriptionException(subscriptionId);
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId, subscriptionError);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        BroadcastSubscriptionListener broadcastListener = mock(BroadcastSubscriptionListener.class);
        broadcastSubscriptionDirectory.put(subscriptionId, broadcastListener);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onFailure(eq(subscriptionError));
        verify(broadcastListener).onError(eq(subscriptionError));
    }

    @Test
    public void testHandleSubscriptionReplyWithSuccess() {
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onSuccess(eq(subscriptionId));
    }

    @Test
    public void testHandleSubscriptionReplyWithSuccessWithSubscriptionListener() {
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        AttributeSubscriptionListener<?> subscriptionListener = mock(AttributeSubscriptionListener.class);
        attributeSubscriptionDirectory.put(subscriptionId, subscriptionListener);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onSuccess(eq(subscriptionId));
        verify(subscriptionListener).onSubscribed(eq(subscriptionId));
    }

    @Test
    public void testHandleSubscriptionReplyWithSuccessWithBroadcastListener() {
        SubscriptionReply subscriptionReply = new SubscriptionReply(subscriptionId);
        @SuppressWarnings("unchecked")
        Future<String> futureMock = mock(Future.class);
        subscriptionFutureMap.put(subscriptionId, futureMock);
        BroadcastSubscriptionListener broadcastListener = mock(BroadcastSubscriptionListener.class);
        broadcastSubscriptionDirectory.put(subscriptionId, broadcastListener);
        subscriptionManager.handleSubscriptionReply(subscriptionReply);
        verify(futureMock).onSuccess(eq(subscriptionId));
        verify(broadcastListener).onSubscribed(eq(subscriptionId));
    }

    private interface TestBroadcastListener extends BroadcastSubscriptionListener {
        void onReceive(String value);
    }

    @Test
    public void testHandleMulticastSubscriptionWithWildcardSubscribers() {
        MulticastPublication multicastPublication = new MulticastPublication(Arrays.asList("one"), "one/two/three");

        Pattern subscriberOnePattern = Pattern.compile("one/[^/]+/three");
        String subscriberOneId = "one";
        multicastSubscribersDirectory.putIfAbsent(subscriberOnePattern, Sets.newHashSet(subscriberOneId));

        Pattern subscriberTwoPattern = Pattern.compile("one/two/three");
        String subscriberTwoId = "two";
        multicastSubscribersDirectory.putIfAbsent(subscriberTwoPattern, Sets.newHashSet(subscriberTwoId));

        Pattern subscriberThreePattern = Pattern.compile("four/five/six");
        String subscriberThreeId = "three";
        multicastSubscribersDirectory.putIfAbsent(subscriberThreePattern, Sets.newHashSet(subscriberThreeId));

        Class[] types = new Class[]{ String.class };
        subscriptionBroadcastTypes.putIfAbsent(subscriberOneId, types);
        subscriptionBroadcastTypes.putIfAbsent(subscriberTwoId, types);
        subscriptionBroadcastTypes.putIfAbsent(subscriberThreeId, types);

        TestBroadcastListener listenerOne = mock(TestBroadcastListener.class);
        broadcastSubscriptionDirectory.putIfAbsent(subscriberOneId, listenerOne);

        TestBroadcastListener listenerTwo = mock(TestBroadcastListener.class);
        broadcastSubscriptionDirectory.putIfAbsent(subscriberTwoId, listenerTwo);

        TestBroadcastListener listenerThree = mock(TestBroadcastListener.class);
        broadcastSubscriptionDirectory.putIfAbsent(subscriberThreeId, listenerThree);

        subscriptionManager.handleMulticastPublication("one/two/three", new Object[]{ "value" });

        verify(listenerOne).onReceive(anyString());
        verify(listenerTwo).onReceive(anyString());
        verify(listenerThree, never()).onReceive(anyString());
    }

}
