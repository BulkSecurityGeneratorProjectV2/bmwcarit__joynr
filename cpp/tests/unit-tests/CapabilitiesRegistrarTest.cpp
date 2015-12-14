/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/CapabilitiesRegistrar.h"
#include "tests/utils/MockObjects.h"

using namespace ::testing;
using namespace joynr;

const std::string participantIdFile = "test_participantids.settings";

class CapabilitiesRegistrarTest : public ::testing::Test {
public:
    CapabilitiesRegistrarTest() :
            mockDispatcher(NULL),
            messagingStubAddress(),
            mockParticipantIdStorage(new MockParticipantIdStorage()),
            mockDiscovery(),
            capabilitiesRegistrar(NULL),
            mockProvider(new MockProvider()),
            domain("testDomain"),
            expectedParticipantId("testParticipantId"),
            mockMessageRouter(new MockMessageRouter())
    {

    }
    void SetUp(){
        std::vector<IDispatcher*> dispatcherList;
        mockDispatcher = new MockDispatcher();
        dispatcherList.push_back(mockDispatcher);

        capabilitiesRegistrar = new CapabilitiesRegistrar(
                    dispatcherList,
                    mockDiscovery,
                    messagingStubAddress,
                    mockParticipantIdStorage,
                    messagingStubAddress,
                    mockMessageRouter
        );
    }
    void TearDown(){
        delete capabilitiesRegistrar;
        delete mockDispatcher;

    }
protected:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesRegistrarTest);
    MockDispatcher* mockDispatcher;
    std::shared_ptr<joynr::system::RoutingTypes::Address> messagingStubAddress;
    std::shared_ptr<MockParticipantIdStorage> mockParticipantIdStorage;
    MockDiscovery mockDiscovery;
    CapabilitiesRegistrar* capabilitiesRegistrar;
    std::shared_ptr<MockProvider> mockProvider;
    std::string domain;
    std::string expectedParticipantId;
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
};

TEST_F(CapabilitiesRegistrarTest, add){

    types::ProviderQos testQos;
    testQos.setPriority(100);
    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    domain,
                    IMockProviderInterface::INTERFACE_NAME(),
                    _
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockProvider, getProviderQos())
            .Times(1)
            .WillOnce(Return(testQos))
    ;
    EXPECT_CALL(*mockDispatcher, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    EXPECT_CALL(
                mockDiscovery,
                add(
                    AllOf(
                        Property(&joynr::types::DiscoveryEntry::getDomain, Eq(domain)),
                        Property(&joynr::types::DiscoveryEntry::getInterfaceName, Eq(IMockProviderInterface::INTERFACE_NAME())),
                        Property(&joynr::types::DiscoveryEntry::getParticipantId, Eq(expectedParticipantId)),
                        Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos))
                    )
                )
    ).WillOnce(Return());

    std::string participantId = capabilitiesRegistrar->add(domain, mockProvider);
    EXPECT_EQ(expectedParticipantId, participantId);
}

TEST_F(CapabilitiesRegistrarTest, removeWithDomainAndProviderObject){
    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    domain,
                    IMockProviderInterface::INTERFACE_NAME(),
                    _
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockDispatcher, removeRequestCaller(expectedParticipantId))
            .Times(1);
    EXPECT_CALL(mockDiscovery, remove(
                    expectedParticipantId
    ))
            .Times(1)
            .WillOnce(Return())
    ;
    std::string participantId = capabilitiesRegistrar->remove(domain, mockProvider);
    EXPECT_EQ(expectedParticipantId, participantId);
}

TEST_F(CapabilitiesRegistrarTest, removeWithParticipantId){
    EXPECT_CALL(*mockDispatcher, removeRequestCaller(expectedParticipantId))
            .Times(1);
    EXPECT_CALL(mockDiscovery, remove(
                    expectedParticipantId
    ))
            .Times(1)
            .WillOnce(Return())
    ;
    capabilitiesRegistrar->remove(expectedParticipantId);
}

