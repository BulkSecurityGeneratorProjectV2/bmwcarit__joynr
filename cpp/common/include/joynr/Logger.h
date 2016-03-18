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
#ifndef LOGGER_H
#define LOGGER_H

#include <memory>
#include <string>
#include <boost/type_index.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <spdlog/spdlog.h>

namespace joynr
{
enum class LogLevel { Trace, Debug, Info, Warn, Error, Fatal };
} // namespace joynr

#ifdef JOYNR_MAX_LOG_LEVEL_FATAL
#define JOYNR_LOG_LEVEL joynr::LogLevel::Fatal
#endif // JOYNR_MAX_LOG_LEVEL_FATAL

#ifdef JOYNR_MAX_LOG_LEVEL_ERROR
#define JOYNR_LOG_LEVEL joynr::LogLevel::Error
#endif // JOYNR_MAX_LOG_LEVEL_ERROR

#ifdef JOYNR_MAX_LOG_LEVEL_WARN
#define JOYNR_LOG_LEVEL joynr::LogLevel::Warn
#endif // JOYNR_MAX_LOG_LEVEL_WARN

#ifdef JOYNR_MAX_LOG_LEVEL_INFO
#define JOYNR_LOG_LEVEL joynr::LogLevel::Info
#endif // JOYNR_MAX_LOG_LEVEL_INFO

#ifdef JOYNR_MAX_LOG_LEVEL_DEBUG
#define JOYNR_LOG_LEVEL joynr::LogLevel::Debug
#endif // JOYNR_MAX_LOG_LEVEL_DEBUG

// default to Trace if no log level is set
#ifndef JOYNR_LOG_LEVEL
#define JOYNR_LOG_LEVEL joynr::LogLevel::Trace
#endif

#define JOYNR_CONDITIONAL_SPDLOG(level, method, logger, ...)                                       \
    do {                                                                                           \
        joynr::LogLevel logLevel = level;                                                          \
        if (JOYNR_LOG_LEVEL <= logLevel) {                                                         \
            logger.spdlog->method(__VA_ARGS__);                                                    \
        }                                                                                          \
    } while (0)

#define JOYNR_LOG_TRACE(logger, ...)                                                               \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Trace, trace, logger, __VA_ARGS__)

#define JOYNR_LOG_DEBUG(logger, ...)                                                               \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Debug, debug, logger, __VA_ARGS__)

#define JOYNR_LOG_INFO(logger, ...)                                                                \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Info, info, logger, __VA_ARGS__)

#define JOYNR_LOG_WARN(logger, ...)                                                                \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Warn, warn, logger, __VA_ARGS__)

#define JOYNR_LOG_ERROR(logger, ...)                                                               \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Error, error, logger, __VA_ARGS__)

#define JOYNR_LOG_FATAL(logger, ...)                                                               \
    JOYNR_CONDITIONAL_SPDLOG(joynr::LogLevel::Fatal, emerg, logger, __VA_ARGS__)

#define ADD_LOGGER(T) static joynr::Logger logger
// this macro allows to pass typenames containing commas (i.e. templates) as a single argument to
// another macro
#define SINGLE_MACRO_ARG(...) __VA_ARGS__
#define INIT_LOGGER(T) joynr::Logger T::logger(joynr::Logger::getPrefix<T>())

namespace joynr
{

struct Logger
{
    Logger(const std::string& prefix) : spdlog(spdlog::stdout_logger_mt(prefix))
    {
        spdlog->set_level(spdlog::level::trace);
        spdlog->set_pattern("%Y-%m-%d %H:%M:%S.%e [thread ID:%t] [%l] %n %v");
    }

    template <typename Parent>
    static std::string getPrefix()
    {
        std::string prefix = boost::typeindex::type_id<Parent>().pretty_name();
        boost::algorithm::erase_all(prefix, "joynr::");
        return prefix;
    }

    std::shared_ptr<spdlog::logger> spdlog;
};

} // namespace joynr
#endif // LOGGER_H
