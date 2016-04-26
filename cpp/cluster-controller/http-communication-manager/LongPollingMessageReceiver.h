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
#ifndef LONGPOLLINGMESSAGERECEIVER_H_
#define LONGPOLLINGMESSAGERECEIVER_H_
#include <condition_variable>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <QByteArray>

#include "joynr/PrivateCopyAssign.h"

#include "joynr/ContentWithDecayTime.h"
#include "joynr/BrokerUrl.h"
#include "joynr/Logger.h"
#include "joynr/Directory.h"
#include "joynr/Semaphore.h"
#include "joynr/Thread.h"

namespace joynr
{

class MessageRouter;

/**
 * Structure used for configuring the long poll message receiver
 */
struct LongPollingMessageReceiverSettings
{
    std::chrono::milliseconds brokerTimeout;
    std::chrono::milliseconds longPollTimeout;
    std::chrono::milliseconds longPollRetryInterval;
    std::chrono::milliseconds createChannelRetryInterval;
};

/**
 * Class that makes long polling requests to the bounce proxy
 */
class LongPollingMessageReceiver : public Thread
{
public:
    LongPollingMessageReceiver(const BrokerUrl& brokerUrl,
                               const std::string& channelId,
                               const std::string& receiverId,
                               const LongPollingMessageReceiverSettings& settings,
                               std::shared_ptr<Semaphore> channelCreatedSemaphore,
                               std::function<void(const std::string&)> onTextMessageReceived);
    void stop() override;
    void run() override;
    void interrupt();
    bool isInterrupted();

    void processReceivedInput(const QByteArray& receivedInput);
    void processReceivedJsonObjects(const std::string& jsonObject);

private:
    void checkServerTime();
    DISALLOW_COPY_AND_ASSIGN(LongPollingMessageReceiver);
    const BrokerUrl brokerUrl;
    const std::string channelId;
    const std::string receiverId;
    const LongPollingMessageReceiverSettings settings;

    bool interrupted;
    std::mutex interruptedMutex;
    std::condition_variable interruptedWait;

    ADD_LOGGER(LongPollingMessageReceiver);

    // Ownership shared between this and HttpReceiver
    std::shared_ptr<Semaphore> channelCreatedSemaphore;

    /*! On text message received callback */
    std::function<void(const std::string&)> onTextMessageReceived;
};

} // namespace joynr
#endif // LONGPOLLINGMESSAGERECEIVER_H_
