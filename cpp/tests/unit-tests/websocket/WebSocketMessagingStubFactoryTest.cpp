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
#include <gtest/gtest.h>

#include <QtTest/QSignalSpy>
#include <QtCore/QTimer>
#include <QtWebSockets/QWebSocket>

#include "joynr/IMessaging.h"
#include "joynr/MessagingStubFactory.h"
#include "joynr/system/RoutingTypes/WebSocketAddress.h"
#include "joynr/system/RoutingTypes/WebSocketClientAddress.h"
#include "joynr/system/RoutingTypes/ChannelAddress.h"
#include "joynr/system/RoutingTypes/CommonApiDbusAddress.h"
#include "joynr/system/RoutingTypes/BrowserAddress.h"

#include "libjoynr/websocket/WebSocketMessagingStubFactory.h"
#include "libjoynr/websocket/WebSocketMessagingStub.h"

#include "utils/MockObjects.h"

using namespace ::testing;

namespace joynr {

class WebSocketMessagingStubFactoryTest : public testing::Test {
public:
    WebSocketMessagingStubFactoryTest() :
        webSocketServerAddress(joynr::system::RoutingTypes::WebSocketProtocol::WS, "localhost", 42, "path"),
        webSocketClientAddress("clientId"),
        channelAddress("endPointUrl", "channelId"),
        commonApiDbusAddress("domain", "serviceName", "participantId"),
        browserAddress("windowId")
    {
    }

