/*jslint es5: true */
/*global Promise: true, WorkerUtils: true, importScripts: true, joynr: true, DatatypesProvider: true, providerParticipantIdDatatypes, domain: true, TestEnd2EndDatatypesTestData: true, interfaceNameDatatypes: true, providerParticipantIdDatatypes: true, providerChannelIdDatatypes: true, globalCapDirCapability: true, channelUrlDirCapability: true */

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

// anything that you load here is served through the jsTestDriverServer, if you add an entry you
// have to make it available through the jsTestDriverIntegrationTests.conf
importScripts("WorkerUtils.js");
importScripts("../joynr/provisioning/provisioning_root.js");
importScripts("LocalStorageSimulator.js");

importScripts("../../classes/js/joynr.js");
importScripts("../joynr/provisioning/provisioning_cc.js");
importScripts("provisioning_end2end_common.js");

importScripts("../joynr/datatypes/exampleTypes/Country.js");
importScripts("../joynr/datatypes/exampleTypes/ComplexRadioStation.js");
importScripts("../joynr/datatypes/DatatypesProvider.js");
importScripts("TestEnd2EndDatatypesTestData.js");
importScripts("../../classes/lib/bluebird.js");

var Promise = Promise.Promise;

// attribute values for provider
var currentAttributeValue, isOn = true;

var datatypesProvider;
var runtime;

var providerQos;

function getObjectType(obj) {
    if (obj === null || obj === undefined) {
        throw new Error("cannot determine the type of an undefined object");
    }
    var funcNameRegex = /function ([$\w]+)\(/;
    var results = funcNameRegex.exec(obj.constructor.toString());
    return (results && results.length > 1) ? results[1] : "";
}

function getter() {
    return currentAttributeValue;
}

function setter(value) {
    currentAttributeValue = value;
}

function initializeTest(provisioningSuffix) {
    return new Promise(function(resolve, reject) {

        // set joynr provisioning
        localStorage.setItem(
                "joynr.participants." + domain + interfaceNameDatatypes,
                providerParticipantIdDatatypes + provisioningSuffix);
        joynr.provisioning.channelId = providerChannelIdDatatypes + provisioningSuffix;
        joynr.provisioning.logging = {
            configuration : {
                name : "TestEnd2EndCommProviderWorkLogging",
                appenders : {
                    WebWorker : {
                        name : "WEBWORKER"
                    }
                },
                loggers : {
                    root : {
                        level : "debug",
                        AppenderRef : {
                            ref : "WEBWORKER"
                        }
                    }
                }
            }
        };

        joynr.provisioning.persistency = "localStorage";

        joynr.load(joynr.provisioning).then(function(newJoynr){
            joynr = newJoynr;
            providerQos = new joynr.types.ProviderQos({
                customParameters : [],
                providerVersion : 1,
                priority : Date.now(),
                scope : joynr.types.ProviderScope.GLOBAL,
                onChangeSubscriptions : true
            });

            // build the provider
            datatypesProvider = joynr.providerBuilder.build(DatatypesProvider, {});

            var i;
            // there are so many attributes for testing different datatypes => register them
            // all by cycling over their names in the attribute
            for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
                var attribute = datatypesProvider[TestEnd2EndDatatypesTestData[i].attribute];
                attribute.registerGetter(getter);
                attribute.registerSetter(setter);
            }

            // registering operation functions
            datatypesProvider.getJavascriptType.registerOperation(function(opArgs) {
                return {
                    javascriptType: getObjectType(opArgs.arg)
                };
            });
            datatypesProvider.getArgumentBack.registerOperation(function(opArgs) {
                return {
                    returnValue : opArgs.arg
                };
            });
            datatypesProvider.multipleArguments.registerOperation(function(opArgs) {
                return {
                    serialized : JSON.stringify(opArgs)
                };
            });

            // register provider at the given domain
            joynr.capabilities.registerCapability("", domain, datatypesProvider, providerQos).then(
                    function() {
                        // signal test driver that we are ready
                        resolve(joynr);
                    }).catch(function(error) {
                        reject(error);
                        throw new Error("error registering provider: " + error);
                    });
            
        }).catch(function(error){
            throw error;
        });
    });
}

function startTest() {
    // nothing to do here, evertything is already performed in initialize
    return Promise.resolve();
}

function terminateTest() {
    return joynr.capabilities.unregisterCapability("", domain, datatypesProvider);

}