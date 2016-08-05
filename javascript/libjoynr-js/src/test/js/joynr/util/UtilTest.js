/*jslint es5: true, nomen: true */

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
            "global/Promise",
            "joynr/util/UtilInternal",
            "joynr/messaging/JoynrMessage",
            "joynr/system/LoggerFactory",
            "joynr/start/TypeRegistry",
            "joynr/vehicle/radiotypes/RadioStation"
        ],
        function(Promise, Util, JoynrMessage, LoggerFactory, TypeRegistry, RadioStation) {

            var argument = {
                someObjectKey : "andValue"
            };

            describe("libjoynr-js.joynr.Util", function() {
                it("is defined and of correct type", function() {
                    expect(Util).toBeDefined();
                    expect(Util).not.toBeNull();
                    expect(typeof Util === "object").toBeTruthy();
                });
            });

            describe("libjoynr-js.joynr.Util.extend", function() {

                it("extends objects", function() {
                    var merged, message, subobject, object1, object2, object3;
                    merged = {};

                    message = new JoynrMessage({
                        type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
                    });
                    message.payload = {
                        payload1 : 1,
                        payload2 : 2
                    };
                    subobject = {
                        sublevel20 : "sublevel20",
                        sublevel21 : "sublevel21"
                    };

                    object1 = {
                        originalField : "originalField"
                    };
                    object2 = {
                        message : message,
                        number : 2.0,
                        array : [
                            1,
                            2,
                            3,
                            4,
                            5
                        ],
                        string : "string",
                        bool : true
                    };
                    object3 = {
                        level11 : "level11",
                        level12 : subobject
                    };

                    Util.extend(merged, object1, object2, object3);

                    expect(merged.originalField).toEqual("originalField");
                    expect(merged.message).toEqual(message);
                    expect(merged.level11).toEqual(object3.level11);
                    expect(merged.level12).toEqual(subobject);

                    expect(typeof merged.number === "number").toBeTruthy();
                    expect(Util.isArray(merged.array)).toBeTruthy();
                    expect(typeof merged.string === "string").toBeTruthy();
                    expect(typeof merged.bool === "boolean").toBeTruthy();

                });
                it("deep extends objects", function() {
                    var merged, from;
                    merged = {};

                    from = {
                        subobject : {
                            number : 2.0,
                            array : [
                                0,
                                1,
                                2,
                                3,
                                4
                            ],
                            string : "string",
                            bool : true
                        }
                    };

                    Util.extendDeep(merged, from);

                    delete from.subobject.number;
                    delete from.subobject.array;
                    delete from.subobject.string;
                    delete from.subobject.bool;

                    expect(typeof merged.subobject.number === "number").toBeTruthy();
                    expect(merged.subobject.number).toEqual(2.0);

                    expect(Util.isArray(merged.subobject.array)).toBeTruthy();
                    expect(merged.subobject.array[0]).toEqual(0);
                    expect(merged.subobject.array[1]).toEqual(1);
                    expect(merged.subobject.array[2]).toEqual(2);
                    expect(merged.subobject.array[3]).toEqual(3);
                    expect(merged.subobject.array[4]).toEqual(4);

                    expect(typeof merged.subobject.string === "string").toBeTruthy();
                    expect(merged.subobject.string).toEqual("string");

                    expect(typeof merged.subobject.bool === "boolean").toBeTruthy();
                    expect(merged.subobject.bool).toEqual(true);
                });
            });

            describe("libjoynr-js.joynr.Util.transform", function() {

                it("transform array", function() {
                    var origin, element, transformed, postFix;
                    postFix = "_transformed";
                    origin = [];
                    element = {
                        a : "a"
                    };
                    origin.push(element);

                    // now, let's transform
                    transformed = Util.transform(origin, function(element, key) {
                        var id, member, result = {};
                        for (id in element) {
                            if (element.hasOwnProperty(id)) {
                                member = element[id];
                                result[id] = member + postFix;
                            }
                        }
                        return result;
                    });

                    expect(transformed.length).toEqual(1);
                    expect(transformed[0].a).toEqual("a" + postFix);
                });
            });

            describe(
                    "libjoynr-js.joynr.Util.firstLower",
                    function() {
                        it(
                                "decapitalizes first character correctly",
                                function() {
                                    expect(Util.firstLower("")).toEqual("");
                                    expect(Util.firstLower("asdf")).toEqual("asdf");
                                    expect(Util.firstLower("b")).toEqual("b");
                                    expect(Util.firstLower("Csdf")).toEqual("csdf");
                                    expect(Util.firstLower("D")).toEqual("d");
                                    expect(Util.firstLower("ESDFASDF")).toEqual("eSDFASDF");
                                    expect(Util.firstLower("FsDfAsDf")).toEqual("fsDfAsDf");
                                    var rettyLongString =
                                            "RETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRING";
                                    expect(Util.firstLower("P" + rettyLongString)).toEqual(
                                            "p" + rettyLongString);
                                });

                        it("throws on nullable input", function() {
                            expect(function() {
                                Util.firstLower(null);
                            }).toThrow();
                            expect(function() {
                                Util.firstLower(undefined);
                            }).toThrow();
                        });
                    });

            describe(
                    "libjoynr-js.joynr.Util.firstUpper",
                    function() {
                        it(
                                "capitalizes first character correctly",
                                function() {
                                    expect(Util.firstUpper("")).toEqual("");
                                    expect(Util.firstUpper("asdf")).toEqual("Asdf");
                                    expect(Util.firstUpper("b")).toEqual("B");
                                    expect(Util.firstUpper("Csdf")).toEqual("Csdf");
                                    expect(Util.firstUpper("D")).toEqual("D");
                                    expect(Util.firstUpper("esdfasdf")).toEqual("Esdfasdf");
                                    expect(Util.firstUpper("fSdFaSdF")).toEqual("FSdFaSdF");
                                    var rettyLongString =
                                            "rettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstringPRETTYLONGSTRINGprettylongstring";
                                    expect(Util.firstUpper("p" + rettyLongString)).toEqual(
                                            "P" + rettyLongString);
                                });

                        it("throws on nullable input", function() {
                            expect(function() {
                                Util.firstLower(null);
                            }).toThrow();
                            expect(function() {
                                Util.firstLower(undefined);
                            }).toThrow();
                        });
                    });

            describe("libjoynr-js.joynr.Util.isPromise", function() {
                it("returns only true if param is promis", function() {
                    expect(Util.isPromise(Promise.resolve())).toBe(true);
                    expect(Util.isPromise("")).toBe(false);
                    expect(Util.isPromise(true)).toBe(false);
                    expect(Util.isPromise()).toBe(false);
                });

            });

            function CustomObj() {}
            function AnotherCustomObj() {}
            var objects = [
                true,
                1,
                "a string",
                [],
                {},
                function() {},
                new CustomObj(),
                new AnotherCustomObj()
            ];
            var types = [
                "Boolean",
                "Number",
                "String",
                "Array",
                "Object",
                "Function",
                CustomObj,
                AnotherCustomObj
            ];

            function testUtilCheckProperty(functionName) {
                it("provides the correct type information", function() {
                    var i, j;
                    function functionBuilder(object, type) {
                        return function() {
                            Util[functionName](object, type, "some description");
                        };
                    }

                    for (i = 0; i < objects.length; ++i) {
                        for (j = 0; j < types.length; ++j) {
                            var test = expect(functionBuilder(objects[i], types[j]));

                            if (i === j) {
                                test.not.toThrow();
                            } else {
                                test.toThrow();
                            }
                        }
                    }
                });

                it("supports type alternatives", function() {
                    var type = [
                        "Object",
                        "CustomObj"
                    ];
                    expect(function() {
                        Util[functionName]({}, type, "some description");
                    }).not.toThrow();
                    expect(function() {
                        Util[functionName](new CustomObj(), type, "some description");
                    }).not.toThrow();
                    expect(function() {
                        Util[functionName](new AnotherCustomObj(), type, "some description");
                    }).toThrow();
                });
            }

            describe("libjoynr-js.joynr.Util.checkProperty", function() {
                testUtilCheckProperty("checkProperty");

                it("throws on null and undefined", function() {
                    expect(function() {
                        Util.checkProperty(undefined, "undefined", "some description");
                    }).toThrow();
                    expect(function() {
                        Util.checkProperty(null, "null", "some description");
                    }).toThrow();
                });
            });

            describe("libjoynr-js.joynr.Util.checkPropertyIfDefined", function() {
                testUtilCheckProperty("checkPropertyIfDefined");

                it("does not throw on null or undefined", function() {
                    expect(function() {
                        Util.checkPropertyIfDefined(undefined, "undefined", "some description");
                    }).not.toThrow();
                    expect(function() {
                        Util.checkPropertyIfDefined(null, "null", "some description");
                    }).not.toThrow();
                });
            });

            describe("libjoynr-js.joynr.Util.ensureTypedValues", function() {
                var typeRegistry = new TypeRegistry();
                typeRegistry.addType("joynr.vehicle.radiotypes.RadioStation", RadioStation);

                it("types untyped objects", function() {

                    var returnValue = null;
                    var untypedValue = {
                        name : "radioStationName",
                        _typeName : "joynr.vehicle.radiotypes.RadioStation"
                    };

                    returnValue = Util.ensureTypedValues(untypedValue, typeRegistry);
                    expect(returnValue instanceof RadioStation).toBe(true);
                    expect(returnValue.name === untypedValue.name).toBe(true);
                });

                it("types untyped arrays", function() {
                    var returnValue = null;
                    var untypedArray = [
                        {
                            name : "radioStationName1",
                            _typeName : "joynr.vehicle.radiotypes.RadioStation"
                        },
                        {
                            name : "radioStationName2",
                            _typeName : "joynr.vehicle.radiotypes.RadioStation"
                        }
                    ];

                    returnValue = Util.ensureTypedValues(untypedArray, typeRegistry);
                    expect(returnValue[0] instanceof RadioStation).toBe(true);
                    expect(returnValue[1] instanceof RadioStation).toBe(true);
                    expect(returnValue[0].name === untypedArray[0].name).toBe(true);
                    expect(returnValue[1].name === untypedArray[1].name).toBe(true);
                });

                it("accepts primitive types", function() {
                    var returnValue = null;
                    var numberValue = 1;
                    var booleanValue = true;
                    var stringValue = "string";

                    returnValue = Util.ensureTypedValues(numberValue, typeRegistry);
                    expect(typeof returnValue === "number").toBe(true);

                    returnValue = Util.ensureTypedValues(booleanValue, typeRegistry);
                    expect(typeof returnValue === "boolean").toBe(true);

                    returnValue = Util.ensureTypedValues(stringValue, typeRegistry);
                    expect(typeof returnValue === "string").toBe(true);
                });
            });

        });
/* jslint nomen: false */
