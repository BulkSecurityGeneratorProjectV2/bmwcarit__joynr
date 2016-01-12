/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#ifndef WEBSOCKETMESSAGINGSTUBFACTORY_H
#define WEBSOCKETMESSAGINGSTUBFACTORY_H

#include <memory>
#include <mutex>
#include <unordered_map>

#include "joynr/Url.h"
#include "joynr/Logger.h"
#include "joynr/IMiddlewareMessagingStubFactory.h"
#include "joynr/system/RoutingTypes/Address.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"

namespace joynr
{
class IWebSocketSendInterface;

class WebSocketMessagingStubFactory : public IMiddlewareMessagingStubFactory
{

public:
    WebSocketMessagingStubFactory();
    std::shared_ptr<IMessaging> create(
            const joynr::system::RoutingTypes::Address& destAddress) override;
    bool canCreate(const joynr::system::RoutingTypes::Address& destAddress) override;
    void addClient(const joynr::system::RoutingTypes::WebSocketClientAddress* clientAddress,
                   IWebSocketSendInterface* webSocket);
    void removeClient(const joynr::system::RoutingTypes::WebSocketClientAddress& clientAddress);
    void addServer(const joynr::system::RoutingTypes::WebSocketAddress& serverAddress,
                   IWebSocketSendInterface* webSocket);
    void onMessagingStubClosed(const joynr::system::RoutingTypes::Address& address);

    static Url convertWebSocketAddressToUrl(
            const joynr::system::RoutingTypes::WebSocketAddress& address);

private:
    std::unordered_map<joynr::system::RoutingTypes::WebSocketAddress, std::shared_ptr<IMessaging>>
            serverStubMap;
    std::unordered_map<joynr::system::RoutingTypes::WebSocketClientAddress,
                       std::shared_ptr<IMessaging>> clientStubMap;
    std::mutex mutex;

    ADD_LOGGER(WebSocketMessagingStubFactory);
};

} // namespace joynr
#endif // WEBSOCKETMESSAGINGSTUBFACTORY_H
