/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
import * as WebSocketAddress from "../../../generated/joynr/system/RoutingTypes/WebSocketAddress";
/*eslint no-unused-vars: "off"*/
import * as Typing from "../../util/Typing";
import JoynrMessage = require("../JoynrMessage");

class WebSocketMulticastAddressCalculator {
    private settings: {
        globalAddress: WebSocketAddress;
    };

    /**
     * @constructor WebSocketMulticastAddressCalculator
     * @param settings
     * @param settings.globalAddress
     */
    public constructor(settings: { globalAddress: WebSocketAddress }) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.globalAddress, "WebSocketAddress", "settings.globalAddress");

        this.settings = settings;
    }

    /**
     * Calculates the multicast address for the submitted joynr message
     *
     * @param _message
     * @return the multicast address
     */
    public calculate(_message: JoynrMessage): WebSocketAddress {
        return this.settings.globalAddress;
    }
}

export = WebSocketMulticastAddressCalculator;
