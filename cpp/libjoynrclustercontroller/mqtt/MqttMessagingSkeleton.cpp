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
#include "libjoynrclustercontroller/mqtt/MqttMessagingSkeleton.h"

#include <smrf/exceptions.h>

#include "joynr/IMessageRouter.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/Message.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"

#include "joynr/MqttReceiver.h"

namespace joynr
{

INIT_LOGGER(MqttMessagingSkeleton);

std::string MqttMessagingSkeleton::translateMulticastWildcard(std::string topic)
{
    static constexpr char MQTT_MULTI_LEVEL_WILDCARD = '#';
    if (topic.length() > 0 && topic.back() == util::MULTI_LEVEL_WILDCARD[0]) {
        topic.back() = MQTT_MULTI_LEVEL_WILDCARD;
    }
    return topic;
}

MqttMessagingSkeleton::MqttMessagingSkeleton(std::weak_ptr<IMessageRouter> messageRouter,
                                             std::shared_ptr<MqttReceiver> mqttReceiver,
                                             const std::string& multicastTopicPrefix,
                                             uint64_t ttlUplift)
        : messageRouter(std::move(messageRouter)),
          mqttReceiver(std::move(mqttReceiver)),
          ttlUplift(ttlUplift),
          multicastSubscriptionCount(),
          multicastSubscriptionCountMutex(),
          multicastTopicPrefix(multicastTopicPrefix)
{
}

void MqttMessagingSkeleton::registerMulticastSubscription(const std::string& multicastId)
{
    std::string mqttTopic = translateMulticastWildcard(multicastId);
    std::lock_guard<std::mutex> lock(multicastSubscriptionCountMutex);
    if (multicastSubscriptionCount.find(mqttTopic) == multicastSubscriptionCount.cend()) {
        mqttReceiver->subscribeToTopic(multicastTopicPrefix + mqttTopic);
        multicastSubscriptionCount[mqttTopic] = 1;
    } else {
        multicastSubscriptionCount[mqttTopic]++;
    }
}

void MqttMessagingSkeleton::unregisterMulticastSubscription(const std::string& multicastId)
{
    std::string mqttTopic = translateMulticastWildcard(multicastId);
    std::lock_guard<std::mutex> lock(multicastSubscriptionCountMutex);
    auto countIterator = multicastSubscriptionCount.find(mqttTopic);
    if (countIterator == multicastSubscriptionCount.cend()) {
        JOYNR_LOG_ERROR(
                logger, "unregister multicast subscription called for non existing subscription");
    } else if (countIterator->second == 1) {
        multicastSubscriptionCount.erase(mqttTopic);
        mqttReceiver->unsubscribeFromTopic(multicastTopicPrefix + mqttTopic);
    } else {
        countIterator->second--;
    }
}

void MqttMessagingSkeleton::transmit(
        std::shared_ptr<ImmutableMessage> message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    message->setReceivedFromGlobal(true);

    try {
        if (auto messageRouterSharedPtr = messageRouter.lock()) {
            messageRouterSharedPtr->route(std::move(message));
        } else {
            std::string errorMessage(
                    "unable to transmit message because messageRouter unavailable: " +
                    message->toLogMessage());
            onFailure(exceptions::JoynrMessageNotSentException(std::move(errorMessage)));
        }
    } catch (const exceptions::JoynrRuntimeException& e) {
        onFailure(e);
    }
}

void MqttMessagingSkeleton::onMessageReceived(smrf::ByteVector&& rawMessage)
{
    std::shared_ptr<ImmutableMessage> immutableMessage;
    try {
        immutableMessage = std::make_shared<ImmutableMessage>(std::move(rawMessage));
    } catch (const smrf::EncodingException& e) {
        JOYNR_LOG_ERROR(logger, "Unable to deserialize message - error: {}", e.what());
        return;
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger, "deserialized message is not valid - error: {}", e.what());
        return;
    }

    JOYNR_LOG_DEBUG(logger, "<<< INCOMING <<< {}", immutableMessage->toLogMessage());

    /*
    // TODO remove uplift ???? cannot modify msg here!
    const JoynrTimePoint maxAbsoluteTime = DispatcherUtils::getMaxAbsoluteTime();
    JoynrTimePoint msgExpiryDate = msg.getHeaderExpiryDate();
    std::int64_t maxDiff = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   maxAbsoluteTime - msgExpiryDate).count();
    if (static_cast<std::int64_t>(ttlUplift) > maxDiff) {
        msg.setHeaderExpiryDate(maxAbsoluteTime);
    } else {
        JoynrTimePoint newExpiryDate = msgExpiryDate + std::chrono::milliseconds(ttlUplift);
        msg.setHeaderExpiryDate(newExpiryDate);
    }
    */
    auto onFailure = [messageId = immutableMessage->getId()](
            const exceptions::JoynrRuntimeException& e)
    {
        JOYNR_LOG_ERROR(logger,
                        "Incoming Message with ID {} could not be sent! reason: {}",
                        messageId,
                        e.getMessage());
    };

    transmit(std::move(immutableMessage), onFailure);
}

} // namespace joynr
