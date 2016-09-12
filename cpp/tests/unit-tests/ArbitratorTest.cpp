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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <unordered_set>
#include "joynr/DiscoveryQos.h"
#include "joynr/Arbitrator.h"
#include "joynr/exceptions/NoCompatibleProviderFoundException.h"
#include "joynr/types/Version.h"
#include "joynr/ArbitrationStrategyFunction.h"
#include "joynr/LastSeenArbitrationStrategyFunction.h"
#include "joynr/QosArbitrationStrategyFunction.h"
#include "joynr/FixedParticipantArbitrationStrategyFunction.h"
#include "joynr/KeywordArbitrationStrategyFunction.h"

#include "tests/utils/MockObjects.h"

using ::testing::AtLeast;
using ::testing::Throw;

using namespace joynr;

static const std::string domain("unittest-domain");
static const std::string interfaceName("unittest-interface");

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

MATCHER_P(discoveryException, msg, "") {
    return arg.getTypeName() == joynr::exceptions::DiscoveryException::TYPE_NAME() && arg.getMessage() == msg;
}
class MockArbitrator : public Arbitrator {
public:
    MockArbitrator(const std::string& domain,
                       const std::string& interfaceName,
                       const joynr::types::Version& interfaceVersion,
                       joynr::system::IDiscoverySync& discoveryProxy,
                       const DiscoveryQos& discoveryQos,
                       std::unique_ptr<const ArbitrationStrategyFunction> arbitrationStrategyFunction) : Arbitrator(domain,
                               interfaceName,
                               interfaceVersion,
                               discoveryProxy,
                               discoveryQos,
                               std::move(arbitrationStrategyFunction)){};

    MOCK_METHOD0(attemptArbitration, void (void));
};

class ArbitratorTest : public ::testing::Test {
public:
    ArbitratorTest() :
        lastSeenArbitrationStrategyFunction(std::make_unique<const LastSeenArbitrationStrategyFunction>()),
        qosArbitrationStrategyFunction(std::make_unique<const QosArbitrationStrategyFunction>()),
        keywordArbitrationStrategyFunction(std::make_unique<const KeywordArbitrationStrategyFunction>()),
        fixedParticipantArbitrationStrategyFunction(std::make_unique<const FixedParticipantArbitrationStrategyFunction>()),
        lastSeenDateMs(0),
        expiryDateMs(0),
        publicKeyId("publicKeyId"),
        mockDiscovery()
        {}

    void SetUp(){
    }
    void TearDown(){
    }

    void testExceptionFromDiscoveryProxy(Arbitrator &arbitrator);
    void testExceptionEmptyResult(Arbitrator &arbitrator);

    std::unique_ptr<const ArbitrationStrategyFunction> lastSeenArbitrationStrategyFunction;
    std::unique_ptr<const ArbitrationStrategyFunction> qosArbitrationStrategyFunction;
    std::unique_ptr<const ArbitrationStrategyFunction> keywordArbitrationStrategyFunction;
    std::unique_ptr<const ArbitrationStrategyFunction> fixedParticipantArbitrationStrategyFunction;

protected:
    std::int64_t lastSeenDateMs;
    std::int64_t expiryDateMs;
    std::string publicKeyId;
    static joynr::Logger logger;
    MockDiscovery mockDiscovery;
};

INIT_LOGGER(ArbitratorTest);

