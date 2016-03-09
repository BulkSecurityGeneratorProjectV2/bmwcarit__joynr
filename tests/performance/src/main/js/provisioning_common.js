/*jslint node: true */

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

var provisioning = {};
provisioning.ccAddress = {
    protocol : "ws",
    host : "localhost",
    port : 4242,
    path : ""
};
provisioning.bounceProxyBaseUrl = "http://localhost:8080";
provisioning.bounceProxyUrl = provisioning.bounceProxyBaseUrl + "/bounceproxy/";
var discoveryChannelId = "discoverydirectory_channelid";
provisioning.channelUrls = {};
provisioning.channelUrls[discoveryChannelId] =
        [ provisioning.bounceProxyBaseUrl + "/discovery/channels/" + discoveryChannelId + "/"];

provisioning.logging = {
    configuration : {
        loggers : {
            root : {
                level : "error"
            }
        }
    }
};

module.exports = provisioning;
