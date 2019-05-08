/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.tests.gracefulshutdown;

import java.util.Properties;

import javax.ejb.Singleton;
import javax.enterprise.inject.Produces;

import io.joynr.jeeintegration.api.JoynrLocalDomain;
import io.joynr.jeeintegration.api.JoynrProperties;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.MqttModule;

@Singleton
public class JoynrConfigurationProvider {

    @Produces
    @JoynrProperties
    public Properties joynrProperties() {
        Properties joynrProperties = new Properties();
        joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT, "/messaging");
        joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, "http://mqttbroker:8080");
        joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID,
                                    "io.joynr.tests.gracefulshutdown.jee.secondlevel.provider");
        joynrProperties.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS, "tcp://mqttbroker:1883");
        joynrProperties.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL, "http://joynrbackend:8080/bounceproxy/");
        joynrProperties.setProperty(MessagingPropertyKeys.DISCOVERYDIRECTORYURL, "tcp://mqttbroker:1883");
        joynrProperties.setProperty(MessagingPropertyKeys.DOMAINACCESSCONTROLLERURL, "tcp://mqttbroker:1883");
        joynrProperties.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                                    "jee-second-level-provider-joynr.properties");
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS, "true");

        return joynrProperties;
    }

    @Produces
    @JoynrLocalDomain
    public String joynrLocalDomain() {
        return "io.joynr.tests.gracefulshutdown.jee.secondlevel.provider";
    }

}
