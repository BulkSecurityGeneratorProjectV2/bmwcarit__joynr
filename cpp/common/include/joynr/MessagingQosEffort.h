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

#ifndef MESSAGINGQOS_EFFORT_H
#define MESSAGINGQOS_EFFORT_H

#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

struct MessagingQosEffort
{
    enum class Enum { BEST_EFFORT = 0, NORMAL = 1 };

    static std::string getLiteral(const MessagingQosEffort::Enum& value)
    {
        switch (value) {
        case Enum::BEST_EFFORT:
            return "Best effort";
        case Enum::NORMAL:
            return "Normal";
        default:
            throw exceptions::JoynrRuntimeException("Invalid messaging QoS effort value");
        }
    }
};

} // namespace joynr

#endif /* MESSAGINGQOS_EFFORT_H */
