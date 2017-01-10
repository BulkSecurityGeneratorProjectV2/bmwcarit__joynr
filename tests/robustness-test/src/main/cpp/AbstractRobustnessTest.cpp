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
#include "AbstractRobustnessTest.h"
#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

INIT_LOGGER(AbstractRobustnessTest);

std::unique_ptr<TestInterfaceProxy> AbstractRobustnessTest::proxy;
std::unique_ptr<ProxyBuilder<TestInterfaceProxy>> AbstractRobustnessTest::proxyBuilder;
JoynrRuntime* AbstractRobustnessTest::runtime = nullptr;
std::string AbstractRobustnessTest::providerDomain = "joynr-robustness-test-domain";

int main(int argc, char** argv)
{
#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JOYT", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
