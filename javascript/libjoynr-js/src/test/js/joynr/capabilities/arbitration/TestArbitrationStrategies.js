/*global joynrTestRequire: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

joynrTestRequire(
        "joynr/capabilities/arbitration/TestArbitrationStrategies",
        [
            "joynr/types/ArbitrationStrategyCollection",
            "joynr/types/DiscoveryEntry",
            "joynr/types/ProviderScope",
            "joynr/types/ProviderQos",
            "joynr/types/CustomParameter",
            "joynr/types/Version",
            "joynr/messaging/inprocess/InProcessAddress"
        ],
        function(
                ArbitrationStrategyCollection,
                DiscoveryEntry,
                ProviderScope,
                ProviderQos,
                CustomParameter,
                Version,
                InProcessAddress) {
            describe(
                    "libjoynr-js.joynr.types.ArbitrationStrategyCollection",
                    function() {
                        it("is defined and of correct type", function() {
                            expect(ArbitrationStrategyCollection).toBeDefined();
                            expect(ArbitrationStrategyCollection).not.toBeNull();
                            expect(typeof ArbitrationStrategyCollection === "object").toBeTruthy();
                        });

                        it("has all required strategies of type function", function() {
                            expect(ArbitrationStrategyCollection.Nothing).toBeDefined();
                            expect(ArbitrationStrategyCollection.HighestPriority).toBeDefined();
                            expect(ArbitrationStrategyCollection.Keyword).toBeDefined();

                            expect(typeof ArbitrationStrategyCollection.Nothing).toBe("function");
                            expect(typeof ArbitrationStrategyCollection.HighestPriority).toBe(
                                    "function");
                            expect(typeof ArbitrationStrategyCollection.Keyword).toBe("function");
                        });

                        function getDiscoveryEntryList() {
                            return [
                                new DiscoveryEntry({
                                    providerVersion : new Version({
                                        majorVersion : 47,
                                        minorVersion : 11
                                    }),
                                    domain : "KeywordmyDomain",
                                    interfaceName : "myInterfaceName",
                                    qos : new ProviderQos({
                                        customParameters : [],
                                        priority : 1,
                                        scope : ProviderScope.GLOBAL,
                                        supportsOnChangeSubscriptions : true
                                    }),
                                    participantId : "1",
                                    publicKeyId : ""
                                }),
                                new DiscoveryEntry({
                                    providerVersion : new Version({
                                        majorVersion : 47,
                                        minorVersion : 11
                                    }),
                                    domain : "myDomain",
                                    interfaceName : "myInterfaceName",
                                    qos : new ProviderQos({
                                        customParameters : [],
                                        priority : 4,
                                        scope : ProviderScope.GLOBAL,
                                        supportsOnChangeSubscriptions : true
                                    }),
                                    participantId : "1",
                                    publicKeyId : ""
                                }),
                                new DiscoveryEntry({
                                    providerVersion : new Version({
                                        majorVersion : 47,
                                        minorVersion : 11
                                    }),
                                    domain : "myWithKeywordDomain",
                                    interfaceName : "myInterfaceName",
                                    lastSeenDateMs : 123,
                                    qos : new ProviderQos({
                                        customParameters : [
                                            new CustomParameter({
                                                name : "keyword",
                                                value : "myKeyword"
                                            }),
                                            new CustomParameter({
                                                name : "thename",
                                                value : "theValue"
                                            })
                                        ],
                                        priority : 3,
                                        scope : ProviderScope.GLOBAL,
                                        supportsOnChangeSubscriptions : true
                                    }),
                                    participantId : "1",
                                    publicKeyId : ""
                                }),
                                new DiscoveryEntry({
                                    providerVersion : new Version({
                                        majorVersion : 47,
                                        minorVersion : 11
                                    }),
                                    domain : "myDomain",
                                    interfaceName : "myInterfaceNameKeyword",
                                    lastSeenDateMs : 123,
                                    qos : new ProviderQos({
                                        customParameters : [
                                            new CustomParameter({
                                                name : "keyword",
                                                value : "wrongKeyword"
                                            }),
                                            new CustomParameter({
                                                name : "theName",
                                                value : "theValue"
                                            })
                                        ],
                                        priority : 5,
                                        scope : ProviderScope.GLOBAL,
                                        supportsOnChangeSubscriptions : true
                                    }),
                                    participantId : "1",
                                    publicKeyId : ""
                                }),
                                new DiscoveryEntry({
                                    providerVersion : new Version({
                                        majorVersion : 47,
                                        minorVersion : 11
                                    }),
                                    domain : "myDomain",
                                    interfaceName : "myInterfaceName",
                                    lastSeenDateMs : 123,
                                    qos : new ProviderQos({
                                        customParameters : [
                                            new CustomParameter({
                                                name : "keyword",
                                                value : "myKeyword"
                                            }),
                                            new CustomParameter({
                                                name : "theName",
                                                value : "theValue"
                                            })
                                        ],
                                        priority : 2,
                                        scope : ProviderScope.GLOBAL,
                                        supportsOnChangeSubscriptions : true
                                    }),
                                    participantId : "1",
                                    publicKeyId : ""
                                })
                            ];
                        }

                        it("Strategy 'Nothing' does nothing", function() {
                            expect(ArbitrationStrategyCollection.Nothing(getDiscoveryEntryList()))
                                    .toEqual(getDiscoveryEntryList());
                        });

                        it("Strategy 'HighestPriority' includes all capability infos", function() {
                            var discoveryEntryList = getDiscoveryEntryList();
                            var discoveryEntryId;
                            var highestPriority =
                                    ArbitrationStrategyCollection
                                            .HighestPriority(discoveryEntryList);
                            for (discoveryEntryId in discoveryEntryList) {
                                if (discoveryEntryList.hasOwnProperty(discoveryEntryId)) {
                                    expect(highestPriority).toContain(
                                            discoveryEntryList[discoveryEntryId]);
                                }
                            }
                        });

                        it(
                                "Strategy 'HighestPriority' sorts according to providerQos priority",
                                function() {
                                    var highestPriority =
                                            ArbitrationStrategyCollection
                                                    .HighestPriority(getDiscoveryEntryList());
                                    var i;
                                    for (i = 0; i < highestPriority.length - 1; ++i) {
                                        expect(highestPriority[i].qos.priority).toBeGreaterThan(
                                                highestPriority[i + 1].qos.priority);
                                    }
                                });

                        it(
                                "Strategy 'Keyword' only includes capability infos that have the keyword Qos set to 'myKeyword'",
                                function() {
                                    var discoveryEntryId;
                                    var qosId;
                                    var found;
                                    var qosParam;
                                    var keyword = "myKeyword";
                                    var discoveryEntryList = getDiscoveryEntryList();
                                    // The arbitrator only calls the strategy with the list
                                    // of capabillities, so Keyword should be tested with bind()
                                    // which however is not supported by PhantomJS 1
                                    /*
                                    var arbitrationStrategy =
                                        ArbitrationStrategyCollection.Keyword.bind(
                                                undefined,
                                                keyword);
                                    var keywordCapInfoList =
                                        arbitrationStrategy(discoveryEntryList);
                                     */
                                    var keywordCapInfoList =
                                            ArbitrationStrategyCollection.Keyword(
                                                    keyword,
                                                    discoveryEntryList);
                                    expect(keywordCapInfoList.length).toBe(2);
                                    for (discoveryEntryId in discoveryEntryList) {
                                        if (discoveryEntryList.hasOwnProperty(discoveryEntryId)) {
                                            var capInfo = discoveryEntryList[discoveryEntryId];
                                            found = false;
                                            if (capInfo.qos.customParameters
                                                && Object.prototype.toString
                                                        .call(capInfo.qos.customParameters) === '[object Array]') {
                                                for (qosId in capInfo.qos.customParameters) {
                                                    if (capInfo.qos.customParameters
                                                            .hasOwnProperty(qosId)) {
                                                        qosParam =
                                                                capInfo.qos.customParameters[qosId];
                                                        if (!found
                                                            && qosParam
                                                            && qosParam.value
                                                            && qosParam.value === keyword) {
                                                            found = true;
                                                        }
                                                    }
                                                }
                                            }
                                            if (found) {
                                                expect(keywordCapInfoList).toContain(capInfo);
                                            } else {
                                                expect(keywordCapInfoList).not.toContain(capInfo);
                                            }
                                        }
                                    }
                                });

                    });

        }); // require
