/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "joynr/RequestCaller.h"

#include "joynr/IJoynrProvider.h"

namespace joynr
{

RequestCaller::RequestCaller(const std::string& interfaceName) : interfaceName(interfaceName)
{
}

RequestCaller::RequestCaller(std::string&& interfaceName) : interfaceName(std::move(interfaceName))
{
}

const std::string& RequestCaller::getInterfaceName() const
{
    return interfaceName;
}

void RequestCaller::registerAttributeListener(const std::string& attributeName,
                                              SubscriptionAttributeListener* attributeListener)
{
    getProvider()->registerAttributeListener(attributeName, attributeListener);
}

void RequestCaller::unregisterAttributeListener(const std::string& attributeName,
                                                SubscriptionAttributeListener* attributeListener)
{
    getProvider()->unregisterAttributeListener(attributeName, attributeListener);
}

void RequestCaller::registerBroadcastListener(
        const std::string& broadcastName,
        std::shared_ptr<UnicastBroadcastListener> broadcastListener)
{
    getProvider()->registerBroadcastListener(broadcastName, std::move(broadcastListener));
}

void RequestCaller::unregisterBroadcastListener(
        const std::string& broadcastName,
        std::shared_ptr<UnicastBroadcastListener> broadcastListener)
{
    getProvider()->unregisterBroadcastListener(broadcastName, std::move(broadcastListener));
}

} // namespace joynr
