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

#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/Request.h"
#include "joynr/Reply.h"
#include "joynr/SubscriptionReply.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/ReplyCaller.h"
#include "joynr/IReplyCaller.h"
#include "tests/utils/MockObjects.h"
#include "utils/MockCallback.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/exceptions/MethodInvocationException.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/serializer/Serializer.h"

using ::testing::A;
using ::testing::_;
using ::testing::Return;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::AllOf;
using ::testing::Property;
using ::testing::Invoke;
using ::testing::Unused;
using ::testing::Pointee;
using namespace joynr;

/**
 * These tests test the communication from X through to the JoynrMessageSender.
 * Some methods are defined here, that are used in ProxyTest and GpsJoynrMessagingConnectorTest
 */

class CallBackActions {
public:
    CallBackActions(joynr::types::Localisation::GpsLocation expectedGpsLocation, int expectedInt) :
        expectedGpsLocation(expectedGpsLocation),
        expectedInt(expectedInt)
    {
    }
    // for test: sync_setAttributeNotCached
    void executeCallBackVoidResult(
            Unused, // sender participant ID
            Unused, // receiver participant ID
            Unused, // messaging QoS
            Unused, // request object to send
            std::shared_ptr<IReplyCaller> callback // reply caller to notify when reply is received
    ) {
        (std::dynamic_pointer_cast<ReplyCaller<void> >(callback))->returnValue();
    }

    // related to test: sync_getAttributeNotCached
    void executeCallBackGpsLocationResult(
            Unused, // sender participant ID
            Unused, // receiver participant ID
            Unused, // messaging QoS
            Unused, // request object to send
            std::shared_ptr<IReplyCaller> callback // reply caller to notify when reply is received
    ) {
       (std::dynamic_pointer_cast<ReplyCaller<types::Localisation::GpsLocation> >(callback))->returnValue(expectedGpsLocation);
    }

    // related to test: sync_OperationWithNoArguments
    void executeCallBackIntResult(
            Unused, // sender participant ID
            Unused, // receiver participant ID
            Unused, // messaging QoS
            Unused, // request object to send
            std::shared_ptr<IReplyCaller> callback // reply caller to notify when reply is received
    ) {

        std::dynamic_pointer_cast<ReplyCaller<int> >(callback)->returnValue(expectedInt);
    }
private:
    joynr::types::Localisation::GpsLocation expectedGpsLocation;
    int expectedInt;
};


/**
 * @brief Fixutre.
 */
class AbstractSyncAsyncTest : public ::testing::Test {
public:
    AbstractSyncAsyncTest():
        expectedGpsLocation(1.1, 1.2, 1.3, types::Localisation::GpsFixEnum::MODE3D, 1.4, 1.5, 1.6, 1.7, 18, 19, 95302963),
        expectedInt(60284917),
        callBackActions(expectedGpsLocation, expectedInt),
        qosSettings(),
        mockDispatcher(),
        mockMessagingStub(),
        callBack(),
        mockJoynrMessageSender(),
        proxyParticipantId(),
        providerParticipantId(),
        mockClientCache(),
        endPointAddress(),
        asyncTestFixture(nullptr),
        error(nullptr)
    {}
    virtual ~AbstractSyncAsyncTest() = default;
    void SetUp(){
        qosSettings = MessagingQos(456000);
        endPointAddress = std::make_shared<const joynr::system::RoutingTypes::ChannelAddress>("endPointUrl", "endPointAddress");
        proxyParticipantId = "participantId";
        providerParticipantId = "providerParticipantId";
        mockJoynrMessageSender = new MockJoynrMessageSender();
        // asyncGpsFixture must be created after derived objects have run Setup()
    }

    void TearDown(){
        delete mockJoynrMessageSender;
        delete asyncTestFixture;
    }



    // sets the expectations on the call expected on the MessageSender from the connector
    virtual testing::internal::TypedExpectation<void(
            const std::string&,
            const std::string&,
            const MessagingQos&,
            const Request&,
            std::shared_ptr<IReplyCaller>
    )>& setExpectationsForSendRequestCall(std::string methodName) = 0;

