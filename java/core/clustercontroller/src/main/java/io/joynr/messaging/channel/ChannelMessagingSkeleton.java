package io.joynr.messaging.channel;

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

import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.MessageArrivedListener;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.messaging.routing.MessageRouter;

import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.RoutingTypesUtil;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

public class ChannelMessagingSkeleton implements IMessagingSkeleton {
    private final MessageRouter messageRouter;

    private static final Logger logger = LoggerFactory.getLogger(ChannelMessagingSkeleton.class);

    private MessageReceiver messageReceiver;

    @Inject
    public ChannelMessagingSkeleton(MessageRouter messageRouter, MessageReceiver messageReceiver) {
        this.messageRouter = messageRouter;
        this.messageReceiver = messageReceiver;
    }

    @Override
    public void transmit(JoynrMessage message, FailureAction failureAction) {
        final String replyToChannelId = message.getHeaderValue(JoynrMessage.HEADER_NAME_REPLY_CHANNELID);
        try {
            addRequestorToMessageRouter(message.getFrom(), replyToChannelId);
            messageRouter.route(message);
        } catch (JoynrSendBufferFullException | JoynrMessageNotSentException exception) {
            logger.error("Error processing incoming message. Message will be dropped: {} ", message.getHeader(), exception);
            failureAction.execute(exception);
        }
    }

    @Override
    public void transmit(String serializedMessage, FailureAction failureAction) {
        // TODO Auto-generated method stub

    }

    private void addRequestorToMessageRouter(String requestorParticipantId, String replyToSerializedAddress) {
        if (replyToSerializedAddress != null && !replyToSerializedAddress.isEmpty()) {
            Address address;
            address = RoutingTypesUtil.fromAddressString(replyToSerializedAddress);
            messageRouter.addNextHop(requestorParticipantId, address);
        } else {
            /*
             * TODO make sure that all requests (ie not one-way) also have replyTo
             * set, otherwise log an error.
             * Caution: the replyToChannelId is not set in case of local communication
             */
        }
    }

    @Override
    public void init() {
        messageReceiver.start(new MessageArrivedListener() {

            @Override
            public void messageArrived(final JoynrMessage message) {
                transmit(message, new FailureAction() {
                    @Override
                    public void execute(Throwable error) {
                        logger.error("error processing incoming message: {} error: {}",
                                     message.getId(),
                                     error.getMessage());
                    }
                });
            }

            @Override
            public void error(JoynrMessage message, Throwable error) {
                logger.error("error receiving incoming message: {} error: {}", message.getId(), error.getMessage());
            }
        }, new ReceiverStatusListener() {

            @Override
            public void receiverStarted() {

            }

            @Override
            public void receiverException(Throwable e) {
                logger.error("error in long polling message receiver error: {}", e.getMessage());
                shutdown();
            }
        });
    }

    @Override
    public void shutdown() {
        messageReceiver.shutdown(false);
    }

}
