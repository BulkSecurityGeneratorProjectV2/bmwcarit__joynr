/*jslint node: true  es5: true */

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

var log = require("./logging.js").log;
var prettyLog = require("./logging.js").prettyLog;
var joynr = require("joynr");
var RadioStation;
var Country;
var currentStationSubscriptionId;
var multicastSubscriptionId;
var multicastPSubscriptionId;
var subscriptionQosOnChange;

var runDemo = function(radioProxy) {
    prettyLog("ATTRIBUTE GET: currentStation...");
    radioProxy.currentStation.get().catch(function(error) {
        prettyLog("ATTRIBUTE GET: currentStation failed: " + error);
    }).then(function(value) {
        prettyLog("ATTRIBUTE GET: currentStation returned: " + JSON.stringify(value));
        prettyLog("RPC: radioProxy.addFavoriteStation(radioStation)...");
        return radioProxy.addFavoriteStation(
            {
                newFavoriteStation : new RadioStation({
                    name : "runDemoFavoriteStation",
                    trafficService : true,
                    country : Country.GERMANY
                })
            }
        );
    }).catch(function(error) {
        prettyLog("RPC: radioProxy.addFavoriteStation(radioStation) failed: " + error);
    }).then(function() {
        prettyLog("RPC: radioProxy.addFavoriteStation(radioStation) returned");
        prettyLog("radioProxy.shuffleStations()...");
        return radioProxy.shuffleStations();
    }).catch(function(error) {
        prettyLog("RPC: radioProxy.shuffleStations() failed: " + error);
    }).then(function(value) {
        prettyLog("RPC: radioProxy.shuffleStations() returned");
        prettyLog("ATTRIBUTE GET: currentStation after shuffle...");
        return radioProxy.currentStation.get();
    }).catch(function(error) {
        prettyLog("ATTRIBUTE GET: currentStation failed: " + error);
    }).then(function(value) {
        prettyLog("ATTRIBUTE GET: currentStation returned: " + JSON.stringify(value));
    });
};

