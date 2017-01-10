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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include "JoynrTest.h"
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "joynr/tests/testProxy.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/tests/testAbstractProvider.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/exceptions/JoynrException.h"

using namespace ::testing;
using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

namespace joynr {

class End2EndSubscriptionTest : public TestWithParam< std::tuple<std::string, std::string> > {
public:
    JoynrClusterControllerRuntime* runtime1;
    JoynrClusterControllerRuntime* runtime2;
    std::unique_ptr<Settings> settings1;
    std::unique_ptr<Settings> settings2;
    std::string baseUuid;
    std::string uuid;
    std::string domainName;
    joynr::Semaphore semaphore;
    unsigned long registerProviderWait;
    unsigned long subscribeToAttributeWait;
    joynr::types::Localisation::GpsLocation gpsLocation;

    End2EndSubscriptionTest() :
        runtime1(nullptr),
        runtime2(nullptr),
        settings1(std::make_unique<Settings>(std::get<0>(GetParam()))),
        settings2(std::make_unique<Settings>(std::get<1>(GetParam()))),
        baseUuid(util::createUuid()),
        uuid( "_" + baseUuid.substr(1, baseUuid.length()-2)),
        domainName("cppEnd2EndSubscriptionTest_Domain" + uuid),
        semaphore(0),
        registerProviderWait(1000),
        subscribeToAttributeWait(2000),
        providerParticipantId()

    {
        Settings integration1Settings{"test-resources/libjoynrSystemIntegration1.settings"};
        Settings::merge(integration1Settings, *settings1, false);

        runtime1 = new JoynrClusterControllerRuntime(std::move(settings1));

        Settings integration2Settings{"test-resources/libjoynrSystemIntegration2.settings"};
        Settings::merge(integration2Settings, *settings2, false);

        runtime2 = new JoynrClusterControllerRuntime(std::move(settings2));
    }

    void SetUp() {
        runtime1->start();
        runtime2->start();
    }

