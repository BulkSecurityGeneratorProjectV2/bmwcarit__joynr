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
#include "AbstractMessagingTest.h"
#include "joynr/system/RoutingTypes/MqttAddress.h"
#include "cluster-controller/messaging/joynr-messaging/MqttMessagingStubFactory.h"

using namespace ::testing;
using namespace joynr;

class MqttMessagingTest : public AbstractMessagingTest {
public:
    ADD_LOGGER(MqttMessagingTest);
    MqttMessagingTest()
    {
        std::string brokerHost = messagingSettings.getBrokerUrl().getBrokerChannelsBaseUrl().getHost();
        std::string brokerPort = std::to_string(messagingSettings.getBrokerUrl().getBrokerChannelsBaseUrl().getPort());
        brokerUri = "tcp://" + brokerHost + ":" + brokerPort;
        // provision global capabilities directory
        auto addressCapabilitiesDirectory = std::make_shared<system::RoutingTypes::MqttAddress>(brokerUri,messagingSettings.getCapabilitiesDirectoryChannelId());
        messageRouter->addProvisionedNextHop(messagingSettings.getCapabilitiesDirectoryParticipantId(), addressCapabilitiesDirectory);
        // provision channel url directory
        auto addressChannelUrlDirectory = std::make_shared<system::RoutingTypes::MqttAddress>(brokerUri,messagingSettings.getChannelUrlDirectoryChannelId());
        messageRouter->addProvisionedNextHop(messagingSettings.getChannelUrlDirectoryParticipantId(), addressChannelUrlDirectory);
        messagingStubFactory->registerStubFactory(std::make_unique<MqttMessagingStubFactory>(mockMessageSender, senderChannelId));
    }

    void WaitXTimes(std::uint64_t x)
    {
        for(std::uint64_t i = 0; i<x; ++i) {
            ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
        }
    }

    ~MqttMessagingTest(){
        std::remove(settingsFileName.c_str());
    }

    std::shared_ptr<system::RoutingTypes::MqttAddress> createJoynrMessagingEndpointAddress();
private:
    DISALLOW_COPY_AND_ASSIGN(MqttMessagingTest);
    std::string brokerUri;
};

INIT_LOGGER(MqttMessagingTest);

TEST_F(MqttMessagingTest, sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager)
{
    // Test Outline: send message from JoynrMessageSender to ICommunicationManager
    // - JoynrMessageSender.sendRequest (IJoynrMessageSender)
    //   -> adds reply caller to dispatcher (IDispatcher.addReplyCaller)
    // - InProcessMessagingStub.transmit (IMessaging)
    // - InProcessClusterControllerMessagingSkeleton.transmit (IMessaging)
    // - MessageRouter.route
    // - MessageRunnable.run
    // - MqttMessagingStub.transmit (IMessaging)
    // - MessageSender.send
    auto joynrMessagingEndpointAddr = std::make_shared<system::RoutingTypes::MqttAddress>();
    joynrMessagingEndpointAddr->setTopic(receiverChannelId);

    sendMsgFromMessageSenderViaInProcessMessagingAndMessageRouterToCommunicationManager(joynrMessagingEndpointAddr);
}


TEST_F(MqttMessagingTest, routeMsgWithInvalidParticipantId)
{
    routeMsgWithInvalidParticipantId();
}

TEST_F(MqttMessagingTest, routeMsgToInProcessMessagingSkeleton)
{
    routeMsgToInProcessMessagingSkeleton();
}

std::shared_ptr<system::RoutingTypes::MqttAddress> MqttMessagingTest::createJoynrMessagingEndpointAddress() {
    auto joynrMessagingEndpointAddr = std::make_shared<system::RoutingTypes::MqttAddress>();
    joynrMessagingEndpointAddr->setTopic(receiverChannelId);
    joynrMessagingEndpointAddr->setBrokerUri(brokerUri);
    return joynrMessagingEndpointAddr;
}

TEST_F(MqttMessagingTest, routeMsgToMqtt)
{
    routeMsgToCommunicationManager(createJoynrMessagingEndpointAddress());
}


TEST_F(MqttMessagingTest, routeMultipleMessages)
{
    routeMultipleMessages(createJoynrMessagingEndpointAddress());
}
