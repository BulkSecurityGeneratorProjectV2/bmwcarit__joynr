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

define("joynr/messaging/browser/BrowserMessagingSkeleton", [
    "joynr/messaging/JoynrMessage",
    "joynr/util/UtilInternal",
    "joynr/util/JSONSerializer",
    "joynr/system/LoggerFactory"
], function(JoynrMessage, Util, JSONSerializer, LoggerFactory) {

    /**
     * @constructor BrowserMessagingSkeleton
     *
     * @param {Object} settings
     * @param {WebMessagingSkeleton} settings.webMessagingSkeleton a web messaging skeleton receiving web messages
     */
    function BrowserMessagingSkeleton(settings) {
        var log = LoggerFactory.getLogger("joynr/messaging/browser/BrowserMessagingSkeleton");
        Util.checkProperty(settings, "Object", "settings");
        Util.checkProperty(settings.webMessagingSkeleton, Object, "settings.webMessagingSkeleton");

        var receiverCallbacks = [];

        settings.webMessagingSkeleton.registerListener(function(message) {
            if (message !== undefined) {
                var joynrMessage = new JoynrMessage(message);

                Util.fire(receiverCallbacks, joynrMessage);
            } else {
                log.warn("message with content \""
                    + JSONSerializer.stringify(message)
                    + "\" could not be processed");
            }
        });

        /**
         * Registers the listener function
         *
         * @function BrowserMessagingSkeleton#registerListener
         *
         * @param {Function} listener a listener function that should be added and should receive messages
         */
        this.registerListener = function(listener) {
            Util.checkProperty(listener, "Function", "listener");

            receiverCallbacks.push(listener);
        };

        /**
         * Unregisters the listener function
         *
         * @function BrowserMessagingSkeleton#unregisterListener
         *
         * @param {Function} listener the listener function that should re removed and shouldn't receive messages any more
         */
        this.unregisterListener = function(listener) {
            Util.checkProperty(listener, "Function", "listener");

            Util.removeElementFromArray(receiverCallbacks, listener);
        };

    }

    return BrowserMessagingSkeleton;

});