    // sets the exception which shall be returned by the ReplyCaller
    virtual testing::internal::TypedExpectation<void(
            const std::string&,
            const std::string&,
            const MessagingQos&,
            const Request&,
            std::shared_ptr<IReplyCaller>
    )>& setExpectedExceptionForSendRequestCall(const exceptions::JoynrException& error) {
        this->error.reset(error.clone());
        return EXPECT_CALL(
                *mockJoynrMessageSender,
                sendRequest(
                        _, // sender participant ID
                        Eq(providerParticipantId), // receiver participant ID
                        _, // messaging QoS
                        _, // request object to send
                        Pointee(_) // reply caller to notify when reply is received A<IReplyCaller>()
                        )
                ).Times(1).WillRepeatedly(Invoke(this, &AbstractSyncAsyncTest::returnError));
    }

    void returnError(const std::string& senderParticipantId,
            const std::string& receiverParticipantId,
            const MessagingQos& qos,
            const Request& request,
            std::shared_ptr<IReplyCaller> callback) {
        callback->returnError(error);
    }

    template <typename T>
    static void checkApplicationException(
            const exceptions::ApplicationException& expected,
            const exceptions::ApplicationException& actual, T expectedEnum) {
        EXPECT_EQ(expected, actual);
        EXPECT_EQ(expectedEnum, actual.getError<T>());
    }

    virtual tests::Itest* createFixture(bool cacheEnabled)=0;

    void testAsync_getAttributeNotCached() {
        asyncTestFixture = createFixture(false);

        MockCallbackWithJoynrException<joynr::types::Localisation::GpsLocation>* callback = new MockCallbackWithJoynrException<joynr::types::Localisation::GpsLocation>();

        setExpectationsForSendRequestCall("getLocation");
        asyncTestFixture->getLocationAsync(
                [callback] (const joynr::types::Localisation::GpsLocation& location) {
                    callback->onSuccess(location);
                });
    }

    void testSync_setAttributeNotCached() {
        tests::Itest* testFixture = createFixture(false);

        EXPECT_CALL(
                    *mockJoynrMessageSender,
                    sendRequest(
                        _, //Eq(proxyParticipantId), // sender participant ID
                        Eq(providerParticipantId), // receiver participant ID
                        _, // messaging QoS
                        AllOf(
                            Property(&Request::getMethodName, Eq("setLocation")),
                            Property(&Request::getParamDatatypes, (Property(&std::vector<std::string>::size, Eq(1))))
                        ), // request object to send
                        Property(&std::shared_ptr<IReplyCaller>::get,NotNull()) // reply caller to notify when reply is received
                    )
        ).WillOnce(Invoke(&callBackActions, &CallBackActions::executeCallBackVoidResult));

        testFixture->setLocation(expectedGpsLocation);
        delete testFixture;
    }


    void testSync_getAttributeNotCached() {
        tests::Itest* testFixture = createFixture(false);
        setExpectationsForSendRequestCall("getLocation")
                .WillOnce(Invoke(&callBackActions, &CallBackActions::executeCallBackGpsLocationResult));

        types::Localisation::GpsLocation gpsLocation;
        try {
            testFixture->getLocation(gpsLocation);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "getLocation was not successful";
        }
        EXPECT_EQ(expectedGpsLocation, gpsLocation);
        delete testFixture;
    }

    void testAsync_getAttributeCached() {
        asyncTestFixture = createFixture(true);

        MockCallbackWithJoynrException<joynr::types::Localisation::GpsLocation>* callback = new MockCallbackWithJoynrException<joynr::types::Localisation::GpsLocation>();

        setExpectationsForSendRequestCall("getLocation").Times(0);
        ON_CALL(mockClientCache, lookUp(_)).WillByDefault(Return(expectedGpsLocation));

        asyncTestFixture->getLocationAsync(
                [callback] (const types::Localisation::GpsLocation& location) {
                    callback->onSuccess(location);
                });
    }

    void testSync_getAttributeCached() {
        tests::Itest* testFixture = createFixture(true);

        setExpectationsForSendRequestCall("getLocation").Times(0);

        ON_CALL(mockClientCache, lookUp(_)).WillByDefault(Return(expectedGpsLocation));

        types::Localisation::GpsLocation gpsLocation;
        try {
            testFixture->getLocation(gpsLocation);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "getLocation was not successful";
        }
        EXPECT_EQ(expectedGpsLocation, gpsLocation);
        delete testFixture;
    }

