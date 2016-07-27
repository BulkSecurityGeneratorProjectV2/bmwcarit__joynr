/*jslint es5: true */
/*global fail: true */

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
            "global/Promise",
            "joynr/messaging/channel/ChannelMessagingSender",
            "joynr/messaging/JoynrMessage",
            "joynr/system/RoutingTypes/ChannelAddress",
            "joynr/util/Typing",
            "joynr/system/LoggerFactory",
            "joynr/provisioning/provisioning_root",
            "global/WaitsFor"
        ],
        function(Promise, ChannelMessagingSender, JoynrMessage, ChannelAddress, Typing, LoggerFactory, provisioning, waitsFor) {

            var log = LoggerFactory.getLogger("joynr.messaging.TestChannelMessagingSender");

            describe(
                    "libjoynr-js.joynr.messaging.ChannelMessagingSender",
                    function() {
                        var communicationModuleSpy, channelMessageSender;
                        var channelAddress, channelUrlInformation, joynrMessage;
                        var resendDelay_ms;

                        function outputPromiseError(error) {
                            expect(error.toString()).toBeFalsy();
                        }

                        beforeEach(function(done) {
                            resendDelay_ms = 500;
                            channelAddress = new ChannelAddress({
                                messagingEndpointUrl : "http://testurl.com",
                                channelId : "myChannel" + Date.now()
                            });
                            var channelQos = {
                                resendDelay_ms : resendDelay_ms
                            };

                            joynrMessage = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
                            });
                            joynrMessage.setHeader(
                                    JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE,
                                    9360686108031);
                            joynrMessage.setHeader(
                                    JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID,
                                    "me");
                            joynrMessage.payload = "hello";

                            // instantiate spies
                            communicationModuleSpy =
                                    jasmine.createSpyObj(
                                            "communicationModule",
                                            [ "createXMLHTTPRequest"
                                            ]);

                            channelMessageSender = new ChannelMessagingSender({
                                communicationModule : communicationModuleSpy,
                                channelQos : channelQos
                            });
                            done();
                        });

                        it("is instantiable and has all members", function(done) {
                            expect(ChannelMessagingSender).toBeDefined();
                            expect(typeof ChannelMessagingSender === "function").toBeTruthy();
                            expect(channelMessageSender).toBeDefined();
                            expect(channelMessageSender instanceof ChannelMessagingSender)
                                    .toBeTruthy();
                            expect(channelMessageSender.send).toBeDefined();
                            expect(typeof channelMessageSender.send === "function").toBeTruthy();
                            done();
                        });

                        it(
                                "if communicationModule.createXMLHTTPRequest call fails, channelMessageSender only fails if message expires",
                                function(done) {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);
                                    var timeStamp;
                                    var relativeExpiryDate = resendDelay_ms * 3;

                                    communicationModuleSpy.createXMLHTTPRequest.and.returnValue(Promise.reject({
                                                status : 500,
                                                responseText : "responseText",
                                                statusText : "errorThrown"
                                            }, "errorStatus"));
                                    joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE] = Date.now() + relativeExpiryDate;
                                    channelMessageSender.start();
                                    timeStamp = Date.now();
                                    channelMessageSender.send(joynrMessage, channelAddress).then(
                                            spy.onFulfilled).catch(spy.onRejected);

                                    waitsFor(function() {
                                        return spy.onRejected.calls.count() > 0;
                                    }, "message send to fail", relativeExpiryDate * 5).then(function() {
                                        expect(spy.onFulfilled).not.toHaveBeenCalled();
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(Date.now()-timeStamp>relativeExpiryDate).toEqual(true);
                                        expect(
                                                Object.prototype.toString
                                                        .call(spy.onRejected.calls.mostRecent().args[0]) === "[object Error]")
                                                .toBeTruthy();
                                        expect(communicationModuleSpy.createXMLHTTPRequest)
                                                .toHaveBeenCalled();
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "if channelMessageSender.send fails after expiryDate, if ChannelMessagingSender is not started",
                                function(done) {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);
                                    var relativeExpiryDate = resendDelay_ms;

                                    joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE] = Date.now() + relativeExpiryDate;
                                    channelMessageSender.send(joynrMessage, channelAddress).then(
                                        spy.onFulfilled).catch(spy.onRejected);

                                    waitsFor(function() {
                                        return spy.onFulfilled.calls.count() > 0 || spy.onRejected.calls.count() > 0;
                                    }, "message send to fail", relativeExpiryDate * 5).then(function() {
                                        expect(spy.onFulfilled).not.toHaveBeenCalled();
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE] < Date.now()).toEqual(true);
                                        expect(
                                                Object.prototype.toString
                                                        .call(spy.onRejected.calls.mostRecent().args[0]) === "[object Error]")
                                                .toBeTruthy();

                                        spy.onRejected.calls.reset();
                                        expect(communicationModuleSpy.createXMLHTTPRequest).not.toHaveBeenCalled();
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "sends message using communicationModule.createXMLHTTPRequest",
                                function(done) {
                                    communicationModuleSpy.createXMLHTTPRequest.and.returnValue(Promise.resolve());
                                    channelMessageSender.start();
                                    channelMessageSender.send(joynrMessage, channelAddress).then(function(result) {
                                        expect(result).toEqual(undefined);
                                        expect(communicationModuleSpy.createXMLHTTPRequest)
                                                .toHaveBeenCalled();
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                    });

        });
