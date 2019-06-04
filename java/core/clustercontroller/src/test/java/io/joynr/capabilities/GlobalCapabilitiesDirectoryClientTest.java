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
package io.joynr.capabilities;

import static io.joynr.runtime.SystemServicesSettings.PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyListOf;
import static org.mockito.Matchers.anyObject;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Properties;
import java.util.stream.Collectors;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.util.StringArrayMatcher;
import joynr.Message;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;

@RunWith(MockitoJUnitRunner.class)
public class GlobalCapabilitiesDirectoryClientTest {
    private static final long DEFAULT_TTL_ADD_AND_REMOVE = 60L * 1000L;
    private static final long CUSTOM_TTL = 3L * 1000L;
    private static final long FRESHNESS_UPDATE_INTERVAL_MS = 42;
    private static final String GBID_DEFAULT_BACKEND = "joynrbackend1";
    private static final String GBID_OTHER_BACKEND = "joynrbackend2";
    private static final String[] GBIDS_ARRAY_PROPERTY_SETTING = { GBID_DEFAULT_BACKEND, GBID_OTHER_BACKEND };

    @Mock
    private ProxyBuilder<GlobalCapabilitiesDirectoryProxy> capabilitiesProxyBuilderMock;

    @Mock
    private GlobalCapabilitiesDirectoryProxy globalCapabilitiesDirectoryProxyMock;

    @Mock
    private CallbackWithModeledError<Void, DiscoveryError> callbackWithModeledErrorMock;

    @Mock
    GlobalDiscoveryEntry capabilitiesDirectoryEntryMock;

    @Mock
    private ProxyBuilderFactory proxyBuilderFactoryMock;

    private GlobalCapabilitiesDirectoryClient subject;

    private final MessagingQos expectedGcdCallMessagingQos = new MessagingQos();

