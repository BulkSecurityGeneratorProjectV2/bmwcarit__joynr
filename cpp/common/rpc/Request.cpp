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
#include "joynr/Request.h"
#include "joynr/Util.h"

namespace joynr
{

bool isRequestTypeRegistered = Variant::registerType<Request>("joynr.Request");

Request::Request() : OneWayRequest(), requestReplyId(util::createUuid())
{
}

bool Request::operator==(const Request& other) const
{
    return getRequestReplyId() == other.getRequestReplyId() && OneWayRequest::operator==(other);
}

const std::string& Request::getRequestReplyId() const
{
    return requestReplyId;
}

void Request::setRequestReplyId(std::string requestReplyId)
{
    this->requestReplyId = std::move(requestReplyId);
}

} // namespace joynr