TEST_F(CapabilitiesRegistrarTest, registerMultipleDispatchersAndRegisterCapability){
    MockDispatcher* mockDispatcher1 = new MockDispatcher();
    MockDispatcher* mockDispatcher2 = new MockDispatcher();
    types::ProviderQos testQos;
    testQos.setPriority(100);

    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    domain,
                    IMockProviderInterface::INTERFACE_NAME(),
                    _
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockProvider, getProviderQos())
            .Times(1)
            .WillRepeatedly(Return(testQos));

    EXPECT_CALL(
                mockDiscovery,
                add(
                    AllOf(
                        Property(&joynr::types::DiscoveryEntry::getDomain, Eq(domain)),
                        Property(&joynr::types::DiscoveryEntry::getInterfaceName, Eq(IMockProviderInterface::INTERFACE_NAME())),
                        Property(&joynr::types::DiscoveryEntry::getParticipantId, Eq(expectedParticipantId)),
                        Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos))
                    )
                )
    ).Times(1).WillOnce(Return())
    ;

    EXPECT_CALL(*mockDispatcher, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    EXPECT_CALL(*mockDispatcher1, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    EXPECT_CALL(*mockDispatcher2, addRequestCaller(expectedParticipantId,_))
            .Times(1);

    capabilitiesRegistrar->addDispatcher(mockDispatcher1);
    capabilitiesRegistrar->addDispatcher(mockDispatcher2);

    std::string participantId = capabilitiesRegistrar->add(domain, mockProvider);
    EXPECT_EQ(expectedParticipantId, participantId);

    delete mockDispatcher1;
    delete mockDispatcher2;
}

TEST_F(CapabilitiesRegistrarTest, removeDispatcher){
    MockDispatcher* mockDispatcher1 = new MockDispatcher();
    MockDispatcher* mockDispatcher2 = new MockDispatcher();
    types::ProviderQos testQos;
    testQos.setPriority(100);

    capabilitiesRegistrar->addDispatcher(mockDispatcher1);
    capabilitiesRegistrar->addDispatcher(mockDispatcher2);

    capabilitiesRegistrar->removeDispatcher(mockDispatcher1);

    EXPECT_CALL(*mockParticipantIdStorage, getProviderParticipantId(
                    domain,
                    IMockProviderInterface::INTERFACE_NAME(),
                    _
    ))
            .Times(1)
            .WillOnce(Return(expectedParticipantId));
    EXPECT_CALL(*mockProvider, getProviderQos())
            .Times(1)
            .WillRepeatedly(Return(testQos));

    EXPECT_CALL(
                mockDiscovery,
                add(
                    AllOf(
                        Property(&joynr::types::DiscoveryEntry::getDomain, Eq(domain)),
                        Property(&joynr::types::DiscoveryEntry::getInterfaceName, Eq(IMockProviderInterface::INTERFACE_NAME())),
                        Property(&joynr::types::DiscoveryEntry::getParticipantId, Eq(expectedParticipantId)),
                        Property(&joynr::types::DiscoveryEntry::getQos, Eq(testQos))
                    )
                )
    ).Times(1).WillOnce(Return())
    ;

    EXPECT_CALL(*mockDispatcher, addRequestCaller(expectedParticipantId,_))
            .Times(1);
    //mockDispatcher1 should not be used as it was removed
    EXPECT_CALL(*mockDispatcher1, addRequestCaller(expectedParticipantId,_))
            .Times(0);
    EXPECT_CALL(*mockDispatcher2, addRequestCaller(expectedParticipantId,_))
            .Times(1);

    std::string participantId = capabilitiesRegistrar->add(domain, mockProvider);
    EXPECT_EQ(expectedParticipantId, participantId);

    delete mockDispatcher1;
    delete mockDispatcher2;
}
