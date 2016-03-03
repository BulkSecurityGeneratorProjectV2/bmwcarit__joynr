/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#ifndef ONCHANGESUBSCRIPTIONQOS_H
#define ONCHANGESUBSCRIPTIONQOS_H

#include <cstdint>
#include "joynr/SubscriptionQos.h"
#include "joynr/JoynrCommonExport.h"

namespace joynr
{

/**
 * @brief Class representing the quality of service settings for subscriptions
 * based on changes
 *
 * Class that stores quality of service settings for subscriptions to
 * <b>broadcasts and attributes</b>.
 *
 * Notifications will only be sent if the subscribed value has changed.
 * The subscription will automatically expire after validity ms. If no
 * publications were received for alertAfterInterval, publicationMissed will be
 * called.
 * minInterval can be used to prevent too many messages being sent.
 */
class JOYNRCOMMON_EXPORT OnChangeSubscriptionQos : public SubscriptionQos
{

public:
    /** @brief Default constructor */
    OnChangeSubscriptionQos();

    /**
     * @brief Copy constructor for OnChangeSubscriptionQos object
     * @param other The OnChangeSubscriptionQos object instance to be copied from
     */
    OnChangeSubscriptionQos(const OnChangeSubscriptionQos& other);

    /**
     * @brief Constructor with full parameter set.
     *
     * @param minInterval Minimum interval in milliseconds.
     * @param validityMs Time span in milliseconds during which publications will be sent
     *
     * It is used to prevent flooding. Publications will be sent maintaining
     * this minimum interval provided, even if the value changes more often.
     * This prevents the consumer from being flooded by updated values.
     * The filtering happens on the provider's side, thus also preventing
     * excessive network traffic.
     *
     * @see SubscriptionQos#setValidityMs
     * @see OnChangeSubscriptionQos#setMinInterval
     * @see SubscriptionQos#setPublicationTtlMs
     */
    OnChangeSubscriptionQos(const std::int64_t& validityMs, const std::int64_t& minInterval);

    /**
     * @brief Gets the minimum interval in milliseconds
     *
     * The provider will maintain at least a minimum interval idle time in milliseconds between
     * successive notifications, even if on-change notifications are enabled and the value
     * changes more often. This prevents the consumer from being flooded by updated values.
     * The filtering happens on the provider's side, thus also preventing excessive network
     * traffic.
     *
     * @return Minimum interval in milliseconds
     */
    virtual std::int64_t getMinInterval() const;

    /**
     * @brief Sets minimum interval in milliseconds
     *
     * The provider will maintain at least a minimum interval idle time in milliseconds
     * between successive notifications, even if on-change notifications are enabled
     * and the value changes more often. This prevents the consumer from being flooded
     * by updated values. The filtering happens on the provider's side, thus also
     * preventing excessive network traffic.<br>
     * <br>
     * <b>Minimum and Maximum Values:</b>
     * <ul>
     * <li><b>Minimum</b> minInterval: 50. Smaller values will be rounded up.
     * <li><b>Maximum</b> minInterval: 2.592.000.000 (30 days). Larger values
     * will be rounded down.
     * </ul>
     *
     * @param minInterval Minimum interval in milliseconds
     */
    virtual void setMinInterval(const std::int64_t& minInterval);

    /** @brief Assignment operator */
    OnChangeSubscriptionQos& operator=(const OnChangeSubscriptionQos& other);

    /** @brief Equality operator */
    bool operator==(const OnChangeSubscriptionQos& other) const;

    /**
     * @brief Returns the default value for the minimum interval setting in
     * milliseconds: 1000
     */
    static const std::int64_t& DEFAULT_MIN_INTERVAL();

    /**
     * @brief Returns the minimum value for the minimum interval setting in
     * milliseconds: 50
     */
    static const std::int64_t& MIN_MIN_INTERVAL();

    /**
     * @brief Returns the maximum value for the minimum interval setting in
     * milliseconds: 2 592 000 000 (30 days)
     */
    static const std::int64_t& MAX_MIN_INTERVAL();

protected:
    /**
     * @brief The minimum interval in milliseconds
     *
     * It is used to prevent flooding. Publications will be sent maintaining
     * this minimum interval provided, even if the value changes more often.
     * This prevents the consumer from being flooded by updated values.
     * The filtering happens on the provider's side, thus also preventing
     * excessive network traffic.
     */
    std::int64_t minInterval;
};

} // namespace joynr

#endif // ONCHANGESUBSCRIPTIONQOS_H
