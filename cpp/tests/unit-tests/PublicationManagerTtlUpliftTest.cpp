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
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/IJoynrMessageSender.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/Logger.h"
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Semaphore.h"
#include "joynr/SingleThreadedIOService.h"

#include "joynr/tests/testRequestInterpreter.h"

#include "tests/utils/MockObjects.h"

using ::testing::A;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Between;
using ::testing::ByRef;
using ::testing::ReturnRef;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::Matcher;
using ::testing::MakeMatcher;
using ::testing::Mock;

using namespace joynr;

ACTION_P(ReleaseSemaphore, semaphore)
{
    semaphore->notify();
}

MATCHER_P3(messagingQosWithTtl, expectedTtlMs, toleranceMs, logger, "") {
    std::int64_t actual = arg.getTtl();
    std::int64_t diff = expectedTtlMs - actual;
    if (diff <= toleranceMs) {
        return true;
    }
    JOYNR_LOG_ERROR(logger, "TTL={} differs {}ms (more than {}ms) from the expected value={}", actual, diff, toleranceMs, expectedTtlMs);
    return false;
}

class PublicationManagerTtlUpliftTest : public testing::Test {
public:
    PublicationManagerTtlUpliftTest() :
        singleThreadedIOService(),
        messageSender(new MockJoynrMessageSender()),
        proxyId("ProxyId"),
        providerId("ProviderId"),
        ttlUpliftMs(300),
        minInterval_ms(0),
        publicationTtlMs(1024),
        toleranceMs(50)
    {
        singleThreadedIOService.start();
        onChangeSubscriptionQos = std::make_shared<OnChangeSubscriptionQos>(
                        0,
                        publicationTtlMs,
                        minInterval_ms
                    );
        onChangeSubscriptionQos->setPublicationTtlMs(publicationTtlMs);
    }

    ~PublicationManagerTtlUpliftTest() {
        //remove stored subscriptions;
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        //remove stored broadcastsubscriptions
        std::remove(LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        delete messageSender;
    }

protected:

    void testSubscriptionWithoutTtlUplift(const std::string& proxyId,
                                          const std::string& providerId,
                                          MockPublicationSender& mockPublicationSender,
                                          std::shared_ptr<PublicationManager> publicationManager,
                                          SubscriptionRequest& subscriptionRequest,
                                          bool isBroadcastSubscription,
                                          std::int64_t sleepDurationMs,
                                          std::int64_t expectedSubscriptionReplyTtlMs,
                                          std::int64_t expectedPublicationTtlMs,
                                          std::function<void()> triggerPublication);

    void expectNoMoreSubscriptionPublications(MockPublicationSender& mockPublicationSender,
                                              std::function<void()> triggerPublication);
    void expectAdditionalSubscriptionPublication(const std::string& proxyId,
                                                                const std::string& providerId,
                                                                MockPublicationSender& mockPublicationSender,
                                                                std::int64_t expectedPublicationTtlMs,
                                                                std::function<void()> triggerPublication);
    void testSubscriptionWithTtlUplift(const std::string& proxyId,
                                       const std::string& providerId,
                                       std::shared_ptr<PublicationManager> publicationManager,
                                       MockPublicationSender& mockPublicationSender,
                                       SubscriptionRequest& subscriptionRequest,
                                       bool isBroadcastSubscription,
                                       std::int64_t sleepDurationMs,
                                       std::int64_t expectedSubscriptionReplyTtlMs,
                                       std::int64_t expectedPublicationTtlMs,
                                       std::function<void()> triggerPublication);
    SingleThreadedIOService singleThreadedIOService;
    IJoynrMessageSender* messageSender;

    std::string proxyId;
    std::string providerId;
    MockPublicationSender mockPublicationSender;

    std::uint64_t ttlUpliftMs;
    std::int64_t minInterval_ms;
    std::int64_t publicationTtlMs;
    std::int64_t toleranceMs;
    std::shared_ptr<OnChangeSubscriptionQos> onChangeSubscriptionQos;

    ADD_LOGGER(PublicationManagerTtlUpliftTest);
};

INIT_LOGGER(PublicationManagerTtlUpliftTest);

void PublicationManagerTtlUpliftTest::testSubscriptionWithoutTtlUplift(
                                      const std::string& proxyId,
                                      const std::string& providerId,
                                      MockPublicationSender& mockPublicationSender,
                                      std::shared_ptr<PublicationManager> publicationManager,
                                      SubscriptionRequest& subscriptionRequest,
                                      bool isBroadcastSubscription,
                                      std::int64_t sleepDurationMs,
                                      std::int64_t expectedSubscriptionReplyTtlMs,
                                      std::int64_t expectedPublicationTtlMs,
                                      std::function<void()> triggerPublication) {
    joynr::Semaphore semaphore(0);
    // Register the request interpreter that calls the request caller
    InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>("tests/Test");

    auto requestCaller = std::make_shared<MockTestRequestCaller>();

    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionReply(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId), // receiver participant ID
                    messagingQosWithTtl(expectedSubscriptionReplyTtlMs, toleranceMs, logger), // messaging QoS
                    _ // subscription reply
                )
    )
            .Times(1);

    // sending initial value plus the attributeValueChanged
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublicationMock(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId), // receiver participant ID
                    messagingQosWithTtl(expectedPublicationTtlMs, 0l, logger), // messaging QoS
                    _ // subscription publication
                )
    )
            .Times(2)
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    JOYNR_LOG_DEBUG(logger, "adding request");

    if (isBroadcastSubscription) {
        publicationManager->add(proxyId, providerId, requestCaller, static_cast<BroadcastSubscriptionRequest&>(subscriptionRequest), &mockPublicationSender);
        // fire initial broadcast
        triggerPublication();
    } else {
        publicationManager->add(proxyId, providerId, requestCaller, subscriptionRequest, &mockPublicationSender);
    }

    // wait for initial publication
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(50)));

    triggerPublication();

    std::this_thread::sleep_for(std::chrono::milliseconds(sleepDurationMs + toleranceMs));
    EXPECT_EQ(1, semaphore.getStatus());

}

