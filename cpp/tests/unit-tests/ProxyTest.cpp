/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "joynr/JoynrMessagingConnectorFactory.h"
#include "joynr/ConnectorFactory.h"
#include "joynr/tests/testProxy.h"
#include "joynr/tests/TestWithoutVersionProxy.h"
#include "AbstractSyncAsyncTest.cpp"

using ::testing::A;
using ::testing::_;
using ::testing::Return;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::AllOf;
using ::testing::Property;
using ::testing::Invoke;
using ::testing::Unused;

using namespace joynr;

/**
 * These tests test the communication from the GpsProxy through to the JoynrMessageSender.
 */

/**
 * @brief Fixture.
 */
class ProxyTest : public AbstractSyncAsyncTest {
public:

    ProxyTest() :
        mockConnectorFactory(),
        mockInProcessConnectorFactory()
    {}
    void SetUp() {
        AbstractSyncAsyncTest::SetUp();
        mockInProcessConnectorFactory = new MockInProcessConnectorFactory();
        JoynrMessagingConnectorFactory* joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(mockJoynrMessageSender, (SubscriptionManager*) nullptr);
        mockConnectorFactory = new ConnectorFactory(mockInProcessConnectorFactory, joynrMessagingConnectorFactory);
    }

    void TearDown(){
        AbstractSyncAsyncTest::TearDown();
        delete mockConnectorFactory;
    }

    // sets the expectations on the call expected on the MessageSender from the connector
    testing::internal::TypedExpectation<void(
            const std::string&, // sender participant ID
            const std::string&, // receiver participant ID
            const MessagingQos&, // messaging QoS
            const Request&, // request object to send
            std::shared_ptr<IReplyCaller> // reply caller to notify when reply is received
    )>& setExpectationsForSendRequestCall(int expectedTypeId, std::string methodName) {
        return EXPECT_CALL(
                    *mockJoynrMessageSender,
                    sendRequest(
                        _, // sender participant ID
                        Eq(providerParticipantId), // receiver participant ID
                        _, // messaging QoS
                        Property(&Request::getMethodName, Eq(methodName)), // request object to send
                        Property(
                            &std::shared_ptr<IReplyCaller>::get,
                            AllOf(NotNull(), Property(&IReplyCaller::getTypeId, Eq(expectedTypeId)))
                        ) // reply caller to notify when reply is received
                    )
        );
    }

    tests::Itest* createFixture(bool cacheEnabled) {
        EXPECT_CALL(*mockInProcessConnectorFactory, canBeCreated(_)).WillRepeatedly(Return(false));
        tests::testProxy* proxy = new tests::testProxy(
                    endPointAddress,
                    mockConnectorFactory,
                    &mockClientCache,
                    "myDomain",
                    MessagingQos(),
                    cacheEnabled
                    );
        proxy->handleArbitrationFinished(providerParticipantId, joynr::types::CommunicationMiddleware::JOYNR);
        return dynamic_cast<tests::Itest*>(proxy);
    }

protected:
    ConnectorFactory* mockConnectorFactory;
    MockInProcessConnectorFactory* mockInProcessConnectorFactory;
private:
    DISALLOW_COPY_AND_ASSIGN(ProxyTest);
};

typedef ProxyTest ProxyTestDeathTest;

// need to stub the connector factory for it to always return a Joynr connector
TEST_F(ProxyTest, async_getAttributeNotCached) {
    testAsync_getAttributeNotCached();
}

TEST_F(ProxyTest, sync_setAttributeNotCached) {
    testSync_setAttributeNotCached();
}

TEST_F(ProxyTest, sync_getAttributeNotCached) {
    testSync_getAttributeNotCached();
}

TEST_F(ProxyTest, async_getAttributeCached) {
    testAsync_getAttributeCached();
}

TEST_F(ProxyTest, sync_getAttributeCached) {
    testSync_getAttributeCached();
}

TEST_F(ProxyTest, async_getterCallReturnsProviderRuntimeException) {
    testAsync_getterCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, sync_getterCallReturnsProviderRuntimeException) {
    testSync_getterCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, async_getterCallReturnsMethodInvocationException) {
    testAsync_getterCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, sync_getterCallReturnsMethodInvocationException) {
    testSync_getterCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, async_setterCallReturnsProviderRuntimeException) {
    testAsync_setterCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, sync_setterCallReturnsProviderRuntimeException) {
    testSync_setterCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, async_setterCallReturnsMethodInvocationException) {
    testAsync_setterCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, sync_setterCallReturnsMethodInvocationException) {
    testSync_setterCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, async_methodCallReturnsProviderRuntimeException) {
    testAsync_methodCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, sync_methodCallReturnsProviderRuntimeException) {
    testSync_methodCallReturnsProviderRuntimeException();
}

TEST_F(ProxyTest, async_methodCallReturnsMethodInvocationException) {
    testAsync_methodCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, sync_methodCallReturnsMethodInvocationException) {
    testSync_methodCallReturnsMethodInvocationException();
}

TEST_F(ProxyTest, async_methodCallReturnsErrorEnum) {
    testAsync_methodCallReturnsErrorEnum();
}

TEST_F(ProxyTest, sync_methodCallReturnsErrorEnum) {
    testSync_methodCallReturnsErrorEnum();
}

TEST_F(ProxyTest, async_methodCallReturnsExtendedErrorEnum) {
    testAsync_methodCallReturnsExtendedErrorEnum();
}

TEST_F(ProxyTest, sync_methodCallReturnsExtendedErrorEnum) {
    testSync_methodCallReturnsExtendedErrorEnum();
}

TEST_F(ProxyTest, async_methodCallReturnsInlineErrorEnum) {
    testAsync_methodCallReturnsInlineErrorEnum();
}

TEST_F(ProxyTest, sync_methodCallReturnsInlineErrorEnum) {
    testSync_methodCallReturnsInlineErrorEnum();
}

TEST_F(ProxyTest, async_OperationWithNoArguments) {
    testAsync_OperationWithNoArguments();
}

TEST_F(ProxyTest, sync_OperationWithNoArguments) {
    testSync_OperationWithNoArguments();
}

TEST_F(ProxyTest, subscribeToAttribute) {
    testSubscribeToAttribute();
}

TEST_F(ProxyTest, versionIsSetCorrectly) {
    std::uint32_t expectedMajorVersion = 47;
    std::uint32_t expectedMinorVersion = 11;
    EXPECT_EQ(expectedMajorVersion, tests::testProxy::MAJOR_VERSION);
    EXPECT_EQ(expectedMinorVersion, tests::testProxy::MINOR_VERSION);
}

TEST_F(ProxyTest, defaultVersionIsSetCorrectly) {
    std::uint32_t expectedDefaultMajorVersion = 0;
    std::uint32_t expectedDefaultMinorVersion = 0;
    EXPECT_EQ(expectedDefaultMajorVersion, tests::TestWithoutVersionProxy::MAJOR_VERSION);
    EXPECT_EQ(expectedDefaultMinorVersion, tests::TestWithoutVersionProxy::MINOR_VERSION);
}
