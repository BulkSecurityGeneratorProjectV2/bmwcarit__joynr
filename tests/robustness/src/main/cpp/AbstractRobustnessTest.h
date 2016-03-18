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
#ifndef ABSTRACTROBUSTNESSTEST_H
#define ABSTRACTROBUSTNESSTEST_H
#include <cstdlib>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/tests/robustness/TestInterfaceProxy.h"

#include "joynr/DiscoveryQos.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"
#include "joynr/ProxyBuilder.h"

using namespace ::testing;
using joynr::DiscoveryQos;
using joynr::MessagingQos;
using joynr::Semaphore;
using joynr::JoynrRuntime;
using joynr::tests::robustness::TestInterfaceProxy;
using joynr::exceptions::JoynrRuntimeException;
using joynr::ProxyBuilder;

class AbstractRobustnessTest : public ::testing::Test
{
public:
    AbstractRobustnessTest() = default;

protected:
    static void SetUpTestCase()
    {
        // Get the provider domain
        JOYNR_LOG_INFO(logger, "Creating proxy for provider on domain {}", providerDomain);

        // Initialise the JOYn runtime
        std::string pathToMessagingSettings("resources/robustness-test-consumer.settings");

        runtime = JoynrRuntime::createRuntime(pathToMessagingSettings);

        // Create proxy builder
        proxyBuilder = runtime->createProxyBuilder<TestInterfaceProxy>(providerDomain);

        // Messaging Quality of service
        std::int64_t qosMsgTtl = 20000;
        std::int64_t qosCacheDataFreshnessMs = 400000; // Only consider data cached for < 400 secs

        // Find the provider with the highest priority set in ProviderQos
        DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryTimeout(60000);
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);

        // Build a proxy
        proxy = proxyBuilder->setMessagingQos(MessagingQos(qosMsgTtl))
                        ->setCached(false)
                        ->setDiscoveryQos(discoveryQos)
                        ->build();

        JOYNR_LOG_INFO(logger, "***********************");
        JOYNR_LOG_INFO(logger, "Proxy built.");
        JOYNR_LOG_INFO(logger, "***********************");
    }

    static void TearDownTestCase()
    {
        if (proxy) {
            delete proxy;
            proxy = nullptr;
        }
        if (proxyBuilder) {
            delete proxyBuilder;
            proxyBuilder = nullptr;
        }
        if (runtime) {
            delete runtime;
            runtime = nullptr;
        }
    }

    void callMethodWithStringParameters()
    {
        std::string stringArg = "testString";
        std::string expected = "received stringArg: " + stringArg;
        std::string stringOut;
        try {
            proxy->methodWithStringParameters(stringOut, stringArg);
            EXPECT_EQ(stringOut, expected);
        } catch (const JoynrRuntimeException& e) {
            ADD_FAILURE() << "callMethodWithStringParameters failed with: " << e.getMessage();
        }
    }

    void callMethodWithStringParametersAfterCcRestart()
    {
        killClusterController();
        startClusterController();
        callMethodWithStringParameters();
    }

    void callMethodWithStringParametersBeforeCcOrProviderRestart(bool killCcFlag,
                                                                 bool killProviderFlag)
    {
        auto semaphore = std::make_shared<Semaphore>(0);
        std::string stringArg = "testStringAsync";
        std::string expected = "received stringArg: " + stringArg;

        if (killCcFlag) {
            killClusterController();
        }
        if (killProviderFlag) {
            killProvider();
        }

        auto onSuccess = [semaphore, expected](const std::string& stringOut) {
            EXPECT_EQ(stringOut, expected);
            semaphore->notify();
        };
        auto onRuntimeError = [semaphore](const JoynrRuntimeException& error) {
            ADD_FAILURE() << "methodWithStringParameters returned error: " << error.getMessage();
            semaphore->notify();
        };
        proxy->methodWithStringParametersAsync(stringArg, onSuccess, onRuntimeError);

        if (killCcFlag) {
            startClusterController();
        }
        if (killProviderFlag) {
            startProvider();
        }

        // Wait for answer
        ASSERT_TRUE(semaphore->waitFor(std::chrono::seconds(60)))
                << "methodWithStringParameters timed out";
    }

    void killClusterController()
    {
        int status = std::system("kill-clustercontroller.sh");
        if (!WIFEXITED(status) || WEXITSTATUS(status)) {
            FAIL() << "kill-clustercontroller failed";
        }
    }

    void startClusterController()
    {
        int status = std::system("start-clustercontroller.sh");
        if (!WIFEXITED(status) || WEXITSTATUS(status)) {
            FAIL() << "start-clustercontroller failed";
        }
    }

    void killProvider()
    {
        int status = std::system("kill-provider.sh");
        if (!WIFEXITED(status) || WEXITSTATUS(status)) {
            FAIL() << "kill-provider failed";
        }
    }

    void startProvider()
    {
        // providerDomain
        std::string cmd = "start-provider.sh cpp " + providerDomain;
        int status = std::system(cmd.c_str());
        if (!WIFEXITED(status) || WEXITSTATUS(status)) {
            FAIL() << "start-provider failed";
        }
    }

    static TestInterfaceProxy* proxy;
    static ProxyBuilder<TestInterfaceProxy>* proxyBuilder;
    static JoynrRuntime* runtime;
    static std::string providerDomain;

private:
    ADD_LOGGER(AbstractRobustnessTest);
};

ACTION_P(ReleaseSemaphore, semaphore)
{
    semaphore->notify();
}
#endif // ABSTRACTROBUSTNESSTEST_H
