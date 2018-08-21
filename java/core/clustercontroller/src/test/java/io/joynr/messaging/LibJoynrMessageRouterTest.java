/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
package io.joynr.messaging;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.concurrent.DelayQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.common.ExpiryDate;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.routing.AddressManager;
import io.joynr.messaging.routing.DelayableImmutableMessage;
import io.joynr.messaging.routing.LibJoynrMessageRouter;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.MulticastReceiverRegistry;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.runtime.JoynrThreadFactory;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.statusmetrics.StatusReceiver;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.WebSocketAddress;

@RunWith(MockitoJUnitRunner.class)
public class LibJoynrMessageRouterTest {

    @Mock
    IMessagingStub messagingStub;
    @Mock
    private RoutingTable routingTable;
    @Mock
    private RoutingProxy messageRouterParent;
    @Mock
    private ChannelAddress parentAddress;
    @Mock
    private ChannelAddress nextHopAddress;
    @Mock
    private WebSocketAddress incomingAddress;
    @Mock
    private MessagingStubFactory messagingStubFactory;
    @Mock
    private MessagingSkeletonFactory messagingSkeletonFactory;
    @Mock
    private AddressManager addressManager;
    @Mock
    private MulticastReceiverRegistry multicastReceiverRegistry;
    @Mock
    private ImmutableMessage message;
    @Mock
    private ShutdownNotifier shutdownNotifier;

    private DelayQueue<DelayableImmutableMessage> messageQueue = new DelayQueue<>();
    private LibJoynrMessageRouter messageRouter;
    private String unknownParticipantId = "unknownParticipantId";
    private Long sendMsgRetryIntervalMs = 10L;
    private int maxParallelSends = 10;
    private long routingTableGracePeriodMs = 60000L;
    private long routingTableCleanupIntervalMs = 60000L;

    private String globalAddress = "global-address";

    @Before
    public void setUp() {
        when(message.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(1000000).getValue());
        when(message.isTtlAbsolute()).thenReturn(true);
        when(message.getRecipient()).thenReturn(unknownParticipantId);
        when(message.isLocalMessage()).thenReturn(false);
        when(message.getType()).thenReturn(Message.VALUE_MESSAGE_TYPE_REQUEST);

        when(routingTable.containsKey(unknownParticipantId)).thenReturn(false);
        when(messageRouterParent.resolveNextHop(unknownParticipantId)).thenReturn(true);
        when(messageRouterParent.getReplyToAddress()).thenReturn(globalAddress);
        when(messagingStubFactory.create(any(Address.class))).thenReturn(messagingStub);
        when(parentAddress.getChannelId()).thenReturn("LibJoynrMessageRouterTestChannel");

        messageRouter = new LibJoynrMessageRouter(routingTable,
                                                  incomingAddress,
                                                  provideMessageSchedulerThreadPoolExecutor(),
                                                  sendMsgRetryIntervalMs,
                                                  maxParallelSends,
                                                  routingTableGracePeriodMs,
                                                  routingTableCleanupIntervalMs,
                                                  messagingStubFactory,
                                                  messagingSkeletonFactory,
                                                  addressManager,
                                                  multicastReceiverRegistry,
                                                  messageQueue,
                                                  shutdownNotifier,
                                                  mock(StatusReceiver.class));
        messageRouter.setParentRouter(messageRouterParent, parentAddress, "parentParticipantId", "proxyParticipantId");
    }

    @Test
    public void itQueriesParentForNextHop() throws Exception {
        messageRouter.route(message);
        Thread.sleep(100);
        verify(messageRouterParent).resolveNextHop(eq(unknownParticipantId));
    }

    @Test
    public void addsNextHopAfterQueryingParent() throws Exception {
        messageRouter.route(message);
        Thread.sleep(100);
        final boolean isGloballyVisible = true;
        final long expiryDateMs = Long.MAX_VALUE;
        final boolean isSticky = false;
        final boolean allowUpdate = false;
        verify(routingTable).put(eq(unknownParticipantId),
                                 eq(parentAddress),
                                 eq(isGloballyVisible),
                                 eq(expiryDateMs),
                                 eq(isSticky),
                                 eq(allowUpdate));
    }

    @Test
    public void passesNextHopToParent() {
        final boolean isGloballyVisible = true;
        messageRouter.addNextHop(unknownParticipantId, nextHopAddress, isGloballyVisible);
        verify(messageRouterParent).addNextHop(eq(unknownParticipantId), eq(incomingAddress), eq(isGloballyVisible));
    }

    ScheduledExecutorService provideMessageSchedulerThreadPoolExecutor() {
        ThreadFactory schedulerNamedThreadFactory = new JoynrThreadFactory("joynr.MessageScheduler-scheduler");
        ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(2, schedulerNamedThreadFactory);
        scheduler.setKeepAliveTime(100, TimeUnit.SECONDS);
        scheduler.allowCoreThreadTimeOut(true);
        return scheduler;
    }

    @Test
    public void addMulticastReceiverForUnknownNotInProcessProvider() {
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(null);

        messageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        // we expect this to be called because the provider is not reachable via an InProcessAddress
        verify(messageRouterParent).addMulticastReceiver(eq(multicastId),
                                                         eq(subscriberParticipantId),
                                                         eq(providerParticipantId));
    }

    @Test
    public void addMulticastReceiverForInProcessProvider() {
        InProcessAddress mockInProcessAddress = mock(InProcessAddress.class);
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(mockInProcessAddress);

        messageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        // we don't expect this to be called
        verify(messageRouterParent, times(0)).addMulticastReceiver(any(String.class),
                                                                   any(String.class),
                                                                   any(String.class));
    }

    @Test
    public void addMulticastReceiverForNotInProcessProvider() {
        WebSocketAddress mockWebSocketAddress = mock(WebSocketAddress.class);
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(mockWebSocketAddress);

        messageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(messageRouterParent).addMulticastReceiver(eq(multicastId),
                                                         eq(subscriberParticipantId),
                                                         eq(providerParticipantId));
    }

    @Test
    public void removeMulticastReceiverForNotInProcessProvider() {
        WebSocketAddress mockWebSocketAddress = mock(WebSocketAddress.class);
        final String multicastId = "multicastIdTest";
        final String subscriberParticipantId = "subscriberParticipantIdTest";
        final String providerParticipantId = "providerParticipantIdTest";
        when(routingTable.get(providerParticipantId)).thenReturn(mockWebSocketAddress);

        messageRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        messageRouter.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        verify(messageRouterParent).removeMulticastReceiver(eq(multicastId),
                                                            eq(subscriberParticipantId),
                                                            eq(providerParticipantId));
    }
}
