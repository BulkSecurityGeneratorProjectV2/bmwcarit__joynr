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
#include <cstring>

#include "MosquittoSubscriber.h"

#include "joynr/JoynrMessage.h"
#include "joynr/MessageRouter.h"

namespace joynr
{

INIT_LOGGER(MosquittoSubscriber);

MosquittoSubscriber::MosquittoSubscriber(const MessagingSettings& settings,
                                         const std::string& channelId,
                                         joynr::Semaphore* channelCreatedSemaphore)
        : joynr::Thread("MosquittoSubscriber"),
          MosquittoConnection(settings),
          mqttSettings(),
          channelId(channelId),
          topic(),
          channelCreatedSemaphore(channelCreatedSemaphore),
          isRunning(false),
          isChannelAvailable(false),
          onTextMessageReceived(nullptr)
{
    mqttSettings.reconnectSleepTimeMs = settings.getMqttReconnectSleepTime();
}

void MosquittoSubscriber::interrupt()
{
    isRunning = false;
}

bool MosquittoSubscriber::isInterrupted()
{
    return !isRunning;
}

void MosquittoSubscriber::stop()
{
    interrupt();
    joynr::Thread::stop();
}

void MosquittoSubscriber::run()
{
    checkServerTime();

    isRunning = true;

    while (!isInterrupted()) {
        int rc = loop();

        if (rc) {
            if (rc == MOSQ_ERR_CONN_LOST) {
                JOYNR_LOG_DEBUG(logger,
                                "error: connection to broker lost ({}), trying to reconnect...",
                                mosqpp::strerror(rc));
            } else if (rc == MOSQ_ERR_NO_CONN) {
                JOYNR_LOG_DEBUG(logger,
                                "error: not connected to a broker ({}), trying to reconnect...",
                                mosqpp::strerror(rc));
            } else {
                // MOSQ_ERR_INVAL || MOSQ_ERR_NOMEM || MOSQ_ERR_PROTOCOL || MOSQ_ERR_ERRNO
                JOYNR_LOG_ERROR(logger,
                                "connection to broker lost, unexpected error: {} ({}), trying to "
                                "reconnect...",
                                std::to_string(rc),
                                mosqpp::strerror(rc));
            }
            std::this_thread::sleep_for(mqttSettings.reconnectSleepTimeMs);
            reconnect();
        }
    }

    JOYNR_LOG_DEBUG(logger, "Try to disconnect Mosquitto Connection");
    int rc = disconnect();
    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_DEBUG(logger, "Mosquitto Connection disconnected");
    } else {
        // MOSQ_ERR_INVAL || MOSQ_ERR_NO_CONN
        JOYNR_LOG_ERROR(logger,
                        "Mosquitto disconnect failed: error: {} ({})",
                        std::to_string(rc),
                        mosqpp::strerror(rc));
    }

    mosqpp::lib_cleanup();
}

void MosquittoSubscriber::registerChannelId(const std::string& channelId)
{
    this->channelId = channelId;
    topic = channelId + "/" + getMqttPrio() + "/" + "#";
    isChannelAvailable = true;
}

void MosquittoSubscriber::registerReceiveCallback(
        std::function<void(const std::string&)> onTextMessageReceived)
{
    this->onTextMessageReceived = onTextMessageReceived;
}

void MosquittoSubscriber::on_connect(int rc)
{
    if (rc > 0) {
        if (rc == 1) {
            JOYNR_LOG_ERROR(logger,
                            "Mosquitto Connection Error {} ({})",
                            rc,
                            "connection refused (unacceptable protocol version)");
        } else if (rc == 2) {
            JOYNR_LOG_ERROR(logger,
                            "Mosquitto Connection Error {} ({})",
                            rc,
                            "connection refused (identifier rejected)");
        } else if (rc == 3) {
            JOYNR_LOG_DEBUG(logger,
                            "Mosquitto Connection Error {} ({})",
                            rc,
                            "connection refused (broker unavailable)");
        } else {
            JOYNR_LOG_ERROR(logger,
                            "Mosquitto Connection Error {} ({})",
                            rc,
                            "unknown error code (reserved for future use)");
        }
    } else {
        JOYNR_LOG_DEBUG(logger, "Mosquitto Connection established");
        subscribeToTopic();
    }
}

void MosquittoSubscriber::subscribeToTopic()
{
    while (!isChannelAvailable) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    // TODO: Check mid in callback on_subscribe instead of generated mid (NULL)
    int rc = subscribe(nullptr, topic.c_str(), getMqttQos());
    if (rc == MOSQ_ERR_SUCCESS) {
        JOYNR_LOG_DEBUG(logger, "Subscribed to {}", topic);
    } else if (rc == MOSQ_ERR_INVAL) {
        JOYNR_LOG_ERROR(logger,
                        "Subscription to {} failed: error: {} (invalid input parameters)",
                        topic,
                        std::to_string(rc));
    } else {
        // MOSQ_ERR_NOMEM || MOSQ_ERR_NO_CONN
        JOYNR_LOG_DEBUG(logger,
                        "error subscribing to {}: {} ({}), retrying...",
                        topic,
                        std::to_string(rc),
                        mosqpp::strerror(rc));
        std::this_thread::sleep_for(mqttSettings.reconnectSleepTimeMs);
        subscribeToTopic();
    }
}

void MosquittoSubscriber::unsubscribeFromTopic()
{
    if (isChannelAvailable) {
        int rc = unsubscribe(nullptr, topic.c_str());
        if (rc == MOSQ_ERR_SUCCESS) {
            JOYNR_LOG_DEBUG(logger, "Unsubscribed from {}", topic);
        } else {
            // MOSQ_ERR_INVAL || MOSQ_ERR_NOMEM || MOSQ_ERR_NO_CONN
            JOYNR_LOG_ERROR(logger,
                            "Unsubscribe from {} failed: error: {} ({})",
                            topic,
                            std::to_string(rc),
                            mosqpp::strerror(rc));
        }
    }
}

void MosquittoSubscriber::on_subscribe(int mid, int qos_count, const int* granted_qos)
{
    JOYNR_LOG_DEBUG(logger, "Subscribed (mid: {} with granted QOS {}", mid, granted_qos[0]);

    for (int i = 1; i < qos_count; i++) {
        JOYNR_LOG_DEBUG(logger, "QOS: {} granted {}", i, granted_qos[i]);
    }

    channelCreatedSemaphore->notify();
}

void MosquittoSubscriber::on_message(const struct mosquitto_message* message)
{
    JOYNR_LOG_DEBUG(logger, "Received raw message len: {}", message->payloadlen);

    std::string jsonObject(static_cast<char*>(message->payload), message->payloadlen);

    JOYNR_LOG_DEBUG(logger, "Received raw message: {}", jsonObject);

    if (onTextMessageReceived) {
        onTextMessageReceived(jsonObject);
    } else {
        JOYNR_LOG_ERROR(
                logger,
                "Discarding received message, since onTextMessageReceived callback is empty.");
    }
}

void MosquittoSubscriber::checkServerTime()
{
}

} // namespace joynr
