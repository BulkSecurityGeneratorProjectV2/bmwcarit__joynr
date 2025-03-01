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

#include <chrono>
#include <memory>
#include <string>

#include <boost/filesystem.hpp>

#include "tests/utils/Gtest.h"

#include "joynr/JoynrClusterControllerRuntime.h"
#include "joynr/Settings.h"
#include "joynr/tests/testProxy.h"

#include "tests/JoynrTest.h"
#include "tests/mock/MockTestProvider.h"
#include "tests/utils/PtrUtils.h"

using namespace ::testing;
using namespace joynr;

class PersistencyCreationTest : public testing::Test
{
public:
    PersistencyCreationTest()
            : testProvider(nullptr),
              testProxy(nullptr),
              domain("PersistencyCreationTest"),
              providerParticipantId(),
              MESSAGINGQOS_TTL(1000)
    {
    }

    void init(const std::string& settings)
    {
        ccRuntime = JoynrClusterControllerRuntime::create(
                std::make_unique<joynr::Settings>(settings), failOnFatalRuntimeError);

        types::ProviderQos providerQos;
        providerQos.setScope(joynr::types::ProviderScope::LOCAL);

        testProvider = std::make_shared<MockTestProvider>();
        providerParticipantId =
                ccRuntime->registerProvider<tests::testProvider>(domain, testProvider, providerQos);

        // Create a proxy
        auto testProxyBuilder = ccRuntime->createProxyBuilder<tests::testProxy>(domain);
        DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryScope(types::DiscoveryScope::LOCAL_ONLY);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setDiscoveryTimeoutMs(1000);

        testProxy = testProxyBuilder->setMessagingQos(MessagingQos(MESSAGINGQOS_TTL))
                            ->setDiscoveryQos(discoveryQos)
                            ->build();
    }

    void SetUp()
    {
        // Make sure there are no persisted files from previous test runs.
        joynr::test::util::removeFileInCurrentDirectory(".*\\.settings");
        joynr::test::util::removeFileInCurrentDirectory(".*\\.persist");
        joynr::test::util::removeFileInCurrentDirectory(".*\\.entries");
    }

    void TearDown()
    {
        testProxy.reset();
        ccRuntime->unregisterProvider(providerParticipantId);
        ccRuntime->shutdown();

        test::util::resetAndWaitUntilDestroyed(testProvider);
        test::util::resetAndWaitUntilDestroyed(ccRuntime);

        // Delete test specific files
        joynr::test::util::removeFileInCurrentDirectory(".*\\.settings");
        joynr::test::util::removeFileInCurrentDirectory(".*\\.persist");
        joynr::test::util::removeFileInCurrentDirectory(".*\\.entries");
    }

protected:
    std::shared_ptr<JoynrClusterControllerRuntime> ccRuntime;

    std::shared_ptr<MockTestProvider> testProvider;
    std::shared_ptr<tests::testProxy> testProxy;

    std::string domain;
    std::string providerParticipantId;

    const std::uint64_t MESSAGINGQOS_TTL;
};

TEST_F(PersistencyCreationTest, testPersistencyFilesAreNotWrittenWhenDisabled)
{
    init("test-resources/persistency-cc-disabled.settings");

    std::string hello;
    testProxy->sayHello(hello);

    EXPECT_FALSE(boost::filesystem::exists(
            ClusterControllerSettings::
                    DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME()));
}

TEST_F(PersistencyCreationTest, testPersistencyFilesAreWrittenWhenEnabled)
{
    init("test-resources/persistency-cc-enabled.settings");

    std::string hello;
    testProxy->sayHello(hello);

    // Timing: subscriptionRequests are not allways persisted in time, before the check.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(boost::filesystem::exists(
            ClusterControllerSettings::
                    DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME()));
}
