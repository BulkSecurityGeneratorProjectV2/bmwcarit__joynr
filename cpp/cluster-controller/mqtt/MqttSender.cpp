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
#include "MqttSender.h"

#include "joynr/Util.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "joynr/MessagingQosEffort.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

INIT_LOGGER(MqttSender);

MqttSender::MqttSender(const MessagingSettings& settings)
        : mosquittoPublisher(settings), waitForReceiveQueueStarted(nullptr)
{
    mosquittoPublisher.start();
}

MqttSender::~MqttSender()
{
    mosquittoPublisher.stop();
}

void MqttSender::sendMessage(
        const system::RoutingTypes::Address& destinationAddress,
        const JoynrMessage& message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    JOYNR_LOG_DEBUG(logger, "sendMessage: ...");

    auto mqttAddress = dynamic_cast<const system::RoutingTypes::MqttAddress*>(&destinationAddress);
    if (mqttAddress == nullptr) {
        JOYNR_LOG_DEBUG(logger, "Invalid destination address type provided");
        onFailure(exceptions::JoynrRuntimeException("Invalid destination address type provided"));
        return;
    }

    std::string topic;
    if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_MULTICAST) {
        topic = message.getHeaderTo();
    } else {
        topic = mqttAddress->getTopic() + "/" + mosquittoPublisher.getMqttPrio() + "/" +
                message.getHeaderTo();
    }

    waitForReceiveQueueStarted();

    std::string serializedMessage = joynr::serializer::serializeToJson(message);

    const int payloadLength = serializedMessage.length();
    const void* payload = serializedMessage.c_str();

    util::logSerializedMessage(logger, "Sending Message: ", serializedMessage);

    int qosLevel = mosquittoPublisher.getMqttQos();
    if (message.containsHeaderEffort() &&
        message.getHeaderEffort() ==
                MessagingQosEffort::getLiteral(MessagingQosEffort::Enum::BEST_EFFORT)) {
        qosLevel = 0;
    }

    mosquittoPublisher.publishMessage(topic, qosLevel, onFailure, payloadLength, payload);
}

void MqttSender::registerReceiveQueueStartedCallback(
        std::function<void(void)> waitForReceiveQueueStarted)
{
    this->waitForReceiveQueueStarted = waitForReceiveQueueStarted;
}

} // namespace joynr
