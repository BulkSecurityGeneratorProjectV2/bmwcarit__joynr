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

#include <string>
#include <tuple>
#include <vector>

#include "tests/utils/Gtest.h"

#include "joynr/CapabilitiesStorage.h"
#include "joynr/ClusterControllerSettings.h"
#include "joynr/ImmutableMessage.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/MutableMessage.h"
#include "joynr/MutableMessageFactory.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Request.h"
#include "joynr/Settings.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/DiscoveryError.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/Version.h"
#include "libjoynrclustercontroller/access-control/AccessController.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

#include "tests/mock/MockLocalCapabilitiesDirectory.h"
#include "tests/mock/MockLocalDomainAccessController.h"
#include "tests/mock/MockMessageRouter.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::types;
using namespace joynr::infrastructure;
using namespace joynr::infrastructure::DacTypes;

class MockConsumerPermissionCallback
        : public joynr::IAccessController::IHasConsumerPermissionCallback
{
public:
    MOCK_METHOD1(hasConsumerPermission, void(IAccessController::Enum hasPermission));
};

template <typename... Ts>
joynr::Request initOutgoingRequest(std::string methodName,
                                   std::vector<std::string> paramDataTypes,
                                   Ts... paramValues)
{
    Request outgoingRequest;
    outgoingRequest.setMethodName(methodName);
    outgoingRequest.setParamDatatypes(std::move(paramDataTypes));
    outgoingRequest.setParams(std::move(paramValues)...);
    return outgoingRequest;
}

// Mock objects cannot make callbacks themselves but can make calls to methods
// with the same arguments as the mocked method call.
class ConsumerPermissionCallbackMaker
{
public:
    explicit ConsumerPermissionCallbackMaker(Permission::Enum permission) : _permission(permission)
    {
    }

    void consumerPermission(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName,
            TrustLevel::Enum trustLevel,
            std::shared_ptr<LocalDomainAccessController::IGetPermissionCallback> callback)
    {
        std::ignore = userId;
        std::ignore = domain;
        std::ignore = interfaceName;
        std::ignore = trustLevel;
        callback->permission(_permission);
    }

    void operationNeeded(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName,
            TrustLevel::Enum trustLevel,
            std::shared_ptr<LocalDomainAccessController::IGetPermissionCallback> callback)
    {
        std::ignore = userId;
        std::ignore = domain;
        std::ignore = interfaceName;
        std::ignore = trustLevel;
        callback->operationNeeded();
    }

private:
    Permission::Enum _permission;
};

class AccessControllerTest : public ::testing::Test
{
public:
    AccessControllerTest()
            : _emptySettings(),
              _clusterControllerSettings(_emptySettings),
              _singleThreadedIOService(std::make_shared<SingleThreadedIOService>()),
              _accessControllerCallback(std::make_shared<MockConsumerPermissionCallback>()),
              _messageRouter(std::make_shared<MockMessageRouter>(
                      _singleThreadedIOService->getIOService())),
              _localCapabilitiesDirectoryStore(std::make_shared<LocalCapabilitiesDirectoryStore>()),
              _localCapabilitiesDirectoryMock(std::make_shared<MockLocalCapabilitiesDirectory>(
                      _clusterControllerSettings,
                      _messageRouter,
                      _localCapabilitiesDirectoryStore,
                      _singleThreadedIOService->getIOService(),
                      _defaultExpiryDateMs)),
              _accessController(),
              _messagingQos(MessagingQos(5000))
    {
        _singleThreadedIOService->start();
    }

    ~AccessControllerTest()
    {
        _localCapabilitiesDirectoryMock->shutdown();
        _singleThreadedIOService->stop();
    }

