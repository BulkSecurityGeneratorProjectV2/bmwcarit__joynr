package io.joynr.capabilities;

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

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.ProviderDirectory;
import io.joynr.dispatching.RequestCaller;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessLibjoynrMessagingSkeleton;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.AbstractSubscriptionPublisher;
import io.joynr.provider.JoynrInterface;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.ProviderContainer;
import io.joynr.provider.ProviderContainerFactory;
import io.joynr.proxy.Callback;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class CapabilitiesRegistrarTests {

    private static final long ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    private final long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
    CapabilitiesRegistrar registrar;
    @Mock
    private LocalDiscoveryAggregator localDiscoveryAggregator;
    @Mock
    private ProviderDirectory providerDirectory;
    @Mock
    private ProviderContainerFactory providerContainerFactory;

    @Mock
    private MessageRouter messageRouter;

    @Mock
    private Dispatcher dispatcher;

    @Mock
    private TestProvider provider;

    @Mock
    private RequestCaller requestCaller;

    @Mock
    private AbstractSubscriptionPublisher subscriptionPublisher;

    @Mock
    private ProviderContainer providerContainer;

    @Mock
    private ParticipantIdStorage participantIdStorage;

    private String domain = "domain";
    private String participantId = "participantId";
    private ProviderQos providerQos = new ProviderQos();

    @JoynrInterface(provides = TestProvider.class, name = TestProvider.INTERFACE_NAME)
    interface TestProvider extends JoynrProvider {
        public static String INTERFACE_NAME = "interfaceName";
    }

    @Before
    public void setUp() {

        registrar = new CapabilitiesRegistrarImpl(localDiscoveryAggregator,
                                                  providerContainerFactory,
                                                  messageRouter,
                                                  providerDirectory,
                                                  participantIdStorage,
                                                  ONE_DAY_IN_MS,
                                                  new InProcessAddress(new InProcessLibjoynrMessagingSkeleton(dispatcher)));
    }

    @Test
    public void registerWithCapRegistrar() {

        when(providerContainer.getInterfaceName()).thenReturn(TestProvider.INTERFACE_NAME);
        RequestCaller requestCallerMock = mock(RequestCaller.class);
        when(providerContainer.getRequestCaller()).thenReturn(requestCallerMock);
        when(providerContainer.getSubscriptionPublisher()).thenReturn(subscriptionPublisher);
        when(participantIdStorage.getProviderParticipantId(eq(domain), eq(TestProvider.INTERFACE_NAME))).thenReturn(participantId);
        when(providerContainerFactory.create(provider)).thenReturn(providerContainer);

        registrar.registerProvider(domain, provider, providerQos);
        verify(localDiscoveryAggregator).add(any(Callback.class),
                                             eq(new DiscoveryEntry(new Version(47, 11),
                                                                   domain,
                                                                   TestProvider.INTERFACE_NAME,
                                                                   participantId,
                                                                   providerQos,
                                                                   System.currentTimeMillis(),
                                                                   expiryDateMs)));

        verify(providerDirectory).add(eq(participantId), eq(providerContainer));
    }

    @Test
    public void unregisterProvider() {
        when(providerContainer.getInterfaceName()).thenReturn(TestProvider.INTERFACE_NAME);
        when(participantIdStorage.getProviderParticipantId(eq(domain), eq(TestProvider.INTERFACE_NAME))).thenReturn(participantId);
        registrar.unregisterProvider(domain, provider);

        verify(localDiscoveryAggregator).remove(any(Callback.class), eq(participantId));
        verify(providerDirectory).remove(eq(participantId));
    }

}
