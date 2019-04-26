/*-
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
package io.joynr.examples.statelessasync.carsim;

import static io.joynr.runtime.AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL;

import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrInjectorFactory;

public class Bootstrap {

    private static final Logger logger = LoggerFactory.getLogger(Bootstrap.class);

    public static final void main(String... args) {
        logger.info("Starting car simulator ...");
        Properties joynrProperties = new Properties();
        joynrProperties.put("joynr.messaging.mqtt.brokerUri", "tcp://mqttbroker:1883");
        joynrProperties.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
        joynrProperties.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, "car-sim-joynr.properties");
        joynrProperties.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, "stateless_async_car_sim_local_domain");
        Module runtimeModule = Modules.combine(new CCInProcessRuntimeModule(), new MqttPahoModule());
        JoynrInjectorFactory joynrInjectorFactory = new JoynrInjectorFactory(joynrProperties, runtimeModule);
        JoynrApplication application = joynrInjectorFactory.createApplication(CarSimApplication.class);

        logger.info("Application created.");

        application.run();

        logger.info("Application finished. Shutting down.");

        application.shutdown();
    }
}
