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
package io.joynr.runtime;

import java.util.Set;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

import io.joynr.messaging.NoBackendGlobalAddressFactory;
import io.joynr.messaging.routing.GlobalAddressFactory;
import io.joynr.messaging.routing.TransportReadyListener;
import joynr.system.RoutingTypes.Address;

public class GlobalAddressProvider implements Provider<Address> {
    public static final String GLOBAL_ADDRESS_FACTORIES = "global_address_factories";

    private GlobalAddressFactory<? extends Address> addressFactory;

    @Inject
    public GlobalAddressProvider(@Named(GLOBAL_ADDRESS_FACTORIES) Set<GlobalAddressFactory<? extends Address>> addressFactories) {
        if (addressFactories.size() == 1) {
            addressFactory = addressFactories.iterator().next();
        } else if (addressFactories.size() == 0) {
            addressFactory = new NoBackendGlobalAddressFactory();
        } else {
            throw new IllegalStateException("Multiple global transports were registered");
        }
    }

    public void registerGlobalAddressesReadyListener(TransportReadyListener listener) {
        addressFactory.registerGlobalAddressReady(listener);
    }

    @Override
    public Address get() {
        try {
            return addressFactory.create();
        } catch (IllegalStateException e) {
            return null;
        }
    }
}