    void invokeOnSuccessCallbackFct(
            std::string participantId,
            const types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
    {
        std::ignore = participantId;
        std::ignore = gbids;
        std::ignore = onError;
        ASSERT_EQ(types::DiscoveryScope::LOCAL_THEN_GLOBAL, discoveryQos.getDiscoveryScope());
        onSuccess(_discoveryEntry);
    }

    void invokeOnErrorCallbackFct(
            std::string participantId,
            const types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
    {
        std::ignore = participantId;
        std::ignore = gbids;
        std::ignore = onSuccess;
        ASSERT_EQ(types::DiscoveryScope::LOCAL_THEN_GLOBAL, discoveryQos.getDiscoveryScope());
        onError(types::DiscoveryError::NO_ENTRY_FOR_PARTICIPANT);
    }

    void invokeOnSuccessCallbackFctLocalRecipient(
            std::string participantId,
            const types::DiscoveryQos& discoveryQos,
            const std::vector<std::string>& gbids,
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)> onSuccess,
            std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError)
    {
        std::ignore = participantId;
        std::ignore = gbids;
        std::ignore = onError;
        ASSERT_EQ(types::DiscoveryScope::LOCAL_ONLY, discoveryQos.getDiscoveryScope());
        onSuccess(_discoveryEntry);
    }

    std::shared_ptr<ImmutableMessage> getImmutableMessage()
    {
        std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
        immutableMessage->setCreator(_DUMMY_USERID);
        return immutableMessage;
    }

    void SetUp()
    {
        _mutableMessage = _messageFactory.createRequest(_fromParticipantId,
                                                        _toParticipantId,
                                                        _messagingQos,
                                                        initOutgoingRequest(_TEST_OPERATION, {}),
                                                        _isLocalMessage);

        std::int64_t lastSeenDateMs = 0;
        std::int64_t expiryDateMs = 0;
        joynr::types::Version providerVersion(47, 11);
        _discoveryEntry = DiscoveryEntryWithMetaInfo(providerVersion,
                                                     _TEST_DOMAIN,
                                                     _TEST_INTERFACE,
                                                     _toParticipantId,
                                                     types::ProviderQos(),
                                                     lastSeenDateMs,
                                                     expiryDateMs,
                                                     _TEST_PUBLICKEYID,
                                                     false);
    }

    void prepareConsumerTest()
    {
        types::DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        discoveryQos.setDiscoveryTimeout(60000);
        EXPECT_CALL(
                *_localCapabilitiesDirectoryMock,
                lookup(_toParticipantId,
                       discoveryQos,
                       std::vector<std::string>{},
                       A<std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)>>(),
                       A<std::function<void(
                               const joynr::types::DiscoveryError::Enum& errorEnum)>>()))
                .Times(1)
                .WillOnce(Invoke(this, &AccessControllerTest::invokeOnSuccessCallbackFct));
    }

    void prepareConsumerTestLocalRecipient()
    {
        types::DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
        discoveryQos.setDiscoveryTimeout(60000);
        EXPECT_CALL(
                *_localCapabilitiesDirectoryMock,
                lookup(_toParticipantId,
                       discoveryQos,
                       std::vector<std::string>{},
                       A<std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)>>(),
                       A<std::function<void(
                               const joynr::types::DiscoveryError::Enum& errorEnum)>>()))
                .Times(1)
                .WillOnce(Invoke(
                        this, &AccessControllerTest::invokeOnSuccessCallbackFctLocalRecipient));
    }

    void prepareConsumerTestInRetryErrorCase()
    {
        types::DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_THEN_GLOBAL);
        discoveryQos.setDiscoveryTimeout(60000);
        EXPECT_CALL(
                *_localCapabilitiesDirectoryMock,
                lookup(_toParticipantId,
                       discoveryQos,
                       std::vector<std::string>{},
                       A<std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo&)>>(),
                       A<std::function<void(
                               const joynr::types::DiscoveryError::Enum& errorEnum)>>()))
                .Times(1)
                .WillOnce(Invoke(this, &AccessControllerTest::invokeOnErrorCallbackFct));
    }

    void testPermission(Permission::Enum testPermission, IAccessController::Enum expectedPermission)
    {
        prepareConsumerTest();
        _localCapabilitiesDirectoryMock->init();
        ConsumerPermissionCallbackMaker makeCallback(testPermission);

        _localDomainAccessControllerMock = std::make_shared<MockLocalDomainAccessController>(
                std::make_unique<LocalDomainAccessStore>());
        EXPECT_CALL(*_localDomainAccessControllerMock,
                    getConsumerPermission(
                            _DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, TrustLevel::HIGH, _))
                .WillOnce(Invoke(
                        &makeCallback, &ConsumerPermissionCallbackMaker::consumerPermission));

        _accessController = std::make_shared<AccessController>(
                _localCapabilitiesDirectoryMock, _localDomainAccessControllerMock);
        EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(expectedPermission)).Times(1);

        std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
        immutableMessage->setCreator(_DUMMY_USERID);
        // pass the immutable message to hasConsumerPermission
        _accessController->hasConsumerPermission(
                immutableMessage,
                std::static_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                        _accessControllerCallback),
                false);
    }

    void testPermissionLocalRecipient(Permission::Enum testPermission,
                                      IAccessController::Enum expectedPermission)
    {
        prepareConsumerTestLocalRecipient();
        _localCapabilitiesDirectoryMock->init();
        ConsumerPermissionCallbackMaker makeCallback(testPermission);

        _localDomainAccessControllerMock = std::make_shared<MockLocalDomainAccessController>(
                std::make_unique<LocalDomainAccessStore>());
        EXPECT_CALL(*_localDomainAccessControllerMock,
                    getConsumerPermission(
                            _DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, TrustLevel::HIGH, _))
                .WillOnce(Invoke(
                        &makeCallback, &ConsumerPermissionCallbackMaker::consumerPermission));

        _accessController = std::make_shared<AccessController>(
                _localCapabilitiesDirectoryMock, _localDomainAccessControllerMock);
        EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(expectedPermission)).Times(1);

        std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
        immutableMessage->setCreator(_DUMMY_USERID);
        // pass the immutable message to hasConsumerPermission
        _accessController->hasConsumerPermission(
                immutableMessage,
                std::static_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                        _accessControllerCallback),
                true);
    }

