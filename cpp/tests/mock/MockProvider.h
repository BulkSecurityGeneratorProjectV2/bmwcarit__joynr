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
#ifndef TESTS_MOCK_MOCKPROVIDER_H
#define TESTS_MOCK_MOCKPROVIDER_H

#include <cstdint>

#include <gmock/gmock.h>

#include "joynr/AbstractJoynrProvider.h"
#include "joynr/RequestCallerFactory.h"
#include "joynr/RequestCaller.h"

class MockProvider : public joynr::AbstractJoynrProvider {
public:
    static const std::uint32_t MAJOR_VERSION;
    static const std::uint32_t MINOR_VERSION;
    MOCK_CONST_METHOD0(getProviderQos, joynr::types::ProviderQos());
    MOCK_CONST_METHOD0(getParticipantId, std::string());
    ~MockProvider() override = default;
    const std::string& getInterfaceName() const override;
    static const std::string& INTERFACE_NAME();
};

namespace joynr
{
template <>
inline std::shared_ptr<RequestCaller> RequestCallerFactory::create<MockProvider>(std::shared_ptr<MockProvider> provider)
{
    std::ignore = provider;
    return std::shared_ptr<RequestCaller>(nullptr);
}
} // namespace joynr;

#endif // TESTS_MOCK_MOCKPROVIDER_H
