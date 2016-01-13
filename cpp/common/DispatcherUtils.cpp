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
#include "joynr/DispatcherUtils.h"

#include <limits>
#include <cassert>
#include <sstream>

namespace joynr
{

// Dispatcher Utils
INIT_LOGGER(DispatcherUtils);

JoynrTimePoint DispatcherUtils::convertTtlToAbsoluteTime(std::int64_t ttl_ms)
{
    JoynrTimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
    JoynrTimePoint expiryDate = now + std::chrono::duration<long long>(ttl_ms);

    // check for overflow
    if (ttl_ms > 0) {
        bool positiveOverflow = expiryDate < now;
        if (positiveOverflow) {
            return getMaxAbsoluteTime();
        }
    } else if (ttl_ms < 0) {
        bool negativeOverflow = expiryDate > now;
        if (negativeOverflow) {
            return std::chrono::time_point_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::time_point::min());
        }
    }

    return expiryDate;
}

JoynrTimePoint DispatcherUtils::getMaxAbsoluteTime()
{
    JoynrTimePoint maxTimePoint = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::time_point::max());
    return maxTimePoint;
}

JoynrTimePoint DispatcherUtils::getMinAbsoluteTime()
{
    JoynrTimePoint maxTimePoint = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::time_point::min());
    return maxTimePoint;
}

std::int64_t DispatcherUtils::convertAbsoluteTimeToTtl(JoynrTimePoint date)
{
    std::int64_t millis =
            std::chrono::duration_cast<std::chrono::milliseconds>(date.time_since_epoch()).count();
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch()).count();
    return millis - now;
}

std::string DispatcherUtils::convertAbsoluteTimeToTtlString(JoynrTimePoint date)
{
    std::int64_t ttlTime = convertAbsoluteTimeToTtl(date);
    return std::to_string(ttlTime);
}

std::string DispatcherUtils::convertAbsoluteTimeToString(JoynrTimePoint date)
{
    std::int64_t ttlTime = convertAbsoluteTimeToTtl(date);
    char buffer[30];
    struct timeval tv;

    time_t curtime;
    struct tm curtimeUtc;

    tv.tv_sec = ttlTime / 1000;
    tv.tv_usec = (ttlTime * 1000) % 1000000;
    curtime = tv.tv_sec;

    strftime(buffer, 30, "%m-%d-%Y  %T.", gmtime_r(&curtime, &curtimeUtc));
    std::stringstream stringStream;
    stringStream << buffer;
    stringStream << tv.tv_usec / 1000;

    return stringStream.str();
}

std::chrono::system_clock::time_point DispatcherUtils::now()
{
    return std::chrono::system_clock::now();
}

std::uint64_t DispatcherUtils::nowInMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                   DispatcherUtils::now().time_since_epoch()).count();
}
} // namespace joynr
