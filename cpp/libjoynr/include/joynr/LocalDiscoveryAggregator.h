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
#ifndef LOCALDISCOVERYAGGREGATOR_H
#define LOCALDISCOVERYAGGREGATOR_H

#include <string>
#include <vector>
#include <map>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include "joynr/system/IDiscovery.h"
#include "joynr/types/DiscoveryEntry.h"

namespace joynr
{
class IRequestCallerDirectory;
class SystemServicesSettings;

namespace types
{
class DiscoveryQos;
} // namespace types

/**
 * @brief The LocalDiscoveryAggregator class is a wrapper for discovery proxies. It holds a list
 * of provisioned discovery entries (for example for the discovery and routing provider). If a
 * lookup is performed by using a participant ID, these entries are checked and returned first
 * before the request is forwarded to the wrapped discovery provider.
 */
class JOYNR_EXPORT LocalDiscoveryAggregator : public joynr::system::IDiscoverySync
{
public:
    LocalDiscoveryAggregator(const SystemServicesSettings& systemServicesSettings);

    void setDiscoveryProxy(std::unique_ptr<IDiscoverySync> discoveryProxy);

    // inherited from joynr::system::IDiscoverySync
    void add(const joynr::types::DiscoveryEntry& entry) override;

    // inherited from joynr::system::IDiscoverySync
    void lookup(std::vector<joynr::types::DiscoveryEntry>& result,
                const std::vector<std::string>& domains,
                const std::string& interfaceName,
                const joynr::types::DiscoveryQos& discoveryQos) override;

    // inherited from joynr::system::IDiscoverySync
    void lookup(joynr::types::DiscoveryEntry& result, const std::string& participantId) override;

    // inherited from joynr::system::IDiscoverySync
    void remove(const std::string& participantId) override;

private:
    DISALLOW_COPY_AND_ASSIGN(LocalDiscoveryAggregator);

    std::unique_ptr<joynr::system::IDiscoverySync> discoveryProxy;
    std::map<std::string, joynr::types::DiscoveryEntry> provisionedDiscoveryEntries;
};
} // namespace joynr
#endif // LOCALDISCOVERYAGGREGATOR_H
