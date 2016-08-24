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
        "joynr/exceptions/JoynrException",
        [
            "joynr/types/TypeRegistrySingleton",
            "joynr/util/UtilInternal",
            "joynr/system/LoggerFactory"
        ],
        function(TypeRegistrySingleton, Util, LoggerFactory) {
            var defaultSettings;

            /**
             * @classdesc
             *
             * @summary
             * Constructor of JoynrException object used for reporting
             * error conditions. This serves as superobject for the underlying
             * ApplicationException and JoynrRuntimeException.
             *
             * @constructor
             * @name JoynrException
             *
             * @param {Object}
             *            [settings] the settings object for the constructor call
             * @param {String}
             *            [settings.detailMessage] message containing details
             *            about the error
             * @returns {JoynrException}
             *            The newly created JoynrException object
             */
            function JoynrException(settings) {
                if (!(this instanceof JoynrException)) {
                    // in case someone calls constructor without new keyword (e.g. var c
                    // = Constructor({..}))
                    return new JoynrException(settings);
                }

                var log = LoggerFactory.getLogger("joynr.exceptions.JoynrException");

                /**
                 * Used for serialization.
                 * @name JoynrException#_typeName
                 * @type String
                 */
                Util.objectDefineProperty(this, "_typeName", "joynr.exceptions.JoynrException");
                Util.checkPropertyIfDefined(settings, "Object", "settings");
                if (settings && settings.detailMessage) {
                    Util.checkPropertyIfDefined(
                            settings.detailMessage,
                            "String",
                            "settings.detailMessage");
                    this.detailMessage = settings.detailMessage;
                } else {
                    /**
                     * See [constructor description]{@link JoynrException}.
                     * @name JoynrException#detailMessage
                     * @type String
                     */
                    this.detailMessage = undefined;
                }
                Util.extend(this, defaultSettings, settings);
            }

            defaultSettings = {};

            TypeRegistrySingleton.getInstance().addType(
                    "joynr.exceptions.JoynrException",
                    JoynrException);

            JoynrException.prototype = new Error();
            JoynrException.prototype.constructor = JoynrException;
            JoynrException.prototype.name = "JoynrException";

            return JoynrException;

        });
