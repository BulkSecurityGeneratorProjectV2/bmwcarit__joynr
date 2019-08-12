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
#ifndef MOSQUITTOCONNECTION_H
#define MOSQUITTOCONNECTION_H

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <string>
#include <unordered_set>
#include <mutex>

#include <mosquitto.h>

#include <smrf/ByteVector.h>

#include "joynr/BrokerUrl.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class MessagingSettings;
class ClusterControllerSettings;

class MosquittoConnection
{

public:
    explicit MosquittoConnection(const ClusterControllerSettings& ccSettings,
                                 BrokerUrl brokerUrl,
                                 std::chrono::seconds mqttKeepAliveTimeSeconds,
                                 std::chrono::seconds mqttReconnectDelayTimeSeconds,
                                 std::chrono::seconds mqttReconnectMaxDelayTimeSeconds,
                                 bool isMqttExponentialBackoffEnabled,
                                 const std::string& clientId);

    virtual ~MosquittoConnection();

    virtual std::uint16_t getMqttQos() const;
    virtual std::string getMqttPrio() const;
    virtual bool isMqttRetain() const;

    /**
     * Starts mosquitto's internal loop in case it is not running or handles reconnect when
     * external communication with MQTT broker needs to be restored.
     */
    virtual void start();

    /**
     * Stops external communication by disconnecting from MQTT broker. Mosquitto loop is
     * not stopped.
     */
    virtual void stop();

    virtual void publishMessage(
            const std::string& topic,
            const int qosLevel,
            const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure,
            std::uint32_t payloadlen,
            const void* payload);
    virtual void subscribeToTopic(const std::string& topic);
    virtual void unsubscribeFromTopic(const std::string& topic);
    virtual void registerChannelId(const std::string& channelId);
    virtual void registerReceiveCallback(std::function<void(smrf::ByteVector&&)> onMessageReceived);
    virtual void registerReadyToSendChangedCallback(std::function<void(bool)> readyToSendCallback);
    virtual bool isSubscribedToChannelTopic() const;
    virtual bool isReadyToSend() const;

private:
    DISALLOW_COPY_AND_ASSIGN(MosquittoConnection);

    static void on_connect(struct mosquitto* mosq, void* userdata, int rc);
    static void on_disconnect(struct mosquitto* mosq, void* userdata, int rc);
    static void on_publish(struct mosquitto* mosq, void* userdata, int mid);
    static void on_message(struct mosquitto* mosq,
                           void* userdata,
                           const struct mosquitto_message* message);
    static void on_subscribe(struct mosquitto* mosq,
                             void* userdata,
                             int mid,
                             int qos_count,
                             const int* granted_qos);
    static void on_log(struct mosquitto* mosq, void* userdata, int level, const char* str);

    /**
     * Starts mosquitto's internal loop. This function wraps internal mosquitto function loop_start
     */
    void startLoop();

    /**
     * Stops mosquitto's internal loop. This function wraps internal mosquitto function loop_stop
     * @param force Set to true to force stopping of the loop. If false, stop (disconnect) must have
     * already been called.
     */
    void stopLoop(bool force = false);

    void cleanupLibrary();

    void createSubscriptions();
    void subscribeToTopicInternal(const std::string& topic, const bool isChannelTopic = false);
    void setReadyToSend(bool readyToSend);
    static std::string getErrorString(int rc);

    const BrokerUrl brokerUrl;
    const std::chrono::seconds mqttKeepAliveTimeSeconds;
    const std::chrono::seconds mqttReconnectDelayTimeSeconds;
    const std::chrono::seconds mqttReconnectMaxDelayTimeSeconds;
    const bool isMqttExponentialBackoffEnabled;
    const std::string host;
    const std::uint16_t port;

    const std::uint16_t mqttQos = 1;
    const bool mqttRetain = false;

    std::string channelId;
    int subscribeChannelMid;
    std::string topic;
    std::unordered_set<std::string> additionalTopics;
    std::recursive_mutex additionalTopicsMutex;

    std::atomic<bool> isConnected;
    std::atomic<bool> isRunning;
    std::atomic<bool> isWaitingForDisconnect;
    std::atomic<bool> isChannelIdRegistered;
    std::atomic<bool> subscribedToChannelTopic;
    std::atomic<bool> readyToSend;
    static std::mutex libUseCountMutex;
    static int libUseCount;

    std::function<void(smrf::ByteVector&&)> onMessageReceived;
    std::mutex onReadyToSendChangedMutex;
    std::function<void(bool)> onReadyToSendChanged;

    std::mutex stopMutex;
    bool isStopped;
    struct mosquitto* mosq;

    ADD_LOGGER(MosquittoConnection)
};

} // namespace joynr

#endif // MOSQUITTOCONNECTION_H
