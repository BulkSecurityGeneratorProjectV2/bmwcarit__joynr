/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import MessagingSkeletonFactory from "joynr/joynr/messaging/MessagingSkeletonFactory";

import BrowserAddress from "joynr/generated/joynr/system/RoutingTypes/BrowserAddress";
import MqttAddress from "joynr/generated/joynr/system/RoutingTypes/MqttAddress";
import InProcessAddress from "joynr/joynr/messaging/inprocess/InProcessAddress";

describe("libjoynr-js.joynr.messaging.MessagingSkeletonFactory", () => {
    let messagingSkeletonFactory: MessagingSkeletonFactory;
    let mqttMessagingSkeleton: any, inProcessMessagingSkeleton: any;

    beforeEach(() => {
        mqttMessagingSkeleton = {
            shutdown: jest.fn()
        };
        inProcessMessagingSkeleton = {
            shutdown: jest.fn()
        };
        messagingSkeletonFactory = new MessagingSkeletonFactory();
        const messagingSkeletons: Record<string, any> = {};
        messagingSkeletons[InProcessAddress._typeName] = inProcessMessagingSkeleton;
        messagingSkeletons[MqttAddress._typeName] = mqttMessagingSkeleton;
        messagingSkeletonFactory.setSkeletons(messagingSkeletons);
    });

    it("provides expected API", () => {
        expect(MessagingSkeletonFactory).toBeDefined();
        expect(messagingSkeletonFactory).toBeDefined();
        expect(messagingSkeletonFactory).toBeInstanceOf(MessagingSkeletonFactory);
        expect(messagingSkeletonFactory.getSkeleton).toBeDefined();
    });

    it("returns the appropriate messaging skeleton depending on object type", () => {
        expect(messagingSkeletonFactory.getSkeleton(new MqttAddress(undefined as any))).toBe(mqttMessagingSkeleton);
        expect(messagingSkeletonFactory.getSkeleton(new InProcessAddress(undefined as any))).toBe(
            inProcessMessagingSkeleton
        );
    });

    it("throws exception if address type is unknown", () => {
        expect(() => {
            messagingSkeletonFactory.getSkeleton(new BrowserAddress(undefined as any));
        }).toThrow();
    });
});