var runInteractiveConsole =
        function(radioProxy, onDone) {
            var readline = require('readline');
            var rl = readline.createInterface(process.stdin, process.stdout);
            rl.setPrompt('>> ');
            var MODES = {
                HELP : {
                    value : "h",
                    description : "help",
                    options : {}
                },
                QUIT : {
                    value : "q",
                    description : "quit",
                    options : {}
                },
                SHUFFLE_STATIONS : {
                    value : "s",
                    description : "shuffle stations",
                    options : {}
                },
                ADD_FAVORITE_STATION : {
                    value : "a",
                    description : "add a Favorite Station",
                    options : {
                        NAME : "name"
                    }
                },
                SUBSCRIBE : {
                    value : "subscribe",
                    description : "subscribe to current station",
                    options : {}
                },
                MULTICAST : {
                    value : "subscribeMulticast",
                    description : "subscribe to weak signal multicast",
                    options : {}
                },
                MULTICASTP : {
                    value : "subscribeMulticastP",
                    description : "subscribe to weak signal multicast with partition \"GERMANY\"",
                    options : {}
                },
                UNSUBSCRIBE : {
                    value : "unsubscribe",
                    description : "unsubscribe from all subscriptions",
                    options : {}
                },
                GET_CURRENT_STATION : {
                    value : "c",
                    description : "get current station",
                    options : {}
                }
            };

            var showHelp = require("./console_common.js");
            rl.on('line', function(line) {
                var input = line.trim().split(' ');

                function subscribeHelper(subscribeToName, partitions) {
                    var partitionsString = partitions ? JSON.stringify(partitions) : "";
                    var onReceiveCallback = function onReceiveCallback(value) {
                        prettyLog("radioProxy." + subscribeToName + partitionsString + ".subscribe.onReceive",
                                  JSON.stringify(value));
                    };

                    var onErrorCallback = function onErrorCallback(error) {
                        prettyLog("radioProxy." + subscribeToName + partitionsString + ".subscribe.onError", error);
                    };

                    return radioProxy[subscribeToName].subscribe({
                        subscriptionQos : subscriptionQosOnChange,
                        onReceive : onReceiveCallback,
                        onError : onErrorCallback,
                        partitions : partitions
                    }).then(function(subscriptionId) {
                        prettyLog("radioProxy." + subscribeToName + partitionsString + ".subscribe.done",
                                  "Subscription ID: "+ subscriptionId);
                        return subscriptionId;
                    }).catch(function(error) {
                        prettyLog("radioProxy." + subscribeToName + partitionsString + ".subscribe.fail", error);
                        return error;
                    });
                }
                function unsubscribeHelper(subscribeToName, subscriptionId, partitions) {
                    var partitionsString = partitions ? JSON.stringify(partitions) : "";
                    return radioProxy[subscribeToName].unsubscribe({
                        subscriptionId : subscriptionId
                    }).then(function() {
                        prettyLog("radioProxy." + subscribeToName + partitionsString + ".unsubscribe.done",
                                  "Subscription ID: " + subscriptionId);
                        return null;
                    }).catch(function(error) {
                        prettyLog("radioProxy." + subscribeToName + partitionsString + ".unsubscribe.fail",
                                  "Subscription ID: " + subscriptionId + " ERROR: " + error
                        );
                        return null;
                    });
                }
                switch (input[0]) {
                    case MODES.HELP.value:
                        showHelp(MODES);
                        break;
                    case MODES.QUIT.value:
                        rl.close();
                        break;
                    case MODES.ADD_FAVORITE_STATION.value:
                        var newFavoriteStation = new RadioStation({
                            name : input[1] || "",
                            trafficService : true,
                            country : Country.GERMANY
                        });

                        radioProxy.addFavoriteStation({newFavoriteStation: newFavoriteStation})
                            .then(
                                function(returnValue) {
                                    prettyLog("RPC: radioProxy.addFavoriteStation("
                                        + JSON.stringify(newFavoriteStation)
                                        + ") returned: "
                                        + JSON.stringify(returnValue));
                                    return returnValue;
                                }).catch(
                                function(error) {
                                    prettyLog("RPC: radioProxy.addFavoriteStation("
                                        + JSON.stringify(newFavoriteStation)
                                        + ") failed."
                                        + error);
                                    return error;
                                });
                        break;
                    case MODES.SHUFFLE_STATIONS.value:
                        radioProxy.shuffleStations().then(function() {
                            prettyLog("RPC: radioProxy.shuffleStations returned. ");
                            return null;
                        }).catch(function(error) {
                            prettyLog("RPC: radioProxy.shuffleStations failed: " + error);
                            return error;
                       });
                        break;
                    case MODES.SUBSCRIBE.value:
                        if (currentStationSubscriptionId === undefined) {
                            subscribeHelper("currentStation").then(function(suscriptionId) {
                                currentStationSubscriptionId = suscriptionId;
                            });
                        }
                        break;
                    case MODES.MULTICAST.value:
                        if (multicastSubscriptionId === undefined) {
                            subscribeHelper("weakSignal").then(function(suscriptionId) {
                                multicastSubscriptionId = suscriptionId;
                            });
                        }
                        break;
                    case MODES.MULTICASTP.value:
                        if (multicastPSubscriptionId === undefined) {
                            subscribeHelper("weakSignal", [ "GERMANY" ]).then(function(suscriptionId) {
                                multicastPSubscriptionId = suscriptionId;
                            });
                        }
                        break;
                    case MODES.UNSUBSCRIBE.value:
                        if (currentStationSubscriptionId !== undefined) {
                            unsubscribeHelper("currentStation", currentStationSubscriptionId).then(function() {
                                currentStationSubscriptionId = undefined;
                            });
                        }
                        if (multicastSubscriptionId !== undefined) {
                            unsubscribeHelper("weakSignal", multicastSubscriptionId).then(function() {
                                multicastSubscriptionId = undefined;
                            });
                        }
                        if (multicastPSubscriptionId !== undefined) {
                            unsubscribeHelper("weakSignal", multicastPSubscriptionId, [ "GERMANY" ]).then(function() {
                                multicastPSubscriptionId = undefined;
                            });
                        }
                        break;
                    case MODES.GET_CURRENT_STATION.value:
                        radioProxy.currentStation.get().then(function(currentStation) {
                            prettyLog("RPC: radioProxy.getCurrentStation returned: " + JSON.stringify(currentStation));
                        }).catch(function(error) {
                            prettyLog("RPC: radioProxy.getCurrentStation failed: " + error);
                        });
                        break;
                    case '':
                        break;
                    default:
                        log('unknown input: ' + input);
                        break;
                }
                rl.prompt();
            });

            rl.on('close', function() {
                if (onDone) {
                    onDone();
                }
            });

            showHelp(MODES);
            rl.prompt();
        };

if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(0);
}
var domain = process.env.domain;
log("domain: " + domain);

var provisioning = require("./provisioning_common.js");

if (process.env.runtime !== undefined) {
    if (process.env.runtime === "inprocess") {
        provisioning.brokerUri = process.env.brokerUri;
        provisioning.bounceProxyBaseUrl = process.env.bounceProxyBaseUrl;
        provisioning.bounceProxyUrl = provisioning.bounceProxyBaseUrl + "/bounceproxy/";
        joynr.selectRuntime("inprocess");
    } else if (process.env.runtime === "websocket") {
        provisioning.ccAddress.host = process.env.cchost;
        provisioning.ccAddress.port = process.env.ccport;
        joynr.selectRuntime("websocket.libjoynr");
    }
}

RadioStation = require("../generated/js/joynr/vehicle/RadioStation");
Country = require("../generated/js/joynr/vehicle/Country");
require("../generated/js/joynr/vehicle/GeoPosition");
var AddFavoriteStationErrorEnum = require("../generated/js/joynr/vehicle/Radio/AddFavoriteStationErrorEnum");
var RadioProxy = require("../generated/js/joynr/vehicle/RadioProxy.js");
joynr.load(provisioning).then(function(loadedJoynr) {
    log("joynr started");
    joynr = loadedJoynr;
    var messagingQos = new joynr.messaging.MessagingQos({
        ttl : 60000
    });

    subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
        minIntervalMs : 50
	});

    joynr.proxyBuilder.build(RadioProxy, {
        domain : domain,
        messagingQos : messagingQos
    }).then(function(radioProxy) {
        log("radio proxy build");
        runDemo(radioProxy);
        runInteractiveConsole(radioProxy, function() {
            log("exiting...");
            process.exit(0);
        });
        return radioProxy;
    }).catch(function(error) {
        log("error running radioProxy: " + error);
    });
    return loadedJoynr;
}).catch(function(error){
    throw error;
});
