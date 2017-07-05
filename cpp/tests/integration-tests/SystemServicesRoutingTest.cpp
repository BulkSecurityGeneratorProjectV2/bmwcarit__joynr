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
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/Settings.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/serializer/Serializer.h"

#include "tests/JoynrTest.h"
#include "tests/utils/MockObjects.h"

using namespace joynr;

class SystemServicesRoutingTest : public ::testing::Test {
public:
    SystemServicesRoutingTest() :
            settingsFilename("test-resources/SystemServicesRoutingTest.settings"),
            settings(std::make_unique<Settings>(settingsFilename)),
            routingDomain(),
            routingProviderParticipantId(),
            runtime(nullptr),
            mockMessageReceiverHttp(std::make_shared<MockTransportMessageReceiver>()),
            mockMessageReceiverMqtt(std::make_shared<MockTransportMessageReceiver>()),
            mockMessageSender(std::make_shared<MockTransportMessageSender>()),
            discoveryQos(),
            routingProxyBuilder(nullptr),
            routingProxy(nullptr)
    {
        SystemServicesSettings systemSettings(*settings);
        systemSettings.printSettings();
        routingDomain = systemSettings.getDomain();
        routingProviderParticipantId = systemSettings.getCcRoutingProviderParticipantId();

        discoveryQos.setCacheMaxAgeMs(1000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
        discoveryQos.addCustomParameter("fixedParticipantId", routingProviderParticipantId);
        discoveryQos.setDiscoveryTimeoutMs(50);

        std::string httpChannelId("http_SystemServicesRoutingTest.ChannelId");
        std::string httpEndPointUrl("http_SystemServicesRoutingTest.endPointUrl");
        std::string mqttTopic("mqtt_SystemServicesRoutingTest.topic");
        std::string mqttBrokerUrl("mqtt_SystemServicesRoutingTest.brokerUrl");

        using system::RoutingTypes::ChannelAddress;
        using system::RoutingTypes::MqttAddress;

        std::string serializedChannelAddress = joynr::serializer::serializeToJson(ChannelAddress(httpEndPointUrl, httpChannelId));
        std::string serializedMqttAddress = joynr::serializer::serializeToJson(MqttAddress(mqttBrokerUrl, mqttTopic));

        EXPECT_CALL(*(std::dynamic_pointer_cast<MockTransportMessageReceiver>(mockMessageReceiverHttp).get()), getGlobalClusterControllerAddress())
                .WillRepeatedly(::testing::ReturnRefOfCopy(serializedChannelAddress));
        EXPECT_CALL(*(std::dynamic_pointer_cast<MockTransportMessageReceiver>(mockMessageReceiverMqtt)), getGlobalClusterControllerAddress())
                .WillRepeatedly(::testing::ReturnRefOfCopy(serializedMqttAddress));

        //runtime can only be created, after MockMessageReceiver has been told to return
        //a channelId for getReceiveChannelId.
        runtime = std::make_unique<JoynrClusterControllerRuntime>(
                std::move(settings),
                mockMessageReceiverHttp,
                mockMessageSender,
                mockMessageReceiverMqtt,
                mockMessageSender);
        // routing provider is normally registered in JoynrClusterControllerRuntime::create
        runtime->registerRoutingProvider();
    }

    ~SystemServicesRoutingTest(){
        runtime->deleteChannel();
        runtime->stopExternalCommunication();
        std::remove(settingsFilename.c_str());
    }

    void SetUp(){
        participantId = util::createUuid();
        isGloballyVisible = true;
        routingProxyBuilder = runtime->createProxyBuilder<joynr::system::RoutingProxy>(routingDomain);
    }

    void TearDown(){
        // Delete persisted files
        std::remove(ClusterControllerSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

protected:
    std::string settingsFilename;
    std::unique_ptr<Settings> settings;
    std::string routingDomain;
    std::string routingProviderParticipantId;
    std::unique_ptr<JoynrClusterControllerRuntime> runtime;
    std::shared_ptr<ITransportMessageReceiver> mockMessageReceiverHttp;
    std::shared_ptr<ITransportMessageReceiver> mockMessageReceiverMqtt;
    std::shared_ptr<MockTransportMessageSender> mockMessageSender;
    DiscoveryQos discoveryQos;
    std::unique_ptr<ProxyBuilder<joynr::system::RoutingProxy>> routingProxyBuilder;
    std::unique_ptr<joynr::system::RoutingProxy> routingProxy;
    std::string participantId;
    bool isGloballyVisible;

private:
    DISALLOW_COPY_AND_ASSIGN(SystemServicesRoutingTest);
};


TEST_F(SystemServicesRoutingTest, routingProviderIsAvailable)
{
    JOYNR_EXPECT_NO_THROW(
        routingProxy = routingProxyBuilder
                ->setMessagingQos(MessagingQos(5000))
                ->setDiscoveryQos(discoveryQos)
                ->build()
    );
}

TEST_F(SystemServicesRoutingTest, unknowParticipantIsNotResolvable)
{
    routingProxy = routingProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setDiscoveryQos(discoveryQos)
            ->build();

    bool isResolvable = false;
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);
}


TEST_F(SystemServicesRoutingTest, addNextHopHttp)
{
    routingProxy = routingProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setDiscoveryQos(discoveryQos)
            ->build();

    joynr::system::RoutingTypes::ChannelAddress address("SystemServicesRoutingTest.ChanneldId.A", "SystemServicesRoutingTest.endPointUrl");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address, isGloballyVisible);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);
}

TEST_F(SystemServicesRoutingTest, removeNextHopHttp)
{
    routingProxy = routingProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setDiscoveryQos(discoveryQos)
            ->build();

    joynr::system::RoutingTypes::ChannelAddress address("SystemServicesRoutingTest.ChanneldId.A", "SystemServicesRoutingTest.endPointUrl");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address, isGloballyVisible);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);

    try {
        routingProxy->removeNextHop(participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "removeNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);
}


TEST_F(SystemServicesRoutingTest, addNextHopMqtt)
{
    routingProxy = routingProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setDiscoveryQos(discoveryQos)
            ->build();

    joynr::system::RoutingTypes::MqttAddress address("brokerUri", "SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address, isGloballyVisible);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);
}

TEST_F(SystemServicesRoutingTest, removeNextHopMqtt)
{
    routingProxy = routingProxyBuilder
            ->setMessagingQos(MessagingQos(5000))
            ->setDiscoveryQos(discoveryQos)
            ->build();

    joynr::system::RoutingTypes::MqttAddress address("brokerUri", "SystemServicesRoutingTest.ChanneldId.A");
    bool isResolvable = false;

    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);

    try {
        routingProxy->addNextHop(participantId, address, isGloballyVisible);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "addNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_TRUE(isResolvable);

    try {
        routingProxy->removeNextHop(participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "removeNextHop was not successful";
    }
    try {
        routingProxy->resolveNextHop(isResolvable, participantId);
    } catch (const exceptions::JoynrException& e) {
        ADD_FAILURE()<< "resolveNextHop was not successful";
    }
    EXPECT_FALSE(isResolvable);
}
