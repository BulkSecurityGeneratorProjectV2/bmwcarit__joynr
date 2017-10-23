/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTS_MOCK_MOCKMOSQUITTOCONNECTION_H
#define TESTS_MOCK_MOCKMOSQUITTOCONNECTION_H

#include <gmock/gmock.h>

#include "libjoynrclustercontroller/mqtt/MosquittoConnection.h"

class MockMosquittoConnection : public joynr::MosquittoConnection
{
public:
    MockMosquittoConnection(const joynr::MessagingSettings& messagingSettings, const joynr::ClusterControllerSettings& ccSettings, const std::string& clientId) : MosquittoConnection(messagingSettings, ccSettings, clientId) {};

    MOCK_CONST_METHOD0(getMqttPrio, std::string());
    MOCK_CONST_METHOD0(getMqttQos, std::uint16_t());
    MOCK_CONST_METHOD0(isMqttRetain, bool());
    MOCK_CONST_METHOD0(isReadyToSend, bool());
    MOCK_CONST_METHOD0(isSubscribedToChannelTopic, bool());
    MOCK_METHOD5(publishMessage, void(const std::string& topic, const int qosLevel, const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>& onFailure, std::uint32_t payloadlen, const void* payload));
    MOCK_METHOD1(registerChannelId, void(const std::string& channelId));
    MOCK_METHOD1(registerReceiveCallback, void(std::function<void(smrf::ByteVector&&)> onMessageReceived));
    MOCK_METHOD1(registerReadyToSendChangedCallback, void(std::function<void(bool)> readyToSendCallback));
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
    MOCK_METHOD1(subscribeToTopic, void(const std::string& topic));
    MOCK_METHOD1(unsubscribeFromTopic, void(const std::string& topic));
};

#endif // TESTS_MOCK_MOCKMOSQUITTOCONNECTION_H
