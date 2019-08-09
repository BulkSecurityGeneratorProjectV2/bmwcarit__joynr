/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTS_MOCK_MOCKGLOBALCAPABILITIESDIRECTORYCLIENT_H
#define TESTS_MOCK_MOCKGLOBALCAPABILITIESDIRECTORYCLIENT_H

#include <gmock/gmock.h>

#include "libjoynrclustercontroller/capabilities-client/IGlobalCapabilitiesDirectoryClient.h"

#include "joynr/types/DiscoveryError.h"

class MockGlobalCapabilitiesDirectoryClient : public joynr::IGlobalCapabilitiesDirectoryClient {
public:
    MOCK_METHOD3(add, void(const joynr::types::GlobalDiscoveryEntry& entry,
                           std::function<void()> onSuccess,
                           std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));
    MOCK_METHOD3(add, void(const std::vector<joynr::types::GlobalDiscoveryEntry>& entry,
                           std::function<void()> onSuccess,
                           std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));
    MOCK_METHOD5(add, void(const joynr::types::GlobalDiscoveryEntry& entry,
                           const std::vector<std::string>& gbids,
                           std::function<void()> onSuccess,
                           std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
                           std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onRuntimeError));

    MOCK_METHOD1(remove, void(std::vector<std::string> participantIdList));
    MOCK_METHOD1(remove, void(const std::string& participantId));
    MOCK_METHOD5(lookup, void(
                     const std::vector<std::string>& domain,
                     const std::string& interfaceName,
                     std::int64_t messagingTtl,
                     std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));
    MOCK_METHOD3(lookup, void(
                     const std::string& participantId,
                     std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& capabilities)> callbackFct,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));
    MOCK_METHOD3(touch, void(const std::string& clusterControllerId,
                     std::function<void()> onSuccess,
                     std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError));
};

#endif // TESTS_MOCK_MOCKGLOBALCAPABILITIESDIRECTORYCLIENT_H
