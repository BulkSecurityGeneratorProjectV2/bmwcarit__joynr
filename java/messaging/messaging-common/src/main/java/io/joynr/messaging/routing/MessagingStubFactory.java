package io.joynr.messaging.routing;

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

import java.util.Map;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.IMessaging;
import joynr.system.RoutingTypes.Address;

@Singleton
public class MessagingStubFactory {

    public static final String MIDDLEWARE_MESSAGING_STUB_FACTORIES = "MIDDLEWARE_MESSAGING_STUB_FACTORIES";
    private Map<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessaging, ? extends Address>> middlewareMessagingStubFactories;

    @Inject
    public MessagingStubFactory(@Named(MIDDLEWARE_MESSAGING_STUB_FACTORIES) Map<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessaging, ? extends Address>> middlewareMessagingStubFactories) {
        this.middlewareMessagingStubFactories = middlewareMessagingStubFactories;
    }

    public IMessaging create(Address address) {
        @SuppressWarnings("unchecked")
        AbstractMiddlewareMessagingStubFactory<? extends IMessaging, Address> messagingStubFactory = (AbstractMiddlewareMessagingStubFactory<? extends IMessaging, Address>) middlewareMessagingStubFactories.get(address.getClass());

        if (messagingStubFactory == null) {
            throw new JoynrMessageNotSentException("Failed to send Request: Address type not supported: "
                    + address.getClass().getName());
        }
        return messagingStubFactory.create(address);
    }

    public void shutdown() {
        for (AbstractMiddlewareMessagingStubFactory<? extends IMessaging, ? extends Address> messagingStubFactory : middlewareMessagingStubFactories.values()) {
            messagingStubFactory.shutdown();
        }
    }

    public void register(Class<? extends Address> address,
                         AbstractMiddlewareMessagingStubFactory<? extends IMessaging, ? extends Address> middlewareMessagingStubFactory) {
        middlewareMessagingStubFactories.put(address, middlewareMessagingStubFactory);
    }
}