TEST_F(ArbitratorTest, arbitrationTimeout) {
    types::Version providerVersion;
    std::int64_t discoveryTimeoutMs = std::chrono::milliseconds(1000).count();
    std::int64_t retryIntervalMs = std::chrono::milliseconds(450).count();
    DiscoveryQos discoveryQos;
    discoveryQos.setDiscoveryTimeoutMs(discoveryTimeoutMs);
    discoveryQos.setRetryIntervalMs(retryIntervalMs);
    Semaphore semaphore;
    auto mockArbitrator = std::make_unique<MockArbitrator>("domain",
                    "interfaceName",
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));
    auto mockArbitrationListener = std::make_unique<MockArbitrationListener>();
    mockArbitrator->setArbitrationListener(mockArbitrationListener.get());

    // Use a semaphore to count and wait on calls to the mock listener
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationError(discoveryException("Arbitration could not be finished in time.")));

    auto start = std::chrono::system_clock::now();

    EXPECT_CALL(*mockArbitrator, attemptArbitration()).Times(AtLeast(1));
    mockArbitrator->startArbitration();

    // Wait for timeout
    // Wait for more than discoveryTimeoutMs milliseconds since it might take some time until the 
    // timeout is reported
    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(discoveryTimeoutMs * 10)));

    auto now = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    JOYNR_LOG_DEBUG(logger, "Time elapsed for unsuccessful arbitration : {}", elapsed.count());
    ASSERT_GE(elapsed.count(), discoveryTimeoutMs);
    mockArbitrator->removeArbitrationListener();
}

// Test that the Arbitrator selects the last seen provider
TEST_F(ArbitratorTest, getLastSeen) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    joynr::types::Version providerVersion(47, 11);
    Arbitrator lastSeenArbitrator(domain,
                    interfaceName,
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    std::int64_t latestLastSeenDateMs = 7;
    std::string lastSeenParticipantId = std::to_string(latestLastSeenDateMs);
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    for (std::int64_t i = 0; i <= latestLastSeenDateMs; i++) {
        int64_t lastSeenDateMs = i;   std::string participantId = std::to_string(i);
        discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId,
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
    }

    // Check that the correct participant was selected
    auto mockArbitrationListener = std::make_unique<MockArbitrationListener>();
    lastSeenArbitrator.setArbitrationListener(mockArbitrationListener.get());
    ON_CALL(mockDiscovery, lookup(_,_,_,_)).WillByDefault(testing::SetArgReferee<0>(discoveryEntries));
    EXPECT_CALL(*mockArbitrationListener, setParticipantId(Eq(lastSeenParticipantId)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationSuccessful)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationCanceledForever))).Times(0);

    lastSeenArbitrator.startArbitration();
}

// Test that the Arbitrator selects the provider with the highest priority
TEST_F(ArbitratorTest, getHighestPriority) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    joynr::types::Version providerVersion(47, 11);
    Arbitrator qosArbitrator(domain,
                    interfaceName,
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries.push_back(types::ProviderQos(
                          std::vector<types::CustomParameter>(),     // custom provider parameters
                          priority,                            // priority
                          joynr::types::ProviderScope::GLOBAL, // discovery scope
                          false                                // supports on change notifications
        ));
        participantId.push_back(std::to_string(priority));
    }

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
    }

    // Check that the correct participant was selected
    auto mockArbitrationListener = std::make_unique<MockArbitrationListener>();
    qosArbitrator.setArbitrationListener(mockArbitrationListener.get());
    ON_CALL(mockDiscovery, lookup(_,_,_,_)).WillByDefault(testing::SetArgReferee<0>(discoveryEntries));
    EXPECT_CALL(*mockArbitrationListener, setParticipantId(Eq(participantId.back())));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationSuccessful)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationCanceledForever))).Times(0);

    qosArbitrator.startArbitration();
}

