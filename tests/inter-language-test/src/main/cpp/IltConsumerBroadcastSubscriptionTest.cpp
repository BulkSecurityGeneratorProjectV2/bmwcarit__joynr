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
#include "joynr/OnChangeSubscriptionQos.h"
#include "joynr/Semaphore.h"

using namespace ::testing;

class IltConsumerBroadcastSubscriptionTest
        : public IltAbstractConsumerTest<::testing::TestWithParam<std::vector<std::string>>>
{
public:
    IltConsumerBroadcastSubscriptionTest()
            : subscriptionIdFutureTimeout(10000), publicationTimeout(10000)
    {
    }

protected:
    std::uint16_t subscriptionIdFutureTimeout;
    std::chrono::milliseconds publicationTimeout;
};

joynr::Logger iltConsumerBroadcastSubscriptionTestLogger("IltConsumerBroadcastSubscriptionTest");

class MockBroadcastWithSinglePrimitiveParameterBroadcastListener
        : public joynr::ISubscriptionListener<std::string>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(onReceive, void(const std::string& stringOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSinglePrimitiveParameter)
{
    Semaphore publicationSemaphore;
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    const std::vector<std::string> partitions = GetParam();
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);

    auto mockBroadcastWithSinglePrimitiveParameterBroadcastListener =
            std::make_shared<MockBroadcastWithSinglePrimitiveParameterBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithSinglePrimitiveParameterBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithSinglePrimitiveParameterBroadcastListener, onReceive(Eq("boom")))
            .Times(1)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    std::shared_ptr<ISubscriptionListener<std::string>> listener(
            mockBroadcastWithSinglePrimitiveParameterBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSinglePrimitiveParameter - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithSinglePrimitiveParameterBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeout, subscriptionId);

        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSinglePrimitiveParameter - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSinglePrimitiveParameter - fire broadcast");
        testInterfaceProxy->methodToFireBroadcastWithSinglePrimitiveParameter(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeout));

        testInterfaceProxy->unsubscribeFromBroadcastWithSinglePrimitiveParameterBroadcast(
                subscriptionId);
    });
}

class MockBroadcastWithMultiplePrimitiveParametersBroadcastListener
        : public joynr::ISubscriptionListener<double, std::string>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(onReceive, void(const double& doubleOut, const std::string& stringOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithMultiplePrimitiveParameters)
{
    Semaphore publicationSemaphore;
    double doubleOut;
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    const std::vector<std::string> partitions = GetParam();
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);

    auto mockBroadcastWithMultiplePrimitiveParametersBroadcastListener =
            std::make_shared<MockBroadcastWithMultiplePrimitiveParametersBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithMultiplePrimitiveParametersBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithMultiplePrimitiveParametersBroadcastListener,
                onReceive(_, Eq("boom")))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<0>(&doubleOut), ReleaseSemaphore(&publicationSemaphore)));
    typedef std::shared_ptr<ISubscriptionListener<double, std::string>> listenerType;
    listenerType listener(mockBroadcastWithMultiplePrimitiveParametersBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithMultiplePrimitiveParametersBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeout, subscriptionId);

        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultiplePrimitiveParameters - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultiplePrimitiveParameters - fire broadast");
        testInterfaceProxy->methodToFireBroadcastWithMultiplePrimitiveParameters(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeout));

        EXPECT_TRUE(IltUtil::cmpDouble(doubleOut, 1.1));

        testInterfaceProxy->unsubscribeFromBroadcastWithMultiplePrimitiveParametersBroadcast(
                subscriptionId);
    });
}

