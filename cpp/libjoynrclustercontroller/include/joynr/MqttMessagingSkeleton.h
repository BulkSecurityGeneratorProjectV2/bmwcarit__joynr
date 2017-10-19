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
#ifndef MQTTMESSAGINGSKELETON_H
#define MQTTMESSAGINGSKELETON_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <smrf/ByteVector.h>

#include "joynr/IMqttMessagingSkeleton.h"
#include "JoynrClusterControllerExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{
namespace exceptions
{
class JoynrRuntimeException;
} // namespace exceptions

class ImmutableMessage;
class IMessageRouter;
class MqttReceiver;

class JOYNRCLUSTERCONTROLLER_EXPORT MqttMessagingSkeleton : public IMqttMessagingSkeleton
{
public:
    /*
     * Make multicastId mqtt compliant: convert Kleene star to Hash symbol.
     * This method assumes the multicastId is valid i.e. the Kleene star appears only at the end.
     */
    static std::string translateMulticastWildcard(std::string multicastId);

    MqttMessagingSkeleton(std::weak_ptr<IMessageRouter> messageRouter,
                          std::shared_ptr<MqttReceiver> mqttReceiver,
                          const std::string& multicastTopicPrefix,
                          std::uint64_t ttlUplift = 0);

    ~MqttMessagingSkeleton() override = default;

    void transmit(std::shared_ptr<joynr::ImmutableMessage> message,
                  const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
            override;

    void registerMulticastSubscription(const std::string& multicastId) override;
    void unregisterMulticastSubscription(const std::string& multicastId) override;

    void onMessageReceived(smrf::ByteVector&& rawMessage) override;

private:
    DISALLOW_COPY_AND_ASSIGN(MqttMessagingSkeleton);
    ADD_LOGGER(MqttMessagingSkeleton)

    std::weak_ptr<IMessageRouter> messageRouter;
    std::shared_ptr<MqttReceiver> mqttReceiver;

    std::uint64_t ttlUplift;

    std::unordered_map<std::string, std::uint64_t> multicastSubscriptionCount;
    std::mutex multicastSubscriptionCountMutex;
    std::string multicastTopicPrefix;
};

} // namespace joynr
#endif // MQTTMESSAGINGSKELETON_H