// Test that the Arbitrator selects a provider with compatible version
TEST_F(ArbitratorTest, getHighestPriorityChecksVersion) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator qosArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    // Create a list of discovery entries
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    joynr::types::Version providerVersion;
    int participantIdCounter = 0;
    std::string expectedParticipantId;
    for (std::int32_t i = -2; i < 2; i++) {
        providerVersion.setMajorVersion(expectedVersion.getMajorVersion() + i);
        for (std::int32_t j = -2; j < 2; j++) {
            providerVersion.setMinorVersion(expectedVersion.getMinorVersion() + j);
            discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                     providerVersion,
                                     domain,
                                     interfaceName,
                                     std::to_string(participantIdCounter),
                                     providerQos,
                                     lastSeenDateMs,
                                     expiryDateMs,
                                     publicKeyId
            ));
            if (providerVersion == expectedVersion) {
                expectedParticipantId = std::to_string(participantIdCounter);
            }
            participantIdCounter++;
        }
    }

    // Check that the correct participant was selected
    auto mockArbitrationListener = std::make_unique<MockArbitrationListener>();
    qosArbitrator.setArbitrationListener(mockArbitrationListener.get());
    ON_CALL(mockDiscovery, lookup(_,_,_,_)).WillByDefault(testing::SetArgReferee<0>(discoveryEntries));
    EXPECT_CALL(*mockArbitrationListener, setParticipantId(Eq(expectedParticipantId)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationSuccessful)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationCanceledForever))).Times(0);

    qosArbitrator.startArbitration();
}

// Test that the Arbitrator selects a provider that supports onChange subscriptions
TEST_F(ArbitratorTest, getHighestPriorityOnChange) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setProviderMustSupportOnChange(true);
    joynr::types::Version providerVersion(47, 11);
    Arbitrator qosArbitrator(domain,
                    interfaceName,
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(),
                                priority,
                                types::ProviderScope::GLOBAL,
                                false));
        participantId.push_back(std::to_string(priority));
    }
    for (int priority = 0; priority < 2; priority++) {
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(),
                                priority,
                                types::ProviderScope::GLOBAL,
                                true));
        participantId.push_back("onChange_%1" + std::to_string(priority));
    }

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
    }

    // Check that the correct participant was selected
    auto mockArbitrationListener = std::make_unique<MockArbitrationListener>();
    qosArbitrator.setArbitrationListener(mockArbitrationListener.get());
    ON_CALL(mockDiscovery, lookup(_,_,_,_)).WillByDefault(testing::SetArgReferee<0>(discoveryEntries));
    EXPECT_CALL(*mockArbitrationListener, setParticipantId(Eq(participantId.back())));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationSuccessful)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationCanceledForever))).Times(0);

    qosArbitrator.startArbitration();
}

// Test that the Arbitrator selects the provider with the correct keyword
TEST_F(ArbitratorTest, getKeywordProvider) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version providerVersion(47, 11);
    Arbitrator qosArbitrator(domain,
                    interfaceName,
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(keywordArbitrationStrategyFunction));

    // Create a list of provider Qos and participant ids
    std::vector<types::ProviderQos> qosEntries;
    std::vector<std::string> participantId;
    for (int priority = 0; priority < 8; priority++) {
        // Entries with no parameters
        qosEntries.push_back(types::ProviderQos(std::vector<types::CustomParameter>(),
                                priority,
                                types::ProviderScope::GLOBAL,
                                false));
        participantId.push_back(std::to_string(priority));
    }

    // An entry with no keyword parameters
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("xxx", "yyy"));
    qosEntries.push_back(types::ProviderQos(parameterList, 1, types::ProviderScope::GLOBAL, false));
    participantId.push_back("no_keyword");

    // An entry with an incorrect keyword parameter
    parameterList.push_back(types::CustomParameter("keyword", "unwanted"));
    qosEntries.push_back(types::ProviderQos(parameterList, 1, types::ProviderScope::GLOBAL, false));
    participantId.push_back("incorrect_keyword");

    // An entry with the correct keyword parameter
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    qosEntries.push_back(types::ProviderQos(parameterList, 1, types::ProviderScope::GLOBAL, false));
    participantId.push_back("correct_keyword");

    // Create a list of discovery entries
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    for (std::size_t i = 0; i < qosEntries.size(); i++) {
        discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 participantId[i],
                                 qosEntries[i],
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
    }

    // Check that the correct participant was selected
    auto mockArbitrationListener = std::make_unique<MockArbitrationListener>();
    qosArbitrator.setArbitrationListener(mockArbitrationListener.get());
    ON_CALL(mockDiscovery, lookup(_,_,_,_)).WillByDefault(testing::SetArgReferee<0>(discoveryEntries));
    EXPECT_CALL(*mockArbitrationListener, setParticipantId(Eq(participantId.back())));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationSuccessful)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationCanceledForever))).Times(0);

    qosArbitrator.startArbitration();
}