class MockBroadcastWithSingleArrayParameterBroadcastListener
        : public joynr::ISubscriptionListener<std::vector<std::string>>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(onReceive, void(const std::vector<std::string>& stringArrayOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSingleArrayParameter)
{
    Semaphore publicationSemaphore;
    std::string subscriptionId;
    std::vector<std::string> stringArrayOut;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    const std::vector<std::string> partitions = GetParam();
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);

    auto mockBroadcastWithSingleArrayParameterBroadcastListener =
            std::make_shared<MockBroadcastWithSingleArrayParameterBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithSingleArrayParameterBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithSingleArrayParameterBroadcastListener, onReceive(_))
            .Times(1)
            .WillRepeatedly(
                    DoAll(SaveArg<0>(&stringArrayOut), ReleaseSemaphore(&publicationSemaphore)));
    std::shared_ptr<ISubscriptionListener<std::vector<std::string>>> listener(
            mockBroadcastWithSingleArrayParameterBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleArrayParameter - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithSingleArrayParameterBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeout, subscriptionId);

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleArrayParameter - subscription registered");
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleArrayParameter - fire broadcast");
        testInterfaceProxy->methodToFireBroadcastWithSingleArrayParameter(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeout));

        EXPECT_TRUE(IltUtil::checkStringArray(stringArrayOut));

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleArrayParameterBroadcast(
                subscriptionId);
    });
}

class MockBroadcastWithMultipleArrayParametersBroadcastListener
        : public joynr::ISubscriptionListener<
                  std::vector<uint64_t>,
                  std::vector<
                          joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(
            onReceive,
            void(const std::vector<uint64_t>& uInt64ArrayOut,
                 const std::vector<
                         joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                         structWithStringArrayArrayOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithMultipleArrayParameters)
{
    Semaphore publicationSemaphore;
    std::string subscriptionId;
    std::vector<uint64_t> uInt64ArrayOut;
    std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>
            structWithStringArrayArrayOut;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    const std::vector<std::string> partitions = GetParam();
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);

    auto mockBroadcastWithMultipleArrayParametersBroadcastListener =
            std::make_shared<MockBroadcastWithMultipleArrayParametersBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithMultipleArrayParametersBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithMultipleArrayParametersBroadcastListener, onReceive(_, _))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<0>(&uInt64ArrayOut),
                                  SaveArg<1>(&structWithStringArrayArrayOut),
                                  ReleaseSemaphore(&publicationSemaphore)));
    typedef std::shared_ptr<ISubscriptionListener<
            std::vector<uint64_t>,
            std::vector<joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>>>
            listenerType;
    listenerType listener(mockBroadcastWithMultipleArrayParametersBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleArrayParameters - register subscription");

        testInterfaceProxy->subscribeToBroadcastWithMultipleArrayParametersBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeout, subscriptionId);

        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleArrayParameters - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleArrayParameters - fire broadcast");
        testInterfaceProxy->methodToFireBroadcastWithMultipleArrayParameters(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeout));

        EXPECT_TRUE(IltUtil::checkUInt64Array(uInt64ArrayOut));
        EXPECT_TRUE(IltUtil::checkStructWithStringArrayArray(structWithStringArrayArrayOut));

        testInterfaceProxy->unsubscribeFromBroadcastWithMultipleArrayParametersBroadcast(
                subscriptionId);
    });
}

class MockBroadcastWithSingleEnumerationParameterBroadcastListener
        : public joynr::ISubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::
                          ExtendedTypeCollectionEnumerationInTypeCollection::Enum>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(
            onReceive,
            void(const joynr::interlanguagetest::namedTypeCollection2::
                         ExtendedTypeCollectionEnumerationInTypeCollection::Enum& enumerationOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSingleEnumerationParameter)
{
    Semaphore publicationSemaphore;
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    const std::vector<std::string> partitions = GetParam();
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);

    auto mockBroadcastWithSingleEnumerationParameterBroadcastListener =
            std::make_shared<MockBroadcastWithSingleEnumerationParameterBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithSingleEnumerationParameterBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithSingleEnumerationParameterBroadcastListener,
                onReceive(Eq(joynr::interlanguagetest::namedTypeCollection2::
                                     ExtendedTypeCollectionEnumerationInTypeCollection::
                                             ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION)))
            .Times(1)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    typedef std::shared_ptr<ISubscriptionListener<
            joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedTypeCollectionEnumerationInTypeCollection::Enum>> listenerType;
    listenerType listener(mockBroadcastWithSingleEnumerationParameterBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithSingleEnumerationParameterBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeout, subscriptionId);

        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithSingleEnumerationParameter - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleEnumerationParameter - fire broadast");
        testInterfaceProxy->methodToFireBroadcastWithSingleEnumerationParameter(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeout));

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(
                subscriptionId);
    });
}

