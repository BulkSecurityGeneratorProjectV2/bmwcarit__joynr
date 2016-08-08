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
#ifndef ILTABSTRACTCONSUMERTEST_H
#define ILTABSTRACTCONSUMERTEST_H
#include <cstdlib>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "JoynrTest.h"
#include "joynr/interlanguagetest/TestInterfaceProxy.h"

#include "joynr/DiscoveryQos.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"
#include "joynr/ProxyBuilder.h"
#include "IltHelper.h"
#include "IltUtil.h"

class IltAbstractConsumerTest : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        // Get the provider domain
        JOYNR_LOG_INFO(logger, "Creating proxy for provider on domain {}", providerDomain);

        // Get the current program directory
        std::string dir(
                IltHelper::getAbsolutePathToExecutable(IltAbstractConsumerTest::programName));

        // Initialise the JOYn runtime
        std::string pathToMessagingSettings(dir + "/resources/test-app-consumer.settings");

        runtime = joynr::JoynrRuntime::createRuntime(pathToMessagingSettings);

        // Create proxy builder
        proxyBuilder = runtime->createProxyBuilder<joynr::interlanguagetest::TestInterfaceProxy>(
                providerDomain);

        // Messaging Quality of service
        std::int64_t qosMsgTtl = 5000;
        std::int64_t qosCacheDataFreshnessMs = 400000; // Only consider data cached for < 400 secs

        // Find the provider with the highest priority set in ProviderQos
        joynr::DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryTimeoutMs(40000);
        discoveryQos.setCacheMaxAgeMs(std::numeric_limits<std::int64_t>::max());
        discoveryQos.setArbitrationStrategy(
                joynr::DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);

        // Build a proxy
        testInterfaceProxy = proxyBuilder->setMessagingQos(joynr::MessagingQos(qosMsgTtl))
                                     ->setCached(false)
                                     ->setDiscoveryQos(discoveryQos)
                                     ->build();

        JOYNR_LOG_INFO(logger, "***********************");
        JOYNR_LOG_INFO(logger, "Proxy built.");
        JOYNR_LOG_INFO(logger, "***********************");
    }

    static void TearDownTestCase()
    {
        if (testInterfaceProxy) {
            delete testInterfaceProxy;
            testInterfaceProxy = nullptr;
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

    static void waitForChange(volatile bool& value, int timeout)
    {
        useconds_t remaining = timeout * 1000;
        useconds_t interval = 100000; // 0.1 seconds

        while (remaining > 0 && !value) {
            usleep(interval);
            remaining -= interval;
        }
    }

    static joynr::interlanguagetest::TestInterfaceProxy* testInterfaceProxy;
    static joynr::ProxyBuilder<joynr::interlanguagetest::TestInterfaceProxy>* proxyBuilder;
    static joynr::JoynrRuntime* runtime;
    static std::string providerDomain;
    static std::string programName;

    ADD_LOGGER(IltAbstractConsumerTest);

public:
    IltAbstractConsumerTest() = default;

    void static setProgramName(std::string programName)
    {
        IltAbstractConsumerTest::programName = programName;
    }
};

ACTION_P(ReleaseSemaphore, semaphore)
{
    semaphore->notify();
}
#endif // ILTABSTRACTCONSUMERTEST_H
