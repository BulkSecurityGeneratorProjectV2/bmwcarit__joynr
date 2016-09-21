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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.runtime.SystemServicesSettings;
import joynr.JoynrMessage;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingProxy;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BrowserAddress;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.CommonApiDbusAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * MessageRouter implementation which adds hops to its parent and tries to resolve unknown addresses at its parent
 */
 @Singleton
public class ChildMessageRouter extends MessageRouterImpl {
     private Logger logger = LoggerFactory.getLogger(ChildMessageRouter.class);

    private static interface DeferrableRegistration {
        void register();
    }

    private Address parentRouterMessagingAddress;
    private RoutingProxy parentRouter;
    private Address incomingAddress;
    private Set<String> deferredParentHopsParticipantIds = new HashSet<>();
    private Map<String, DeferrableRegistration> deferredMulticastRegististrations = new HashMap<>();


    @Inject
    public ChildMessageRouter(RoutingTable routingTable,
                              @Named(SystemServicesSettings.LIBJOYNR_MESSAGING_ADDRESS) Address incomingAddress,
                              @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                              @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                              MessagingStubFactory messagingStubFactory, AddressManager addressManager, MulticastReceiverRegistry multicastReceiverRegistry) {
        super(routingTable, scheduler, sendMsgRetryIntervalMs, messagingStubFactory, addressManager, multicastReceiverRegistry);
        this.incomingAddress = incomingAddress;
    }

    @Override
    protected Address getAddress(JoynrMessage message) {
        Address address;
        JoynrMessageNotSentException noAddressException = null;
        try {
            address = super.getAddress(message);
        } catch (JoynrMessageNotSentException e) {
            noAddressException = e;
            address = null;
        }
        String toParticipantId = message.getTo();
        if (address == null && parentRouter != null) {
            Boolean parentHasNextHop = parentRouter.resolveNextHop(toParticipantId);
            if (parentHasNextHop) {
                super.addNextHop(toParticipantId, parentRouterMessagingAddress);
                address = parentRouterMessagingAddress;
            }
        }
        if (address == null && noAddressException != null) {
            throw noAddressException;
        }
        return address;
    }

    @Override
    public void addNextHop(final String participantId, final Address address) {
        super.addNextHop(participantId, address);
        if (parentRouter != null) {
            addNextHopToParent(participantId);
        } else {
            deferredParentHopsParticipantIds.add(participantId);
        }
    }

    private void addNextHopToParent(String participantId) {
        logger.debug("Adding next hop with participant id " + participantId + " to parent router");
        if (incomingAddress instanceof ChannelAddress) {
            parentRouter.addNextHop(participantId, (ChannelAddress) incomingAddress);
        } else if (incomingAddress instanceof CommonApiDbusAddress) {
            parentRouter.addNextHop(participantId, (CommonApiDbusAddress) incomingAddress);
        } else if (incomingAddress instanceof BrowserAddress) {
            parentRouter.addNextHop(participantId, (BrowserAddress) incomingAddress);
        } else if (incomingAddress instanceof WebSocketAddress) {
            parentRouter.addNextHop(participantId, (WebSocketAddress) incomingAddress);
        } else if (incomingAddress instanceof WebSocketClientAddress) {
            parentRouter.addNextHop(participantId, (WebSocketClientAddress) incomingAddress);
        } else {
            throw new ProviderRuntimeException("Failed to add next hop to parent: unknown address type"
                    + incomingAddress.getClass().getSimpleName());
        }
    }

    @Override
    public void addMulticastReceiver(final String multicastId, final String subscriberParticipantId, final String providerParticipantId) {
        super.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        DeferrableRegistration registerWithParent = new DeferrableRegistration() {
            @Override
            public void register() {
                parentRouter.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
            }
        };
        if (parentRouter != null) {
            registerWithParent.register();
        } else {
            synchronized (deferredMulticastRegististrations) {
                deferredMulticastRegististrations.put(multicastId + subscriberParticipantId + providerParticipantId,
                    registerWithParent);
            }
        }
    }

    @Override
    public void removeMulticastReceiver(String multicastId, String subscriberParticipantId,
        String providerParticipantId) {
        super.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        if (parentRouter == null) {
            synchronized (deferredMulticastRegististrations) {
                deferredMulticastRegististrations.remove(multicastId + subscriberParticipantId + providerParticipantId);
            }
        }
    }

    public void setParentRouter(RoutingProxy parentRouter,
                                Address parentRouterMessagingAddress,
                                String parentRoutingProviderParticipantId,
                                String routingProxyParticipantId) {
        this.parentRouter = parentRouter;
        this.parentRouterMessagingAddress = parentRouterMessagingAddress;

        super.addNextHop(parentRoutingProviderParticipantId, parentRouterMessagingAddress);
        addNextHopToParent(routingProxyParticipantId);
        for (String participantIds : deferredParentHopsParticipantIds) {
            addNextHopToParent(participantIds);
        }
        synchronized (deferredMulticastRegististrations) {
            for (DeferrableRegistration registerWithParent : deferredMulticastRegististrations.values()) {
                registerWithParent.register();
            }
        }
        deferredParentHopsParticipantIds.clear();
    }

    /**
     * Sets the address which will be registered at the parent router for the next hop
     * to contact this child message router
     * @param incomingAddress address of this libjoynr instance. Used by the cluster controller's
     *                        message router to forward messages
     */
    public void setIncomingAddress(Address incomingAddress) {
        this.incomingAddress = incomingAddress;
    }
}
