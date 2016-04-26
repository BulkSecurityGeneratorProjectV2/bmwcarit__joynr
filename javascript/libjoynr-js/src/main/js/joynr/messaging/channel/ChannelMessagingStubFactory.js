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

define(
        "joynr/messaging/channel/ChannelMessagingStubFactory",
        [ "joynr/messaging/channel/ChannelMessagingStub"
        ],
        function(ChannelMessagingStub) {

            /**
             * @constructor
             * @name ChannelMessagingStubFactory
             *
             * @param {Object} settings
             * @param {ChannelMessagingSender|Object} settings.channelMessagingSender
             */
            function ChannelMessagingStubFactory(settings) {
                var globalAddress;

                /**
                 * This method is called when the global address has been created
                 *
                 * @function
                 * @name CapabilityDiscovery#globalAddressReady
                 *
                 * @param {Address}
                 *            globalAddress the address used to register discovery entries globally
                 */
                this.globalAddressReady = function globalAddressReady(newGlobalAddress) {
                    globalAddress = newGlobalAddress;
                };

                /**
                 * @name ChannelMessagingStubFactory#build
                 * @function
                 *
                 * @param {ChannelAddress} address the address to generate a messaging stub for
                 */
                this.build = function build(address) {
                    if (!globalAddress) {
                        var error = new Error("global channel address not yet set");
                        error.delay = true;
                        throw error;
                    }
                    return new ChannelMessagingStub({
                        destinationChannelAddress : address,
                        channelMessagingSender : settings.channelMessagingSender,
                        myChannelAddress : globalAddress
                    });
                };
            }

            return ChannelMessagingStubFactory;

        });