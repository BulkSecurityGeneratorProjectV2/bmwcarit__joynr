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
#include "joynr/SystemServicesSettings.h"
#include "joynr/Settings.h"
#include "joynr/TypeUtil.h"

#include <cassert>

namespace joynr
{

INIT_LOGGER(SystemServicesSettings);
SystemServicesSettings::SystemServicesSettings(Settings& settings) : settings(settings)
{
    Settings defaultSystemServicesSettings{DEFAULT_SYSTEM_SERVICES_SETTINGS_FILENAME()};
    Settings::merge(defaultSystemServicesSettings, this->settings, false);
    checkSettings();
}

const std::string& SystemServicesSettings::SETTING_DOMAIN()
{
    static const std::string value("system.services/domain");
    return value;
}

const std::string& SystemServicesSettings::SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN()
{
    static const std::string value("system.services/cc-routingprovider-authenticationtoken");
    return value;
}

const std::string& SystemServicesSettings::SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()
{
    static const std::string value("system.services/cc-routingprovider-participantid");
    return value;
}

const std::string& SystemServicesSettings::SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN()
{
    static const std::string value("system.services/cc-discoveryprovider-authenticationtoken");
    return value;
}

const std::string& SystemServicesSettings::SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()
{
    static const std::string value("system.services/cc-discoveryprovider-participantid");
    return value;
}

const std::string& SystemServicesSettings::DEFAULT_SYSTEM_SERVICES_SETTINGS_FILENAME()
{
    static const std::string value("resources/default-system-services.settings");
    return value;
}

std::string SystemServicesSettings::getDomain() const
{
    return settings.get<std::string>(SETTING_DOMAIN());
}

void SystemServicesSettings::setJoynrSystemServicesDomain(const std::string& systemServicesDomain)
{
    settings.set(SETTING_DOMAIN(), systemServicesDomain);
}

std::string SystemServicesSettings::getCcRoutingProviderAuthenticationToken() const
{
    return settings.get<std::string>(SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN());
}

void SystemServicesSettings::setCcRoutingProviderAuthenticationToken(
        const std::string& authenticationToken)
{
    settings.set(SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN(), authenticationToken);
}

std::string SystemServicesSettings::getCcRoutingProviderParticipantId() const
{
    return settings.get<std::string>(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID());
}

void SystemServicesSettings::setCcRoutingProviderParticipantId(const std::string& participantId)
{
    settings.set(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID(), participantId);
}

std::string SystemServicesSettings::getCcDiscoveryProviderAuthenticationToken() const
{
    return settings.get<std::string>(SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN());
}

void SystemServicesSettings::setCcDiscoveryProviderAuthenticationToken(
        const std::string& authenticationToken)
{
    settings.set(SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN(), authenticationToken);
}

std::string SystemServicesSettings::getCcDiscoveryProviderParticipantId() const
{
    return settings.get<std::string>(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID());
}

void SystemServicesSettings::setCcDiscoveryProviderParticipantId(const std::string& participantId)
{
    settings.set(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID(), participantId);
}

bool SystemServicesSettings::contains(const std::string& key) const
{
    return settings.contains(key);
}

// Checks messaging settings and sets defaults
void SystemServicesSettings::checkSettings() const
{
    assert(settings.contains(SETTING_DOMAIN()));
    assert(settings.contains(SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN()));
    assert(settings.contains(SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID()));
    assert(settings.contains(SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN()));
    assert(settings.contains(SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID()));
}

void SystemServicesSettings::printSettings() const
{
    JOYNR_LOG_DEBUG(logger) << "SETTING: " << SETTING_DOMAIN() << " = "
                            << settings.get<std::string>(SETTING_DOMAIN());
    JOYNR_LOG_DEBUG(logger) << "SETTING: " << SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN()
                            << " = "
                            << settings.get<std::string>(
                                       SETTING_CC_ROUTINGPROVIDER_AUTHENTICATIONTOKEN());
    JOYNR_LOG_DEBUG(logger) << "SETTING: " << SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID() << "  = "
                            << settings.get<std::string>(
                                       SETTING_CC_ROUTINGPROVIDER_PARTICIPANTID());
    JOYNR_LOG_DEBUG(logger) << "SETTING: " << SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN()
                            << "  = "
                            << settings.get<std::string>(
                                       SETTING_CC_DISCOVERYPROVIDER_AUTHENTICATIONTOKEN());
    JOYNR_LOG_DEBUG(logger) << "SETTING: " << SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID() << " = "
                            << settings.get<std::string>(
                                       SETTING_CC_DISCOVERYPROVIDER_PARTICIPANTID());
}

} // namespace joynr
