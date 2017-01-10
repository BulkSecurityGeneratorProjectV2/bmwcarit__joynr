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

#include <memory>
#include <string>
#include <cstdint>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "tests/utils/MockObjects.h"
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/Future.h"
#include "joynr/Util.h"
#include "joynr/Settings.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/PrivateCopyAssign.h"

using namespace ::testing;
using namespace joynr;

class End2EndSSLTest : public Test{
public:
    std::string domain;
    JoynrClusterControllerRuntime* runtime;

    End2EndSSLTest() :
        domain(),
        runtime(nullptr)
    {
        auto settings = std::make_unique<Settings>("test-resources/integrationtest.settings");
        Settings sslSettings{"test-resources/sslintegrationtest.settings"};
        Settings integrationTestSettings{"test-resources/libjoynrintegrationtest.settings"};
        Settings::merge(sslSettings, *settings, false);
        Settings::merge(integrationTestSettings, *settings, false);
        runtime = new JoynrClusterControllerRuntime(std::move(settings));
        std::string uuid = util::createUuid();
        domain = "cppEnd2EndSSLTest_Domain_" + uuid;
    }

    // Sets up the test fixture.
    void SetUp(){
       runtime->start();
    }

    // Tears down the test fixture.
    void TearDown(){
        bool deleteChannel = true;
        runtime->stop(deleteChannel);

        // Delete persisted files
        std::remove(LibjoynrSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());

        std::this_thread::sleep_for(std::chrono::milliseconds(550));
    }

    ~End2EndSSLTest(){
        delete runtime;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(End2EndSSLTest);
};

TEST_F(End2EndSSLTest, DISABLED_call_rpc_method_and_get_expected_result)
{

    // Create a provider
    auto mockProvider = std::make_shared<MockGpsProvider>();
    types::ProviderQos providerQos;
    std::chrono::milliseconds millisSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    providerQos.setPriority(millisSinceEpoch.count());
    providerQos.setScope(joynr::types::ProviderScope::GLOBAL);
    providerQos.setSupportsOnChangeSubscriptions(true);
    runtime->registerProvider<vehicle::GpsProvider>(domain, mockProvider, providerQos);
    std::this_thread::sleep_for(std::chrono::milliseconds(550));

    // Build a proxy
    std::unique_ptr<ProxyBuilder<vehicle::GpsProxy>> gpsProxyBuilder =
            runtime->createProxyBuilder<vehicle::GpsProxy>(domain);
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeoutMs(1000);

    std::int64_t qosRoundTripTTL = 40000;
    std::unique_ptr<vehicle::GpsProxy> gpsProxy = gpsProxyBuilder
            ->setMessagingQos(MessagingQos(qosRoundTripTTL))
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

    // Call the provider and wait for a result
    std::shared_ptr<Future<int> >gpsFuture (gpsProxy->calculateAvailableSatellitesAsync());
    gpsFuture->wait();

    int expectedValue = 42; //as defined in MockGpsProvider
    int actualValue;
    gpsFuture->get(actualValue);
    EXPECT_EQ(expectedValue, actualValue);
}
