/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.messaging.mqtt;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

import io.joynr.exceptions.JoynrIllegalStateException;

public class MqttMultipleBackendPropertyProviderTest {

    private final String gbids = "gbid1,gbid2,gbid3";

    private final String validBrokerUris = "tcp:\\broker1,tcp:\\broker2,tcp:\\broker3";
    private final String differentLengthBrokerUris = "tcp:\\broker1,tcp:\\broker2";
    private final String brokerUrisWithemptyEntry = "tcp:\\broker1,,tcp:\\broker3";

    private final String validKeepAliveTimers = "60,0,45";
    private final String differentLengthKeepAliveTimers = "60,0";
    private final String nonIntKeepAliveTimers = "a,0,45";
    private final String keepAliveTimersWithEmptyEntry = "60,,45";

    private final String validConnectionTimeouts = "60,0,45";
    private final String differentLengthConnectionTimeouts = "60,0";
    private final String nonIntConnectionTimeouts = "a,0,45";
    private final String connectionTimeoutsWithEmptyEntry = "60,,45";

    @Test
    public void testSuccessfulPropertyInitialization() {
        MqttMultipleBackendPropertyProvider mqttMultipleBackendPropertyProvider = new MqttMultipleBackendPropertyProvider(validBrokerUris,
                                                                                                                          gbids,
                                                                                                                          validKeepAliveTimers,
                                                                                                                          validConnectionTimeouts);
        assertEquals(mqttMultipleBackendPropertyProvider.provideBrokerUris().length, gbids.split(",").length);
        assertEquals(mqttMultipleBackendPropertyProvider.provideConnectionTimeoutTimers().length,
                     gbids.split(",").length);
        assertEquals(mqttMultipleBackendPropertyProvider.provideKeepAliveTimers().length, gbids.split(",").length);
        assertEquals(mqttMultipleBackendPropertyProvider.provideGbidToBrokerUriMap().values().size(),
                     gbids.split(",").length);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testFailureOnDifferentLengthBrokerUris() {
        new MqttMultipleBackendPropertyProvider(differentLengthBrokerUris,
                                                gbids,
                                                validKeepAliveTimers,
                                                validConnectionTimeouts);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testFailureOnDifferentLengthKeepAliveTimers() {
        new MqttMultipleBackendPropertyProvider(validBrokerUris,
                                                gbids,
                                                differentLengthKeepAliveTimers,
                                                validConnectionTimeouts);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testFailureOnDifferentLengthConnectionTimeouts() {
        new MqttMultipleBackendPropertyProvider(validBrokerUris,
                                                gbids,
                                                validKeepAliveTimers,
                                                differentLengthConnectionTimeouts);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testFailureOnBrokerUrisWithEmptyEntry() {
        new MqttMultipleBackendPropertyProvider(brokerUrisWithemptyEntry,
                                                gbids,
                                                validKeepAliveTimers,
                                                validConnectionTimeouts);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testFailureOnKeepAliveTimersWithEmptyEntry() {
        new MqttMultipleBackendPropertyProvider(validBrokerUris,
                                                gbids,
                                                keepAliveTimersWithEmptyEntry,
                                                validConnectionTimeouts);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testFailureOnConnectionTimeoutsWithemptyEntry() {
        new MqttMultipleBackendPropertyProvider(validBrokerUris,
                                                gbids,
                                                validKeepAliveTimers,
                                                connectionTimeoutsWithEmptyEntry);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testFailureOnNonIntKeepAliveTimers() {
        new MqttMultipleBackendPropertyProvider(validBrokerUris, gbids, nonIntKeepAliveTimers, validConnectionTimeouts);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testFailureOnNonIntConnectionTimeouts() {
        new MqttMultipleBackendPropertyProvider(validBrokerUris, gbids, validKeepAliveTimers, nonIntConnectionTimeouts);
    }
}
