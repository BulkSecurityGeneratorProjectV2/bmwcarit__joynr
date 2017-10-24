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
#ifndef TESTS_MOCK_MOCKINPROCESSMESSAGINGSKELETON_H
#define TESTS_MOCK_MOCKINPROCESSMESSAGINGSKELETON_H

#include <gmock/gmock.h>

#include "libjoynr/in-process/InProcessMessagingSkeleton.h"

class MockInProcessMessagingSkeleton : public joynr::InProcessMessagingSkeleton
{
public:
    MockInProcessMessagingSkeleton(std::weak_ptr<joynr::IDispatcher> dispatcher) : InProcessMessagingSkeleton(dispatcher){}
    MOCK_METHOD2(transmit, void(std::shared_ptr<joynr::ImmutableMessage> message, const std::function<void(const joynr::exceptions::JoynrRuntimeException&)>& onFailure));
};

#endif // TESTS_MOCK_MOCKINPROCESSMESSAGINGSKELETON_H