protected:
    Settings _emptySettings;
    ClusterControllerSettings _clusterControllerSettings;
    std::shared_ptr<SingleThreadedIOService> _singleThreadedIOService;
    std::shared_ptr<MockLocalDomainAccessController> _localDomainAccessControllerMock;
    std::shared_ptr<MockConsumerPermissionCallback> _accessControllerCallback;
    std::shared_ptr<MockMessageRouter> _messageRouter;
    std::shared_ptr<LocalCapabilitiesDirectoryStore> _localCapabilitiesDirectoryStore;
    std::shared_ptr<MockLocalCapabilitiesDirectory> _localCapabilitiesDirectoryMock;
    std::shared_ptr<AccessController> _accessController;
    MutableMessageFactory _messageFactory;
    MutableMessage _mutableMessage;
    MessagingQos _messagingQos;
    const bool _isLocalMessage = true;
    const std::int64_t _defaultExpiryDateMs = 60 * 60 * 1000;
    DiscoveryEntryWithMetaInfo _discoveryEntry;
    static const std::string _fromParticipantId;
    static const std::string _toParticipantId;
    static const std::string _subscriptionId;
    static const std::string _replyToChannelId;
    static const std::string _DUMMY_USERID;
    static const std::string _TEST_DOMAIN;
    static const std::string _TEST_INTERFACE;
    static const std::string _TEST_OPERATION;
    static const std::string _TEST_PUBLICKEYID;

private:
    DISALLOW_COPY_AND_ASSIGN(AccessControllerTest);
};

//----- Constants --------------------------------------------------------------
const std::string AccessControllerTest::_fromParticipantId("sender");
const std::string AccessControllerTest::_toParticipantId("receiver");
const std::string AccessControllerTest::_subscriptionId("testSubscriptionId");
const std::string AccessControllerTest::_replyToChannelId("replyToId");
const std::string AccessControllerTest::_DUMMY_USERID("testUserId");
const std::string AccessControllerTest::_TEST_DOMAIN("testDomain");
const std::string AccessControllerTest::_TEST_INTERFACE("testInterface");
const std::string AccessControllerTest::_TEST_OPERATION("testOperation");
const std::string AccessControllerTest::_TEST_PUBLICKEYID("publicKeyId");

//----- Tests ------------------------------------------------------------------

