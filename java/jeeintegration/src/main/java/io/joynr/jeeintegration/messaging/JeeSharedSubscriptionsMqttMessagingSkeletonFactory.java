/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.jeeintegration.messaging;

import java.util.Set;

import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.mqtt.AbstractMqttMessagingSkeletonFactory;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttTopicPrefixProvider;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import joynr.system.RoutingTypes.MqttAddress;

public class JeeSharedSubscriptionsMqttMessagingSkeletonFactory extends AbstractMqttMessagingSkeletonFactory {

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public JeeSharedSubscriptionsMqttMessagingSkeletonFactory(String[] gbids,
                                                              MqttAddress ownAddress,
                                                              int maxIncomingMqttRequests,
                                                              boolean backpressureEnabled,
                                                              int backpressureIncomingMqttRequestsUpperThreshold,
                                                              int backpressureIncomingMqttRequestsLowerThreshold,
                                                              MqttAddress replyToAddress,
                                                              MessageRouter messageRouter,
                                                              MessageProcessedHandler messageProcessedHandler,
                                                              MqttClientFactory mqttClientFactory,
                                                              String channelId,
                                                              MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                                              RawMessagingPreprocessor rawMessagingPreprocessor,
                                                              Set<JoynrMessageProcessor> messageProcessors,
                                                              JoynrStatusMetricsReceiver joynrStatusMetricsReceiver,
                                                              RoutingTable routingTable) {
        super();
        for (String gbid : gbids) {
            mqttMessagingSkeletons.put(gbid,
                                       new JeeSharedSubscriptionsMqttMessagingSkeleton(ownAddress.getTopic(),
                                                                                       maxIncomingMqttRequests,
                                                                                       backpressureEnabled,
                                                                                       backpressureIncomingMqttRequestsUpperThreshold,
                                                                                       backpressureIncomingMqttRequestsLowerThreshold,
                                                                                       replyToAddress.getTopic(),
                                                                                       messageRouter,
                                                                                       messageProcessedHandler,
                                                                                       mqttClientFactory,
                                                                                       channelId,
                                                                                       mqttTopicPrefixProvider,
                                                                                       rawMessagingPreprocessor,
                                                                                       messageProcessors,
                                                                                       joynrStatusMetricsReceiver,
                                                                                       gbid,
                                                                                       routingTable));
        }
        messagingSkeletonList.addAll(mqttMessagingSkeletons.values());
    }

}