void PublicationManagerTtlUpliftTest::expectNoMoreSubscriptionPublications(MockPublicationSender& mockPublicationSender,
                                                                  std::function<void()> triggerPublication) {
    Mock::VerifyAndClearExpectations(&mockPublicationSender);

    joynr::Semaphore semaphore(0);
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublicationMock(
                    _, // sender participant ID
                    _, // receiver participant ID
                    _, // messaging QoS
                    _ // subscription publication
                )
    )
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    triggerPublication();
    EXPECT_FALSE(semaphore.waitFor(std::chrono::milliseconds(200)));
}

void PublicationManagerTtlUpliftTest::expectAdditionalSubscriptionPublication(const std::string& proxyId,
                                                                                    const std::string& providerId,
                                                                                    MockPublicationSender& mockPublicationSender,
                                                                                    std::int64_t expectedPublicationTtlMs,
                                                                                    std::function<void()> triggerPublication) {
    Mock::VerifyAndClearExpectations(&mockPublicationSender);

    joynr::Semaphore semaphore(0);
    EXPECT_CALL(
                mockPublicationSender,
                sendSubscriptionPublicationMock(
                    Eq(providerId), // sender participant ID
                    Eq(proxyId), // receiver participant ID
                    messagingQosWithTtl(expectedPublicationTtlMs, 0l, logger), // messaging QoS
                    _ // subscription publication
                )
    )
            .Times(1)
            .WillOnce(ReleaseSemaphore(&semaphore));

    triggerPublication();
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(200)));
}

void PublicationManagerTtlUpliftTest::testSubscriptionWithTtlUplift(const std::string& proxyId,
                                                           const std::string& providerId,
                                                           std::shared_ptr<PublicationManager> publicationManager,
                                                           MockPublicationSender& mockPublicationSender,
                                                           SubscriptionRequest& subscriptionRequest,
                                                           bool isBroadcastSubscription,
                                                           std::int64_t sleepDurationMs,
                                                           std::int64_t expectedSubscriptionReplyTtlMs,
                                                           std::int64_t expectedPublicationTtlMs,
                                                           std::function<void()> triggerPublication) {
    testSubscriptionWithoutTtlUplift(proxyId, providerId, mockPublicationSender,
                                     publicationManager, subscriptionRequest,
                                     isBroadcastSubscription, sleepDurationMs,
                                     expectedSubscriptionReplyTtlMs, expectedPublicationTtlMs,
                                     triggerPublication);

    expectAdditionalSubscriptionPublication(proxyId, providerId, mockPublicationSender,
                                                           expectedPublicationTtlMs, triggerPublication);

    std::this_thread::sleep_for(std::chrono::milliseconds(ttlUpliftMs));
}

