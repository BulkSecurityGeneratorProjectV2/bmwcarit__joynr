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
#ifndef JOYNR_SCHEDULER_H
#define JOYNR_SCHEDULER_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrCommonExport.h"

#include <deque>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <atomic>

namespace joynr
{
namespace joynr_logging
{
class Logger;
} // namespace joynr_logging
class Runnable;

/**
 * @class BlockingQueue
 * @brief A thread safe queue for submitting tasks
 *
 * This class provides a FIFO queue to add and take tasks of type
 * @ref Runnable. In case of an empty queue, calling @ref take will
 * block until a task is available.
 */
class JOYNRCOMMON_EXPORT BlockingQueue
{
public:
    /**
     * @brief Constructor
     */
    BlockingQueue();

    /**
     * @brief Destructor
     * @note Be sure to call @ref shutdown and wait for return before
     *      destroying this object
     */
    virtual ~BlockingQueue();

    /**
     * @brief Submit task to be done
     * @param task Task to be added to the queue
     */
    void add(Runnable* task);

    /**
     * @brief Does an ordinary shutdown of @ref BlockingQueue
     * @note Must be called before destructor is called
     */
    void shutdown();

    /**
     * @brief Take some work
     * @return Work to be done or @c nullptr if scheduler is shutting down
     *
     * @note This method will block until work is available or the scheduler is
     *      going to shutdown. If so, this method will return @c nullptr.
     */
    Runnable* take();

    /**
     * @brief Returns the current size of the queue
     * @return Number of pending @ref Runnable objects
     */
    int getQueueLength() const;

private:
    /*! Not allowed to copy @ref BlockingQueue */
    DISALLOW_COPY_AND_ASSIGN(BlockingQueue);

private:
    /*! Logger */
    static joynr_logging::Logger* logger;

    /*! Flag indicating scheduler is shutting down */
    std::atomic_bool stoppingScheduler;

    /*! Queue of waiting work */
    std::deque<Runnable*> queue;

    /*! Cond to wait for task on calling @ref take */
    std::condition_variable condition;

    /*! Mutual exclusion of the @ref queue and for @ref condition */
    mutable std::mutex conditionMutex;
};
} // namespace joynr
#endif // JOYNR_SCHEDULER_H
