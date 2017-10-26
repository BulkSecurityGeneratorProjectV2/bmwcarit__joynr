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
#ifndef TESTS_MOCK_MOCKMESSAGINGSTUBFACTORY_H
#define TESTS_MOCK_MOCKMESSAGINGSTUBFACTORY_H

#include <gmock/gmock.h>

#include "joynr/IMessagingStubFactory.h"

class MockMessagingStubFactory : public joynr::IMessagingStubFactory {
public:
    MOCK_METHOD1(create, std::shared_ptr<joynr::IMessagingStub>(const std::shared_ptr<const joynr::system::RoutingTypes::Address>&));
    MOCK_METHOD1(remove, void(const std::shared_ptr<const joynr::system::RoutingTypes::Address>&));
    MOCK_METHOD1(contains, bool(const std::shared_ptr<const joynr::system::RoutingTypes::Address>&));
    MOCK_METHOD0(shutdown, void ());
};

#endif // TESTS_MOCK_MOCKMESSAGINGSTUBFACTORY_H
