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
#include "joynr/JoynrMessage.h"

#include <iostream>
#include <string>
#include <utility>
#include <memory>
#include <chrono>

#include "joynr/Util.h"
#include "joynr/Variant.h"

namespace joynr
{
INIT_LOGGER(JoynrMessage);

static bool isJoynrMessageRegistered = Variant::registerType<JoynrMessage>("joynr.JoynrMessage");

const std::string& JoynrMessage::HEADER_CONTENT_TYPE()
{
    static const std::string headerContentType("contentType");
    return headerContentType;
}

const std::string& JoynrMessage::HEADER_MESSAGE_ID()
{
    static const std::string headerMessageId("msgId");
    return headerMessageId;
}

const std::string& JoynrMessage::HEADER_CREATOR_USER_ID()
{
    static const std::string headerCreatorUserId("creator");
    return headerCreatorUserId;
}

const std::string& JoynrMessage::HEADER_TO()
{
    static const std::string headerTo("to");
    return headerTo;
}
const std::string& JoynrMessage::HEADER_FROM()
{
    static const std::string headerFrom("from");
    return headerFrom;
}
const std::string& JoynrMessage::HEADER_EXPIRY_DATE()
{
    static const std::string headerExpiryDate("expiryDate");
    return headerExpiryDate;
}
const std::string& JoynrMessage::HEADER_REPLY_ADDRESS()
{
    static const std::string headerReplyAddress("replyChannelId");
    return headerReplyAddress;
}
const std::string& JoynrMessage::CUSTOM_HEADER_PREFIX()
{
    static const std::string customHeaderPrefix("custom-");
    return customHeaderPrefix;
}

const std::string JoynrMessage::VALUE_MESSAGE_TYPE_ONE_WAY = "oneWay";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_REPLY = "reply";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST = "request";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_PUBLICATION = "subscriptionPublication";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY = "subscriptionReply";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST = "subscriptionRequest";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST =
        "broadcastSubscriptionRequest";
const std::string JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP = "subscriptionStop";

const std::string JoynrMessage::VALUE_CONTENT_TYPE_TEXT_PLAIN = "text/plain";
const std::string JoynrMessage::VALUE_CONTENT_TYPE_APPLICATION_JSON = "application/json";

JoynrMessage::JoynrMessage() : type(""), header(), payload()
{
    generateAndSetMsgIdHeaderIfAbsent();
}

JoynrMessage::JoynrMessage(const JoynrMessage& message)
        : type(message.type), header(message.header), payload(message.payload)
{
    generateAndSetMsgIdHeaderIfAbsent();
}

JoynrMessage& JoynrMessage::operator=(const JoynrMessage& message)
{
    type = message.type;
    header = message.header;
    payload = message.payload;
    generateAndSetMsgIdHeaderIfAbsent();
    return *this;
}

JoynrMessage::JoynrMessage(JoynrMessage&& message)
        : type(std::move(message.type)),
          header(std::move(message.header)),
          payload(std::move(message.payload))
{
    generateAndSetMsgIdHeaderIfAbsent();
}

JoynrMessage& JoynrMessage::operator=(JoynrMessage&& message)
{
    type = std::move(message.type);
    header = std::move(message.header);
    payload = std::move(message.payload);
    generateAndSetMsgIdHeaderIfAbsent();
    return *this;
}

void JoynrMessage::generateAndSetMsgIdHeaderIfAbsent()
{
    if (!containsHeader(HEADER_MESSAGE_ID())) {
        std::string msgId = util::createUuid();
        setHeaderForKey(HEADER_MESSAGE_ID(), msgId);
    }
}

bool JoynrMessage::operator==(const JoynrMessage& message) const
{
    return type == message.getType() && payload == message.payload && header == message.header;
}

std::string JoynrMessage::getType() const
{
    return type;
}

void JoynrMessage::setType(const std::string& type)
{
    this->type = type;
}

std::map<std::string, std::string> JoynrMessage::getHeader() const
{
    return header;
}

bool JoynrMessage::containsHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator pos = header.find(key);
    if (pos == header.end()) {
        return false;
    }

    return true;
}

void JoynrMessage::setHeader(const std::map<std::string, std::string>& newHeaders)
{
    std::map<std::string, std::string>::const_iterator i = newHeaders.begin();
    while (i != newHeaders.end()) {
        if (!containsHeader(i->first)) {
            header.insert(std::pair<std::string, std::string>(i->first, i->second));
            JOYNR_LOG_DEBUG(logger, "insert header: {} = {}", i->second, i->first);
        } else {
            header[i->first] = i->second;
        }
        i++;
    }
}

