/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#include "InProcessMessagingStubFactory.h"
#include "joynr/InProcessMessagingAddress.h"
#include "common/in-process/InProcessMessagingStub.h"

namespace joynr
{

InProcessMessagingStubFactory::InProcessMessagingStubFactory()
{
}

bool InProcessMessagingStubFactory::canCreate(
        const joynr::system::RoutingTypes::Address& destAddress)
{
    return dynamic_cast<const InProcessMessagingAddress*>(&destAddress);
}

std::shared_ptr<IMessaging> InProcessMessagingStubFactory::create(
        const joynr::system::RoutingTypes::Address& destAddress)
{
    const InProcessMessagingAddress* inprocessAddress =
            dynamic_cast<const InProcessMessagingAddress*>(&destAddress);
    return std::shared_ptr<IMessaging>(new InProcessMessagingStub(inprocessAddress->getSkeleton()));
}

} // namespace joynr
