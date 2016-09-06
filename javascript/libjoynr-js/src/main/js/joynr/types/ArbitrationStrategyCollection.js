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
        "joynr/types/ArbitrationStrategyCollection",
        [],
        function() {

            /**
             * The ArbitrationStrategyCollection provides three different strategies that can be
             * passed to the Arbitrator to sort the retrieved Capabilities: Nothing, HighestPriority
             * or Keyword
             *
             * @class
             * @name ArbitrationStrategyCollection
             * @see Arbitrator
             * @see DiscoveryQos
             */
            var ArbitrationStrategyCollection = {};

            /**
             * The ArbitrationStrategyCollection.Nothing just returns the passed array
             * @function ArbitrationStrategyCollection#Nothing
             *
             * @param {Array}
             *            capabilities the array of capability entries
             * @param {DiscoveryEntry}
             *            capabilities.array the array of DiscoveryEntries
             *
             * @returns {Array} an array of capabilities as it came in through the
             *          capabilities parameter
             */
            ArbitrationStrategyCollection.Nothing = function(capabilities) {
                return capabilities;
            };

            /**
             * The ArbitrationStrategyCollection.HighestPriority favors higher priorities
             * @function ArbitrationStrategyCollection#HighestPriority
             *
             * @param {Array}
             *            capabilities the array of capability entries
             * @param {DiscoveryEntry}
             *            capabilities.array the array of capability entries
             *
             * @returns {Array} an array of capabilities sorted by the highest priority
             */
            ArbitrationStrategyCollection.HighestPriority = function(capabilities) {
                if (!(Object.prototype.toString.call(capabilities) === "[object Array]")) {
                    throw new Error("provided argument capabilities is not of type Array");
                }

                // sort with descending priority
                return capabilities.sort(function(a, b) {
                    return b.qos.priority - a.qos.priority;
                });
            };

            /**
             * The ArbitrationStrategyCollection.Keyword searches for the given keyword in the
             * "keyword" qos parameter and returns those.
             * NOTE: to use this function, you must bind your keyword as follows:
             * arbitrationStrategy = Keyword.bind(undefined, "myKeyword")
             * @function ArbitrationStrategyCollection#Keyword
             *
             * @param {Array}
             *            capabilities the array of capability entries
             * @param {DiscoveryEntry}
             *            capabilities.array the array of capability entries
             * @param {String}
             *            keyword the keyword to search for
             * @returns {Array} an array of capabilities sorted by the highest priority
             */
            ArbitrationStrategyCollection.Keyword =
                    function(keyword, capabilities) {
                        var capId, qosId, qosParam, cap, keywordCaps = [];

                        for (capId in capabilities) {
                            if (capabilities.hasOwnProperty(capId)) {
                                cap = capabilities[capId];
                                if (cap.qos.customParameters
                                    && Object.prototype.toString.call(cap.qos.customParameters) === "[object Array]") {
                                    for (qosId in cap.qos.customParameters) {
                                        if (cap.qos.customParameters.hasOwnProperty(qosId)) {
                                            qosParam = cap.qos.customParameters[qosId];
                                            if (qosParam
                                                && qosParam.value
                                                && qosParam.value === keyword) {
                                                keywordCaps.push(cap);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        return keywordCaps;
                    };

            /**
             * The ArbitrationStrategyCollection.LastSeen favors latest lastSeenDateMs
             * @function ArbitrationStrategyCollection#LastSeen
             *
             * @param {Array}
             *            capabilities the array of capability entries
             * @param {DiscoveryEntry}
             *            capabilities.array the array of capability entries
             *
             * @returns {Array} an array of capabilities sorted by the lastSeenDateMs
             */
            ArbitrationStrategyCollection.LastSeen = function(capabilities) {
                if (!(Object.prototype.toString.call(capabilities) === "[object Array]")) {
                    throw new Error("provided argument capabilities is not of type Array");
                }

                // sort with descending priority
                return capabilities.sort(function(a, b) {
                    return b.lastSeenDateMs - a.lastSeenDateMs;
                });
            };

            return ArbitrationStrategyCollection;

        });
