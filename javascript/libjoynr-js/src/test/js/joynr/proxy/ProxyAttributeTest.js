/*jslint es5: true, nomen: true */
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
            "joynr/proxy/ProxyAttribute",
            "joynr/proxy/ProxyAttributeNotifyReadWrite",
            "joynr/proxy/ProxyAttributeNotifyRead",
            "joynr/proxy/ProxyAttributeNotifyWrite",
            "joynr/proxy/ProxyAttributeNotify",
            "joynr/proxy/ProxyAttributeReadWrite",
            "joynr/proxy/ProxyAttributeRead",
            "joynr/proxy/ProxyAttributeWrite",
            "joynr/proxy/DiscoveryQos",
            "joynr/messaging/MessagingQos",
            "joynr/proxy/OnChangeWithKeepAliveSubscriptionQos",
            "joynr/dispatching/RequestReplyManager",
            "joynr/dispatching/types/Request",
            "joynr/tests/testTypes/TestEnum",
            "joynr/types/TypeRegistrySingleton",
            "global/Promise"
        ],
        function(
                ProxyAttribute,
                ProxyAttributeNotifyReadWrite,
                ProxyAttributeNotifyRead,
                ProxyAttributeNotifyWrite,
                ProxyAttributeNotify,
                ProxyAttributeReadWrite,
                ProxyAttributeRead,
                ProxyAttributeWrite,
                DiscoveryQos,
                MessagingQos,
                OnChangeWithKeepAliveSubscriptionQos,
                RequestReplyManager,
                Request,
                TestEnum,
                TypeRegistrySingleton,
                Promise) {

            var asyncTimeout = 5000;

            describe(
                    "libjoynr-js.joynr.proxy.ProxyAttribute",
                    function() {
                        var settings;
                        var isOn;
                        var isOnNotifyReadOnly;
                        var isOnNotifyWriteOnly;
                        var isOnNotify;
                        var isOnReadWrite;
                        var isOnReadOnly;
                        var isOnWriteOnly;
                        var isOnProxyAttributeNotifyReadWrite;
                        var isOnProxyAttributeNotifyRead;
                        var isOnProxyAttributeNotifyWrite;
                        var isOnProxyAttributeNotify;
                        var isOnProxyAttributeReadWrite;
                        var isOnProxyAttributeRead;
                        var isOnProxyAttributeWrite;
                        var subscriptionQos;
                        var messagingQos;
                        var requestReplyManagerSpy;
                        var subscriptionId;
                        var subscriptionManagerSpy;
                        var proxyParticipantId;
                        var providerParticipantId;

                        function RadioStation(name, station, source) {
                            if (!(this instanceof RadioStation)) {
                                // in case someone calls constructor without new keyword (e.g. var c
                                // = Constructor({..}))
                                return new RadioStation(name, station, source);
                            }
                            this.name = name;
                            this.station = station;
                            this.source = source;
                            this.checkMembers = jasmine.createSpy("checkMembers");

                            Object.defineProperty(this, "_typeName", {
                                configurable : false,
                                writable : false,
                                enumerable : true,
                                value : "test.RadioStation"
                            });
                        }

                        beforeEach(function(done) {
                            subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos();
                            messagingQos = new MessagingQos();

                            requestReplyManagerSpy =
                                    jasmine.createSpyObj("requestReplyManager", [ "sendRequest"
                                    ]);
                            requestReplyManagerSpy.sendRequest.and.returnValue(Promise.resolve({
                                result : {
                                    resultKey : "resultValue"
                                }
                            }));

                            subscriptionId = {
                                tokenId : "someId",
                                tokenUserData : "some additional data, do not touch!"
                            };
                            subscriptionManagerSpy = jasmine.createSpyObj("subscriptionManager", [
                                "registerSubscription",
                                "unregisterSubscription"
                            ]);
                            subscriptionManagerSpy.registerSubscription.and.returnValue(Promise.resolve(subscriptionId));
                            subscriptionManagerSpy.unregisterSubscription.and.returnValue(Promise.resolve(subscriptionId));

                            settings = {
                                dependencies : {
                                    requestReplyManager : requestReplyManagerSpy,
                                    subscriptionManager : subscriptionManagerSpy
                                }
                            };

                            proxyParticipantId = "proxyParticipantId";
                            providerParticipantId = "providerParticipantId";
                            var proxy = {
                                proxyParticipantId : proxyParticipantId,
                                providerParticipantId : providerParticipantId
                            };

                            isOn =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOn",
                                            "Boolean",
                                            "NOTIFYREADWRITE");
                            isOnNotifyReadOnly =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnNotifyReadOnly",
                                            "Boolean",
                                            "NOTIFYREADONLY");
                            isOnNotifyWriteOnly =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnNotifyWriteOnly",
                                            "Boolean",
                                            "NOTIFYWRITEONLY");
                            isOnNotify =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnNotify",
                                            "Boolean",
                                            "NOTIFY");
                            isOnReadWrite =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnReadWrite",
                                            "Boolean",
                                            "READWRITE");
                            isOnReadOnly =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnReadOnly",
                                            "Boolean",
                                            "READONLY");
                            isOnWriteOnly =
                                    new ProxyAttribute(
                                            proxy,
                                            settings,
                                            "isOnWriteOnly",
                                            "Boolean",
                                            "WRITEONLY");

                            isOnProxyAttributeNotifyReadWrite =
                                    new ProxyAttributeNotifyReadWrite(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeNotifyReadWrite",
                                            "Boolean");
                            isOnProxyAttributeNotifyRead =
                                    new ProxyAttributeNotifyRead(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeNotifyRead",
                                            "Boolean");
                            isOnProxyAttributeNotifyWrite =
                                    new ProxyAttributeNotifyWrite(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeNotifyWrite",
                                            "Boolean");
                            isOnProxyAttributeNotify =
                                    new ProxyAttributeNotify(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeNotify",
                                            "Boolean");
                            isOnProxyAttributeReadWrite =
                                    new ProxyAttributeReadWrite(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeReadWrite",
                                            "Boolean");
                            isOnProxyAttributeRead =
                                    new ProxyAttributeRead(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeRead",
                                            "Boolean");
                            isOnProxyAttributeWrite =
                                    new ProxyAttributeWrite(
                                            proxy,
                                            settings,
                                            "isOnProxyAttributeWrite",
                                            "Boolean");
                            TypeRegistrySingleton.getInstance().getTypeRegisteredPromise("joynr.tests.testTypes.TestEnum", 1000).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("is of correct type (ProxyAttribute)", function(done) {
                            expect(isOn).toBeDefined();
                            expect(isOn).not.toBeNull();
                            expect(typeof isOn === "object").toBeTruthy();
                            expect(isOn instanceof ProxyAttribute).toBeTruthy();
                            done();
                        });

                        it("has correct members (ProxyAttribute with NOTIFYREADWRITE)", function(done) {
                            expect(isOn.get).toBeDefined();
                            expect(isOn.set).toBeDefined();
                            expect(isOn.subscribe).toBeDefined();
                            expect(isOn.unsubscribe).toBeDefined();
                            done();
                        });

                        it("has correct members (ProxyAttribute with NOTIFYREADONLY)", function(done) {
                            expect(isOnNotifyReadOnly.get).toBeDefined();
                            expect(isOnNotifyReadOnly.set).toBeUndefined();
                            expect(isOnNotifyReadOnly.subscribe).toBeDefined();
                            expect(isOnNotifyReadOnly.unsubscribe).toBeDefined();
                            done();
                        });

                        it("has correct members (ProxyAttribute with NOTIFYWRITEONLY)", function(done) {
                            expect(isOnNotifyWriteOnly.get).toBeUndefined();
                            expect(isOnNotifyWriteOnly.set).toBeDefined();
                            expect(isOnNotifyWriteOnly.subscribe).toBeDefined();
                            expect(isOnNotifyWriteOnly.unsubscribe).toBeDefined();
                            done();
                        });

                        it("has correct members (ProxyAttribute with NOTIFY)", function(done) {
                            expect(isOnNotify.get).toBeUndefined();
                            expect(isOnNotify.set).toBeUndefined();
                            expect(isOnNotify.subscribe).toBeDefined();
                            expect(isOnNotify.unsubscribe).toBeDefined();
                            done();
                        });

                        it("has correct members (ProxyAttribute with READWRITE)", function(done) {
                            expect(isOnReadWrite.get).toBeDefined();
                            expect(isOnReadWrite.set).toBeDefined();
                            expect(isOnReadWrite.subscribe).toBeUndefined();
                            expect(isOnReadWrite.unsubscribe).toBeUndefined();
                            done();
                        });

                        it("has correct members (ProxyAttribute with READONLY)", function(done) {
                            expect(isOnReadOnly.get).toBeDefined();
                            expect(isOnReadOnly.set).toBeUndefined();
                            expect(isOnReadOnly.subscribe).toBeUndefined();
                            expect(isOnReadOnly.unsubscribe).toBeUndefined();
                            done();
                        });

                        it("has correct members (ProxyAttribute with WRITEONLY)", function(done) {
                            expect(isOnWriteOnly.get).toBeUndefined();
                            expect(isOnWriteOnly.set).toBeDefined();
                            expect(isOnWriteOnly.subscribe).toBeUndefined();
                            expect(isOnWriteOnly.unsubscribe).toBeUndefined();
                            done();
                        });

                        it("has correct members (ProxyAttributeNotifyReadWrite)", function(done) {
                            expect(isOnProxyAttributeNotifyReadWrite.get).toBeDefined();
                            expect(isOnProxyAttributeNotifyReadWrite.set).toBeDefined();
                            expect(isOnProxyAttributeNotifyReadWrite.subscribe).toBeDefined();
                            expect(isOnProxyAttributeNotifyReadWrite.unsubscribe).toBeDefined();
                            done();
                        });

                        it("has correct members (ProxyAttributeNotifyRead)", function(done) {
                            expect(isOnProxyAttributeNotifyRead.get).toBeDefined();
                            expect(isOnProxyAttributeNotifyRead.set).toBeUndefined();
                            expect(isOnProxyAttributeNotifyRead.subscribe).toBeDefined();
                            expect(isOnProxyAttributeNotifyRead.unsubscribe).toBeDefined();
                            done();
                        });

                        it("has correct members (ProxyAttributeNotifyWrite)", function(done) {
                            expect(isOnProxyAttributeNotifyWrite.get).toBeUndefined();
                            expect(isOnProxyAttributeNotifyWrite.set).toBeDefined();
                            expect(isOnProxyAttributeNotifyWrite.subscribe).toBeDefined();
                            expect(isOnProxyAttributeNotifyWrite.unsubscribe).toBeDefined();
                            done();
                        });

                        it("has correct members (ProxyAttributeNotify)", function(done) {
                            expect(isOnProxyAttributeNotify.get).toBeUndefined();
                            expect(isOnProxyAttributeNotify.set).toBeUndefined();
                            expect(isOnProxyAttributeNotify.subscribe).toBeDefined();
                            expect(isOnProxyAttributeNotify.unsubscribe).toBeDefined();
                            done();
                        });

                        it("has correct members (ProxyAttributeReadWrite)", function(done) {
                            expect(isOnProxyAttributeReadWrite.get).toBeDefined();
                            expect(isOnProxyAttributeReadWrite.set).toBeDefined();
                            expect(isOnProxyAttributeReadWrite.subscribe).toBeUndefined();
                            expect(isOnProxyAttributeReadWrite.unsubscribe).toBeUndefined();
                            done();
                        });

                        it("has correct members (ProxyAttributeRead)", function(done) {
                            expect(isOnProxyAttributeRead.get).toBeDefined();
                            expect(isOnProxyAttributeRead.set).toBeUndefined();
                            expect(isOnProxyAttributeRead.subscribe).toBeUndefined();
                            expect(isOnProxyAttributeRead.unsubscribe).toBeUndefined();
                            done();
                        });

                        it("has correct members (ProxyAttributeWrite)", function(done) {
                            expect(isOnProxyAttributeWrite.get).toBeUndefined();
                            expect(isOnProxyAttributeWrite.set).toBeDefined();
                            expect(isOnProxyAttributeWrite.subscribe).toBeUndefined();
                            expect(isOnProxyAttributeWrite.unsubscribe).toBeUndefined();
                            done();
                        });

                        it(
                                "get calls through to RequestReplyManager",
                                function(done) {
                                    var requestReplyId;
                                    isOn.get().then(function() {
                                        expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
                                        requestReplyId =
                                            requestReplyManagerSpy.sendRequest.calls.argsFor(0)[0].request.requestReplyId;
                                        expect(requestReplyManagerSpy.sendRequest)
                                            .toHaveBeenCalledWith({
                                                to : providerParticipantId,
                                                from : proxyParticipantId,
                                                messagingQos : messagingQos,
                                                request : new Request({
                                                    methodName : "getIsOn",
                                                    requestReplyId : requestReplyId
                                                })
                                        });
                                        done();
                                        return null;
                                    }).catch(function() {
                                        fail("get call unexpectedly failed.");
                                        return null;
                                    });
                                });

                        it("get notifies", function(done) {
                            expect(isOn.get).toBeDefined();
                            expect(typeof isOn.get === "function").toBeTruthy();

                            isOn.get().then(function() {
                                done();
                                return null;
                            }).catch(function() {
                                fail("get notifies rejected unexpectedly");
                                return null;
                            });
                        });

                        it("get returns correct joynr objects", function(done) {
                            var fixture = new ProxyAttribute(
                                    {
                                        proxyParticipantId : "proxy",
                                        providerParticipantId : "provider"
                                    },
                                    settings,
                                    "attributeOfTypeTestEnum",
                                    TestEnum.ZERO._typeName,
                                    "NOTIFYREADWRITE");

                            expect(fixture.get).toBeDefined();
                            expect(typeof fixture.get === "function").toBeTruthy();

                            requestReplyManagerSpy.sendRequest.and.returnValue(Promise.resolve([ "ZERO" ]));
                            fixture.get().then(function(data) {
                                expect(data).toEqual(TestEnum.ZERO);
                                done();
                                return null;
                            }).catch(function() {
                                return null;
                            });
                        });

                        it(
                                "set calls through to RequestReplyManager",
                                function(done) {
                                    var requestReplyId;
                                    isOn.set({
                                        value : true
                                    }).then(function() {
                                        expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
                                        requestReplyId =
                                                requestReplyManagerSpy.sendRequest.calls.argsFor(0)[0].request.requestReplyId;
                                        expect(requestReplyManagerSpy.sendRequest)
                                                .toHaveBeenCalledWith({
                                                    to : providerParticipantId,
                                                    from : proxyParticipantId,
                                                    messagingQos : messagingQos,
                                                    request : new Request({
                                                        methodName : "setIsOn",
                                                        requestReplyId : requestReplyId,
                                                        paramDatatypes : [ "Boolean"
                                                        ],
                                                        params : [ true
                                                        ]
                                                    })
                                                }

                                            );
                                        done();
                                        return null;
                                    }).catch(function() {
                                        fail("got unexpected reject from setter");
                                        return null;
                                    });
                                });

                        it("subscribe calls through to SubscriptionManager", function(done) {
                            var spy = jasmine.createSpyObj("spy", [
                                "publication",
                                "publicationMissed"
                            ]);

                            isOn.subscribe({
                                messagingQos : messagingQos,
                                subscriptionQos : subscriptionQos,
                                onReceive : spy.publication,
                                onError : spy.publicationMissed
                            });

                            expect(subscriptionManagerSpy.registerSubscription).toHaveBeenCalled();
                            expect(subscriptionManagerSpy.registerSubscription)
                                    .toHaveBeenCalledWith({
                                        proxyId : proxyParticipantId,
                                        providerId : providerParticipantId,
                                        attributeName : "isOn",
                                        attributeType : "Boolean",
                                        qos : subscriptionQos,
                                        subscriptionId : undefined,
                                        onReceive : spy.publication,
                                        onError : spy.publicationMissed
                                    });
                            done();
                        });

                        it("subscribe notifies", function(done) {
                            expect(isOn.subscribe).toBeDefined();
                            expect(typeof isOn.subscribe === "function").toBeTruthy();

                            isOn.subscribe({
                                subscriptionQos : subscriptionQos,
                                onReceive : function(value) {},
                                onError : function(value) {}
                            }).then(function() {
                                done();
                                return null;
                            }).catch(function() {
                                fail("got reject from subscribe operation");
                                return null;
                            });
                        });

                        it("subscribe provides a subscriptionId", function(done) {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            isOn.subscribe({
                                subscriptionQos : subscriptionQos,
                                onReceive : function(value) {},
                                onError : function(value) {}
                            }).then(function(id) {
                                expect(id).toEqual(subscriptionId);
                                done();
                                return null;
                            }).catch(function() {
                                return null;
                            });
                        });

                        // TODO: implement mock for publication of the value
                        // it("subscribe publishes a value", function () {
                        // var publishedValue;
                        //
                        // runs(function () {
                        // isOn.subscribe({subscriptionQos: subscriptionQos, publication: function
                        // (value) { publishedValue = value; } });
                        // });
                        //
                        // waitsFor(function () {
                        // return publishedValue !== undefined;
                        // }, "The publication callback is fired and provides a value !==
                        // undefined", asyncTimeout);
                        // });

                        it("unsubscribe calls through to SubscriptionManager", function(done) {
                            isOn.unsubscribe({
                                subscriptionId : subscriptionId
                            }).then(function() {
                                expect(subscriptionManagerSpy.unregisterSubscription)
                                        .toHaveBeenCalled();
                                expect(subscriptionManagerSpy.unregisterSubscription)
                                        .toHaveBeenCalledWith({
                                            messagingQos : new MessagingQos(),
                                            subscriptionId : subscriptionId
                                        });
                                done();
                                return null;
                            }).catch(function() {
                                return null;
                            });
                        });

                        it("unsubscribe notifies", function(done) {
                            expect(isOn.unsubscribe).toBeDefined();
                            expect(typeof isOn.unsubscribe === "function").toBeTruthy();

                            isOn.subscribe({
                                subscriptionQos : subscriptionQos,
                                onReceive : function(value) {},
                                onError : function(value) {}
                            }).then(function(subscriptionId) {
                                return isOn.unsubscribe({
                                    subscriptionId : subscriptionId
                                });
                            }).then(function() {
                                done();
                                return null;
                            }).catch(function() {
                                fail("subscribe or unsubscribe unexpectedly failed");
                                return null;
                            });
                        });

                        it(
                                "throws if caller sets a generic object without a declared _typeName attribute with the name of a registrered type",
                                function(done) {
                                    var proxy = {};
                                    var radioStationProxyAttributeWrite =
                                            new ProxyAttributeWrite(
                                                    proxy,
                                                    settings,
                                                    "radioStationProxyAttributeWrite",
                                                    RadioStation);

                                    expect(function() {
                                        radioStationProxyAttributeWrite.set({
                                            value : {
                                                name : "radiostationname",
                                                station : "station"
                                            }
                                        });
                                    }).toThrow();
                                    done();
                                });
                    });

        }); // require
/*jslint nomen: false */
