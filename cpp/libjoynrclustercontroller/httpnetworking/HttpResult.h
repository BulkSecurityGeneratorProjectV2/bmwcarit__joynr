/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#ifndef HTTPRESULT_H
#define HTTPRESULT_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "joynr/JoynrClusterControllerExport.h"

namespace joynr
{

/**
  * Encapsulates the result of an http request. Stores the body, the returned headers and the status
 * code.
  * In case of a network error the status code contains the CURL error code instead.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT HttpResult
{
public:
    HttpResult(std::int32_t curlError,
               std::int64_t statusCode,
               std::string* body,
               std::unordered_multimap<std::string, std::string>* headers);
    ~HttpResult() = default;

    bool isCurlError() const;
    std::int32_t getCurlError() const;
    std::int64_t getStatusCode() const;
    std::string getErrorMessage() const;
    const std::string& getBody() const;
    const std::unordered_multimap<std::string, std::string>& getHeaders() const;

private:
    std::int32_t curlError;
    std::int64_t statusCode;
    std::shared_ptr<std::string> body;
    std::shared_ptr<std::unordered_multimap<std::string, std::string>> headers;
};

} // namespace joynr
#endif // HTTPRESULT_H