    void TearDown() {
        if (!providerParticipantId.empty()) {
            runtime1->unregisterProvider(providerParticipantId);
        }
        bool deleteChannel = true;
        runtime1->stop(deleteChannel);
        runtime2->stop(deleteChannel);

        // Delete persisted files
        std::remove(LibjoynrSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

    /*
     *  This wait is necessary, because subcriptions are async, and a publication could occur
     * before the subscription has started.
     */
    void waitForAttributeSubscriptionArrivedAtProvider(
            std::shared_ptr<tests::testAbstractProvider> testProvider,
            const std::string& attributeName)
    {
        std::uint64_t delay = 0;

        while (testProvider->attributeListeners.find(attributeName) == testProvider->attributeListeners.cend()
               && delay <= subscribeToAttributeWait
        ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            delay+=50;
        }

        EXPECT_FALSE(testProvider->attributeListeners.find(attributeName) == testProvider->attributeListeners.cend() ||
                    testProvider->attributeListeners.find(attributeName)->second.empty());
    }

    ~End2EndSubscriptionTest(){
        delete runtime1;
        delete runtime2;
    }

private:
    std::string providerParticipantId;
    DISALLOW_COPY_AND_ASSIGN(End2EndSubscriptionTest);

protected:
    std::shared_ptr<tests::DefaulttestProvider> registerProvider() {
        auto testProvider = std::make_shared<tests::DefaulttestProvider>();
        providerParticipantId = runtime1->registerProvider<tests::testProvider>(domainName, testProvider);

        //This wait is necessary, because registerProvider is async, and a lookup could occur
        // before the register has finished.
        std::this_thread::sleep_for(std::chrono::milliseconds(registerProviderWait));
        return testProvider;
    }

    std::unique_ptr<tests::testProxy> buildProxy() {
        std::unique_ptr<ProxyBuilder<tests::testProxy>> testProxyBuilder
                = runtime2->createProxyBuilder<tests::testProxy>(domainName);
        DiscoveryQos discoveryQos;
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(1000);
        discoveryQos.setRetryIntervalMs(250);

        std::int64_t qosRoundTripTTL = 500;

        std::unique_ptr<tests::testProxy> testProxy = testProxyBuilder
                ->setMessagingQos(MessagingQos(qosRoundTripTTL))
                ->setCached(false)
                ->setDiscoveryQos(discoveryQos)
                ->build();
        return std::move(testProxy);
    }

    template <typename ChangeAttribute, typename SubscribeTo, typename T>
    void testOneShotAttributeSubscription(const T& expectedValue,
                                          SubscribeTo subscribeTo,
                                          ChangeAttribute setAttribute,
                                          const std::string& attributeName) {
        MockSubscriptionListenerOneType<T>* mockListener =
                new MockSubscriptionListenerOneType<T>();

        // Use a semaphore to count and wait on calls to the mock listener
        ON_CALL(*mockListener, onReceive(Eq(expectedValue)))
                .WillByDefault(ReleaseSemaphore(&semaphore));

        std::shared_ptr<ISubscriptionListener<T>> subscriptionListener(
                        mockListener);

        std::shared_ptr<tests::DefaulttestProvider> testProvider = registerProvider();

        (*testProvider.*setAttribute)(expectedValue, [](){}, [](const joynr::exceptions::ProviderRuntimeException&) {});

        std::unique_ptr<tests::testProxy> testProxy = buildProxy();

        std::int64_t minInterval_ms = 50;
        auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                    500000,   // validity_ms
                    minInterval_ms);  // minInterval_ms

        subscribeTo(testProxy, subscriptionListener, subscriptionQos);
        waitForAttributeSubscriptionArrivedAtProvider(testProvider, attributeName);

        // Wait for a subscription message to arrive
        EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    }
};

} // namespace joynr

TEST_P(End2EndSubscriptionTest, waitForSuccessfulSubscriptionRegistration) {
    auto mockListener = new MockSubscriptionListenerOneType<int32_t>();

    // Use a semaphore to wait for calls to the mock listener
    std::string subscriptionIdFromListener;
    std::string subscriptionIdFromFuture;
    ON_CALL(*mockListener, onSubscribed(_))
            .WillByDefault(DoAll(SaveArg<0>(&subscriptionIdFromListener), ReleaseSemaphore(&semaphore)));

    std::shared_ptr<ISubscriptionListener<int32_t>> subscriptionListener(mockListener);

    std::shared_ptr<tests::DefaulttestProvider> testProvider = registerProvider();

    testProvider->setTestAttribute(42, [](){}, [](const joynr::exceptions::ProviderRuntimeException& error) {
        ADD_FAILURE() << "exception from setTestAttribute: " << error.getMessage(); });

    std::unique_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms
    std::shared_ptr<Future<std::string>> subscriptionIdFuture = testProxy->subscribeToTestAttribute(subscriptionListener, subscriptionQos);

    waitForAttributeSubscriptionArrivedAtProvider(testProvider, "testAttribute");

    // Wait for a subscription reply message to arrive
    JOYNR_EXPECT_NO_THROW(
        subscriptionIdFuture->get(5000, subscriptionIdFromFuture);
    );
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    EXPECT_EQ(subscriptionIdFromFuture, subscriptionIdFromListener);
}

TEST_P(End2EndSubscriptionTest, waitForSuccessfulSubscriptionUpdate) {
    auto mockListener = new MockSubscriptionListenerOneType<int32_t>();

    // Use a semaphore to wait for calls to the mock listener
    std::string subscriptionIdFromListener;
    std::string subscriptionIdFromFuture;
    ON_CALL(*mockListener, onSubscribed(_))
            .WillByDefault(DoAll(SaveArg<0>(&subscriptionIdFromListener), ReleaseSemaphore(&semaphore)));

    std::shared_ptr<ISubscriptionListener<int32_t>> subscriptionListener(mockListener);

    std::shared_ptr<tests::DefaulttestProvider> testProvider = registerProvider();

    testProvider->setTestAttribute(42, [](){}, [](const joynr::exceptions::ProviderRuntimeException& error) {
        ADD_FAILURE() << "exception from setTestAttribute: " << error.getMessage(); });

    std::unique_ptr<tests::testProxy> testProxy = buildProxy();

    std::int64_t minInterval_ms = 50;
    auto subscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                500000,   // validity_ms
                minInterval_ms);  // minInterval_ms
    std::shared_ptr<Future<std::string>> subscriptionIdFuture = testProxy->subscribeToTestAttribute(subscriptionListener, subscriptionQos);

