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
#include "joynr/Util.h"

#include <fstream>
#include <regex>
#include <cctype>
#include <iterator>
#include <stdexcept>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "joynr/Logger.h"

namespace joynr
{

namespace util
{

void saveStringToFile(const std::string& fileName, const std::string& strToSave)
{
    std::fstream file;
    file.open(fileName, std::ios::out);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file " + fileName + " for writing.");
    }

    // save input string to file
    file << strToSave;
}

std::string loadStringFromFile(const std::string& fileName)
{
    // read from file
    std::ifstream inStream(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!inStream.is_open()) {
        throw std::runtime_error("Could not open file " + fileName + " for reading.");
    }

    std::string fileContents;
    inStream.seekg(0, std::ios::end);
    fileContents.resize(inStream.tellg());
    inStream.seekg(0, std::ios::beg);
    inStream.read(&fileContents[0], fileContents.size());
    inStream.close();
    return fileContents;
}

std::vector<std::string> splitIntoJsonObjects(const std::string& jsonStream)
{
    // This code relies assumes jsonStream is a valid JSON string
    std::vector<std::string> jsonObjects;
    int parenthesisCount = 0;
    int currentObjectStart = -1;
    bool isInsideString = false;
    /*A string starts with an unescaped " and ends with an unescaped "
     * } or { within a string must be ignored.
    */
    for (std::size_t i = 0; i < jsonStream.size(); i++) {
        if (jsonStream.at(i) == '"' && (i > 0) && jsonStream.at(i - 1) != '\\') {
            // only switch insideString if " is not escaped
            isInsideString = !isInsideString;
        } else if (!isInsideString && jsonStream.at(i) == '{') {
            parenthesisCount++;
        } else if (!isInsideString && jsonStream.at(i) == '}') {
            parenthesisCount--;
        }

        if (parenthesisCount == 1 && currentObjectStart < 0) {
            // found start of object
            currentObjectStart = i;
        }
        if (parenthesisCount == 0 && currentObjectStart >= 0) {
            // found end of object
            jsonObjects.push_back(
                    jsonStream.substr(currentObjectStart, i - currentObjectStart + 1));

            currentObjectStart = -1;
        }
    }
    return jsonObjects;
}

std::string attributeGetterFromName(const std::string& attributeName)
{
    std::string result = attributeName;
    result[0] = std::toupper(result[0]);
    result.insert(0, "get");
    return result;
}

std::string createUuid()
{
    // instantiation of random generator is expensive,
    // therefore we use a static one:
    static boost::uuids::random_generator uuidGenerator;
    // uuid generator is not threadsafe
    static std::mutex uuidMutex;
    std::lock_guard<std::mutex> uuidLock(uuidMutex);
    return boost::uuids::to_string(uuidGenerator());
}

void logSerializedMessage(Logger& logger,
                          const std::string& explanation,
                          const std::string& message)
{
    if (message.size() > 2048) {
        JOYNR_LOG_DEBUG(logger,
                        "{} {}<**truncated, length {}",
                        explanation,
                        message.substr(0, 2048),
                        message.length());
    } else {
        JOYNR_LOG_DEBUG(logger, "{} {}, length {}", explanation, message, message.length());
    }
}

std::string toDateString(const std::chrono::system_clock::time_point& timePoint)
{
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    return std::ctime(&time);
}

std::uint64_t toMilliseconds(const std::chrono::system_clock::time_point& timePoint)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch())
            .count();
}

} // namespace util

} // namespace joynr