    void testAsync_getterCallReturnsProviderRuntimeException() {
        asyncTestFixture = createFixture(false);

        MockCallback<std::int32_t>* callback = new MockCallback<std::int32_t>();

        exceptions::ProviderRuntimeException expected("getterCallReturnsProviderRuntimeExceptionAsyncError");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess(_)).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        asyncTestFixture->getAttributeWithProviderRuntimeExceptionAsync(
                [callback] (const std::int32_t& value) {
                    callback->onSuccess(value);
                }, [callback, expected] (const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_getterCallReturnsProviderRuntimeException() {
        tests::Itest* testFixture = createFixture(false);

        exceptions::ProviderRuntimeException expected("getterCallReturnsProviderRuntimeExceptionError");
        setExpectedExceptionForSendRequestCall(expected);

        std::int32_t result;
        try {
            testFixture->getAttributeWithProviderRuntimeException(result);
            ADD_FAILURE()<< "getterCallReturnsProviderRuntimeException was not successful (expected ProviderRuntimeException)";
        } catch (const exceptions::ProviderRuntimeException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "getterCallReturnsProviderRuntimeException was not successful (unexpected exception)";
        }

        delete testFixture;
    }

    void testAsync_getterCallReturnsMethodInvocationException() {
        asyncTestFixture = createFixture(false);

        MockCallback<std::int32_t>* callback = new MockCallback<std::int32_t>();

        exceptions::MethodInvocationException expected("getterCallReturnsMethodInvocationExceptionAsyncError");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess(_)).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        asyncTestFixture->getAttributeWithProviderRuntimeExceptionAsync(
                [callback] (const std::int32_t& value) {
                    callback->onSuccess(value);
                }, [callback, expected] (const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_getterCallReturnsMethodInvocationException() {
        tests::Itest* testFixture = createFixture(false);

        exceptions::MethodInvocationException expected("getterCallReturnsMethodInvocationExceptionError");
        setExpectedExceptionForSendRequestCall(expected);

        std::int32_t result;
        try {
            testFixture->getAttributeWithProviderRuntimeException(result);
            ADD_FAILURE()<< "getterCallReturnsMethodInvocationException was not successful (expected MethodInvocationException)";
        } catch (const exceptions::MethodInvocationException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "getterCallReturnsMethodInvocationException was not successful (unexpected exception)";
        }

        delete testFixture;
    }

    void testAsync_setterCallReturnsProviderRuntimeException() {
        asyncTestFixture = createFixture(false);

        MockCallback<void>* callback = new MockCallback<void>();

        exceptions::ProviderRuntimeException expected("setterCallReturnsProviderRuntimeExceptionAsyncError");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        std::int32_t value = 0;
        asyncTestFixture->setAttributeWithProviderRuntimeExceptionAsync(
                value,
                [callback] () {
                    callback->onSuccess();
                }, [callback, expected] (const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_setterCallReturnsProviderRuntimeException() {
        tests::Itest* testFixture = createFixture(false);

        exceptions::ProviderRuntimeException expected("setterCallReturnsProviderRuntimeExceptionError");
        setExpectedExceptionForSendRequestCall(expected);

        std::int32_t value;
        try {
            testFixture->setAttributeWithProviderRuntimeException(value);
            ADD_FAILURE()<< "setterCallReturnsProviderRuntimeException was not successful (expected ProviderRuntimeException)";
        } catch (const exceptions::ProviderRuntimeException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "setterCallReturnsProviderRuntimeException was not successful (unexpected exception)";
        }

        delete testFixture;
    }

    void testAsync_setterCallReturnsMethodInvocationException() {
        asyncTestFixture = createFixture(false);

        MockCallback<void>* callback = new MockCallback<void>();

        exceptions::MethodInvocationException expected ("setterCallReturnsMethodInvocationExceptionAsyncError");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        std::int32_t value = 0;
        asyncTestFixture->setAttributeWithProviderRuntimeExceptionAsync(
                value,
                [callback] () {
                    callback->onSuccess();
                }, [callback, expected] (const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;    }

    void testSync_setterCallReturnsMethodInvocationException() {
        tests::Itest* testFixture = createFixture(false);

        exceptions::MethodInvocationException expected("setterCallReturnsMethodInvocationExceptionError");
        setExpectedExceptionForSendRequestCall(expected);

        std::int32_t value;
        try {
            testFixture->setAttributeWithProviderRuntimeException(value);
            ADD_FAILURE()<< "setterCallReturnsMethodInvocationException was not successful (expected MethodInvocationException)";
        } catch (const exceptions::MethodInvocationException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "setterCallReturnsMethodInvocationException was not successful (unexpected exception)";
        }

        delete testFixture;
    }

    void testAsync_methodCallReturnsProviderRuntimeException() {
        asyncTestFixture = createFixture(false);

        MockCallback<void>* callback = new MockCallback<void>();

        exceptions::ProviderRuntimeException expected ("testAsync_methodCallReturnsProviderRuntimeException-ERROR");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        asyncTestFixture->methodWithProviderRuntimeExceptionAsync(
                [callback] () {
                    callback->onSuccess();
                }, [callback, expected] (const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_methodCallReturnsProviderRuntimeException() {
        tests::Itest* testFixture = createFixture(false);

        exceptions::ProviderRuntimeException expected("testSync_methodCallReturnsProviderRuntimeException-ERROR");
        setExpectedExceptionForSendRequestCall(expected);

        try {
            testFixture->methodWithProviderRuntimeException();
            ADD_FAILURE()<< "testSync_methodCallReturnsProviderRuntimeException was not successful (expected ProviderRuntimeException)";
        } catch (const exceptions::ProviderRuntimeException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "testSync_methodCallReturnsProviderRuntimeException was not successful (unexpected exception)";
        }
        delete testFixture;
    }

    void testAsync_methodCallReturnsMethodInvocationException() {
        asyncTestFixture = createFixture(false);

        MockCallback<void>* callback = new MockCallback<void>();

        exceptions::MethodInvocationException expected("testAsync_methodCallReturnsMethodInvocationException-ERROR");
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onError(_)).Times(1);

        asyncTestFixture->methodWithProviderRuntimeExceptionAsync(
                [callback] () {
                    callback->onSuccess();
                }, [callback, expected] (const exceptions::JoynrRuntimeException& error) {
                    EXPECT_EQ(expected.getTypeName(), error.getTypeName());
                    EXPECT_EQ(expected.getMessage(), error.getMessage());
                    callback->onError(error);
                });

        delete callback;
    }

    void testSync_methodCallReturnsMethodInvocationException() {
        tests::Itest* testFixture = createFixture(false);

        exceptions::MethodInvocationException expected("testSync_methodCallReturnsMethodInvocationException-ERROR");
        setExpectedExceptionForSendRequestCall(expected);

        try {
            testFixture->methodWithProviderRuntimeException();
            ADD_FAILURE()<< "testSync_methodCallReturnsMethodInvocationException was not successful (expected MethodInvocationException)";
        } catch (const exceptions::MethodInvocationException& e) {
            EXPECT_EQ(expected.getTypeName(), e.getTypeName());
            EXPECT_EQ(expected.getMessage(), e.getMessage());
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "testSync_methodCallReturnsMethodInvocationException was not successful (unexpected exception)";
        }

        delete testFixture;
    }

    void testAsync_methodCallReturnsErrorEnum() {
        asyncTestFixture = createFixture(false);

        using tests::testTypes::ErrorEnumBase;
        auto callback = std::make_shared<MockCallbackWithApplicationError<void, ErrorEnumBase::Enum>>();

        ErrorEnumBase::Enum expectedErrorEnum = ErrorEnumBase::BASE_ERROR_TYPECOLLECTION;
        std::string literal = ErrorEnumBase::getLiteral(expectedErrorEnum);
        std::string typeName = ErrorEnumBase::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected (typeName + "::" + literal,
                                                   std::make_shared<ErrorEnumBase>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onRuntimeError(_)).Times(0);
        EXPECT_CALL(*callback, onApplicationError(_)).Times(1);

        asyncTestFixture->methodWithErrorEnumAsync(
                [callback] () {
                    callback->onSuccess();
                },
                [callback, expectedErrorEnum] (const ErrorEnumBase::Enum& errorEnum) {
                    EXPECT_EQ(expectedErrorEnum, errorEnum);
                    callback->onApplicationError(errorEnum);
                });

    }

    void testSync_methodCallReturnsErrorEnum() {
        tests::Itest* testFixture = createFixture(false);

        using tests::testTypes::ErrorEnumBase;

        ErrorEnumBase::Enum expectedErrorEnum = ErrorEnumBase::BASE_ERROR_TYPECOLLECTION;
        std::string literal = ErrorEnumBase::getLiteral(expectedErrorEnum);
        std::string typeName = ErrorEnumBase::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(typeName + "::" + literal,
                                                  std::make_shared<ErrorEnumBase>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        try {
            testFixture->methodWithErrorEnum();
            ADD_FAILURE()<< "testSync_methodCallReturnsErrorEnum was not successful (expected MethodInvocationException)";
        } catch (const exceptions::ApplicationException& e) {
            checkApplicationException(expected, e, expectedErrorEnum);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "testSync_methodCallReturnsErrorEnum was not successful (unexpected exception)";
        }

        delete testFixture;
    }

    void testAsync_methodCallReturnsExtendedErrorEnum() {
        asyncTestFixture = createFixture(false);
        using tests::test::MethodWithErrorEnumExtendedErrorEnum;

        auto callback = std::make_shared<MockCallbackWithApplicationError<void, MethodWithErrorEnumExtendedErrorEnum::Enum>>();

        MethodWithErrorEnumExtendedErrorEnum::Enum expectedErrorEnum = MethodWithErrorEnumExtendedErrorEnum::IMPLICIT_ERROR_TYPECOLLECTION;
        std::string literal = MethodWithErrorEnumExtendedErrorEnum::getLiteral(expectedErrorEnum);
        std::string typeName = MethodWithErrorEnumExtendedErrorEnum::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(typeName + "::" + literal,
                                                  std::make_shared<MethodWithErrorEnumExtendedErrorEnum>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onRuntimeError(_)).Times(0);
        EXPECT_CALL(*callback, onApplicationError(_)).Times(1);

        asyncTestFixture->methodWithErrorEnumExtendedAsync(
                [callback] () {
                    callback->onSuccess();
                },
                [callback, expectedErrorEnum] (const MethodWithErrorEnumExtendedErrorEnum::Enum& errorEnum) {
                    EXPECT_EQ(expectedErrorEnum, errorEnum);
                    callback->onApplicationError(errorEnum);
                });

    }

    void testSync_methodCallReturnsExtendedErrorEnum() {
        tests::Itest* testFixture = createFixture(false);

        using tests::test::MethodWithErrorEnumExtendedErrorEnum;
        MethodWithErrorEnumExtendedErrorEnum::Enum error = MethodWithErrorEnumExtendedErrorEnum::IMPLICIT_ERROR_TYPECOLLECTION;
        std::string literal = MethodWithErrorEnumExtendedErrorEnum::getLiteral(error);
        std::string typeName = MethodWithErrorEnumExtendedErrorEnum::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(typeName + "::" + literal,
                                                  std::make_shared<MethodWithErrorEnumExtendedErrorEnum>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        try {
            testFixture->methodWithErrorEnumExtended();
            ADD_FAILURE()<< "testSync_methodCallReturnsExtendedErrorEnum was not successful (expected MethodInvocationException)";
        } catch (const exceptions::ApplicationException& e) {
            checkApplicationException(expected, e, error);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "testSync_methodCallReturnsExtendedErrorEnum was not successful (unexpected exception)";
        }

        delete testFixture;
    }

    void testAsync_methodCallReturnsInlineErrorEnum() {
        asyncTestFixture = createFixture(false);

        using tests::test::MethodWithImplicitErrorEnumErrorEnum;

        auto callback = std::make_shared<MockCallbackWithApplicationError<void, MethodWithImplicitErrorEnumErrorEnum::Enum>>();

        MethodWithImplicitErrorEnumErrorEnum::Enum expectedErrorEnum = MethodWithImplicitErrorEnumErrorEnum::IMPLICIT_ERROR;
        std::string literal = MethodWithImplicitErrorEnumErrorEnum::getLiteral(expectedErrorEnum);
        std::string typeName = MethodWithImplicitErrorEnumErrorEnum::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(typeName + "::" + literal,
                                                  std::make_shared<MethodWithImplicitErrorEnumErrorEnum>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        EXPECT_CALL(*callback, onSuccess()).Times(0);
        EXPECT_CALL(*callback, onRuntimeError(_)).Times(0);
        EXPECT_CALL(*callback, onApplicationError(_)).Times(1);

        asyncTestFixture->methodWithImplicitErrorEnumAsync(
                [callback] () {
                    callback->onSuccess();
                },
                [callback, expectedErrorEnum] (const MethodWithImplicitErrorEnumErrorEnum::Enum& errorEnum) {
                    EXPECT_EQ(expectedErrorEnum, errorEnum);
                    callback->onApplicationError(errorEnum);
                });

    }

    void testSync_methodCallReturnsInlineErrorEnum() {
        tests::Itest* testFixture = createFixture(false);

        using tests::test::MethodWithImplicitErrorEnumErrorEnum;

        MethodWithImplicitErrorEnumErrorEnum::Enum error = MethodWithImplicitErrorEnumErrorEnum::IMPLICIT_ERROR;
        std::string literal = MethodWithImplicitErrorEnumErrorEnum::getLiteral(error);
        std::string typeName = MethodWithImplicitErrorEnumErrorEnum::getTypeName();
        // TODO remove workaround after the new serializer has been introduced: until then, the correct error enumeration has to be reconstructed by the connector
        exceptions::ApplicationException expected(typeName + "::" + literal,
                                                  std::make_shared<MethodWithImplicitErrorEnumErrorEnum>(literal));
        setExpectedExceptionForSendRequestCall(expected);

        try {
            testFixture->methodWithImplicitErrorEnum();
            ADD_FAILURE()<< "testSync_methodCallReturnsInlineErrorEnum was not successful (expected MethodInvocationException)";
        } catch (const exceptions::ApplicationException& e) {
            checkApplicationException(expected, e, error);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "testSync_methodCallReturnsInlineErrorEnum was not successful (unexpected exception)";
        }

        delete testFixture;
    }

    void testAsync_OperationWithNoArguments() {
        asyncTestFixture = createFixture(false);

        MockCallbackWithJoynrException<int>* callback = new MockCallbackWithJoynrException<int>();

        setExpectationsForSendRequestCall("methodWithNoInputParameters");

        asyncTestFixture->methodWithNoInputParametersAsync(
                [callback] (const int& value) {
                    callback->onSuccess(value);
                });
    }

    void testSync_OperationWithNoArguments() {
        tests::Itest* testFixture = createFixture(false);
        setExpectationsForSendRequestCall("methodWithNoInputParameters")
                .WillOnce(Invoke(&callBackActions, &CallBackActions::executeCallBackIntResult));

        int result;
        try {
            testFixture->methodWithNoInputParameters(result);
        } catch (const exceptions::JoynrException& e) {
            ADD_FAILURE()<< "methodWithNoInputParameters was not successful";
        }
        EXPECT_EQ(expectedInt, result);
        delete testFixture;
    }

    void testSubscribeToAttribute() {
        //EXPECT_CALL(*mockJoynrMessageSender,
        //            sendSubscriptionRequest(_,_,_,_)).Times(1);

        std::shared_ptr<ISubscriptionListener<types::Localisation::GpsLocation> > subscriptionListener(
                    new MockGpsSubscriptionListener());
        //TODO uncomment once the connector has the correct signature!
        //vehicle::IGps* gpsFixture = createFixture(false);
        //QtSubscriptionQos  subscriptionQos(100, 200, true, 80, 80);
        //gpsFixture->subscribeToLocation(subscriptionListener, subscriptionQos);
        //delete gpsFixture;
    }

protected:
    joynr::types::Localisation::GpsLocation expectedGpsLocation;
    int expectedInt;
    CallBackActions callBackActions;
    MessagingQos qosSettings;
    MockDispatcher mockDispatcher;
    MockMessaging mockMessagingStub;
    std::shared_ptr<IReplyCaller> callBack;
    MockJoynrMessageSender* mockJoynrMessageSender;
    std::string proxyParticipantId;
    std::string providerParticipantId;
    MockClientCache mockClientCache;
    std::shared_ptr<const joynr::system::RoutingTypes::Address> endPointAddress;
    tests::Itest* asyncTestFixture;
    std::shared_ptr<exceptions::JoynrException> error;
private:
    DISALLOW_COPY_AND_ASSIGN(AbstractSyncAsyncTest);
};
