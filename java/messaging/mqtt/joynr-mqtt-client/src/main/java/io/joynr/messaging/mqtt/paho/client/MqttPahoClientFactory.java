package io.joynr.messaging.mqtt.paho.client;

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

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttModule;
import joynr.system.RoutingTypes.MqttAddress;

@Singleton
public class MqttPahoClientFactory implements MqttClientFactory {

    private static final Logger logger = LoggerFactory.getLogger(MqttPahoClientFactory.class);
    private MqttAddress ownAddress;
    private JoynrMqttClient mqttClient = null;
    private int reconnectSleepMs;

    @Inject
    public MqttPahoClientFactory(@Named(MqttModule.PROPERTY_MQTT_ADDRESS) MqttAddress ownAddress,
                                 @Named(MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS) int reconnectSleepMs) {
        this.ownAddress = ownAddress;
        this.reconnectSleepMs = reconnectSleepMs;
    }

    @Override
    public synchronized JoynrMqttClient create() {
        if (mqttClient == null) {
            mqttClient = createInternal();
        }
        return mqttClient;
    }

    private JoynrMqttClient createInternal() {

        MqttPahoClient pahoClient = null;
        try {
            logger.debug("Create Mqtt Client. Address: {}", ownAddress);

            MqttClient mqttClient = new MqttClient(ownAddress.getBrokerUri(),
                                                   ownAddress.getTopic(),
                                                   new MemoryPersistence());
            pahoClient = new MqttPahoClient(mqttClient, new MqttAddress(ownAddress), reconnectSleepMs);
            pahoClient.start();

        } catch (MqttException e) {
            logger.error("Create MqttClient failed", e);
        }

        return pahoClient;
    }

}