std::string JoynrMessage::getHeaderForKey(const std::string& key) const
{
    // to avoid adding default-constructed value to the map, I use find instead of operator[]
    std::map<std::string, std::string>::const_iterator pos = header.find(key);
    if (pos == header.end()) {
        return std::string();
    }
    std::string value = pos->second;
    return value;
}

void JoynrMessage::setHeaderForKey(const std::string& key, const std::string& value)
{
    header[key] = value;
}

bool JoynrMessage::containsCustomHeader(const std::string& key) const
{
    return containsHeader(CUSTOM_HEADER_PREFIX() + key);
}

std::string JoynrMessage::getCustomHeader(const std::string& key) const
{
    return getHeaderForKey(CUSTOM_HEADER_PREFIX() + key);
}

void JoynrMessage::setCustomHeader(const std::string& key, const std::string& value)
{
    setHeaderForKey(CUSTOM_HEADER_PREFIX() + key, value);
}

std::string JoynrMessage::getPayload() const
{
    return payload;
}

void JoynrMessage::setPayload(const std::string& payload)
{
    this->payload = payload;
}

void JoynrMessage::setPayload(std::string&& payload)
{
    this->payload = std::move(payload);
}

bool JoynrMessage::containsHeaderContentType() const
{
    return containsHeader(HEADER_CONTENT_TYPE());
}

std::string JoynrMessage::getHeaderContentType() const
{
    return getHeaderForKey(HEADER_CONTENT_TYPE());
}

void JoynrMessage::setHeaderContentType(const std::string& contentType)
{
    setHeaderForKey(HEADER_CONTENT_TYPE(), contentType);
}

bool JoynrMessage::containsHeaderMessageId() const
{
    return containsHeader(HEADER_MESSAGE_ID());
}

std::string JoynrMessage::getHeaderMessageId() const
{
    return getHeaderForKey(HEADER_MESSAGE_ID());
}

void JoynrMessage::setHeaderMessageId(const std::string& msgId)
{
    setHeaderForKey(HEADER_MESSAGE_ID(), msgId);
}

bool JoynrMessage::containsHeaderCreatorUserId() const
{
    return containsHeader(HEADER_CREATOR_USER_ID());
}

std::string JoynrMessage::getHeaderCreatorUserId() const
{
    return getHeaderForKey(HEADER_CREATOR_USER_ID());
}

void JoynrMessage::setHeaderCreatorUserId(const std::string& creatorUserId)
{
    JOYNR_LOG_TRACE(logger, "########## header creater user id: {}", HEADER_CREATOR_USER_ID());
    setHeaderForKey(HEADER_CREATOR_USER_ID(), creatorUserId);
}

bool JoynrMessage::containsHeaderTo() const
{
    return containsHeader(HEADER_TO());
}

std::string JoynrMessage::getHeaderTo() const
{
    return getHeaderForKey(HEADER_TO());
}

void JoynrMessage::setHeaderTo(const std::string& to)
{
    setHeaderForKey(HEADER_TO(), to);
}

bool JoynrMessage::containsHeaderFrom() const
{
    return containsHeader(HEADER_FROM());
}

std::string JoynrMessage::getHeaderFrom() const
{
    return getHeaderForKey(HEADER_FROM());
}

void JoynrMessage::setHeaderFrom(const std::string& from)
{
    setHeaderForKey(HEADER_FROM(), from);
}

bool JoynrMessage::containsHeaderExpiryDate() const
{
    return containsHeader(HEADER_EXPIRY_DATE());
}

JoynrTimePoint JoynrMessage::getHeaderExpiryDate() const
{
    std::string expiryDateString = getHeaderForKey(HEADER_EXPIRY_DATE());
    JoynrTimePoint expiryDate(std::chrono::milliseconds(std::stoll(expiryDateString)));
    return expiryDate;
}

void JoynrMessage::setHeaderExpiryDate(const JoynrTimePoint& expiryDate)
{
    setHeaderForKey(HEADER_EXPIRY_DATE(), std::to_string(expiryDate.time_since_epoch().count()));
}

bool JoynrMessage::containsHeaderReplyAddress() const
{
    return containsHeader(HEADER_REPLY_ADDRESS());
}

std::string JoynrMessage::getHeaderReplyAddress() const
{
    return getHeaderForKey(HEADER_REPLY_ADDRESS());
}

void JoynrMessage::setHeaderReplyAddress(const std::string& replyAddress)
{
    setHeaderForKey(HEADER_REPLY_ADDRESS(), replyAddress);
}

} // namespace joynr