TEST_F(PublicationManagerTtlUpliftTest, testAttributeSubscriptionWithoutTtlUplift) {
    auto publicationManager = std::make_shared<PublicationManager>(singleThreadedIOService.getIOService(), messageSender, 0);

    //SubscriptionRequest
    std::string attributeName("Location");
    std::int64_t validity_ms = 300;
    onChangeSubscriptionQos->setValidityMs(validity_ms);
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = validity_ms;
    std::int64_t expectedSubscriptionReplyTtlMs = validity_ms;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation attributeValue;
        publicationManager->attributeValueChanged(subscriptionId, attributeValue);
    };

    testSubscriptionWithoutTtlUplift(proxyId,
                                     providerId,
                                     mockPublicationSender,
                                     publicationManager,
                                     subscriptionRequest,
                                     false,
                                     sleepDurationMs,
                                     expectedSubscriptionReplyTtlMs,
                                     expectedPublicationTtlMs,
                                     triggerPublication);

    expectNoMoreSubscriptionPublications(mockPublicationSender,
                                         triggerPublication);
}

TEST_F(PublicationManagerTtlUpliftTest, testBroadcastSubscriptionWithoutTtlUplift) {

    auto publicationManager = std::make_shared<PublicationManager>(singleThreadedIOService.getIOService(), messageSender, 0);

    //SubscriptionRequest
    std::string broadcastName("Location");
    std::int64_t validity_ms = 300;
    onChangeSubscriptionQos->setValidityMs(validity_ms);
    BroadcastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = validity_ms;
    std::int64_t expectedSubscriptionReplyTtlMs = validity_ms;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation valueToPublish;
        publicationManager->broadcastOccurred(subscriptionId, valueToPublish);
    };

    testSubscriptionWithoutTtlUplift(proxyId,
                                     providerId,
                                     mockPublicationSender,
                                     publicationManager,
                                     subscriptionRequest,
                                     true,
                                     sleepDurationMs,
                                     expectedSubscriptionReplyTtlMs,
                                     expectedPublicationTtlMs,
                                     triggerPublication);

    expectNoMoreSubscriptionPublications(mockPublicationSender,
                                         triggerPublication);
}

TEST_F(PublicationManagerTtlUpliftTest, testAttributeSubscriptionWithTtlUplift) {

    auto publicationManager = std::make_shared<PublicationManager>(singleThreadedIOService.getIOService(), messageSender, ttlUpliftMs);

    //SubscriptionRequest
    std::string attributeName("Location");
    std::int64_t validity_ms = 300;
    onChangeSubscriptionQos->setValidityMs(validity_ms);
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = validity_ms;
    std::int64_t expectedSubscriptionReplyTtlMs = validity_ms;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation attributeValue;
        publicationManager->attributeValueChanged(subscriptionId, attributeValue);
    };

    testSubscriptionWithTtlUplift(proxyId,
                                  providerId,
                                  publicationManager,
                                  mockPublicationSender,
                                  subscriptionRequest,
                                  false,
                                  sleepDurationMs,
                                  expectedSubscriptionReplyTtlMs,
                                  expectedPublicationTtlMs,
                                  triggerPublication);

    expectNoMoreSubscriptionPublications(mockPublicationSender, triggerPublication);
}

TEST_F(PublicationManagerTtlUpliftTest, testBroadcastSubscriptionWithTtlUplift) {

    auto publicationManager = std::make_shared<PublicationManager>(singleThreadedIOService.getIOService(), messageSender, ttlUpliftMs);

    //SubscriptionRequest
    std::string broadcastName("Location");
    std::int64_t validity_ms = 300;
    onChangeSubscriptionQos->setValidityMs(validity_ms);
    BroadcastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = validity_ms;
    std::int64_t expectedSubscriptionReplyTtlMs = validity_ms;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation valueToPublish;
        publicationManager->broadcastOccurred(subscriptionId, valueToPublish);
    };

    testSubscriptionWithTtlUplift(proxyId,
                                  providerId,
                                  publicationManager,
                                  mockPublicationSender,
                                  subscriptionRequest,
                                  true,
                                  sleepDurationMs,
                                  expectedSubscriptionReplyTtlMs,
                                  expectedPublicationTtlMs,
                                  triggerPublication);

    expectNoMoreSubscriptionPublications(mockPublicationSender, triggerPublication);
}

TEST_F(PublicationManagerTtlUpliftTest, testAttributeSubscriptionWithTtlUpliftWithNoExpiryDate) {

    auto publicationManager = std::make_shared<PublicationManager>(singleThreadedIOService.getIOService(), messageSender, ttlUpliftMs);

    //SubscriptionRequest
    std::string attributeName("Location");
    onChangeSubscriptionQos->setExpiryDateMs(SubscriptionQos::NO_EXPIRY_DATE());
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = 300;
    std::int64_t expectedSubscriptionReplyTtlMs = INT64_MAX;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation attributeValue;
        publicationManager->attributeValueChanged(subscriptionId, attributeValue);
    };

    testSubscriptionWithTtlUplift(proxyId,
                                  providerId,
                                  publicationManager,
                                  mockPublicationSender,
                                  subscriptionRequest,
                                  false,
                                  sleepDurationMs,
                                  expectedSubscriptionReplyTtlMs,
                                  expectedPublicationTtlMs,
                                  triggerPublication);

    expectAdditionalSubscriptionPublication(proxyId, providerId, mockPublicationSender,
                                                           expectedPublicationTtlMs, triggerPublication);
}

