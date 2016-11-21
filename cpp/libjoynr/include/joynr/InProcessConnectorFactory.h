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
#ifndef INPROCESSCONNECTORFACTORY_H
#define INPROCESSCONNECTORFACTORY_H

#include <string>
#include <memory>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/InProcessAddress.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/IRequestCallerDirectory.h"

namespace joynr
{

class InProcessPublicationSender;
class ISubscriptionManager;
class PublicationManager;
class IPlatformSecurityManager;

// traits class which is specialized for every Interface
// this links Interface with the respective Connector
template <typename Interface>
struct InProcessTraits;

// A factory that creates an InProcessConnector for a generated interface
class JOYNR_EXPORT InProcessConnectorFactory
{
public:
    InProcessConnectorFactory(ISubscriptionManager* subscriptionManager,
                              PublicationManager* publicationManager,
                              InProcessPublicationSender* inProcessPublicationSender,
                              IRequestCallerDirectory* requestCallerDirectory);

    virtual ~InProcessConnectorFactory() = default;

    template <class T>
    std::unique_ptr<T> create(const std::string& proxyParticipantId,
                              const std::string& providerParticipantId)
    {
        std::shared_ptr<RequestCaller> requestCaller =
                requestCallerDirectory->lookupRequestCaller(providerParticipantId);

        // early exit if the providerParticipantId could not be found
        if (requestCaller == nullptr) {
            JOYNR_LOG_ERROR(logger, "Cannot create connector: Provider participant ID not found.");
            return nullptr;
        }

        auto inProcessEndpointAddress = std::make_shared<InProcessAddress>(requestCaller);

        using Connector = typename InProcessTraits<T>::Connector;
        return std::make_unique<Connector>(subscriptionManager,
                                           publicationManager,
                                           inProcessPublicationSender,
                                           securityManager,
                                           proxyParticipantId,
                                           providerParticipantId,
                                           inProcessEndpointAddress);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(InProcessConnectorFactory);
    ISubscriptionManager* subscriptionManager;
    PublicationManager* publicationManager;
    InProcessPublicationSender* inProcessPublicationSender;
    IRequestCallerDirectory* requestCallerDirectory;
    std::shared_ptr<IPlatformSecurityManager> securityManager;
    ADD_LOGGER(InProcessConnectorFactory);
};

} // namespace joynr
#endif // INPROCESSCONNECTORFACTORY_H