// Test that the Arbitrator selects the provider with compatible version
TEST_F(ArbitratorTest, getKeywordProviderChecksVersion) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator qosArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(keywordArbitrationStrategyFunction));

    // Create a list of discovery entries with the correct keyword
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    types::ProviderQos providerQos(
                      parameterList,                        // custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries;
    joynr::types::Version providerVersion;
    int participantIdCounter = 0;
    std::string expectedParticipantId;
    for (std::int32_t i = -2; i < 2; i++) {
        providerVersion.setMajorVersion(expectedVersion.getMajorVersion() + i);
        for (std::int32_t j = -2; j < 2; j++) {
            providerVersion.setMinorVersion(expectedVersion.getMinorVersion() + j);
            discoveryEntries.push_back(joynr::types::DiscoveryEntry(
                                     providerVersion,
                                     domain,
                                     interfaceName,
                                     std::to_string(participantIdCounter),
                                     providerQos,
                                     lastSeenDateMs,
                                     expiryDateMs,
                                     publicKeyId
            ));
            if (providerVersion == expectedVersion) {
                expectedParticipantId = std::to_string(participantIdCounter);
            }
            participantIdCounter++;
        }
    }

    // Check that the correct participant was selected
    auto mockArbitrationListener = std::make_unique<MockArbitrationListener>();
    qosArbitrator.setArbitrationListener(mockArbitrationListener.get());
    ON_CALL(mockDiscovery, lookup(_,_,_,_)).WillByDefault(testing::SetArgReferee<0>(discoveryEntries));
    EXPECT_CALL(*mockArbitrationListener, setParticipantId(Eq(expectedParticipantId)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationSuccessful)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(joynr::ArbitrationStatus::ArbitrationCanceledForever))).Times(0);

    qosArbitrator.startArbitration();
}

TEST_F(ArbitratorTest, retryFiveTimes) {
    std::vector<joynr::types::DiscoveryEntry> result;
    EXPECT_CALL(
                mockDiscovery,
                lookup(
                    A<std::vector<joynr::types::DiscoveryEntry>&>(),
                    A<const std::vector<std::string>&>(),
                    A<const std::string&>(),
                    A<const joynr::types::DiscoveryQos&>()
                )
    )
            .Times(5)
            .WillRepeatedly(
                testing::DoAll(
                    testing::SetArgReferee<0>(result),
                    testing::Return()
                )
            );

    DiscoveryQos discoveryQos;
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.setDiscoveryTimeoutMs(450);
    joynr::types::Version providerVersion(47, 11);
    Arbitrator lastSeenArbitrator(domain,
                    interfaceName,
                    providerVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    lastSeenArbitrator.startArbitration();
}

/*
 * Tests that the arbitrators report a NoCompatibleProviderFoundException if only providers
 * with incompatible versions were found
 */

MATCHER_P(noCompatibleProviderFoundException, expectedVersions, "") {
    try {
        auto exception = dynamic_cast<const exceptions::NoCompatibleProviderFoundException&>(arg);
        if (expectedVersions.size() != exception.getDiscoveredIncompatibleVersions().size()) {
            return false;
        }
        for (const auto& version : expectedVersions) {
            if (exception.getDiscoveredIncompatibleVersions().find(version) == exception.getDiscoveredIncompatibleVersions().end()) {
                return false;
            }
        }
        std::string expectedErrorMessage = "Unable to find a provider with a compatible version. " +
                std::to_string(expectedVersions.size()) + " incompabible versions found:";
        if (expectedErrorMessage != exception.getMessage().substr(0, expectedErrorMessage.size())) {
            return false;
        }
        for (const auto& version : expectedVersions) {
            if (exception.getMessage().find(version.toString()) == std::string::npos) {
                return false;
            }
        }
    } catch (const std::bad_cast& e) {
        return false;
    }
    return true;
}

// Test that the Arbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getHighestPriorityReturnsNoCompatibleProviderFoundException) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator qosArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    // Create a list of discovery entries
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }

    EXPECT_CALL(mockDiscovery, lookup(_,_,_,_))
            .WillOnce(testing::SetArgReferee<0>(discoveryEntries1))
            .WillRepeatedly(testing::SetArgReferee<0>(discoveryEntries2));

    MockArbitrationListener* mockArbitrationListener = new MockArbitrationListener();
    qosArbitrator.setArbitrationListener(mockArbitrationListener);

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationError(noCompatibleProviderFoundException(expectedVersions)));

    qosArbitrator.startArbitration();

    delete mockArbitrationListener;
}

