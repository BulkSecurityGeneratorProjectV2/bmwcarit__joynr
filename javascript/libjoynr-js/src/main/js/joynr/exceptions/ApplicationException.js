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

define("joynr/exceptions/ApplicationException", [
    "joynr/types/TypeRegistrySingleton",
    "joynr/util/Typing",
    "joynr/util/UtilInternal",
    "joynr/exceptions/JoynrException",
    "joynr/system/LoggerFactory"
], function(TypeRegistrySingleton, Typing, Util, JoynrException, LoggerFactory) {
    var defaultSettings;

    /**
     * @classdesc
     *
     * @summary
     * Constructor of ApplicationException object used for reporting
     * error conditions from method implementations. The settings.error
     * object must be filled with _typeName and name as serialization
     * of an enum object of the matching error enum type defined in
     * Franca.
     *
     * @constructor
     * @name ApplicationException
     *
     * @param {Object}
     *            [settings] the settings object for the constructor call
     * @param {String}
     *            [settings.detailMessage] message containing details
     *            about the error
     * @returns {ApplicationException}
     *            The newly created ApplicationException object
     */
    function ApplicationException(settings) {
        if (!(this instanceof ApplicationException)) {
            // in case someone calls constructor without new keyword (e.g. var c
            // = Constructor({..}))
            return new ApplicationException(settings);
        }

        var log = LoggerFactory.getLogger("joynr.exceptions.ApplicationException");
        var exception = new JoynrException(settings);

        /**
         * Used for serialization.
         * @name ApplicationException#_typeName
         * @type String
         */
        Util.objectDefineProperty(this, "_typeName", "joynr.exceptions.ApplicationException");
        Typing.checkPropertyIfDefined(settings, "Object", "settings");
        if (settings && settings.error) {
            Typing.checkProperty(settings.error.name, "String", "settings.error.name");
            Typing.checkProperty(settings.error.value, [
                "String",
                "Number"
            ], "settings.error.value");
        }

        Util.extend(this, defaultSettings, settings, exception);
    }

    defaultSettings = {
        detailMessage : "This is an application exception."
    };

    TypeRegistrySingleton.getInstance().addType(
            "joynr.exceptions.ApplicationException",
            ApplicationException);

    ApplicationException.prototype = new Error();
    ApplicationException.prototype.constructor = ApplicationException;
    ApplicationException.prototype.name = "ApplicationException";

    return ApplicationException;

});
