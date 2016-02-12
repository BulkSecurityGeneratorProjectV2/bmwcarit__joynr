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
#include "joynr/BrokerUrl.h"
#include <boost/algorithm/string/predicate.hpp>

namespace joynr
{

const std::string& BrokerUrl::URL_PATH_SEPARATOR()
{
    static const std::string value("/");
    return value;
}

const std::string& BrokerUrl::CREATE_CHANNEL_QUERY_ITEM()
{
    static const std::string value("ccid");
    return value;
}

const std::string& BrokerUrl::SEND_MESSAGE_PATH_APPENDIX()
{
    static const std::string value("message");
    return value;
}

const std::string& BrokerUrl::CHANNEL_PATH_SUFFIX()
{
    static const std::string value("channels");
    return value;
}

const std::string& BrokerUrl::TIMECHECK_PATH_SUFFIX()
{
    static const std::string value("time");
    return value;
}

BrokerUrl::BrokerUrl(const std::string& brokerBaseUrl)
        : brokerBaseUrl(brokerBaseUrl), brokerChannelsBaseUrl()
{
    std::string channelsBaseUrl = brokerBaseUrl;
    channelsBaseUrl.append(CHANNEL_PATH_SUFFIX());
    channelsBaseUrl.append(URL_PATH_SEPARATOR());
    this->brokerChannelsBaseUrl = Url(channelsBaseUrl);
}

BrokerUrl& BrokerUrl::operator=(const BrokerUrl& brokerUrl)
{
    brokerBaseUrl = brokerUrl.brokerBaseUrl;
    brokerChannelsBaseUrl = brokerUrl.brokerChannelsBaseUrl;
    return *this;
}

bool BrokerUrl::operator==(const BrokerUrl& brokerUrl) const
{
    return brokerChannelsBaseUrl == brokerUrl.getBrokerChannelsBaseUrl();
}

Url BrokerUrl::getCreateChannelUrl(const std::string& mcid) const
{
    Url createChannelUrl(brokerChannelsBaseUrl);
    UrlQuery query;
    query.addQueryItem(CREATE_CHANNEL_QUERY_ITEM(), mcid);
    createChannelUrl.setQuery(query);

    return createChannelUrl;
}

Url BrokerUrl::getSendUrl(const std::string& channelId) const
{
    Url sendUrl(brokerChannelsBaseUrl);
    std::string path = sendUrl.getPath();
    path.append(channelId);
    path.append(URL_PATH_SEPARATOR());
    path.append(SEND_MESSAGE_PATH_APPENDIX());
    path.append(URL_PATH_SEPARATOR());
    sendUrl.setPath(path);
    return sendUrl;
}

Url BrokerUrl::getBrokerChannelsBaseUrl() const
{
    Url sendUrl(brokerChannelsBaseUrl);
    return sendUrl;
}

Url BrokerUrl::getDeleteChannelUrl(const std::string& mcid) const
{
    Url sendUrl(brokerChannelsBaseUrl);
    std::string path = sendUrl.getPath();
    path.append(mcid);
    path.append(URL_PATH_SEPARATOR());
    sendUrl.setPath(path);
    return sendUrl;
}

Url BrokerUrl::getTimeCheckUrl() const
{
    Url timeCheckUrl(brokerBaseUrl);
    std::string path = timeCheckUrl.getPath();
    path.append(TIMECHECK_PATH_SUFFIX());
    path.append(URL_PATH_SEPARATOR());
    timeCheckUrl.setPath(path);
    return timeCheckUrl;
}

} // namespace joynr