// Test that the Arbitrator returns a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getKeywordProviderReturnsNoCompatibleProviderFoundException) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator keywordArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(keywordArbitrationStrategyFunction));

    // Create a list of discovery entries with the correct keyword
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("keyword", keywordValue));
    types::ProviderQos providerQos(
                      parameterList,                        // custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }

    EXPECT_CALL(mockDiscovery, lookup(_,_,_,_))
            .WillOnce(testing::SetArgReferee<0>(discoveryEntries1))
            .WillRepeatedly(testing::SetArgReferee<0>(discoveryEntries2));

    MockArbitrationListener* mockArbitrationListener = new MockArbitrationListener();
    keywordArbitrator.setArbitrationListener(mockArbitrationListener);

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationError(noCompatibleProviderFoundException(expectedVersions)));

    keywordArbitrator.startArbitration();

    delete mockArbitrationListener;
}

// Test that the Arbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getFixedParticipantProviderReturnsNoCompatibleProviderFoundException) {
    // Search for this keyword value
    const std::string participantId("unittests-participantId");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator fixedParticipantArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(fixedParticipantArbitrationStrategyFunction));

    // Create a discovery entries with the correct participantId
    std::vector<types::CustomParameter> parameterList;
    parameterList.push_back(types::CustomParameter("fixedParticipantId", participantId));
    types::ProviderQos providerQos(
                      parameterList,// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    // discovery entries for first lookup
    joynr::types::Version providerVersion1(7, 8);
    joynr::types::DiscoveryEntry discoveryEntry1(
                             providerVersion1,
                             domain,
                             interfaceName,
                             participantId,
                             providerQos,
                             lastSeenDateMs,
                             expiryDateMs,
                             publicKeyId
    );
    // discoveryEntries for subsequent lookups
    joynr::types::Version providerVersion2(23, 12);
    joynr::types::DiscoveryEntry discoveryEntry2(
                             providerVersion2,
                             domain,
                             interfaceName,
                             participantId,
                             providerQos,
                             lastSeenDateMs,
                             expiryDateMs,
                             publicKeyId
    );

    EXPECT_CALL(mockDiscovery, lookup(_,Eq(participantId)))
            .WillOnce(testing::SetArgReferee<0>(discoveryEntry1))
            .WillRepeatedly(testing::SetArgReferee<0>(discoveryEntry2));

    MockArbitrationListener* mockArbitrationListener = new MockArbitrationListener();
    fixedParticipantArbitrator.setArbitrationListener(mockArbitrationListener);

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersion2);

    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationError(noCompatibleProviderFoundException(expectedVersions)));

    fixedParticipantArbitrator.startArbitration();

    delete mockArbitrationListener;
}

