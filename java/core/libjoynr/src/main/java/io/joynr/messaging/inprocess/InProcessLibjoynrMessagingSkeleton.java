package io.joynr.messaging.inprocess;

import com.google.inject.Inject;

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

import io.joynr.dispatching.Dispatcher;
import io.joynr.messaging.FailureAction;
import joynr.JoynrMessage;

public class InProcessLibjoynrMessagingSkeleton implements InProcessMessagingSkeleton {

    private final Dispatcher dispatcher;

    @Inject
    public InProcessLibjoynrMessagingSkeleton(Dispatcher dispatcher) {
        this.dispatcher = dispatcher;
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        try {
            transmit(message);
        } catch (Exception exception) {
            failureAction.execute(exception);
        }
    }

    @Override
    public void transmit(JoynrMessage message) {
        dispatcher.messageArrived(message);
    }

    @Override
    public void transmit(String serializedMessage, FailureAction failureAction) {
        throw new IllegalStateException("InProcessMessagingSkeleton does not handle serialized messages");
    }

    @Override
    public void init() {
        //do nothing
    }

    @Override
    public void shutdown() {
        //do nothing
    }
}