class MockBroadcastWithMultipleEnumerationParametersBroadcastListener
        : public joynr::ISubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::
                          ExtendedEnumerationWithPartlyDefinedValues::Enum,
                  joynr::interlanguagetest::Enumeration::Enum>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(
            onReceive,
            void(const joynr::interlanguagetest::namedTypeCollection2::
                         ExtendedEnumerationWithPartlyDefinedValues::Enum& extendedEnumerationOut,
                 const joynr::interlanguagetest::Enumeration::Enum& enumerationOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest,
       callSubscribeBroadcastWithMultipleEnumerationParameters)
{
    Semaphore publicationSemaphore;
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    const std::vector<std::string> partitions = GetParam();
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);

    auto mockBroadcastWithMultipleEnumerationParametersBroadcastListener =
            std::make_shared<MockBroadcastWithMultipleEnumerationParametersBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithMultipleEnumerationParametersBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(
            *mockBroadcastWithMultipleEnumerationParametersBroadcastListener,
            onReceive(Eq(joynr::interlanguagetest::namedTypeCollection2::
                                 ExtendedEnumerationWithPartlyDefinedValues::
                                         ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES),
                      Eq(joynr::interlanguagetest::Enumeration::ENUM_0_VALUE_1)))
            .Times(1)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    typedef std::shared_ptr<
            ISubscriptionListener<joynr::interlanguagetest::namedTypeCollection2::
                                          ExtendedEnumerationWithPartlyDefinedValues::Enum,
                                  joynr::interlanguagetest::Enumeration::Enum>> listenerType;
    listenerType listener(mockBroadcastWithMultipleEnumerationParametersBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleEnumerationParameters - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithMultipleEnumerationParametersBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeout, subscriptionId);

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMulti"
                       "pleEnumerationParameters - "
                       "subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleEnumerationParameters - fire broadast");
        testInterfaceProxy->methodToFireBroadcastWithMultipleEnumerationParameters(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeout));

        testInterfaceProxy->unsubscribeFromBroadcastWithMultipleEnumerationParametersBroadcast(
                subscriptionId);
    });
}

class MockBroadcastWithSingleStructParameterBroadcastListener
        : public joynr::ISubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(
            onReceive,
            void(const joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives&
                         extendedStructOfPrimitivesOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithSingleStructParameter)
{
    Semaphore publicationSemaphore;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives
            extendedStructOfPrimitivesOut;
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    const std::vector<std::string> partitions = GetParam();
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);

    auto mockBroadcastWithSingleStructParameterBroadcastListener =
            std::make_shared<MockBroadcastWithSingleStructParameterBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithSingleStructParameterBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithSingleStructParameterBroadcastListener, onReceive(_))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<0>(&extendedStructOfPrimitivesOut),
                                  ReleaseSemaphore(&publicationSemaphore)));
    typedef std::shared_ptr<ISubscriptionListener<
            joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives>>
            listenerType;
    listenerType listener(mockBroadcastWithSingleStructParameterBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleStructParameter - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithSingleStructParameterBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeout, subscriptionId);

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleStructParameter - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithSingleStructParameter - fire broadast");
        testInterfaceProxy->methodToFireBroadcastWithSingleStructParameter(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeout));

        EXPECT_TRUE(IltUtil::checkExtendedStructOfPrimitives(extendedStructOfPrimitivesOut));

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleStructParameterBroadcast(
                subscriptionId);
    });
}

class MockBroadcastWithMultipleStructParametersBroadcastListener
        : public joynr::ISubscriptionListener<
                  joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements,
                  joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct>
{
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2(
            onReceive,
            void(const joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements&
                         baseStructWithoutElementsOut,
                 const joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct&
                         extendedExtendedBaseStructOut));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException& error));
};