    waitForAttributeSubscriptionArrivedAtProvider(testProvider, "testAttribute");

    // Wait for a subscription reply message to arrive
    JOYNR_EXPECT_NO_THROW(
        subscriptionIdFuture->get(5000, subscriptionIdFromFuture);
    );
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    EXPECT_EQ(subscriptionIdFromFuture, subscriptionIdFromListener);

    // update subscription
    subscriptionIdFuture = nullptr;
    std::string subscriptionId = subscriptionIdFromFuture;
    subscriptionIdFromFuture.clear();
    subscriptionIdFromListener.clear();
    subscriptionIdFuture = testProxy->subscribeToTestAttribute(subscriptionListener, subscriptionQos, subscriptionId);

    // Wait for a subscription reply message to arrive
    JOYNR_EXPECT_NO_THROW(
        subscriptionIdFuture->get(5000, subscriptionIdFromFuture);
    );
    EXPECT_TRUE(semaphore.waitFor(std::chrono::seconds(3)));
    EXPECT_EQ(subscriptionIdFromFuture, subscriptionIdFromListener);
    // subscription id from update is the same as the original subscription id
    EXPECT_EQ(subscriptionId, subscriptionIdFromFuture);
}

TEST_P(End2EndSubscriptionTest, subscribeToEnumAttribute) {
    tests::testTypes::TestEnum::Enum expectedTestEnum = tests::testTypes::TestEnum::TWO;

    testOneShotAttributeSubscription(expectedTestEnum,
                                 [](std::unique_ptr<tests::testProxy>& testProxy,
                                    std::shared_ptr<ISubscriptionListener<tests::testTypes::TestEnum::Enum>> subscriptionListener,
                                    std::shared_ptr<OnChangeSubscriptionQos> subscriptionQos) {
                                    testProxy->subscribeToEnumAttribute(subscriptionListener, subscriptionQos);
                                 },
                                 &tests::testProvider::setEnumAttribute,
                                 "enumAttribute");
}

TEST_P(End2EndSubscriptionTest, subscribeToByteBufferAttribute) {
    joynr::ByteBuffer expectedByteBuffer {0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0};

    testOneShotAttributeSubscription(expectedByteBuffer,
                                 [](std::unique_ptr<tests::testProxy>& testProxy,
                                    std::shared_ptr<ISubscriptionListener<joynr::ByteBuffer>> subscriptionListener,
                                    std::shared_ptr<OnChangeSubscriptionQos> subscriptionQos) {
                                    testProxy->subscribeToByteBufferAttribute(subscriptionListener, subscriptionQos);
                                 },
                                 &tests::testProvider::setByteBufferAttribute,
                                 "byteBufferAttribute");

}

INSTANTIATE_TEST_CASE_P(Http,
        End2EndSubscriptionTest,
        testing::Values(
            std::make_tuple(
                "test-resources/HttpSystemIntegrationTest1.settings",
                "test-resources/HttpSystemIntegrationTest2.settings"
            )
        )
);

INSTANTIATE_TEST_CASE_P(MqttWithHttpBackend,
        End2EndSubscriptionTest,
        testing::Values(
            std::make_tuple(
                "test-resources/MqttWithHttpBackendSystemIntegrationTest1.settings",
                "test-resources/MqttWithHttpBackendSystemIntegrationTest2.settings"
            )
        )
);
