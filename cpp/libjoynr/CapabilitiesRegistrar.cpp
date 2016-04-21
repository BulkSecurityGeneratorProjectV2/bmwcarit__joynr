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
#include "joynr/CapabilitiesRegistrar.h"
#include "joynr/ParticipantIdStorage.h"

namespace joynr
{

INIT_LOGGER(CapabilitiesRegistrar);

CapabilitiesRegistrar::CapabilitiesRegistrar(
        std::vector<IDispatcher*> dispatcherList,
        joynr::system::IDiscoverySync& discoveryProxy,
        std::shared_ptr<ParticipantIdStorage> participantIdStorage,
        std::shared_ptr<const joynr::system::RoutingTypes::Address> dispatcherAddress,
        std::shared_ptr<MessageRouter> messageRouter)
        : dispatcherList(dispatcherList),
          discoveryProxy(discoveryProxy),
          participantIdStorage(participantIdStorage),
          dispatcherAddress(dispatcherAddress),
          messageRouter(messageRouter)
{
}

void CapabilitiesRegistrar::remove(const std::string& participantId)
{
    for (IDispatcher* currentDispatcher : dispatcherList) {
        currentDispatcher->removeRequestCaller(participantId);
    }
    try {
        discoveryProxy.remove(participantId);
    } catch (const exceptions::JoynrException& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to remove provider (participant ID: {}) to discovery. Error: {}",
                        participantId,
                        e.getMessage());
    }

    auto future = std::make_shared<Future<void>>();
    auto onSuccess = [future]() { future->onSuccess(); };
    auto onError = [future](const joynr::exceptions::ProviderRuntimeException& error) {
        future->onError(error);
    };
    messageRouter->removeNextHop(participantId, onSuccess, onError);
    try {
        future->get();
    } catch (const exceptions::JoynrRuntimeException& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to remove next hop (participant ID: {}) from message router.",
                        participantId);
    }
}

void CapabilitiesRegistrar::addDispatcher(IDispatcher* dispatcher)
{
    dispatcherList.push_back(dispatcher);
}

void CapabilitiesRegistrar::removeDispatcher(IDispatcher* dispatcher)
{
    util::removeAll(dispatcherList, dispatcher);
}

} // namespace joynr
