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
#ifndef THREADPOOLDELAYEDSCHEDULER_H_
#define THREADPOOLDELAYEDSCHEDULER_H_

#include "joynr/JoynrCommonExport.h"
#include "joynr/PrivateCopyAssign.h"

#include "joynr/DelayedScheduler.h"
#include "joynr/ThreadPool.h"

#include <chrono>

namespace joynr
{

/**
 * @class ThreadPoolDelayedScheduler
 * @brief An implementation of the @ref DelayedScheduler using multiple threads
 *      to execute scheduled @ref Runnable
 */
class JOYNRCOMMON_EXPORT ThreadPoolDelayedScheduler : public joynr::DelayedScheduler
{

public:
    /**
     * @brief Constructor
     * @param numberOfThreads Number of threads to be allocated and available
     * @param name Name of the threads to be used for debugging reasons
     * @param defaultDelayMs Default delay for work without delay
     */
    ThreadPoolDelayedScheduler(
            uint8_t numberOfThreads,
            const std::string& name,
            std::chrono::milliseconds defaultDelayMs = std::chrono::milliseconds::zero());

    /**
     * @brief Destructor
     * @note @ref shutdown must be called before destroying this object
     */
    ~ThreadPoolDelayedScheduler() override;

    /**
     * @brief Does an ordinary shutdown of @ref SingleThreadedDelayedScheduler
     *      and its parent @ref DelayedScheduler and child @ref Thread
     * @note Must be called before destructor is called
     */
    void shutdown() override;

private:
    /*! Disallow copy and assign */
    DISALLOW_COPY_AND_ASSIGN(ThreadPoolDelayedScheduler);

private:
    /*! A collection of threads to do work */
    ThreadPool threadPool;
};

} // namespace joynr

#endif // THREADPOOLDELAYEDSCHEDULER_H_
