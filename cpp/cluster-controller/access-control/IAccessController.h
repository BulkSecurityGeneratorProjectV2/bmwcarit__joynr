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

#ifndef IACCESSCONTROLLER_H
#define IACCESSCONTROLLER_H

#include "joynr/infrastructure/DacTypes_QtTrustLevel.h"

#include <memory>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace joynr
{

class JoynrMessage;

/**
 * Interface for objects that control access to providers
 */
class IAccessController
{
public:
    /**
     * Callback interface for hasConsumerPermission
     *
     * Calls to hasConsumerPermission are asynchronous and a callback object must
     * be supplied.
     */
    class IHasConsumerPermissionCallback
    {
    public:
        virtual ~IHasConsumerPermissionCallback()
        {
        }

        /**
         * Called with the result of hasConsumerPermission
         */
        virtual void hasConsumerPermission(bool hasPermission) = 0;
    };

    virtual ~IAccessController()
    {
    }

    /**
     * Does the given request message have permission to reach the provider?
     *
     * @param message The message to check
     * @param callback An object that will be called back with the result
     */
    virtual void hasConsumerPermission(
            const JoynrMessage& message,
            std::shared_ptr<IHasConsumerPermissionCallback> callback) = 0;

    /**
     * Does the provider with given userId and given trust level, have permission to expose given
     *domain interface?
     *
     * @param userId The provider userId
     * @param trustLevel The trustLevel for given userId
     * @param domain The domain where provider interface belongs to
     * @param interfaceName The interface provider wants to register
     * @return true if the message has permission, false otherwise
     */
    virtual bool hasProviderPermission(const QString& userId,
                                       infrastructure::DacTypes::QtTrustLevel::Enum trustLevel,
                                       const QString& domain,
                                       const QString& interfaceName) = 0;
    /**
     * @brief addParticipantToWhitelist Adds a participant to the internal
     * whitelist. Access control to participants on the whitelist is always
     * granted.
     *
     * @param participantId the participant ID to add to the whitelist.
     */
    virtual void addParticipantToWhitelist(const QString& participantId) = 0;
};

} // namespace joynr
#endif // IACCESSCONTROLLER_H
