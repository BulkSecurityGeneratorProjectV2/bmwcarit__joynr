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
#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>
#include <mutex>
#include <unordered_map>
#include <memory>

#include <boost/system/error_code.hpp>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/SingleThreadedDelayedScheduler.h"
#include "joynr/Runnable.h"
#include "joynr/ITimeoutListener.h"
#include "joynr/Logger.h"
#include "joynr/IReplyCaller.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/SteadyTimer.h"

namespace boost
{
namespace asio
{
class io_service;
} // namespace asio
} // namespace boost

namespace joynr
{

/**
  * The directory.h offers the interface of a Map. However, in contrast to a Map,
  * one can choose to use two different add methods. The first add/remove behave as expected by a
  *Map.
  * The second option is to specify a time to live for entries when adding them (in milli-secondes).
  *The
  * entry will be removed automatically after this time. The methods are thread-safe.
  *
  * This template can be used on libJoynr and ClusterController sides:
  *     MessagingEndpointDirectory,           CC
  *     ParticipantDirectory,                 CC
  *     \<Middleware\>RequestCallerDirectory, libjoynr
  *     ReplyCallerDirectory,                 libjoynr
  */

template <typename Key, typename T>
class Directory
{
public:
    Directory() = default;

    Directory(const std::string& directoryName, boost::asio::io_service& ioService)
            : callbackMap(), timeoutTimerMap(), mutex(), ioService(ioService)
    {
        std::ignore = directoryName;
    }

    ~Directory()
    {
        JOYNR_LOG_TRACE(logger, "destructor: number of entries = {}", callbackMap.size());
    }

    /*
     * Returns the element with the given keyId. In case the element could not be found nullptr is
     * returned.
     */
    std::shared_ptr<T> lookup(const Key& keyId)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto found = callbackMap.find(keyId);
        if (found == callbackMap.cend()) {
            return nullptr;
        }
        return found->second;
    }

    /*
     * Returns true if an element with the given keyId could be found. False otherwise.
     */
    bool contains(const Key& keyId)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return callbackMap.find(keyId) != callbackMap.cend();
    }

    /*
     * Adds an element and keeps it until actively removed (using the 'remove' method)
     */
    void add(const Key& keyId, std::shared_ptr<T> value)
    {
        std::lock_guard<std::mutex> lock(mutex);
        callbackMap[keyId] = std::move(value);
    }

    /*
     * Adds an element and removes it automatically after ttl_ms milliseconds have past.
     */
    void add(const Key& keyId, std::shared_ptr<T> value, std::int64_t ttl_ms)
    {
        // Insert the value
        {
            std::lock_guard<std::mutex> lock(mutex);

            // An existing entry shall be overwritten by the new entry.
            // When we use unordered_map::emplace, we must remove the
            // existing entry first.
            auto existingTimerIt = timeoutTimerMap.find(keyId);
            if (existingTimerIt != timeoutTimerMap.end()) {
                timeoutTimerMap.erase(existingTimerIt);
            }

            auto insertionResult = timeoutTimerMap.emplace(std::piecewise_construct,
                                                           std::forward_as_tuple(keyId),
                                                           std::forward_as_tuple(ioService));

            assert(insertionResult.second); // Success indication
            auto timerIt = insertionResult.first;

            timerIt->second.expiresFromNow(std::chrono::milliseconds(ttl_ms));
            timerIt->second.asyncWait([keyId, this](const boost::system::error_code& errorCode) {
                if (!errorCode) {
                    this->removeAfterTimeout<T>(keyId);
                } else if (errorCode != boost::system::errc::operation_canceled) {
                    JOYNR_LOG_TRACE(this->logger,
                                    "Timer removal of entry from directory failed : {}",
                                    errorCode.message());
                }
            });

            callbackMap[keyId] = std::move(value);
        }
    }

    /*
     * Remove element with key == keyID
     */
    void remove(const Key& keyId)
    {
        std::lock_guard<std::mutex> lock(mutex);

        callbackMap.erase(keyId);
        timeoutTimerMap.erase(keyId);
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(callbackMap));
    }

private:
    template <typename ValueType, typename KeyType>
    std::enable_if_t<std::is_same<ValueType, IReplyCaller>::value> removeAfterTimeout(
            const KeyType& keyId)
    {
        auto value = lookup(keyId);

        if (value) {
            value->timeOut();
            remove(keyId);
        }
    }

    template <typename ValueType, typename KeyType>
    std::enable_if_t<!std::is_same<ValueType, IReplyCaller>::value> removeAfterTimeout(
            const KeyType& keyId)
    {
        remove(keyId);
    }

protected:
    std::unordered_map<Key, std::shared_ptr<T>> callbackMap;
    std::unordered_map<Key, SteadyTimer> timeoutTimerMap;
    ADD_LOGGER(Directory);

private:
    DISALLOW_COPY_AND_ASSIGN(Directory);
    std::mutex mutex;
    boost::asio::io_service& ioService;
};

template <typename Key, typename T>
INIT_LOGGER(SINGLE_MACRO_ARG(Directory<Key, T>));
} // namespace joynr

#endif // DIRECTORY_H
