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

import java.text.DateFormat;
import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.inject.Inject;
import javax.inject.Singleton;

import com.google.inject.name.Named;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessaging;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MessageRouterImpl implements MessageRouter {
    private static final long TERMINATION_TIMEOUT = 5000;

    private Logger logger = LoggerFactory.getLogger(MessageRouterImpl.class);
    private final RoutingTable routingTable;
    private static final int UUID_TAIL = 32;
    private static final DateFormat DateFormatter = new SimpleDateFormat("dd/MM HH:mm:ss:sss");
    private ScheduledExecutorService scheduler;
    private long sendMsgRetryIntervalMs;
    private MessagingStubFactory messagingStubFactory;
    private AddressManager addressManager;
    protected final MulticastReceiverRegistry multicastReceiverRegistry;

    @Inject
    @Singleton
    public MessageRouterImpl(RoutingTable routingTable,
                             @Named(SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduler,
                             @Named(ConfigurableMessagingSettings.PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                             MessagingStubFactory messagingStubFactory,
                             AddressManager addressManager,
                             MulticastReceiverRegistry multicastReceiverRegistry) {
        this.routingTable = routingTable;
        this.scheduler = scheduler;
        this.sendMsgRetryIntervalMs = sendMsgRetryIntervalMs;
        this.messagingStubFactory = messagingStubFactory;
        this.addressManager = addressManager;
        this.multicastReceiverRegistry = multicastReceiverRegistry;
    }

    @Override
    public void removeNextHop(String participantId) {
        routingTable.remove(participantId);
    }

    @Override
    public boolean resolveNextHop(String participantId) {
        return routingTable.containsKey(participantId);
    }

    @Override
    public void addMulticastReceiver(String multicastId, String subscriberParticipantId, String providerParticipantId) {
        multicastReceiverRegistry.registerMulticastReceiver(multicastId, subscriberParticipantId);
    }

    @Override
    public void removeMulticastReceiver(String multicastId, String subscriberParticipantId, String providerParticipantId) {
        multicastReceiverRegistry.unregisterMulticastReceiver(multicastId, subscriberParticipantId);
    }

    @Override
    public void addNextHop(String participantId, Address address) {
        routingTable.put(participantId, address);
    }

    @Override
    public void route(final JoynrMessage message) {
        checkExpiry(message);
        routeInternal(message, 0, 0);
    }

    protected void schedule(Runnable runnable, String messageId, long delay, TimeUnit timeUnit) {
        if (scheduler.isShutdown()) {
            JoynrShutdownException joynrShutdownEx = new JoynrShutdownException("MessageScheduler is shutting down already. Unable to send message [messageId: "
                    + messageId + "].");
            throw joynrShutdownEx;
        }
        scheduler.schedule(runnable, delay, timeUnit);
    }

    protected Address getAddress(JoynrMessage message) {
        return addressManager.getAddress(message);
    }

    private void routeInternal(final JoynrMessage message, final long delayMs, final int retriesCount) {
        try {
            logger.debug("Scheduling {} with delay {} and retries {}", new Object[]{ message, delayMs, retriesCount });
            schedule(new Runnable() {
                @Override
                public void run() {
                    logger.debug("Starting processing of message {}", message);
                    try {
                        checkExpiry(message);
                        Address address = getAddress(message);
                        String messageId = message.getId().substring(UUID_TAIL);
                        logger.info(">>>>> SEND  ID:{}:{} from: {} to: {} header: {}", new String[]{ messageId,
                                message.getType(),
                                message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                                message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID),
                                message.getHeader().toString() });
                        logger.debug(">>>>> body  ID:{}:{}: {}", new String[]{ messageId, message.getType(),
                                message.getPayload() });
                        IMessaging messagingStub = messagingStubFactory.create(address);
                        messagingStub.transmit(message, createFailureAction(message, retriesCount));
                    } catch (Exception error) {
                        logger.error("error in scheduled message router thread: {}", error.getMessage());
                        FailureAction failureAction = createFailureAction(message, retriesCount);
                        failureAction.execute(error);
                    }
                }
            }, message.getId(), delayMs, TimeUnit.MILLISECONDS);
        } catch (RejectedExecutionException e) {
            logger.error("Execution rejected while scheduling SendSerializedMessageRequest ", e);
            throw new JoynrSendBufferFullException(e);
        }
    }

    private void checkExpiry(final JoynrMessage message) {
        long currentTimeMillis = System.currentTimeMillis();
        long ttlExpirationDateMs = message.getExpiryDate();

        if (ttlExpirationDateMs <= currentTimeMillis) {
            String errorMessage = MessageFormat.format("ttl must be greater than 0 / ttl timestamp must be in the future: now: {0} abs_ttl: {1}",
                                                       currentTimeMillis,
                                                       ttlExpirationDateMs);
            logger.error(errorMessage);
            throw new JoynrMessageNotSentException(errorMessage);
        }
    }

    private FailureAction createFailureAction(final JoynrMessage message, final int retriesCount) {
        final FailureAction failureAction = new FailureAction() {
            final String messageId = message.getId();

            @Override
            public void execute(Throwable error) {
                if (error instanceof JoynrShutdownException) {
                    logger.warn("{}", error.getMessage());
                    return;
                } else if (error instanceof JoynrMessageNotSentException) {
                    logger.error(" ERROR SENDING:  aborting send of messageId: {}. Error: {}", new Object[]{ messageId,
                            error.getMessage() });
                    return;
                }
                logger.warn("PROBLEM SENDING, will retry. messageId: {}. Error: {} Message: {}", new Object[]{
                        messageId, error.getClass().getName(), error.getMessage() });

                long delayMs;
                if (error instanceof JoynrDelayMessageException) {
                    delayMs = ((JoynrDelayMessageException) error).getDelayMs();
                } else {
                    delayMs = sendMsgRetryIntervalMs;
                    delayMs += exponentialBackoff(delayMs, retriesCount);
                }

                try {
                    logger.error("Rescheduling messageId: {} with delay " + delayMs
                                         + " ms, new TTL expiration date: {}",
                                 messageId,
                                 DateFormatter.format(message.getExpiryDate()));
                    routeInternal(message, delayMs, retriesCount + 1);
                    return;
                } catch (JoynrSendBufferFullException e) {
                    try {
                        logger.error("Rescheduling message: {} delayed {} ms because send buffer is full",
                                     delayMs,
                                     messageId);
                        Thread.sleep(delayMs);
                        this.execute(e);
                    } catch (InterruptedException e1) {
                        Thread.currentThread().interrupt();
                        return;
                    }
                }
            }
        };
        return failureAction;
    }

    @Override
    public void shutdown() {
        scheduler.shutdown();
        try {
            scheduler.awaitTermination(TERMINATION_TIMEOUT, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            logger.error("Message Scheduler did not shut down in time: {}", e.getMessage());
        }
    }

    private long exponentialBackoff(long delayMs, int retries) {
        logger.debug("TRIES: " + retries);
        long millis = delayMs + (long) ((2 ^ (retries)) * delayMs * Math.random());
        logger.debug("MILLIS: " + millis);
        return millis;
    }

}
