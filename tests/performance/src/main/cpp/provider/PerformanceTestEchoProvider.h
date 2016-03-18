/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

#ifndef PERFORMANCETEST_PROVIDER_H
#define PERFORMANCETEST_PROVIDER_H

#include "joynr/tests/performance/DefaultEchoProvider.h"

namespace joynr
{

class PerformanceTestEchoProvider : public joynr::tests::performance::DefaultEchoProvider
{
public:
    PerformanceTestEchoProvider();
    virtual ~PerformanceTestEchoProvider() override = default;

    void echoString(const std::string& data,
                    std::function<void(const std::string& responseData)> onSuccess,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

    void echoByteArray(const std::vector<std::int8_t>& data,
                       std::function<void(const std::vector<std::int8_t>& responseData)> onSuccess,
                       std::function<void(const joynr::exceptions::ProviderRuntimeException&)>
                               onError) override;

    void echoComplexStruct(
            const joynr::tests::performance::Types::ComplexStruct& data,
            std::function<void(const joynr::tests::performance::Types::ComplexStruct& responseData)>
                    onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;
};
} // namespace joynr

#endif // PERFORMANCETEST_PROVIDER_H