// Test that the lastSeenArbitrator reports a NoCompatibleProviderFoundException to the listener
// containing the versions that were found during the last retry
TEST_F(ArbitratorTest, getDefaultReturnsNoCompatibleProviderFoundException) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator lastSeenArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    // Create a list of discovery entries
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    // discovery entries for first lookup
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries1;
    std::vector<joynr::types::Version> providerVersions1;
    int participantIdCounter = 0;
    for (std::int32_t i = 0; i < 8; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions1.push_back(providerVersion);
        discoveryEntries1.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }
    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries2;
    std::vector<joynr::types::Version> providerVersions2;
    for (std::int32_t i = 30; i < 36; i++) {
        joynr::types::Version providerVersion(i, i);
        providerVersions2.push_back(providerVersion);
        discoveryEntries2.push_back(joynr::types::DiscoveryEntry(
                                 providerVersion,
                                 domain,
                                 interfaceName,
                                 std::to_string(participantIdCounter),
                                 providerQos,
                                 lastSeenDateMs,
                                 expiryDateMs,
                                 publicKeyId
        ));
        participantIdCounter++;
    }

    EXPECT_CALL(mockDiscovery, lookup(_,_,_,_))
            .WillOnce(testing::SetArgReferee<0>(discoveryEntries1))
            .WillRepeatedly(testing::SetArgReferee<0>(discoveryEntries2));

    MockArbitrationListener* mockArbitrationListener = new MockArbitrationListener();
    lastSeenArbitrator.setArbitrationListener(mockArbitrationListener);

    std::unordered_set<joynr::types::Version> expectedVersions;
    expectedVersions.insert(providerVersions2.begin(), providerVersions2.end());

    EXPECT_CALL(*mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(*mockArbitrationListener, setArbitrationError(noCompatibleProviderFoundException(expectedVersions)));

    lastSeenArbitrator.startArbitration();

    delete mockArbitrationListener;
}


/*
 * Tests that the arbitrators report the exception from the discoveryProxy if the lookup fails
 * during the last retry
 */
MATCHER_P(exceptionFromDiscoveryProxy, originalException, "") {
    std::string expectedErrorMsg = "Unable to lookup provider (domain: " + domain +
            ", interface: " + interfaceName + ") from discovery. Error: " +
            originalException.getMessage();
    return arg.getMessage() == expectedErrorMsg;
}

void ArbitratorTest::testExceptionFromDiscoveryProxy(Arbitrator &arbitrator){
    exceptions::JoynrRuntimeException exception1("first exception");
    exceptions::JoynrRuntimeException expectedException("expected exception");
    EXPECT_CALL(mockDiscovery, lookup(_,_,_,_))
            .WillOnce(Throw(exception1))
            .WillRepeatedly(Throw(expectedException));

    MockArbitrationListener mockArbitrationListener;
    arbitrator.setArbitrationListener(&mockArbitrationListener);

    EXPECT_CALL(mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(mockArbitrationListener, setArbitrationError(exceptionFromDiscoveryProxy(expectedException)));

    arbitrator.startArbitration();
}

TEST_F(ArbitratorTest, getHighestPriorityReturnsExceptionFromDiscoveryProxy) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator qosArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    testExceptionFromDiscoveryProxy(qosArbitrator);
}

TEST_F(ArbitratorTest, getKeywordProviderReturnsExceptionFromDiscoveryProxy) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator keywordArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(keywordArbitrationStrategyFunction));

    testExceptionFromDiscoveryProxy(keywordArbitrator);
}