TEST_P(IltConsumerBroadcastSubscriptionTest, callSubscribeBroadcastWithMultipleStructParameters)
{
    Semaphore publicationSemaphore;
    joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements
            baseStructWithoutElementsOut;
    joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct
            extendedExtendedBaseStructOut;
    std::string subscriptionId;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    const std::vector<std::string> partitions = GetParam();
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);

    auto mockBroadcastWithMultipleStructParametersBroadcastListener =
            std::make_shared<MockBroadcastWithMultipleStructParametersBroadcastListener>();
    EXPECT_CALL(*mockBroadcastWithMultipleStructParametersBroadcastListener, onError(_))
            .Times(0)
            .WillRepeatedly(ReleaseSemaphore(&publicationSemaphore));
    EXPECT_CALL(*mockBroadcastWithMultipleStructParametersBroadcastListener, onReceive(_, _))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<0>(&baseStructWithoutElementsOut),
                                  SaveArg<1>(&extendedExtendedBaseStructOut),
                                  ReleaseSemaphore(&publicationSemaphore)));
    typedef std::shared_ptr<ISubscriptionListener<
            joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements,
            joynr::interlanguagetest::namedTypeCollection2::ExtendedExtendedBaseStruct>>
            listenerType;
    listenerType listener(mockBroadcastWithMultipleStructParametersBroadcastListener);
    JOYNR_ASSERT_NO_THROW({
        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleStructParameters - register subscription");
        testInterfaceProxy->subscribeToBroadcastWithMultipleStructParametersBroadcast(
                                    listener, subscriptionQos, partitions)
                ->get(subscriptionIdFutureTimeout, subscriptionId);

        JOYNR_LOG_INFO(
                iltConsumerBroadcastSubscriptionTestLogger,
                "callSubscribeBroadcastWithMultipleStructParameters - subscription registered");

        JOYNR_LOG_INFO(iltConsumerBroadcastSubscriptionTestLogger,
                       "callSubscribeBroadcastWithMultipleStructParameters - fire broadast");
        testInterfaceProxy->methodToFireBroadcastWithMultipleStructParameters(partitions);
        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeout));

        EXPECT_TRUE(IltUtil::checkBaseStructWithoutElements(baseStructWithoutElementsOut));
        EXPECT_TRUE(IltUtil::checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut));

        testInterfaceProxy->unsubscribeFromBroadcastWithMultipleStructParametersBroadcast(
                subscriptionId);
    });
}

TEST_F(IltConsumerBroadcastSubscriptionTest, doNotReceivePublicationsForOtherPartitions)
{
    Semaphore publicationSemaphore;
    int64_t minInterval_ms = 0;
    int64_t validity = 60000;
    auto subscriptionQos =
            std::make_shared<joynr::OnChangeSubscriptionQos>(validity, minInterval_ms);
    auto mockBroadcastWithSingleEnumerationParameter =
            std::make_shared<MockBroadcastWithSingleEnumerationParameterBroadcastListener>();

    EXPECT_CALL(*mockBroadcastWithSingleEnumerationParameter, onReceive(_)).Times(1).WillRepeatedly(
            ReleaseSemaphore(&publicationSemaphore));

    const std::vector<std::string> subscribeToPartitions{"partition0", "partition1"};
    const std::vector<std::string> broadcastPartitions{"otherPartition"};

    JOYNR_ASSERT_NO_THROW({
        std::string subscriptionId;
        auto subscriptionIdFuture =
                testInterfaceProxy->subscribeToBroadcastWithSingleEnumerationParameterBroadcast(
                        mockBroadcastWithSingleEnumerationParameter,
                        subscriptionQos,
                        subscribeToPartitions);

        subscriptionIdFuture->get(subscriptionIdFutureTimeout, subscriptionId);

        testInterfaceProxy->methodToFireBroadcastWithSingleEnumerationParameter(
                broadcastPartitions);

        // No broadcast shall be received because the partitions do not match.
        ASSERT_FALSE(publicationSemaphore.waitFor(std::chrono::milliseconds(2000)));

        // Ensure that we did not receive a publication for another reasons
        testInterfaceProxy->methodToFireBroadcastWithSingleEnumerationParameter(
                subscribeToPartitions);

        ASSERT_TRUE(publicationSemaphore.waitFor(publicationTimeout));

        testInterfaceProxy->unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(
                subscriptionId);
    });
}

INSTANTIATE_TEST_CASE_P(NoPartitions,
                        IltConsumerBroadcastSubscriptionTest,
                        ::testing::Values(std::vector<std::string>()));

INSTANTIATE_TEST_CASE_P(WithPartitions,
                        IltConsumerBroadcastSubscriptionTest,
                        ::testing::Values(std::vector<std::string>({"partition0", "partition1"})));
