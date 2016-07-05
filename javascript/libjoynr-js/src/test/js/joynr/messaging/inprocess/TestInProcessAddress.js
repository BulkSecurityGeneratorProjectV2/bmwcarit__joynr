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

define([ "joynr/messaging/inprocess/InProcessAddress"
], function(InProcessAddress) {

    describe("libjoynr-js.joynr.messaging.inprocess.InProcessAddress", function() {
        var skeleton, inProcessAddress;

        beforeEach(function() {
            skeleton = {
                key : "skeleton"
            };
            inProcessAddress = new InProcessAddress(skeleton);
        });

        it("is instantiable and of correct type", function() {
            expect(InProcessAddress).toBeDefined();
            expect(typeof InProcessAddress === "function").toBeTruthy();
            expect(inProcessAddress).toBeDefined();
            expect(inProcessAddress instanceof InProcessAddress).toBeTruthy();
            expect(inProcessAddress.getSkeleton).toBeDefined();
            expect(typeof inProcessAddress.getSkeleton === "function").toBeTruthy();
        });

        it("retrieves skeleton correctly", function() {
            expect(inProcessAddress.getSkeleton()).toEqual(skeleton);
        });

    });

});
