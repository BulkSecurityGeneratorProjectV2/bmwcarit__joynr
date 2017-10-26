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
#ifndef TESTS_MOCK_MOCKRUNNABLE_H
#define TESTS_MOCK_MOCKRUNNABLE_H

#include <gmock/gmock.h>

#include "joynr/Runnable.h"

class MockRunnable : public joynr::Runnable
{
public:
    MockRunnable() : Runnable()
    {
    }

    MOCK_CONST_METHOD0(dtorCalled, void ());
    ~MockRunnable()
    {
        dtorCalled();
    }

    MOCK_METHOD0(shutdown, void ());
    MOCK_METHOD0(run, void ());
};

#endif // TESTS_MOCK_MOCKRUNNABLE_H
