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

define(
        [
            "joynr/dispatching/RequestReplyManager",
            "joynr/dispatching/types/OneWayRequest",
            "joynr/dispatching/types/Request",
            "joynr/dispatching/types/Reply",
            "joynr/types/TypeRegistrySingleton",
            "joynr/util/Typing",
            "joynr/util/UtilInternal",
            "joynr/exceptions/MethodInvocationException",
            "global/Promise",
            "global/WaitsFor"
        ],
        function(RequestReplyManager, OneWayRequest, Request, Reply, TypeRegistrySingleton, Typing, UtilInternal, MethodInvocationException, Promise, waitsFor) {
            describe(
                    "libjoynr-js.joynr.dispatching.RequestReplyManager",
                    function() {

                        var requestReplyManager;
                        var typeRegistry;
                        var ttl_ms = 50;
                        var requestReplyId = "requestReplyId";
                        var testResponse = [ "testResponse"
                        ];
                        var reply = new Reply({
                            requestReplyId : requestReplyId,
                            response : testResponse
                        });

                        function RadioStation(name, station, source) {
                            if (!(this instanceof RadioStation)) {
                                // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
                                return new RadioStation(name, station, source);
                            }
                            this.name = name;
                            this.station = station;
                            this.source = source;

                            Object.defineProperty(this, "checkMembers", {
                                configurable : false,
                                writable : false,
                                enumerable : false,
                                value : jasmine.createSpy("checkMembers")
                            });

                            Object.defineProperty(this, "_typeName", {
                                configurable : false,
                                writable : false,
                                enumerable : true,
                                value : "test.RadioStation"
                            });
                        }

                        var Country = {
                            AUSTRALIA : "AUSTRALIA",
                            AUSTRIA : "AUSTRIA",
                            CANADA : "CANADA",
                            GERMANY : "GERMANY",
                            ITALY : "ITALY",
                            UNITED_KINGDOM : "UNITED_KINGDOM"
                        };

                        function ComplexTypeWithComplexAndSimpleProperties(
                                radioStation,
                                myBoolean,
                                myString) {
                            if (!(this instanceof ComplexTypeWithComplexAndSimpleProperties)) {
                                // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
                                return new ComplexTypeWithComplexAndSimpleProperties(
                                        radioStation,
                                        myBoolean,
                                        myString);
                            }
                            this.radioStation = radioStation;
                            this.myBoolean = myBoolean;
                            this.myString = myString;

                            Object.defineProperty(this, "checkMembers", {
                                configurable : false,
                                writable : false,
                                enumerable : false,
                                value : jasmine.createSpy("checkMembers")
                            });

                            Object.defineProperty(this, "_typeName", {
                                configurable : false,
                                writable : false,
                                enumerable : true,
                                value : "test.ComplexTypeWithComplexAndSimpleProperties"
                            });
                        }

                        /**
                         * Called before each test.
                         */
                        beforeEach(function() {
                            typeRegistry = TypeRegistrySingleton.getInstance();
                            typeRegistry.addType("test.RadioStation", RadioStation);
                            typeRegistry.addType(
                                    "test.ComplexTypeWithComplexAndSimpleProperties",
                                    ComplexTypeWithComplexAndSimpleProperties);
                            requestReplyManager = new RequestReplyManager({}, typeRegistry);
                        });

                        it("is instantiable", function(done) {
                            expect(requestReplyManager).toBeDefined();
                            done();
                        });

                        var tripleJ = new RadioStation("TripleJ", "107.7", "AUSTRALIA");
                        var fm4 = new RadioStation("FM4", "104.0", "AUSTRIA");
                        var complex =
                                new ComplexTypeWithComplexAndSimpleProperties(
                                        tripleJ,
                                        true,
                                        "hello");
                        var testData =
                                [
                                    {
                                        paramDatatype : [ "Boolean"
                                        ],
                                        params : [ true
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "Integer"
                                        ],
                                        params : [ 123456789
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "Double"
                                        ],
                                        params : [ -123.456789
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "String"
                                        ],
                                        params : [ "lalala"
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "Integer[]"
                                        ],
                                        params : [ [
                                            1,
                                            2,
                                            3,
                                            4,
                                            5
                                        ]
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "joynr.vehicle.radiotypes.RadioStation[]"
                                        ],
                                        params : [ [
                                            fm4,
                                            tripleJ
                                        ]
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "joynr.vehicle.radiotypes.RadioStation"
                                        ],
                                        params : [ tripleJ
                                        ]
                                    },
                                    {
                                        paramDatatype : [
                                            "joynr.vehicle.radiotypes.RadioStation",
                                            "String"
                                        ],
                                        params : [
                                            tripleJ,
                                            "testParam"
                                        ]
                                    },
                                    {
                                        paramDatatype : [ "vehicle.ComplexTypeWithComplexAndSimpleProperties"
                                        ],
                                        params : [ complex
                                        ]
                                    }
                                ];

                        function testHandleRequestForGetterSetterMethod(attributeName, params, promiseChain) {
                            var providerParticipantId = "providerParticipantId";
                            var provider = {};
                            provider[attributeName] = {
                                get : jasmine.createSpy("getSpy"),
                                set : jasmine.createSpy("setSpy")
                            };

                            provider[attributeName].get.and.returnValue([]);
                            provider[attributeName].set.and.returnValue([]);

                            return promiseChain.then(function() {
                                var request = new Request({
                                    methodName : "get" + UtilInternal.firstUpper(attributeName),
                                    paramDatatypes : [],
                                    params : []
                                });

                                requestReplyManager.addRequestCaller(
                                        providerParticipantId,
                                        provider);

                                requestReplyManager.handleRequest(
                                        providerParticipantId,
                                        request,
                                        jasmine.createSpy);

                                return waitsFor(function() {
                                    return provider[attributeName].get.calls.count() > 0;
                                }, "getAttribute to be called", 100);
                            }).then(function() {
                                var request = new Request({
                                    methodName : "set" + UtilInternal.firstUpper(attributeName),
                                    paramDatatypes : [],
                                    // untype objects through serialization and deserialization
                                    params : JSON.parse(JSON.stringify(params))
                                });

                                requestReplyManager.addRequestCaller(
                                        providerParticipantId,
                                        provider);

                                requestReplyManager.handleRequest(
                                        providerParticipantId,
                                        request,
                                        jasmine.createSpy);

                                return waitsFor(function() {
                                    return provider[attributeName].set.calls.count() > 0;
                                }, "setAttribute to be called", 100);
                            }).then(function() {
                                expect(provider[attributeName].get).toHaveBeenCalledWith();

                                expect(provider[attributeName].set).toHaveBeenCalledWith(params[0]);

                                var result =
                                        provider[attributeName].set.calls.argsFor(0)[0];
                                expect(result).toEqual(params[0]);
                            });
                        }

                        function testHandleRequestWithExpectedType(paramDatatypes, params, promiseChain) {
                            var providerParticipantId = "providerParticipantId";
                            var provider = {
                                testFunction : {
                                    callOperation : jasmine.createSpy("operationSpy")
                                }
                            };

                            provider.testFunction.callOperation.and.returnValue([]);

                            return promiseChain.then(function() {
                                var request = new Request({
                                    methodName : "testFunction",
                                    paramDatatypes : paramDatatypes,
                                    // untype objects through serialization and deserialization
                                    params : JSON.parse(JSON.stringify(params))
                                });

                                requestReplyManager.addRequestCaller(
                                        providerParticipantId,
                                        provider);
                                requestReplyManager.handleRequest(
                                        providerParticipantId,
                                        request,
                                        jasmine.createSpy);

                                return waitsFor(function() {
                                    return provider.testFunction.callOperation.calls.count() > 0;
                                }, "callOperation to be called", 100);
                            }).then(function() {
                                expect(provider.testFunction.callOperation).toHaveBeenCalled();
                                expect(provider.testFunction.callOperation).toHaveBeenCalledWith(
                                        params,
                                        paramDatatypes);

                                var result =
                                        provider.testFunction.callOperation.calls.argsFor(0)[0];
                                expect(result).toEqual(params);
                                expect(Typing.getObjectType(result)).toEqual(
                                        Typing.getObjectType(params));
                            });
                        }

                        it("calls registered requestCaller for attribute", function(done) {
                            var promiseChain = Promise.resolve();
                            testHandleRequestForGetterSetterMethod("attributeA", [ "attributeA"], promiseChain);
                            testHandleRequestForGetterSetterMethod("AttributeWithStartingCapitalLetter", [ "AttributeWithStartingCapitalLetter"], promiseChain);
                            promiseChain.then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "calls registered requestCaller with correctly typed object",
                                function(done) {
                                    var i, test;
                                    var promiseChain = Promise.resolve();
                                    for (i = 0; i < testData.length; ++i) {
                                        test = testData[i];
                                        promiseChain = testHandleRequestWithExpectedType(
                                                test.paramDatatype,
                                                test.params, promiseChain);
                                    }
                                    promiseChain.then(function() {
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it("calls registered replyCaller when a reply arrives", function(done) {
                            var replyCallerSpy = jasmine.createSpyObj("promise", [
                                "resolve",
                                "reject"
                            ]);

                            requestReplyManager.addReplyCaller(
                                requestReplyId,
                                replyCallerSpy,
                                ttl_ms);
                            requestReplyManager.handleReply(reply);

                            waitsFor(function() {
                                return replyCallerSpy.resolve.calls.count() > 0
                                    || replyCallerSpy.reject.calls.count() > 0;
                            }, "reject or fulfill to be called", ttl_ms * 2).then(function() {
                                expect(replyCallerSpy.resolve).toHaveBeenCalled();
                                expect(replyCallerSpy.resolve).toHaveBeenCalledWith(testResponse);
                                expect(replyCallerSpy.reject).not.toHaveBeenCalled();
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "calls registered replyCaller fail if no reply arrives in time",
                                function(done) {
                                    var replyCallerSpy = jasmine.createSpyObj("deferred", [
                                        "resolve",
                                        "reject"
                                    ]);

                                    requestReplyManager.addReplyCaller(
                                        "requestReplyId",
                                        replyCallerSpy,
                                        ttl_ms);

                                    waitsFor(function() {
                                        return replyCallerSpy.resolve.calls.count() > 0
                                            || replyCallerSpy.reject.calls.count() > 0;
                                    }, "reject or fulfill to be called", ttl_ms * 2).then(function() {
                                        expect(replyCallerSpy.resolve).not.toHaveBeenCalled();
                                        expect(replyCallerSpy.reject).toHaveBeenCalled();
                                        done();
                                        return null;
                                    }).catch(fail);

                                });

                        function testHandleReplyWithExpectedType(paramDatatypes, params, promiseChain) {
                            var replyCallerSpy = jasmine.createSpyObj("deferred", [
                                "resolve",
                                "reject"
                            ]);
                            return promiseChain.then(function() {

                                var reply = new Reply({
                                    requestReplyId : requestReplyId,
                                    // untype object by serializing and deserializing it
                                    response : [ JSON.parse(JSON.stringify(params))
                                    ]
                                });

                                requestReplyManager.addReplyCaller(
                                        "requestReplyId",
                                        replyCallerSpy,
                                        ttl_ms);
                                requestReplyManager.handleReply(reply);

                                return waitsFor(function() {
                                    return replyCallerSpy.resolve.calls.count() > 0
                                        || replyCallerSpy.reject.calls.count() > 0;
                                }, "reject or fulfill to be called", ttl_ms * 2);
                            }).then(function() {
                                expect(replyCallerSpy.resolve).toHaveBeenCalled();
                                expect(replyCallerSpy.reject).not.toHaveBeenCalled();

                                var result = replyCallerSpy.resolve.calls.argsFor(0)[0];
                                expect(result).toEqual([ params
                                ]);
                                expect(Typing.getObjectType(result)).toEqual(
                                    Typing.getObjectType(params));
                            }).catch(function(error) {
                                fail(error);
                            });
                        }

                        it("calls registered replyCaller with correctly typed object", function(done) {
                            var i, test;
                            var promiseChain = Promise.resolve();
                            for (i = 0; i < testData.length; ++i) {
                                test = testData[i];
                                promiseChain = testHandleReplyWithExpectedType(test.paramDatatype, test.params, promiseChain);
                            }
                            promiseChain.then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        function callRequestReplyManagerSync(
                                methodName,
                                testParam,
                                testParamDatatype,
                                useInvalidProviderParticipantId) {
                            var providerParticipantId = "providerParticipantId";
                            var TestProvider = function() {};
                            TestProvider.MAJOR_VERSION = 47;
                            TestProvider.MINOR_VERSION = 11;
                            var provider = new TestProvider();
                            UtilInternal.extend(provider, {
                                attributeName : {
                                    get : jasmine.createSpy("getterSpy"),
                                    set : jasmine.createSpy("setterSpy")
                                },
                                operationName : {
                                    callOperation : jasmine.createSpy("operationSpy")
                                },
                                getOperationStartingWithGet : {
                                    callOperation : jasmine.createSpy("operationSpy")
                                },
                                getOperationHasPriority : {
                                    callOperation : jasmine.createSpy("operationSpy")
                                },
                                operationHasPriority : {
                                    get : jasmine.createSpy("getterSpy"),
                                    set : jasmine.createSpy("setterSpy")
                                }
                            }, true);
                            provider.attributeName.get.and.returnValue([ testParam
                            ]);
                            provider.attributeName.set.and.returnValue([]);
                            provider.operationName.callOperation.and.returnValue([ testParam
                            ]);
                            provider.getOperationStartingWithGet.callOperation.and
                                    .returnValue([ testParam
                                    ]);
                            provider.getOperationHasPriority.callOperation.and
                                    .returnValue([ testParam
                                    ]);

                            var callbackDispatcher = jasmine.createSpy("callbackDispatcher");

                            var request = new Request({
                                methodName : methodName,
                                paramDatatypes : [ testParamDatatype
                                ],
                                params : [ testParam
                                ]
                            });

                            requestReplyManager.addRequestCaller(providerParticipantId, provider);

                            if (useInvalidProviderParticipantId) {
                                providerParticipantId = "nonExistentProviderId";
                            }

                            requestReplyManager.handleRequest(
                                    providerParticipantId,
                                    request,
                                    callbackDispatcher);

                            return {
                                provider : provider,
                                callbackDispatcher : callbackDispatcher,
                                request : request
                            };
                        }

                        function callRequestReplyManager(
                                methodName,
                                testParam,
                                testParamDatatype,
                                useInvalidProviderParticipantId) {
                            var test =
                                    callRequestReplyManagerSync(
                                            methodName,
                                            testParam,
                                            testParamDatatype,
                                            useInvalidProviderParticipantId);

                            return waitsFor(function() {
                                return test.callbackDispatcher.calls.count() > 0;
                            }, "callbackDispatcher to be called", 100).then(function() {
                                return Promise.resolve(test);
                            });

                            //return test;
                        }

                        var testParam = "myTestParameter";
                        var testParamDatatype = "String";

                        it("calls attribute getter correctly", function(done) {
                            callRequestReplyManager(
                                "getAttributeName",
                                testParam,
                            testParamDatatype).then(function(test) {
                                expect(test.provider.attributeName.get).toHaveBeenCalled();
                                expect(test.provider.attributeName.get).toHaveBeenCalledWith();
                                expect(test.provider.attributeName.set).not.toHaveBeenCalled();
                                expect(test.provider.operationName.callOperation).not
                                        .toHaveBeenCalled();
                                expect(test.provider.getOperationStartingWithGet.callOperation).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.set).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.get).not
                                        .toHaveBeenCalled();
                                expect(test.provider.getOperationHasPriority.callOperation).not
                                        .toHaveBeenCalled();

                                expect(test.callbackDispatcher).toHaveBeenCalled();
                                expect(test.callbackDispatcher).toHaveBeenCalledWith(new Reply({
                                    response : [ testParam
                                    ],
                                    requestReplyId : test.request.requestReplyId
                                }));
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("calls attribute setter correctly", function(done) {
                            callRequestReplyManager(
                                "setAttributeName",
                                testParam,
                            testParamDatatype).then(function(test) {
                                expect(test.provider.attributeName.get).not.toHaveBeenCalled();
                                expect(test.provider.attributeName.set).toHaveBeenCalled();
                                expect(test.provider.attributeName.set).toHaveBeenCalledWith(
                                        testParam);
                                expect(test.provider.operationName.callOperation).not
                                        .toHaveBeenCalled();
                                expect(test.provider.getOperationStartingWithGet.callOperation).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.set).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.get).not
                                        .toHaveBeenCalled();
                                expect(test.provider.getOperationHasPriority.callOperation).not
                                        .toHaveBeenCalled();

                                expect(test.callbackDispatcher).toHaveBeenCalled();
                                expect(test.callbackDispatcher).toHaveBeenCalledWith(new Reply({
                                    response : [],
                                    requestReplyId : test.request.requestReplyId
                                }));
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("calls operation function correctly", function(done) {
                            callRequestReplyManager(
                                "operationName",
                                testParam,
                                testParamDatatype).then(function(test) {
                                expect(test.provider.attributeName.set).not.toHaveBeenCalled();
                                expect(test.provider.attributeName.get).not.toHaveBeenCalled();
                                expect(test.provider.operationName.callOperation)
                                        .toHaveBeenCalled();
                                expect(test.provider.operationName.callOperation)
                                        .toHaveBeenCalledWith([ testParam
                                        ], [ testParamDatatype
                                        ]);
                                expect(test.provider.getOperationStartingWithGet.callOperation).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.set).not
                                        .toHaveBeenCalled();
                                expect(test.provider.operationHasPriority.get).not
                                        .toHaveBeenCalled();
                                expect(test.provider.getOperationHasPriority.callOperation).not
                                        .toHaveBeenCalled();

                                expect(test.callbackDispatcher).toHaveBeenCalled();
                                expect(test.callbackDispatcher).toHaveBeenCalledWith(new Reply({
                                    response : [ testParam
                                    ],
                                    requestReplyId : test.request.requestReplyId
                                }));
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("calls operation function for one-way request correctly", function(done) {
                            var providerParticipantId = "oneWayProviderParticipantId";
                            var provider = {
                                fireAndForgetMethod : {
                                    callOperation : jasmine.createSpy("operationSpy")
                                }
                            };

                            var callbackDispatcher = jasmine.createSpy("callbackDispatcher");

                            var oneWayRequest = new OneWayRequest({
                                methodName : "fireAndForgetMethod",
                                paramDatatypes : [ testParamDatatype
                                ],
                                params : [ testParam
                                ]
                            });

                            requestReplyManager.addRequestCaller(
                                providerParticipantId,
                                provider);

                            requestReplyManager.handleOneWayRequest(
                                providerParticipantId,
                                oneWayRequest);

                            waitsFor(function() {
                                return provider.fireAndForgetMethod.callOperation.calls.count() > 0;
                            }, "callOperation to be called", 1000).then(function() {
                                expect(provider.fireAndForgetMethod.callOperation)
                                        .toHaveBeenCalled();
                                expect(provider.fireAndForgetMethod.callOperation)
                                        .toHaveBeenCalledWith([ testParam
                                        ], [ testParamDatatype
                                        ]);
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "calls operation \"getOperationStartingWithGet\" when no attribute \"operationStartingWithGet\" exists",
                                function(done) {
                                    callRequestReplyManager(
                                        "getOperationStartingWithGet",
                                        testParam,
                                        testParamDatatype).then(function(test) {
                                        expect(
                                                test.provider.getOperationStartingWithGet.callOperation)
                                                .toHaveBeenCalled();
                                        expect(
                                                test.provider.getOperationStartingWithGet.callOperation)
                                                .toHaveBeenCalledWith([ testParam
                                                ], [ testParamDatatype
                                                ]);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "calls operation \"getOperationHasPriority\" when attribute \"operationHasPriority\" exists",
                                function(done) {
                                    callRequestReplyManager(
                                        "getOperationHasPriority",
                                        testParam,
                                        testParamDatatype).then(function(test) {
                                        expect(test.provider.operationHasPriority.set).not
                                                .toHaveBeenCalled();
                                        expect(test.provider.operationHasPriority.get).not
                                                .toHaveBeenCalled();
                                        expect(test.provider.getOperationHasPriority.callOperation)
                                                .toHaveBeenCalled();
                                        expect(test.provider.getOperationHasPriority.callOperation)
                                                .toHaveBeenCalledWith([ testParam
                                                ], [ testParamDatatype
                                                ]);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "delivers exception upon non-existent provider",
                                function(done) {
                                    callRequestReplyManager(
                                        "testFunction",
                                        testParam,
                                        testParamDatatype,
                                        true).then(function(test) {
                                        expect(test.callbackDispatcher).toHaveBeenCalled();
                                        expect(test.callbackDispatcher)
                                                .toHaveBeenCalledWith(
                                                        new Reply(
                                                                {
                                                                    error : Typing
                                                                            .augmentTypes(
                                                                                    {
                                                                                        "_typeName" : "joynr.exceptions.MethodInvocationException",
                                                                                        "detailMessage" : 'error handling request: {"paramDatatypes":["String"],"params":["myTestParameter"],"methodName":"testFunction","requestReplyId":"'
                                                                                            + test.request.requestReplyId
                                                                                            + '","_typeName":"joynr.Request"} for providerParticipantId nonExistentProviderId'
                                                                                    },
                                                                                    typeRegistry),
                                                                    requestReplyId : test.request.requestReplyId
                                                                }));
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "delivers exception when calling not existing operation",
                                function(done) {
                                    callRequestReplyManager(
                                        "notExistentOperationOrAttribute",
                                        testParam,
                                        testParamDatatype).then(function(test) {
                                        expect(test.callbackDispatcher).toHaveBeenCalled();
                                        expect(test.callbackDispatcher)
                                                .toHaveBeenCalledWith(
                                                        new Reply(
                                                                {
                                                                    error : Typing
                                                                            .augmentTypes(
                                                                                    {
                                                                                        "_typeName" : "joynr.exceptions.MethodInvocationException",
                                                                                        "detailMessage" : 'Could not find an operation "notExistentOperationOrAttribute" in the provider',
                                                                                        "providerVersion" : {
                                                                                            "_typeName" : "joynr.types.Version",
                                                                                            "majorVersion" : 47,
                                                                                            "minorVersion" : 11
                                                                                        }
                                                                                    },
                                                                                    typeRegistry),
                                                                    requestReplyId : test.request.requestReplyId
                                                                }));
                                        done();
                                        return null;
                                    }).catch(fail);
                                });
                        it(
                                "delivers exception when calling getter for not existing attribute",
                                function(done) {
                                    callRequestReplyManager(
                                        "getNotExistentOperationOrAttribute",
                                        testParam,
                                        testParamDatatype).then(function(test) {
                                        expect(test.callbackDispatcher).toHaveBeenCalled();
                                        expect(test.callbackDispatcher)
                                                .toHaveBeenCalledWith(
                                                        new Reply(
                                                                {
                                                                    error : Typing
                                                                            .augmentTypes(
                                                                                    {
                                                                                        "_typeName" : "joynr.exceptions.MethodInvocationException",
                                                                                        "detailMessage" : 'Could not find an operation "getNotExistentOperationOrAttribute" or an attribute "notExistentOperationOrAttribute" in the provider',
                                                                                        "providerVersion" : {
                                                                                            "_typeName" : "joynr.types.Version",
                                                                                            "majorVersion" : 47,
                                                                                            "minorVersion" : 11
                                                                                        }
                                                                                    },
                                                                                    typeRegistry),
                                                                    requestReplyId : test.request.requestReplyId
                                                                }));
                                        done();
                                        return null;
                                    }).catch(fail);
                                });
                        it(
                                "delivers exception when calling setter for not existing attribute",
                                function(done) {
                                    callRequestReplyManager(
                                        "setNotExistentOperationOrAttribute",
                                        testParam,
                                        testParamDatatype).then(function(test) {
                                        expect(test.callbackDispatcher).toHaveBeenCalled();
                                        expect(test.callbackDispatcher)
                                                .toHaveBeenCalledWith(
                                                        new Reply(
                                                                {
                                                                    error : Typing
                                                                            .augmentTypes(
                                                                                    {
                                                                                        "_typeName" : "joynr.exceptions.MethodInvocationException",
                                                                                        "detailMessage" : 'Could not find an operation "setNotExistentOperationOrAttribute" or an attribute "notExistentOperationOrAttribute" in the provider',
                                                                                        "providerVersion" : {
                                                                                            "_typeName" : "joynr.types.Version",
                                                                                            "majorVersion" : 47,
                                                                                            "minorVersion" : 11
                                                                                        }
                                                                                    },
                                                                                    typeRegistry),
                                                                    requestReplyId : test.request.requestReplyId
                                                                }));
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                " throws exception when called while shut down",
                                function(done) {
                                    requestReplyManager.shutdown();
                                    expect(function() {
                                        requestReplyManager.removeRequestCaller(
                                                "providerParticipantId");
                                    }).toThrow();
                                    var callbackDispatcherSpy = jasmine.createSpy('callbackDispatcherSpy');
                                    requestReplyManager.handleRequest(
                                            "providerParticipantId",
                                            {
                                                requestReplyId : requestReplyId
                                            },
                                            callbackDispatcherSpy);
                                    expect(callbackDispatcherSpy).toHaveBeenCalled();
                                    expect(callbackDispatcherSpy.calls.argsFor(0)[0] instanceof Reply);
                                    expect(callbackDispatcherSpy.calls.argsFor(0)[0].error instanceof MethodInvocationException);
                                    expect(function() {
                                        var replyCallerSpy = jasmine.createSpyObj("promise", [
                                                                                              "resolve",
                                                                                              "reject"
                                                                                          ]);

                                        requestReplyManager.addReplyCaller(requestReplyId, replyCallerSpy);
                                    }).toThrow();
                                    expect(function() {
                                        requestReplyManager.addRequestCaller("providerParticipantId", {});
                                    }).toThrow();
                                    expect(function() {
                                        requestReplyManager.sendOneWayRequest({});
                                    }).toThrow();
                                    expect(function() {
                                        requestReplyManager.sendRequest({});
                                    }).toThrow();
                                    done();
                                });
                        it(" rejects reply callers when shut down", function(done) {
                            var replyCallerSpy = jasmine.createSpyObj("promise", [
                                                                                  "resolve",
                                                                                  "reject"
                                                                                  ]);

                            requestReplyManager.addReplyCaller(
                                  requestReplyId,
                                  replyCallerSpy,
                                  ttl_ms);
                            expect(replyCallerSpy.reject).not.toHaveBeenCalled();
                            requestReplyManager.shutdown();
                            expect(replyCallerSpy.reject).toHaveBeenCalled();
                            done();
                        });

                    });
        }); // require