TEST_F(PublicationManagerTtlUpliftTest, testBroadcastSubscriptionWithTtlUpliftWithNoExpiryDate) {

    auto publicationManager = std::make_shared<PublicationManager>(singleThreadedIOService.getIOService(), messageSender, ttlUpliftMs);

    //SubscriptionRequest
    std::string broadcastName("Location");
    onChangeSubscriptionQos->setExpiryDateMs(SubscriptionQos::NO_EXPIRY_DATE());
    BroadcastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = 300;
    std::int64_t expectedSubscriptionReplyTtlMs = INT64_MAX;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation valueToPublish;
        publicationManager->broadcastOccurred(subscriptionId, valueToPublish);
    };

    testSubscriptionWithTtlUplift(proxyId,
                                  providerId,
                                  publicationManager,
                                  mockPublicationSender,
                                  subscriptionRequest,
                                  true,
                                  sleepDurationMs,
                                  expectedSubscriptionReplyTtlMs,
                                  expectedPublicationTtlMs,
                                  triggerPublication);

    expectAdditionalSubscriptionPublication(proxyId, providerId, mockPublicationSender,
                                                           expectedPublicationTtlMs, triggerPublication);
}

TEST_F(PublicationManagerTtlUpliftTest, testAttributeSubscriptionWithTtlUpliftWithLargeExpiryDate) {

    auto publicationManager = std::make_shared<PublicationManager>(singleThreadedIOService.getIOService(), messageSender, ttlUpliftMs);

    //SubscriptionRequest
    std::string attributeName("Location");
    std::int64_t expiryDateMs = INT64_MAX - ttlUpliftMs + 1;
    onChangeSubscriptionQos->setExpiryDateMs(expiryDateMs);
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = 300;
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::int64_t expectedSubscriptionReplyTtlMs = expiryDateMs - now;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation attributeValue;
        publicationManager->attributeValueChanged(subscriptionId, attributeValue);
    };

    testSubscriptionWithTtlUplift(proxyId,
                                  providerId,
                                  publicationManager,
                                  mockPublicationSender,
                                  subscriptionRequest,
                                  false,
                                  sleepDurationMs,
                                  expectedSubscriptionReplyTtlMs,
                                  expectedPublicationTtlMs,
                                  triggerPublication);

    expectAdditionalSubscriptionPublication(proxyId, providerId, mockPublicationSender,
                                                           expectedPublicationTtlMs, triggerPublication);
}

TEST_F(PublicationManagerTtlUpliftTest, testBroadcastSubscriptionWithTtlUpliftWithLargeExpiryDate) {

    auto publicationManager = std::make_shared<PublicationManager>(singleThreadedIOService.getIOService(), messageSender, ttlUpliftMs);

    //SubscriptionRequest
    std::string broadcastName("Location");
    std::int64_t expiryDateMs = INT64_MAX - ttlUpliftMs + 1;
    onChangeSubscriptionQos->setExpiryDateMs(expiryDateMs);
    BroadcastSubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscribeToName(broadcastName);
    subscriptionRequest.setQos(onChangeSubscriptionQos);

    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    std::int64_t sleepDurationMs = 300;
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::int64_t expectedSubscriptionReplyTtlMs = expiryDateMs - now;
    std::int64_t expectedPublicationTtlMs = publicationTtlMs;

    auto triggerPublication = [publicationManager, subscriptionId]() {
        joynr::types::Localisation::GpsLocation valueToPublish;
        publicationManager->broadcastOccurred(subscriptionId, valueToPublish);
    };

    testSubscriptionWithTtlUplift(proxyId,
                                  providerId,
                                  publicationManager,
                                  mockPublicationSender,
                                  subscriptionRequest,
                                  true,
                                  sleepDurationMs,
                                  expectedSubscriptionReplyTtlMs,
                                  expectedPublicationTtlMs,
                                  triggerPublication);

    expectAdditionalSubscriptionPublication(proxyId, providerId, mockPublicationSender,
                                                           expectedPublicationTtlMs, triggerPublication);
}