TEST_F(AccessControllerTest, accessWithInterfaceLevelAccessControl)
{
    prepareConsumerTest();
    _localCapabilitiesDirectoryMock->init();
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);

    _localDomainAccessControllerMock = std::make_shared<MockLocalDomainAccessController>(
            std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*_localDomainAccessControllerMock,
                getConsumerPermission(
                        _DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, TrustLevel::HIGH, _))
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::consumerPermission));

    _accessController = std::make_shared<AccessController>(
            _localCapabilitiesDirectoryMock, _localDomainAccessControllerMock);
    EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(IAccessController::Enum::YES))
            .Times(1);

    _accessController->hasConsumerPermission(
            getImmutableMessage(),
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                    _accessControllerCallback),
            false);
}

TEST_F(AccessControllerTest, accessWithOperationLevelAccessControl)
{
    prepareConsumerTest();
    _localCapabilitiesDirectoryMock->init();
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);

    _localDomainAccessControllerMock = std::make_shared<MockLocalDomainAccessController>(
            std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*_localDomainAccessControllerMock,
                getConsumerPermission(
                        _DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, TrustLevel::HIGH, _))
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::operationNeeded));

    Permission::Enum permissionYes = Permission::YES;
    DefaultValue<Permission::Enum>::Set(permissionYes);
    EXPECT_CALL(*_localDomainAccessControllerMock,
                getConsumerPermission(_DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, _TEST_OPERATION,
                                      TrustLevel::HIGH))
            .WillOnce(Return(permissionYes));

    _accessController = std::make_shared<AccessController>(
            _localCapabilitiesDirectoryMock, _localDomainAccessControllerMock);
    EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(IAccessController::Enum::YES))
            .Times(1);

    _accessController->hasConsumerPermission(
            getImmutableMessage(),
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                    _accessControllerCallback),
            false);
}

TEST_F(AccessControllerTest, accessWithOperationLevelAccessControlAndFaultyMessage)
{
    prepareConsumerTest();
    _localCapabilitiesDirectoryMock->init();
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);

    _localDomainAccessControllerMock = std::make_shared<MockLocalDomainAccessController>(
            std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*_localDomainAccessControllerMock,
                getConsumerPermission(
                        _DUMMY_USERID, _TEST_DOMAIN, _TEST_INTERFACE, TrustLevel::HIGH, _))
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::operationNeeded));

    _accessController = std::make_shared<AccessController>(
            _localCapabilitiesDirectoryMock, _localDomainAccessControllerMock);
    EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(IAccessController::Enum::NO))
            .Times(1);

    std::string payload("invalid serialization of Request object");
    _mutableMessage.setPayload(payload);

    _accessController->hasConsumerPermission(
            getImmutableMessage(),
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                    _accessControllerCallback),
            false);
}

TEST_F(AccessControllerTest, retryAccessControlCheckIfNoDiscoveryEntry)
{
    prepareConsumerTestInRetryErrorCase();
    _localCapabilitiesDirectoryMock->init();

    _localDomainAccessControllerMock = std::make_shared<MockLocalDomainAccessController>(
            std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*_accessControllerCallback, hasConsumerPermission(IAccessController::Enum::RETRY))
            .Times(1);

    _accessController = std::make_shared<AccessController>(
            _localCapabilitiesDirectoryMock, _localDomainAccessControllerMock);
    std::shared_ptr<ImmutableMessage> immutableMessage = _mutableMessage.getImmutableMessage();
    immutableMessage->setCreator(_DUMMY_USERID);
    // pass the immutable message to hasConsumerPermission
    _accessController->hasConsumerPermission(
            immutableMessage,
            std::static_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(
                    _accessControllerCallback),
            false);
}

TEST_F(AccessControllerTest, hasProviderPermission)
{
    _localCapabilitiesDirectoryMock->init();
    Permission::Enum permissionYes = Permission::YES;
    DefaultValue<Permission::Enum>::Set(permissionYes);

    _localDomainAccessControllerMock = std::make_shared<MockLocalDomainAccessController>(
            std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*_localDomainAccessControllerMock, getProviderPermission(_, _, _, _))
            .Times(1)
            .WillOnce(Return(permissionYes));

    _accessController = std::make_shared<AccessController>(
            _localCapabilitiesDirectoryMock, _localDomainAccessControllerMock);
    bool retval = _accessController->hasProviderPermission(
            _DUMMY_USERID, TrustLevel::HIGH, _TEST_DOMAIN, _TEST_INTERFACE);
    EXPECT_TRUE(retval);
}

