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

#include <tuple>
#include <string>

#include <gtest/gtest.h>

#include "joynr/Request.h"
#include "joynr/PrivateCopyAssign.h"
#include "tests/utils/MockObjects.h"
#include "cluster-controller/access-control/AccessController.h"
#include "cluster-controller/access-control/LocalDomainAccessStore.h"
#include "joynr/types/DiscoveryEntry.h"
#include "joynr/types/Version.h"
#include "joynr/SingleThreadedIOService.h"
#include "joynr/serializer/Serializer.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::types;
using namespace joynr::infrastructure;
using namespace joynr::infrastructure::DacTypes;


template <typename... Ts>
joynr::Request initOutgoingRequest(std::string methodName, std::vector<std::string> paramDataTypes, Ts... paramValues)
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
    explicit ConsumerPermissionCallbackMaker(Permission::Enum permission) :
        permission(permission)
    {}

    void consumerPermission(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName,
            TrustLevel::Enum trustLevel,
            std::shared_ptr<LocalDomainAccessController::IGetConsumerPermissionCallback> callback
    ) {
        std::ignore = userId;
        std::ignore = domain;
        std::ignore = interfaceName;
        std::ignore = trustLevel;
        callback->consumerPermission(permission);
    }

    void operationNeeded(
            const std::string& userId,
            const std::string& domain,
            const std::string& interfaceName,
            TrustLevel::Enum trustLevel,
            std::shared_ptr<LocalDomainAccessController::IGetConsumerPermissionCallback> callback
    ) {
        std::ignore = userId;
        std::ignore = domain;
        std::ignore = interfaceName;
        std::ignore = trustLevel;
        callback->operationNeeded();
    }

private:
    Permission::Enum permission;
};


class AccessControllerTest : public ::testing::Test {
public:
    AccessControllerTest() :
        singleThreadedIOService(),
        localDomainAccessControllerMock(std::make_unique<LocalDomainAccessStore>(
                        true // start with clean database
        )),
        accessControllerCallback(std::make_shared<MockConsumerPermissionCallback>()),
        settings(),
        messagingSettingsMock(settings),
        localCapabilitiesDirectoryMock(messagingSettingsMock, settings, singleThreadedIOService.getIOService()),
        accessController(
                localCapabilitiesDirectoryMock,
                localDomainAccessControllerMock
        )
    {

    }

    ~AccessControllerTest() = default;

    void invokeOnSuccessCallbackFct (std::string participantId,
                            std::function<void(const joynr::types::DiscoveryEntry&)> onSuccess,
                            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError) {
        std::ignore = participantId;
        onSuccess(discoveryEntry);
    }

    void SetUp(){
        messagingQos = MessagingQos(5000);
        message = messageFactory.createRequest(fromParticipantId,
                                     toParticipantId,
                                     messagingQos,
                                     initOutgoingRequest(TEST_OPERATION, {}));
        message.setHeaderCreatorUserId(DUMMY_USERID);

        ON_CALL(
                messagingSettingsMock,
                getDiscoveryDirectoriesDomain()
        )
                .WillByDefault(Return("fooDomain"));
        ON_CALL(
                messagingSettingsMock,
                getCapabilitiesDirectoryParticipantId()
        )
                .WillByDefault(Return("fooParticipantId"));

        std::int64_t lastSeenDateMs = 0;
        std::int64_t expiryDateMs = 0;
        joynr::types::Version providerVersion(47, 11);
        discoveryEntry = DiscoveryEntry(
                providerVersion,
                TEST_DOMAIN,
                TEST_INTERFACE,
                toParticipantId,
                types::ProviderQos(),
                lastSeenDateMs,
                expiryDateMs,
                TEST_PUBLICKEYID
        );
        EXPECT_CALL(
                localCapabilitiesDirectoryMock,
                lookup(toParticipantId,
                       A<std::function<void(const joynr::types::DiscoveryEntry&)>>(),
                       A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>())
        )
                .Times(1)
                .WillOnce(Invoke(this, &AccessControllerTest::invokeOnSuccessCallbackFct));
    }

protected:
    SingleThreadedIOService singleThreadedIOService;
    MockLocalDomainAccessController localDomainAccessControllerMock;
    std::shared_ptr<MockConsumerPermissionCallback> accessControllerCallback;
    Settings settings;
    MockMessagingSettings messagingSettingsMock;
    MockLocalCapabilitiesDirectory localCapabilitiesDirectoryMock;
    AccessController accessController;
    JoynrMessageFactory messageFactory;
    JoynrMessage message;
    MessagingQos messagingQos;
    DiscoveryEntry discoveryEntry;
    static const std::string fromParticipantId;
    static const std::string toParticipantId;
    static const std::string replyToChannelId;
    static const std::string DUMMY_USERID;
    static const std::string TEST_DOMAIN;
    static const std::string TEST_INTERFACE;
    static const std::string TEST_OPERATION;
    static const std::string TEST_PUBLICKEYID;
private:
    DISALLOW_COPY_AND_ASSIGN(AccessControllerTest);
};

