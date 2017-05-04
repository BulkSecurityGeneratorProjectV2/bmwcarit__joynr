/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "joynr/SubscriptionQos.h"
#include <chrono>
#include <limits>

namespace joynr
{

const std::int64_t& SubscriptionQos::NO_EXPIRY_DATE()
{
    static std::int64_t noExpiryDate = 0;
    return noExpiryDate;
}

SubscriptionQos::SubscriptionQos() : expiryDateMs(NO_EXPIRY_DATE())
{
}

SubscriptionQos::SubscriptionQos(const int64_t validityMs) : expiryDateMs(NO_EXPIRY_DATE())
{
    setValidityMs(validityMs);
}

std::int64_t SubscriptionQos::getExpiryDateMs() const
{
    return expiryDateMs;
}

void SubscriptionQos::setExpiryDateMs(const std::int64_t& expiryDateMs)
{
    this->expiryDateMs = expiryDateMs;
}

void SubscriptionQos::clearExpiryDate()
{
    this->expiryDateMs = NO_EXPIRY_DATE();
}

void SubscriptionQos::setValidityMs(const std::int64_t& validityMs)
{
    if (validityMs == -1) {
        setExpiryDateMs(SubscriptionQos::NO_EXPIRY_DATE());
    } else {
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        setExpiryDateMs(now + validityMs);
    }
}

SubscriptionQos& SubscriptionQos::operator=(const SubscriptionQos& subscriptionQos)
{
    expiryDateMs = subscriptionQos.getExpiryDateMs();
    return *this;
}

bool SubscriptionQos::operator==(const SubscriptionQos& subscriptionQos) const
{
    return getExpiryDateMs() == subscriptionQos.getExpiryDateMs();
}

} // namespace joynr
