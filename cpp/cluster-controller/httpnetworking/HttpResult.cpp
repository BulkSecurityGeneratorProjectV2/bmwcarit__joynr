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
#include "cluster-controller/httpnetworking/HttpResult.h"
#include <curl/curl.h>
#include "joynr/FormatString.h"
namespace joynr
{

HttpResult::HttpResult(std::int32_t curlError,
                       std::int64_t statusCode,
                       QByteArray* body,
                       QMultiMap<std::string, std::string>* headers)
        : curlError(curlError), statusCode(statusCode), body(body), headers(headers)
{
}

bool HttpResult::isCurlError() const
{
    return (curlError != 0);
}

std::int32_t HttpResult::getCurlError() const
{
    return curlError;
}

std::int64_t HttpResult::getStatusCode() const
{
    return statusCode;
}

std::string HttpResult::getErrorMessage() const
{
    if (isCurlError()) {
        switch (curlError) {
        case CURLE_COULDNT_RESOLVE_PROXY:
            return std::string("Could not resolve network proxy address");
        case CURLE_COULDNT_RESOLVE_HOST:
            return std::string("Could not resolve host address");
        case CURLE_COULDNT_CONNECT:
            return std::string("Curl reported connection failure");
        case CURLE_OPERATION_TIMEDOUT:
            return std::string("Curl operation timeout");
        case CURLE_SSL_CONNECT_ERROR:
            return std::string("SSL connection error");
        default:
            return FormatString("Error during HTTP request/response, curl error code: %1: %2")
                    .arg(curlError)
                    .arg(curl_easy_strerror(static_cast<CURLcode>(curlError)))
                    .str();
        }
    } else {
        switch (statusCode) {
        case 407:
            return std::string("407 Proxy authentication required");
        case 500:
            return std::string("500 Internal server error");
        case 502:
            return std::string("502 Bad gateway");
        case 503:
            return std::string("503 Service unavailable");
        default:
            return FormatString("HTTP error, status code : %1").arg(statusCode).str();
        }
    }
}

const QByteArray& HttpResult::getBody() const
{
    return *body;
}

const QMultiMap<std::string, std::string>& HttpResult::getHeaders() const
{
    return *headers;
}

} // namespace joynr