//----- Constants --------------------------------------------------------------
const std::string AccessControllerTest::fromParticipantId("sender");
const std::string AccessControllerTest::toParticipantId("receiver");
const std::string AccessControllerTest::replyToChannelId("replyToId");
const std::string AccessControllerTest::DUMMY_USERID("testUserId");
const std::string AccessControllerTest::TEST_DOMAIN("testDomain");
const std::string AccessControllerTest::TEST_INTERFACE("testInterface");
const std::string AccessControllerTest::TEST_OPERATION("testOperation");
const std::string AccessControllerTest::TEST_PUBLICKEYID("publicKeyId");

//----- Tests ------------------------------------------------------------------

TEST_F(AccessControllerTest, accessWithInterfaceLevelAccessControl) {
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);
    EXPECT_CALL(
            localDomainAccessControllerMock,
            getConsumerPermission(DUMMY_USERID, TEST_DOMAIN, TEST_INTERFACE, TrustLevel::HIGH, _)
    )
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::consumerPermission));

    EXPECT_CALL(*accessControllerCallback, hasConsumerPermission(true))
            .Times(1);

    accessController.hasConsumerPermission(
            message,
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(accessControllerCallback)
    );
}

TEST_F(AccessControllerTest, accessWithOperationLevelAccessControl) {
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);
    EXPECT_CALL(
            localDomainAccessControllerMock,
            getConsumerPermission(DUMMY_USERID, TEST_DOMAIN, TEST_INTERFACE, TrustLevel::HIGH, _)
    )
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::operationNeeded));

    Permission::Enum permissionYes = Permission::YES;
    DefaultValue<Permission::Enum>::Set(permissionYes);
    EXPECT_CALL(
            localDomainAccessControllerMock,
            getConsumerPermission(
                    DUMMY_USERID,
                    TEST_DOMAIN,
                    TEST_INTERFACE,
                    TEST_OPERATION,
                    TrustLevel::HIGH
            )
    )
            .WillOnce(Return(permissionYes));

    EXPECT_CALL(*accessControllerCallback, hasConsumerPermission(true))
            .Times(1);

    accessController.hasConsumerPermission(
            message,
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(accessControllerCallback)
    );
}

TEST_F(AccessControllerTest, accessWithOperationLevelAccessControlAndFaultyMessage) {
    ConsumerPermissionCallbackMaker makeCallback(Permission::YES);
    EXPECT_CALL(
            localDomainAccessControllerMock,
            getConsumerPermission(DUMMY_USERID, TEST_DOMAIN, TEST_INTERFACE, TrustLevel::HIGH, _)
    )
            .Times(1)
            .WillOnce(Invoke(&makeCallback, &ConsumerPermissionCallbackMaker::operationNeeded));

    EXPECT_CALL(*accessControllerCallback, hasConsumerPermission(false))
            .Times(1);

    std::string payload("invalid serialization of Request object");
    message.setPayload(payload);

    accessController.hasConsumerPermission(
            message,
            std::dynamic_pointer_cast<IAccessController::IHasConsumerPermissionCallback>(accessControllerCallback)
    );
}