    virtual void TearDown() {
    }

protected:
    ADD_LOGGER(WebSocketMessagingStubFactoryTest);
    joynr::system::RoutingTypes::WebSocketAddress webSocketServerAddress;
    joynr::system::RoutingTypes::WebSocketClientAddress webSocketClientAddress;
    joynr::system::RoutingTypes::ChannelAddress channelAddress;
    joynr::system::RoutingTypes::CommonApiDbusAddress commonApiDbusAddress;
    joynr::system::RoutingTypes::BrowserAddress browserAddress;
};

INIT_LOGGER(WebSocketMessagingStubFactoryTest);

TEST_F(WebSocketMessagingStubFactoryTest, canCreateWebSocketAddressses) {
    WebSocketMessagingStubFactory factory;

    EXPECT_TRUE(factory.canCreate(webSocketClientAddress));
    EXPECT_TRUE(factory.canCreate(webSocketServerAddress));
}

TEST_F(WebSocketMessagingStubFactoryTest, canOnlyCreateWebSocketAddressses) {
    WebSocketMessagingStubFactory factory;

    EXPECT_FALSE(factory.canCreate(channelAddress));
    EXPECT_FALSE(factory.canCreate(commonApiDbusAddress));
    EXPECT_FALSE(factory.canCreate(browserAddress));
}

TEST_F(WebSocketMessagingStubFactoryTest, createReturnsNullForUnknownClient) {
    WebSocketMessagingStubFactory factory;

    EXPECT_TRUE((factory.create(webSocketClientAddress)).get() == 0);
}

TEST_F(WebSocketMessagingStubFactoryTest, createReturnsMessagingStub) {
    WebSocketMessagingStubFactory factory;
    Settings settings;
    WebSocketSettings wsSettings(settings);
    MockWebSocketClient* clientWebsocket = new MockWebSocketClient(wsSettings);
    QWebSocket* serverWebsocket = new QWebSocket();
    MockQWebSocketSendWrapper* wrapper = new MockQWebSocketSendWrapper(serverWebsocket);

    factory.addClient(joynr::system::RoutingTypes::WebSocketClientAddress(webSocketClientAddress), clientWebsocket);
    factory.addServer(webSocketServerAddress, wrapper);
    EXPECT_TRUE(factory.create(webSocketClientAddress).get() != nullptr);
    EXPECT_TRUE(factory.create(webSocketServerAddress).get() != nullptr);
}

TEST_F(WebSocketMessagingStubFactoryTest, closedMessagingStubsAreRemovedFromWebSocketMessagingStubFactory) {
    WebSocketMessagingStubFactory factory;
    Settings settings;
    WebSocketSettings wsSettings(settings);
    auto addressCopy = joynr::system::RoutingTypes::WebSocketClientAddress(webSocketClientAddress);

    MockWebSocketClient* websocket = new MockWebSocketClient(wsSettings);
    websocket->registerDisconnectCallback([&factory,&addressCopy](){ factory.onMessagingStubClosed(addressCopy); });

    factory.addClient(webSocketClientAddress, websocket);

    EXPECT_TRUE(factory.canCreate(webSocketClientAddress));
    std::shared_ptr<IMessaging> messagingStub(factory.create(webSocketClientAddress));
    EXPECT_TRUE(messagingStub.get() != nullptr);

    EXPECT_CALL(*websocket, dtorCalled());
    std::thread(&MockWebSocketClient::signalDisconnect, websocket).detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE((factory.create(webSocketClientAddress)).get() == nullptr);
}

TEST_F(WebSocketMessagingStubFactoryTest, closedMessagingStubsAreRemovedFromMessagingStubFactory) {
    auto factory = std::make_shared<WebSocketMessagingStubFactory>();
    auto address = std::make_shared<system::RoutingTypes::WebSocketClientAddress>(webSocketClientAddress);
    auto addressCopy = std::make_shared<system::RoutingTypes::WebSocketClientAddress>(webSocketClientAddress);
    Settings settings;
    WebSocketSettings wsSettings(settings);
    MockWebSocketClient* websocket = new MockWebSocketClient(wsSettings);
    websocket->registerDisconnectCallback([factory,addressCopy](){ factory->onMessagingStubClosed(*addressCopy); });

    factory->addClient(joynr::system::RoutingTypes::WebSocketClientAddress(*address), websocket);

    auto messagingStubFactory = std::make_shared<MessagingStubFactory>();
    factory->registerOnMessagingStubClosedCallback([messagingStubFactory](
            const std::shared_ptr<const joynr::system::RoutingTypes::Address>& destinationAddress) {
        messagingStubFactory->remove(destinationAddress);
    });
    messagingStubFactory->registerStubFactory(std::move(factory));

    EXPECT_NE(nullptr, messagingStubFactory->create(address).get());
    EXPECT_TRUE(messagingStubFactory->contains(address));
    EXPECT_TRUE(messagingStubFactory->contains(addressCopy));

    EXPECT_CALL(*websocket, dtorCalled());
    std::thread(&MockWebSocketClient::signalDisconnect, websocket).detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_FALSE(messagingStubFactory->contains(address));
    EXPECT_FALSE(messagingStubFactory->contains(addressCopy));
    EXPECT_TRUE(messagingStubFactory->create(address).get() == nullptr);
}

TEST_F(WebSocketMessagingStubFactoryTest, removeClientRemovesMessagingStub) {
    WebSocketMessagingStubFactory factory;
    Settings settings;
    WebSocketSettings wsSettings(settings);
    WebSocketPpClient* websocket = new MockWebSocketClient(wsSettings);

    websocket->registerDisconnectCallback([](){});

    factory.addClient(joynr::system::RoutingTypes::WebSocketClientAddress(webSocketClientAddress), websocket);
    EXPECT_TRUE(factory.create(webSocketClientAddress).get() != nullptr);
    EXPECT_CALL(*static_cast<MockWebSocketClient*>(websocket), dtorCalled());
    factory.removeClient(webSocketClientAddress);
    EXPECT_TRUE((factory.create(webSocketClientAddress)).get() == nullptr);
}

TEST_F(WebSocketMessagingStubFactoryTest, convertWebSocketAddressToUrl) {
    joynr::system::RoutingTypes::WebSocketAddress wsAddress(
                joynr::system::RoutingTypes::WebSocketProtocol::WS,
                "localhost",
                42,
                "/some/path/"
    );
    Url expectedWsUrl("ws://localhost:42/some/path/");

    Url wsUrl(WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(wsAddress));
    EXPECT_EQ(expectedWsUrl, wsUrl);

    joynr::system::RoutingTypes::WebSocketAddress wssAddress(
                joynr::system::RoutingTypes::WebSocketProtocol::WSS,
                "localhost",
                42,
                "/some/path"
    );
    Url expectedWssUrl("wss://localhost:42/some/path");

    Url wssUrl(WebSocketMessagingStubFactory::convertWebSocketAddressToUrl(wssAddress));
    EXPECT_EQ(expectedWssUrl, wssUrl);
}

} // namespace joynr
