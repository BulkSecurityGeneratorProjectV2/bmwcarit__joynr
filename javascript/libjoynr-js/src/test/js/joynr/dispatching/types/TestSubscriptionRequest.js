/*global joynrTestRequire: true */

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

joynrTestRequire("joynr/dispatching/types/TestSubscriptionRequest", [
    "joynr/dispatching/types/SubscriptionRequest",
    "joynr/proxy/PeriodicSubscriptionQos",
    "joynr/proxy/OnChangeWithKeepAliveSubscriptionQos"
], function(SubscriptionRequest, PeriodicSubscriptionQos) {

    describe("libjoynr-js.joynr.dispatching.types.SubscriptionRequest", function() {

        var qosSettings = {
            period : 50,
            expiryDateMs : 3,
            alertAfterInterval : 80,
            publicationTtl : 100
        };

        it("is defined", function() {
            expect(SubscriptionRequest).toBeDefined();
        });

        it("is instantiable", function() {
            var subscriptionRequest = new SubscriptionRequest({
                subscribedToName : "attributeName",
                subscriptionId : "testSubscriptionId"
            });
            expect(subscriptionRequest).toBeDefined();
            expect(subscriptionRequest).not.toBeNull();
            expect(typeof subscriptionRequest === "object").toBeTruthy();
            expect(subscriptionRequest instanceof SubscriptionRequest).toBeTruthy();
        });

        it("handles missing parameters correctly", function() {
            // does not throw, with qos
            expect(function() {
                var subReq = new SubscriptionRequest({
                    subscribedToName : "attributeName",
                    subscriptionId : "testSubscriptionId",
                    subscriptionQos : new PeriodicSubscriptionQos(qosSettings)
                });
            }).not.toThrow();

            // does not throw, without qos
            expect(function() {
                var subReq = new SubscriptionRequest({
                    subscribedToName : "attributeName",
                    subscriptionId : "testSubscriptionId"
                });
            }).not.toThrow();

            // throws on wrongly typed attributeName
            expect(function() {
                var subReq = new SubscriptionRequest({
                    subscribedToName : {},
                    subscriptionId : "testSubscriptionId"
                });
            }).toThrow();

            // throws on missing attributeName
            expect(function() {
                var subReq = new SubscriptionRequest({
                    subscriptionId : "testSubscriptionId"
                });
            }).toThrow();

            // throws on missing subscriptionId
            expect(function() {
                var subReq = new SubscriptionRequest({
                    subscribedToName : "attributeName",
                    subscriptionQos : new PeriodicSubscriptionQos(qosSettings)
                });
            }).toThrow();

            // throws on missing settings object type
            expect(function() {
                var subReq = new SubscriptionRequest();
            }).toThrow();

            // throws on wrong settings object type
            expect(function() {
                var subReq = new SubscriptionRequest("wrong type");
            }).toThrow();

            // throws on incorrect qos
            // expect(function() {
            // var subReq = new SubscriptionRequest({
            // subscribedToName : "attributeName",
            // subscriptionId : "testSubscriptionId",
            // qos : {}
            // });
            // }).toThrow();
        });

        it("is constructs with correct member values", function() {
            var subscribedToName = "attributeName";
            var subscriptionQos = new PeriodicSubscriptionQos(qosSettings);
            var subscriptionId = "testSubscriptionId";

            var subscriptionRequest = new SubscriptionRequest({
                subscribedToName : subscribedToName,
                subscriptionQos : subscriptionQos,
                subscriptionId : subscriptionId
            });

            expect(subscriptionRequest.subscribedToName).toEqual(subscribedToName);
            expect(subscriptionRequest.subscriptionQos).toEqual(subscriptionQos);
            expect(subscriptionRequest.subscriptionId).toEqual(subscriptionId);
        });

    });

}); // require