    @Before
    public void setup() {
        final String domainMock = "domainMock";
        when(capabilitiesDirectoryEntryMock.getDomain()).thenReturn(domainMock);

        Properties properties = new Properties();
        properties.put(PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS,
                       String.valueOf(FRESHNESS_UPDATE_INTERVAL_MS));

        subject = createGCDClientWithProperties(properties);

        when(proxyBuilderFactoryMock.get(domainMock,
                                         GlobalCapabilitiesDirectoryProxy.class)).thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.setDiscoveryQos(any(DiscoveryQos.class))).thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.setMessagingQos(any(MessagingQos.class))).thenReturn(capabilitiesProxyBuilderMock);
        when(capabilitiesProxyBuilderMock.build()).thenReturn(globalCapabilitiesDirectoryProxyMock);

        // expect default ttl if not changed in the test case
        expectedGcdCallMessagingQos.setTtl_ms(DEFAULT_TTL_ADD_AND_REMOVE);
        // expect the default backend in the custom header if not changed in the test case
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, GBID_DEFAULT_BACKEND);
    }

    private GlobalCapabilitiesDirectoryClient createGCDClientWithProperties(final Properties properties) {
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            public void configure() {
                bind(ProxyBuilderFactory.class).toInstance(proxyBuilderFactoryMock);
                bind(GlobalDiscoveryEntry.class).annotatedWith(Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY))
                                                .toInstance(capabilitiesDirectoryEntryMock);
                bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                    .toInstance(GBIDS_ARRAY_PROPERTY_SETTING);
            }
        }, new JoynrPropertiesModule(properties));

        return injector.getInstance(GlobalCapabilitiesDirectoryClient.class);
    }

    @Test
    public void testAdd() {
        String[] gbids = new String[]{ GBID_DEFAULT_BACKEND, GBID_OTHER_BACKEND };
        // given some discovery entry
        GlobalDiscoveryEntry capabilitiesDirectoryEntryMock = mock(GlobalDiscoveryEntry.class);

        // when we call the add method with it
        subject.add(callbackWithModeledErrorMock, capabilitiesDirectoryEntryMock, gbids);

        // then the GCD proxy is called with the expected parameters and QoS
        verify(globalCapabilitiesDirectoryProxyMock).add(eq(callbackWithModeledErrorMock),
                                                         eq(capabilitiesDirectoryEntryMock),
                                                         argThat(new StringArrayMatcher(gbids)),
                                                         eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testAddWithCustomTTL() {
        // given a GCD client with custom ttl...
        final String[] gbids = new String[]{ GBID_DEFAULT_BACKEND };
        Properties properties = new Properties();
        properties.put(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_GLOBAL_ADD_AND_REMOVE_TTL_MS,
                       String.valueOf(CUSTOM_TTL));
        GlobalCapabilitiesDirectoryClient subjectInject = createGCDClientWithProperties(properties);
        // ...and some discovery entry
        GlobalDiscoveryEntry capabilitiesDirectoryEntryMock = mock(GlobalDiscoveryEntry.class);

        // when we call the add method on this client
        subjectInject.add(callbackWithModeledErrorMock, capabilitiesDirectoryEntryMock, gbids);

        // then the GCD proxy is called with the expected parameters and QoS
        expectedGcdCallMessagingQos.setTtl_ms(CUSTOM_TTL);
        verify(globalCapabilitiesDirectoryProxyMock).add(eq(callbackWithModeledErrorMock),
                                                         eq(capabilitiesDirectoryEntryMock),
                                                         argThat(new StringArrayMatcher(gbids)),
                                                         eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testAddWithNonDefaultSingleGbid() {
        // given a desired gbid and some global discovery entry
        final String targetGbid = "myjoynrbackend";
        final String[] gbids = new String[]{ targetGbid };
        final GlobalDiscoveryEntry capabilitiesDirectoryEntryMock = mock(GlobalDiscoveryEntry.class);
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbid);

        // when we call the add method with them
        subject.add(callbackWithModeledErrorMock, capabilitiesDirectoryEntryMock, gbids);

        // then the custom header in the GCD proxy call contains the desired gbid
        // and the call as well gets the desired callback and global discovery entry
        verify(globalCapabilitiesDirectoryProxyMock).add(eq(callbackWithModeledErrorMock),
                                                         eq(capabilitiesDirectoryEntryMock),
                                                         argThat(new StringArrayMatcher(gbids)),
                                                         eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testRemoveSingleParticipant() {
        // given some participantId
        final String testParticipantId = "testParticipantId";
        final String[] targetGbids = new String[]{ "myjoynrbackend", "myjoynrbackend2" };
        // when we call the remove method with it
        subject.remove(callbackWithModeledErrorMock, testParticipantId, targetGbids);

        // then the GCD proxy is called with the expected parameters and QoS
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbids[0]);
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(callbackWithModeledErrorMock),
                                                            eq(testParticipantId),
                                                            eq(targetGbids.clone()),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testRemoveSingleParticipantWithCustomTTL() {
        // given a GCD client with custom ttl...
        Properties properties = new Properties();
        properties.put(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_GLOBAL_ADD_AND_REMOVE_TTL_MS,
                       String.valueOf(CUSTOM_TTL));
        GlobalCapabilitiesDirectoryClient subjectInject = createGCDClientWithProperties(properties);
        // ...and some participantId list
        final String testParticipantId = "testParticipantId";
        final String[] targetGbids = new String[]{ "myjoynrbackend" };
        // when we call the remove method on this client
        subjectInject.remove(callbackWithModeledErrorMock, testParticipantId, targetGbids);

        // then the GCD proxy is called with the expected parameters and QoS
        expectedGcdCallMessagingQos.setTtl_ms(CUSTOM_TTL);
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbids[0]);
        verify(globalCapabilitiesDirectoryProxyMock).remove(eq(callbackWithModeledErrorMock),
                                                            eq(testParticipantId),
                                                            eq(targetGbids),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test(expected = IllegalStateException.class)
    public void testFailOnRemoveWithoutGbids() {
        // given some participantId
        final String testParticipantId = "testParticipantId";
        final String[] targetGbids = new String[]{};

        // when we call the remove method with it
        subject.remove(callbackWithModeledErrorMock, testParticipantId, targetGbids);
    }

    @Test
    public void testLookupParticipantId() {
        // given some callback...
        @SuppressWarnings("unchecked")
        Callback<GlobalDiscoveryEntry> callbackGlobalDiscoveryEntryMock = mock(Callback.class);
        // ...and a participantId
        final String testParticipantId = "testParticipantId";

        // when we call the lookup method with a custom ttl
        subject.lookup(callbackGlobalDiscoveryEntryMock, testParticipantId, CUSTOM_TTL);

        // then the GCD proxy is called with the expected parameters and QoS
        expectedGcdCallMessagingQos.setTtl_ms(CUSTOM_TTL);
        verify(globalCapabilitiesDirectoryProxyMock).lookup(eq(callbackGlobalDiscoveryEntryMock),
                                                            eq(testParticipantId),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testLookupParticipantIdWithGbid() {
        // given a desired gbid, some participantId and a callback
        @SuppressWarnings("unchecked")
        Callback<GlobalDiscoveryEntry> callbackGlobalDiscoveryEntryMock = mock(Callback.class);
        final String testParticipantId = "testParticipantId";
        final String targetGbid = "myjoynrbackend";

        // when we call the lookup method with them as well as with a custom ttl
        subject.lookup(callbackGlobalDiscoveryEntryMock, testParticipantId, CUSTOM_TTL, targetGbid);

        // then the custom header in the GCD proxy call contains the desired gbid
        // and the call as well gets the desired callback and participantId
        expectedGcdCallMessagingQos.setTtl_ms(CUSTOM_TTL);
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbid);
        verify(globalCapabilitiesDirectoryProxyMock).lookup(eq(callbackGlobalDiscoveryEntryMock),
                                                            eq(testParticipantId),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testLookupDomainsWithGbid() {
        // given some callback
        @SuppressWarnings("unchecked")
        Callback<List<GlobalDiscoveryEntry>> callbackListOfGlobalDiscoveryEntriesMock = mock(Callback.class);
        // ...and an interface plus an array of domains
        String[] domainsStrArrayDummy = new String[]{ "dummyDomain1", "dummyDomain2", "dummyDomain3" };
        String interfaceNameDummy = "interfaceNameDummy";
        // ...and a desired backend
        final String targetGbid = "myjoynrbackend";

        // when we call this GCD client with them as well as with custom ttl
        subject.lookup(callbackListOfGlobalDiscoveryEntriesMock,
                       domainsStrArrayDummy,
                       interfaceNameDummy,
                       CUSTOM_TTL,
                       targetGbid);

        // then the GCD proxy is called with the expected parameters
        expectedGcdCallMessagingQos.setTtl_ms(CUSTOM_TTL);
        expectedGcdCallMessagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, targetGbid);
        verify(globalCapabilitiesDirectoryProxyMock).lookup(Mockito.<Callback<GlobalDiscoveryEntry[]>> any(),
                                                            eq(domainsStrArrayDummy),
                                                            eq(interfaceNameDummy),
                                                            eq(expectedGcdCallMessagingQos));
    }

    private void checkLookupDomainsIsCalledCorrectly(Callback<List<GlobalDiscoveryEntry>> callbackListOfGlobalDiscoveryEntriesMock) {
        // given some callback (the method parameter)...
        // ...and an interface plus an array of domains
        String[] domainsStrArrayDummy = new String[]{ "dummyDomain1", "dummyDomain2", "dummyDomain3" };
        String interfaceNameDummy = "interfaceNameDummy";

        // when we call this GCD client with them as well as with custom ttl
        subject.lookup(callbackListOfGlobalDiscoveryEntriesMock, domainsStrArrayDummy, interfaceNameDummy, CUSTOM_TTL);

        // then the GCD proxy is called with the expected parameters
        expectedGcdCallMessagingQos.setTtl_ms(CUSTOM_TTL);
        verify(globalCapabilitiesDirectoryProxyMock).lookup(Mockito.<Callback<GlobalDiscoveryEntry[]>> any(),
                                                            eq(domainsStrArrayDummy),
                                                            eq(interfaceNameDummy),
                                                            eq(expectedGcdCallMessagingQos));
    }

    @Test
    public void testLookupDomainsOnFailure() {
        // given the GCD proxy returns an error on lookup with domain/interface...
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                Callback<GlobalDiscoveryEntry[]> callback = (Callback<GlobalDiscoveryEntry[]>) invocation.getArguments()[0];
                JoynrRuntimeException error = new JoynrRuntimeException();
                callback.onFailure(error);
                return null;
            }
        }).when(globalCapabilitiesDirectoryProxyMock)
          .lookup(anyObject(), any(String[].class), any(String.class), any(MessagingQos.class));
        // ...and given some callback
        @SuppressWarnings("unchecked")
        Callback<List<GlobalDiscoveryEntry>> callbackListOfGlobalDiscoveryEntriesMock = mock(Callback.class);

        // when we execute the lookup method of the GCD client with this callback
        checkLookupDomainsIsCalledCorrectly(callbackListOfGlobalDiscoveryEntriesMock);

        // then the callback we used for the GCD client call is correctly completed
        verify(callbackListOfGlobalDiscoveryEntriesMock).onFailure(eq(new JoynrRuntimeException()));
        verify(callbackListOfGlobalDiscoveryEntriesMock, times(0)).onSuccess(anyListOf(GlobalDiscoveryEntry.class));
    }

    @Test
    public void testLookupDomainsOnSuccess() {
        // given the GCD proxy returns a regular answer on lookup with domain/interface...
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                Callback<GlobalDiscoveryEntry[]> callback = (Callback<GlobalDiscoveryEntry[]>) invocation.getArguments()[0];
                GlobalDiscoveryEntry[] result = new GlobalDiscoveryEntry[]{ new GlobalDiscoveryEntry(),
                        new GlobalDiscoveryEntry() };
                callback.onSuccess(result);
                return null;
            }
        }).when(globalCapabilitiesDirectoryProxyMock)
          .lookup(anyObject(), any(String[].class), any(String.class), any(MessagingQos.class));
        // ...and given some callback
        @SuppressWarnings("unchecked")
        Callback<List<GlobalDiscoveryEntry>> callbackListOfGlobalDiscoveryEntriesMock = mock(Callback.class);

        // when we execute the lookup method of the GCD client with this callback
        checkLookupDomainsIsCalledCorrectly(callbackListOfGlobalDiscoveryEntriesMock);

        // then the callback we used for the GCD client call is correctly completed
        verify(callbackListOfGlobalDiscoveryEntriesMock).onSuccess(anyListOf(GlobalDiscoveryEntry.class));
        verify(callbackListOfGlobalDiscoveryEntriesMock, times(0)).onFailure(any(JoynrRuntimeException.class));
    }

    @Test
    public void testTouch() {
        // given the freshness interval setting

        // when we call the touch method
        subject.touch();

        // then the GCD proxy's touch method gets invoked exactly once for every backend (placed in
        // the custom headers of MessagingQos)...
        ArgumentCaptor<MessagingQos> messagingQosCaptor = ArgumentCaptor.forClass(MessagingQos.class);
        verify(globalCapabilitiesDirectoryProxyMock,
               times(GBIDS_ARRAY_PROPERTY_SETTING.length)).touch(any(String.class), messagingQosCaptor.capture());

        List<String> allUsedGbidCustomHeaders = messagingQosCaptor.getAllValues()
                                                                  .stream()
                                                                  .map(qos -> qos.getCustomMessageHeaders()
                                                                                 .get(Message.CUSTOM_HEADER_GBID_KEY))
                                                                  .collect(Collectors.toList());
        assertTrue(allUsedGbidCustomHeaders.containsAll(Arrays.asList(GBIDS_ARRAY_PROPERTY_SETTING)));
        assertTrue(Arrays.asList(GBIDS_ARRAY_PROPERTY_SETTING).containsAll(allUsedGbidCustomHeaders));
        // ... and all proxy calls have the correct freshness QoS
        assertTrue(messagingQosCaptor.getAllValues()
                                     .stream()
                                     .allMatch(qos -> qos.getRoundTripTtl_ms() == FRESHNESS_UPDATE_INTERVAL_MS));
    }
}
