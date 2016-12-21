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

define([
    "joynr/messaging/websocket/WebSocketMessagingSkeleton",
    "joynr/messaging/JoynrMessage",
    "joynr/system/RoutingTypes/WebSocketAddress",
    "joynr/system/RoutingTypes/WebSocketClientAddress",
    "joynr/messaging/websocket/SharedWebSocket"
], function(
        WebSocketMessagingSkeleton,
        JoynrMessage,
        WebSocketAddress,
        WebSocketClientAddress,
        SharedWebSocket) {

    describe("libjoynr-js.joynr.messaging.websocket.WebSocketMessagingSkeleton", function() {

        var window = null;
        var sharedWebSocket = null;
        var webSocketMessagingSkeleton = null;
        var listener = null;
        var data = null;
        var event = null;
        var multicastEvent;
        var ccAddress = new WebSocketAddress({
            protocol : "ws",
            host : "host",
            port : 1234,
            path : "/test"
        });
        var localAddress = new WebSocketClientAddress({
            id : "1234"
        });

        beforeEach(function(done) {
            function Window() {}
            window = new Window();
            window.addEventListener = jasmine.createSpy("addEventListener");

            sharedWebSocket = new SharedWebSocket({
                remoteAddress : ccAddress,
                localAddress : localAddress
            });

            spyOn(sharedWebSocket, "send").and.callThrough();

            webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
                sharedWebSocket : sharedWebSocket,
                mainTransport : true
            });

            listener = jasmine.createSpy("listener");
            function MessageEvent() {}
            event = new MessageEvent();
            data = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            event.data = JSON.stringify(data);
            multicastEvent = new MessageEvent();
            multicastEvent.data = JSON.stringify(new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST
            }));
            done();
        });

        it("is of correct type and has all members", function(done) {
            expect(WebSocketMessagingSkeleton).toBeDefined();
            expect(typeof WebSocketMessagingSkeleton === "function").toBeTruthy();
            expect(webSocketMessagingSkeleton).toBeDefined();
            expect(webSocketMessagingSkeleton instanceof WebSocketMessagingSkeleton).toBeTruthy();
            expect(webSocketMessagingSkeleton.registerListener).toBeDefined();
            expect(typeof webSocketMessagingSkeleton.registerListener === "function").toBeTruthy();
            expect(webSocketMessagingSkeleton.unregisterListener).toBeDefined();
            expect(typeof webSocketMessagingSkeleton.unregisterListener === "function")
                    .toBeTruthy();
            done();
        });

        it("throws if arguments are missing or of wrong type", function(done) {
            expect(function() {
                webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
                    sharedWebSocket : sharedWebSocket,
                    mainTransport : true
                });
            }).not.toThrow(); // correct call
            expect(function() {
                webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
                    sharedWebSocket : sharedWebSocket
                });
            }).toThrow(); // mainTransport missing
            expect(function() {
                webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({});
            }).toThrow(); // incorrect call

            expect(function() {
                webSocketMessagingSkeleton.registerListener(function() {});
            }).not.toThrow(); // correct call
            expect(function() {
                webSocketMessagingSkeleton.registerListener("");
            }).toThrow(); // listener is of wrong type
            expect(function() {
                webSocketMessagingSkeleton.registerListener({});
            }).toThrow(); // listener is of wrong type

            expect(function() {
                webSocketMessagingSkeleton.unregisterListener(function() {});
            }).not.toThrow(); // correct call
            expect(function() {
                webSocketMessagingSkeleton.unregisterListener("");
            }).toThrow(); // listener is of wrong type
            expect(function() {
                webSocketMessagingSkeleton.unregisterListener({});
            }).toThrow(); // listener is of wrong type
            done();
        });

        it("event calls through to registered listener", function() {
            webSocketMessagingSkeleton.registerListener(listener);
            expect(listener).not.toHaveBeenCalled();

            sharedWebSocket.onmessage(event);

            expect(listener).toHaveBeenCalledWith(data);
            expect(listener.calls.count()).toBe(1);
        });

        it("event does not call through to unregistered listener", function() {
            webSocketMessagingSkeleton.registerListener(listener);
            sharedWebSocket.onmessage(event);
            expect(listener).toHaveBeenCalled();
            expect(listener.calls.count()).toBe(1);
            webSocketMessagingSkeleton.unregisterListener(listener);
            sharedWebSocket.onmessage(event);
            expect(listener.calls.count()).toBe(1);
        });

        function receiveMessageAndCheckForIsReceivedFromGlobalFlag(expectedValue) {
            webSocketMessagingSkeleton.registerListener(listener);

            sharedWebSocket.onmessage(multicastEvent);

            expect(listener).toHaveBeenCalled();
            expect(listener.calls.count()).toBe(1);
            expect(listener.calls.argsFor(0)[0].isReceivedFromGlobal).toBe(expectedValue);
        }

        it("sets isReceivedFromGlobal if web socket is main transport", function() {
            receiveMessageAndCheckForIsReceivedFromGlobalFlag(true);
        });

        it("does not set isReceivedFromGlobal if web socket is NOT main transport", function() {
            webSocketMessagingSkeleton = new WebSocketMessagingSkeleton({
                sharedWebSocket : sharedWebSocket,
                mainTransport : false
            });

            receiveMessageAndCheckForIsReceivedFromGlobalFlag(false);
        });

    });
});
