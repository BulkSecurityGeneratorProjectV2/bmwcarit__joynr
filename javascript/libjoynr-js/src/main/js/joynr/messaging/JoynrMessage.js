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

var MESSAGE_CUSTOM_HEADER_PREFIX = "custom-";
define(
        "joynr/messaging/JoynrMessage",
        [
            "joynr/util/UtilInternal",
            "uuid"
        ],
        function(Util, uuid) {

            /**
             * @name JoynrMessage
             * @constructor
             *
             * @param {Object}
             *            settings the settings object holding values for the JoynrMessage
             * @param {String}
             *            settings.type the message type as defined by JoynrMessage.JOYNRMESSAGE_TYPE_*
             */
            function JoynrMessage(settings) {
                settings = settings || {};

                Object.defineProperties(this, {
                    /**
                     * The joynr type name
                     *
                     * @name JoynrMessage#_typeName
                     * @type String
                     */
                    "_typeName" : {
                        value : "joynr.JoynrMessage",
                        readable : true,
                        writable : false,
                        enumerable : true,
                        configurable : false
                    },
                    /**
                     * The message type as defined by JoynrMessage.JOYNRMESSAGE_TYPE_*
                     *
                     * @name JoynrMessage#type
                     * @type String
                     */
                    "type" : {
                        value : settings.type,
                        readable : true,
                        writable : false,
                        enumerable : true,
                        configurable : false
                    },
                    /**
                     * The message header holding additional values
                     *
                     * @name JoynrMessage#header
                     * @type Object
                     */
                    "header" : {
                        value : settings.header || {},
                        readable : true,
                        writable : false,
                        enumerable : true,
                        configurable : false
                    },
                    /**
                     * The serialized message payload
                     *
                     * @name JoynrMessage#payload
                     * @type String
                     */
                    "payload" : {
                        value : settings.payload || "",
                        readable : true,
                        writable : false,
                        enumerable : true,
                        configurable : false
                    }
                });

                this.header[JoynrMessage.JOYNRMESSAGE_HEADER_CONTENT_TYPE] = "application/json";

                if (this.header[JoynrMessage.JOYNRMESSAGE_HEADER_MESSAGE_ID] === undefined) {
                    this.header[JoynrMessage.JOYNRMESSAGE_HEADER_MESSAGE_ID] = uuid();
                }
            }

            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY
             */
            JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY = "oneWay";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
             */
            JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST = "request";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_TYPE_REPLY
             */
            JoynrMessage.JOYNRMESSAGE_TYPE_REPLY = "reply";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST
             */
            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST = "subscriptionRequest";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST
             */
            JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST =
                    "multicastSubscriptionRequest";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST
             */
            JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST =
                    "broadcastSubscriptionRequest";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY
             */
            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY = "subscriptionReply";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION
             */
            JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION = "subscriptionPublication";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST
             */
            JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST = "multicast";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP
             */
            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP = "subscriptionStop";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_HEADER_MESSAGE_ID
             */
            JoynrMessage.JOYNRMESSAGE_HEADER_MESSAGE_ID = "msgId";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_HEADER_CREATOR_USER_ID
             */
            JoynrMessage.JOYNRMESSAGE_HEADER_CREATOR_USER_ID = "creator";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE
             */
            JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE = "expiryDate";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_HEADER_EFFORT
             */
            JoynrMessage.JOYNRMESSAGE_HEADER_EFFORT = "effort";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID
             */
            JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID = "replyChannelId";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_HEADER_CONTENT_TYPE
             */
            JoynrMessage.JOYNRMESSAGE_HEADER_CONTENT_TYPE = "contentType";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_HEADER_SUBSCRIPTION_ATTRIBUTE
             */
            JoynrMessage.JOYNRMESSAGE_HEADER_SUBSCRIPTION_ATTRIBUTE = "subscriptionAttribute";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID
             */
            JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID = "to";
            /**
             * @static
             * @readonly
             * @type String
             * @name JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID
             */
            JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID = "from";

            /**
             * @name JoynrMessage#setHeader
             * @function
             *
             * @param {String}
             *            key is one of the header keys defined in JoynrMessagingDefines
             * @param {Any}
             *            value of the header
             * @returns {JoynrMessage}
             */
            Object.defineProperty(JoynrMessage.prototype, "setHeader", {
                enumerable : false,
                configurable : false,
                writable : false,
                value : function(key, value) {
                    this.header[key] = value;
                    return this;
                }
            });

            /**
             * @name JoynrMessage#setCustomHeaders
             * @function
             * @param {Object}
             *            a map containing key/value pairs of headers to be set as custom
             *            headers. The keys will be added to the header field with the prefix
             *            MESSAGE_CUSTOM_HEADER_PREFIX
             * @returns {JoynrMessage}
             */
            Object.defineProperty(JoynrMessage.prototype, "setCustomHeaders", {
                enumerable : false,
                configurable : false,
                writable : false,
                value : function(customHeaders) {
                    var headerKey;
                    for (headerKey in customHeaders) {
                        if (customHeaders.hasOwnProperty(headerKey)) {
                            this.header[MESSAGE_CUSTOM_HEADER_PREFIX + headerKey] =
                                    customHeaders[headerKey];
                        }
                    }
                    return this;
                }
            });

            /**
             * @name JoynrMessage#getCustomHeaders
             * @function
             * @returns {Object} customHeader object containing all headers that begin with the
             *          prefix MESSAGE_CUSTOM_HEADER_PREFIX
             */
            Object
                    .defineProperty(
                            JoynrMessage.prototype,
                            "getCustomHeaders",
                            {
                                enumerable : false,
                                configurable : false,
                                writable : false,
                                value : function() {
                                    var headerKey, trimmedKey, customHeaders = {};
                                    for (headerKey in this.header) {
                                        if (this.header.hasOwnProperty(headerKey)
                                            && headerKey.substr(
                                                    0,
                                                    MESSAGE_CUSTOM_HEADER_PREFIX.length) === MESSAGE_CUSTOM_HEADER_PREFIX) {
                                            trimmedKey =
                                                    headerKey
                                                            .substr(MESSAGE_CUSTOM_HEADER_PREFIX.length);

                                            customHeaders[trimmedKey] = this.header[headerKey];
                                        }
                                    }
                                    return customHeaders;
                                }
                            });

            var headerProperties = [
                JoynrMessage.JOYNRMESSAGE_HEADER_MESSAGE_ID,
                JoynrMessage.JOYNRMESSAGE_HEADER_CREATOR_USER_ID,
                JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID,
                JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID,
                JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID,
                JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE,
                JoynrMessage.JOYNRMESSAGE_HEADER_EFFORT
            ];

            /**
             * The user ID of the message creator
             *
             * @name JoynrMessage#creator
             * @type String
             */
            /**
             * The participant id the message is from
             *
             * @name JoynrMessage#from
             * @type String
             */
            /**
             * The participant id the message is to
             *
             * @name JoynrMessage#to
             * @type String
             */
            /**
             * The reply channel Id to return response messages to
             *
             * @name JoynrMessage#replyChannelId
             * @type String
             */
            /**
             * The expiry date of the message
             *
             * @name JoynrMessage#expiryDate
             * @type String
             */
            /**
             * The effort to be expent while delivering the message
             *
             * @name JoynrMessage#effort
             * @type String
             */
            (function defineHeaders(headerProperties) {
                var i;
                headerProperties.forEach(function(headerProperty) {
                    Object.defineProperty(JoynrMessage.prototype, headerProperty, {
                        set : function(value) {
                            this.header[headerProperty] = value;
                        },
                        get : function() {
                            return this.header[headerProperty];
                        }
                    });
                });
            }(headerProperties));

            JoynrMessage.prototype.isReceivedFromGlobal = false;

            Object.defineProperty(JoynrMessage.prototype, "setReceivedFromGlobal", {
                enumerable : false,
                configurable : false,
                writable : false,
                value : function(receivedFromGlobal) {
                    this.isReceivedFromGlobal = receivedFromGlobal;
                }
            });

            return JoynrMessage;

        });