TEST_F(ArbitratorTest, getFixedParticipantProviderReturnsExceptionFromDiscoveryProxy) {
    // Search for this keyword value
    const std::string participantId("unittests-participantId");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("fixedParticipantId", participantId);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator fixedParticipantArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(fixedParticipantArbitrationStrategyFunction));

    exceptions::JoynrRuntimeException exception1("first exception");
    exceptions::JoynrRuntimeException expectedException("expected exception");
    EXPECT_CALL(mockDiscovery, lookup(_,Eq(participantId)))
            .WillOnce(Throw(exception1))
            .WillRepeatedly(Throw(expectedException));

    MockArbitrationListener mockArbitrationListener;
    fixedParticipantArbitrator.setArbitrationListener(&mockArbitrationListener);

    EXPECT_CALL(mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(mockArbitrationListener, setArbitrationError(exceptionFromDiscoveryProxy(expectedException)));

    fixedParticipantArbitrator.startArbitration();
}

TEST_F(ArbitratorTest, getLastSeenReturnsExceptionFromDiscoveryProxy) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator lastSeenArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    testExceptionFromDiscoveryProxy(lastSeenArbitrator);
}


/*
 * Tests that the arbitrators report an exception if no entries were found during the last retry
 */

MATCHER_P(exceptionEmptyResult, originalException, "") {
    std::string expectedErrorMsg = "No entries found for domain: " + domain +
            ", interface: " + interfaceName;
    return arg.getMessage() == expectedErrorMsg;
}

void ArbitratorTest::testExceptionEmptyResult(Arbitrator &arbitrator){
    // discovery entries for first lookup
    types::ProviderQos providerQos(
                      std::vector<types::CustomParameter>(),// custom provider parameters
                      42,                                   // priority
                      joynr::types::ProviderScope::GLOBAL,  // discovery scope
                      false                                 // supports on change notifications
    );
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries1;
    joynr::types::Version providerVersion(22, 23);
    discoveryEntries1.push_back(joynr::types::DiscoveryEntry(
                             providerVersion,
                             domain,
                             interfaceName,
                             "testParticipantId",
                             providerQos,
                             lastSeenDateMs,
                             expiryDateMs,
                             publicKeyId
    ));

    // discoveryEntries for subsequent lookups
    std::vector<joynr::types::DiscoveryEntry> discoveryEntries2;

    EXPECT_CALL(mockDiscovery, lookup(_,_,_,_))
            .WillOnce(testing::SetArgReferee<0>(discoveryEntries1))
            .WillRepeatedly(testing::SetArgReferee<0>(discoveryEntries2));

    MockArbitrationListener mockArbitrationListener;
    arbitrator.setArbitrationListener(&mockArbitrationListener);
    exceptions::JoynrRuntimeException expectedException("expected exception");

    EXPECT_CALL(mockArbitrationListener, setArbitrationStatus(Eq(ArbitrationStatus::ArbitrationCanceledForever)));
    EXPECT_CALL(mockArbitrationListener, setArbitrationError(exceptionEmptyResult(expectedException)));

    arbitrator.startArbitration();
}

TEST_F(ArbitratorTest, getHighestPriorityReturnsExceptionEmptyResult) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator qosArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(qosArbitrationStrategyFunction));

    testExceptionEmptyResult(qosArbitrator);
}

TEST_F(ArbitratorTest, getKeywordProviderReturnsExceptionEmptyResult) {
    // Search for this keyword value
    const std::string keywordValue("unittests-keyword");

    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::KEYWORD);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    discoveryQos.addCustomParameter("keyword", keywordValue);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator keywordArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(keywordArbitrationStrategyFunction));

    testExceptionEmptyResult(keywordArbitrator);
}

// Arbitrator has no special exception for empty results

TEST_F(ArbitratorTest, getLastSeenReturnsExceptionEmptyResult) {
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::LAST_SEEN);
    discoveryQos.setDiscoveryTimeoutMs(199);
    discoveryQos.setRetryIntervalMs(100);
    joynr::types::Version expectedVersion(47, 11);
    Arbitrator lastSeenArbitrator(domain,
                    interfaceName,
                    expectedVersion,
                    mockDiscovery,
                    discoveryQos,
                    move(lastSeenArbitrationStrategyFunction));

    testExceptionEmptyResult(lastSeenArbitrator);
}
