/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "DummyPlatformSecurityManager.h"
#include "joynr/JoynrMessage.h"
#include <cassert>
#include <tuple>

namespace joynr
{

INIT_LOGGER(DummyPlatformSecurityManager);

std::string DummyPlatformSecurityManager::getCurrentProcessUserId()
{
    return std::string("USER");
}

JoynrMessage DummyPlatformSecurityManager::sign(JoynrMessage message)
{
    std::ignore = message;
    assert(false && "Not implemented yet");
    return JoynrMessage();
}

bool DummyPlatformSecurityManager::validate(const JoynrMessage& message) const
{
    std::ignore = message;
    return true;
}

std::string DummyPlatformSecurityManager::encrypt(const std::string& unencryptedBytes)
{
    std::ignore = unencryptedBytes;
    assert(false && "Not implemented yet");
    return std::string("");
}

std::string DummyPlatformSecurityManager::decrypt(const std::string& encryptedBytes)
{
    std::ignore = encryptedBytes;
    assert(false && "Not implemented yet");
    return std::string("");
}

} // namespace joynr
