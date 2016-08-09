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
#ifndef REQUEST_H
#define REQUEST_H

#include <string>
#include <vector>

#include "joynr/OneWayRequest.h"
#include "joynr/serializer/Serializer.h"
#include "joynr/JoynrCommonExport.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class JOYNRCOMMON_EXPORT Request : public OneWayRequest
{
public:
    Request();

    Request(Request&&) = default;
    Request& operator=(Request&&) = default;

    bool operator==(const Request& other) const;

    const std::string& getRequestReplyId() const;
    void setRequestReplyId(std::string requestReplyId);

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(muesli::BaseClass<OneWayRequest>(this), MUESLI_NVP(requestReplyId));
    }

private:
    DISALLOW_COPY_AND_ASSIGN(Request);
    std::string requestReplyId;
};

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::Request, "joynr.Request")

#endif // REQUEST_H
