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
#include "IltAbstractConsumerTest.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/SubscriptionListener.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include "joynr/Semaphore.h"

using namespace ::testing;

class IltConsumerAttributeSubscriptionTest : public IltAbstractConsumerTest
{
public:
    IltConsumerAttributeSubscriptionTest()
            : subscriptionIdFutureTimeout(10000),
              subscriptionRegisteredTimeout(10000),
              publicationTimeout(10000)
    {
    }

protected:
    std::uint16_t subscriptionIdFutureTimeout;
    std::chrono::milliseconds subscriptionRegisteredTimeout;
    std::chrono::milliseconds publicationTimeout;
};

joynr::Logger iltConsumerAttributeSubscriptionTestLogger("IltConsumerAttributeSubscriptionTest");

// SUBSCRIBE ATTRIBUTE ENUMERATION

class MockEnumerationSubscriptionListener
        : public joynr::ISubscriptionListener<joynr::interlanguagetest::Enumeration::Enum>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(onReceive,
                 void(const joynr::interlanguagetest::Enumeration::Enum& enumerationOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_F(IltConsumerAttributeSubscriptionTest, callSubscribeAttributeEnumeration)
{
    Semaphore subscriptionRegisteredSemaphore;
    Semaphore publicationSemaphore;
    joynr::interlanguagetest::Enumeration::Enum enumerationArg =
            joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_2;
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);

    auto mockEnumerationSubscriptionListener =
            std::make_shared<MockEnumerationSubscriptionListener>();
    ON_CALL(*mockEnumerationSubscriptionListener, onSubscribed(_))
            .WillByDefault(ReleaseSemaphore(&subscriptionRegisteredSemaphore));
    EXPECT_CALL(*mockEnumerationSubscriptionListener, onError(_)).Times(0).WillRepeatedly(
            ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockEnumerationSubscriptionListener, onReceive(Eq(enumerationArg)))
            .Times(1)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    std::shared_ptr<ISubscriptionListener<joynr::interlanguagetest::Enumeration::Enum>> listener(
            mockEnumerationSubscriptionListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                       "callSubscribeAttributeEnumeration - set attributeEnumeration");
        testInterfaceProxy->setAttributeEnumeration(enumerationArg);

        JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                       "callSubscribeAttributeEnumeration - register subscription");
        testInterfaceProxy->subscribeToAttributeEnumeration(listener, subscriptionQos)
                ->get(subscriptionId);

        ASSERT_TRUE(subscriptionRegisteredSemaphore.waitFor(std::chrono::milliseconds(5000)));
        JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                       "callSubscribeAttributeEnumeration - subscription registered");

        ASSERT_TRUE(publicationSemaphore.waitFor(std::chrono::milliseconds(5000)));

        testInterfaceProxy->unsubscribeFromAttributeEnumeration(subscriptionId);
    });
}

// SUBSCRIBE ATTRIBUTE WITH EXCEPTION FROM GETTER

class MockBoolSubscriptionListener : public joynr::ISubscriptionListener<bool>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(onReceive, void(const bool& boolOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

MATCHER_P(providerRuntimeException, msg, "")
{
    return arg.getTypeName() == joynr::exceptions::ProviderRuntimeException::TYPE_NAME() &&
           arg.getMessage() == msg;
}

TEST_F(IltConsumerAttributeSubscriptionTest, callSubscribeAttributeWithExceptionFromGetter)
{
    Semaphore subscriptionRegisteredSemaphore;
    Semaphore publicationSemaphore;
    exceptions::JoynrRuntimeException error;
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);

    auto mockEnumerationSubscriptionListener = std::make_shared<MockBoolSubscriptionListener>();
    ON_CALL(*mockEnumerationSubscriptionListener, onSubscribed(_))
            .WillByDefault(ReleaseSemaphore(&subscriptionRegisteredSemaphore));
    EXPECT_CALL(
            *mockEnumerationSubscriptionListener,
            onError(providerRuntimeException("Exception from getAttributeWithExceptionFromGetter")))
            .Times(1)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockEnumerationSubscriptionListener, onReceive(_)).Times(0).WillRepeatedly(
            ReleaseSemaphore(&publicationSemaphore));
    std::shared_ptr<ISubscriptionListener<bool>> listener(mockEnumerationSubscriptionListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                       "callSubscribeAttributeWithExceptionFromGetter - register subscription");
        testInterfaceProxy->subscribeToAttributeWithExceptionFromGetter(listener, subscriptionQos)
                ->get(subscriptionId);
        ASSERT_TRUE(subscriptionRegisteredSemaphore.waitFor(std::chrono::milliseconds(5000)));
        JOYNR_LOG_INFO(iltConsumerAttributeSubscriptionTestLogger,
                       "callSubscribeAttributeWithExceptionFromGetter - subscription registered");

        ASSERT_TRUE(publicationSemaphore.waitFor(std::chrono::milliseconds(5000)));

        testInterfaceProxy->unsubscribeFromAttributeWithExceptionFromGetter(subscriptionId);
    });
}
