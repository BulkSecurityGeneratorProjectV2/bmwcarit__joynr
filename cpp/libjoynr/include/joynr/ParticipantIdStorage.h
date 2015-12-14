/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#ifndef PARTICIPANTIDSTORAGE_H
#define PARTICIPANTIDSTORAGE_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include <string>

namespace joynr
{

/**
 * Creates and persists participant ids.
 *
 * This class is thread safe.
 */
class JOYNR_EXPORT ParticipantIdStorage
{
public:
    /**
     * Persist participant ids using the given file
     */
    ParticipantIdStorage(const std::string& filename);
    virtual ~ParticipantIdStorage()
    {
    }

    static const std::string& STORAGE_FORMAT_STRING();

    /**
     * @brief setProviderParticipantId Sets a participant ID for a specific
     * provider. This is useful for provisioning of provider participant IDs.
     * @param domain the domain of the provider.
     * @param interfaceName the interface name of the provider.
     * @param authenticationToken the authentication token of the provider.
     * @param participantId the participantId to set.
     */
    virtual void setProviderParticipantId(
            const std::string& domain,
            const std::string& interfaceName,
            const std::string& participantId,
            const std::string& authenticationToken = "defaultAuthenticationToken");

    /**
     * Get a provider participant id
     */
    virtual std::string getProviderParticipantId(
            const std::string& domain,
            const std::string& interfaceName,
            const std::string& authenticationToken = "defaultAuthenticationToken");

    /**
     * Get a provider participant id or use a default
     */
    virtual std::string getProviderParticipantId(
            const std::string& domain,
            const std::string& interfaceName,
            const std::string& defaultValue,
            const std::string& authenticationToken = "defaultAuthenticationToken");

private:
    DISALLOW_COPY_AND_ASSIGN(ParticipantIdStorage);
    std::string createProviderKey(const std::string& domain,
                                  const std::string& interfaceName,
                                  const std::string& authenticationToken);
    std::string filename;
};

} // namespace joynr
#endif // PARTICIPANTIDSTORAGE_H