TEST_F(AccessControllerTest, hasNoProviderPermission)
{
    _localCapabilitiesDirectoryMock->init();
    Permission::Enum permissionNo = Permission::NO;
    DefaultValue<Permission::Enum>::Set(permissionNo);

    _localDomainAccessControllerMock = std::make_shared<MockLocalDomainAccessController>(
            std::make_unique<LocalDomainAccessStore>());
    EXPECT_CALL(*_localDomainAccessControllerMock, getProviderPermission(_, _, _, _))
            .Times(1)
            .WillOnce(Return(permissionNo));

    _accessController = std::make_shared<AccessController>(
            _localCapabilitiesDirectoryMock, _localDomainAccessControllerMock);
    bool retval = _accessController->hasProviderPermission(
            _DUMMY_USERID, TrustLevel::HIGH, _TEST_DOMAIN, _TEST_INTERFACE);
    EXPECT_FALSE(retval);
}

//----- Test Types --------------------------------------------------------------
typedef ::testing::
        Types<SubscriptionRequest, MulticastSubscriptionRequest, BroadcastSubscriptionRequest>
                SubscriptionTypes;

template <typename T>
class AccessControllerSubscriptionTest : public AccessControllerTest
{
public:
    template <typename U = T>
    typename std::enable_if_t<std::is_same<U, SubscriptionRequest>::value> createMutableMessage()
    {
        SubscriptionRequest subscriptionRequest;
        subscriptionRequest.setSubscriptionId(_subscriptionId);
        _mutableMessage = _messageFactory.createSubscriptionRequest(_fromParticipantId,
                                                                    _toParticipantId,
                                                                    _messagingQos,
                                                                    subscriptionRequest,
                                                                    _isLocalMessage);
    }

    template <typename U = T>
    typename std::enable_if_t<std::is_same<U, MulticastSubscriptionRequest>::value>
    createMutableMessage()
    {
        MulticastSubscriptionRequest subscriptionRequest;
        _mutableMessage = _messageFactory.createMulticastSubscriptionRequest(_fromParticipantId,
                                                                             _toParticipantId,
                                                                             _messagingQos,
                                                                             subscriptionRequest,
                                                                             _isLocalMessage);
    }

    template <typename U = T>
    typename std::enable_if_t<std::is_same<U, BroadcastSubscriptionRequest>::value>
    createMutableMessage()
    {
        BroadcastSubscriptionRequest subscriptionRequest;
        _mutableMessage = _messageFactory.createBroadcastSubscriptionRequest(_fromParticipantId,
                                                                             _toParticipantId,
                                                                             _messagingQos,
                                                                             subscriptionRequest,
                                                                             _isLocalMessage);
    }
};

TYPED_TEST_SUITE(AccessControllerSubscriptionTest, SubscriptionTypes, );

TYPED_TEST(AccessControllerSubscriptionTest, hasNoConsumerPermission)
{
    const Permission::Enum permissionNo = Permission::NO;
    const IAccessController::Enum expectedPermissionFalse = IAccessController::Enum::NO;

    this->createMutableMessage();

    this->testPermission(permissionNo, expectedPermissionFalse);
}

TYPED_TEST(AccessControllerSubscriptionTest, hasConsumerPermission)
{
    const Permission::Enum permissionNo = Permission::YES;
    const IAccessController::Enum expectedPermissionFalse = IAccessController::Enum::YES;

    this->createMutableMessage();

    this->testPermission(permissionNo, expectedPermissionFalse);
}

TYPED_TEST(AccessControllerSubscriptionTest, hasNoConsumerPermissionLocalRecipient)
{
    const Permission::Enum permissionNo = Permission::NO;
    const IAccessController::Enum expectedPermissionFalse = IAccessController::Enum::NO;

    this->createMutableMessage();

    this->testPermissionLocalRecipient(permissionNo, expectedPermissionFalse);
}

TYPED_TEST(AccessControllerSubscriptionTest, hasConsumerPermissionLocalRecipient)
{
    const Permission::Enum permissionNo = Permission::YES;
    const IAccessController::Enum expectedPermissionFalse = IAccessController::Enum::YES;

    this->createMutableMessage();

    this->testPermissionLocalRecipient(permissionNo, expectedPermissionFalse);
}
