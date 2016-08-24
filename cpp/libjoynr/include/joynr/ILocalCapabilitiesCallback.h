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
#ifndef ILOCALCAPABILITIESCALLBACK_H
#define ILOCALCAPABILITIESCALLBACK_H

#include "joynr/JoynrExport.h"
#include "joynr/CapabilityEntry.h"
#include <vector>

namespace joynr
{

class JOYNR_EXPORT ILocalCapabilitiesCallback
{
public:
    virtual ~ILocalCapabilitiesCallback() = default;

    virtual void capabilitiesReceived(const std::vector<CapabilityEntry>& capabilities) = 0;
    virtual void onError(const joynr::exceptions::JoynrRuntimeException&) = 0;
};

} // namespace joynr
#endif // ILOCALCAPABILITIESCALLBACK_H
