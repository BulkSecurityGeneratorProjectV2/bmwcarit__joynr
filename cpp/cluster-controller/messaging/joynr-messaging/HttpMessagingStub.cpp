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
#include "HttpMessagingStub.h"

#include "joynr/IMessageSender.h"
#include "joynr/MessagingQos.h"
#include "joynr/JoynrMessage.h"

namespace joynr
{

INIT_LOGGER(HttpMessagingStub);

HttpMessagingStub::HttpMessagingStub(std::shared_ptr<IMessageSender> messageSender,
                                     const system::RoutingTypes::ChannelAddress& destinationAddress,
                                     const std::string& globalClusterControllerAddress)
        : messageSender(messageSender),
          destinationAddress(destinationAddress),
          globalClusterControllerAddress(globalClusterControllerAddress)
{
}

void HttpMessagingStub::transmit(
        JoynrMessage& message,
        const std::function<void(const exceptions::JoynrRuntimeException&)>& onFailure)
{
    if (message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_REQUEST ||
        message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
        message.getType() == JoynrMessage::VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) {
        message.setHeaderReplyAddress(globalClusterControllerAddress);
    }
    JOYNR_LOG_DEBUG(logger, ">>> OUTGOING >>> {}", message.toLogMessage());
    messageSender->sendMessage(destinationAddress, message, onFailure);
}

} // namespace joynr
