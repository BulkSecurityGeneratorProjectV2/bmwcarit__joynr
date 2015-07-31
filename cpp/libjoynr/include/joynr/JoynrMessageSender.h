/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#ifndef JOYNRMESSAGESENDER_H
#define JOYNRMESSAGESENDER_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include "joynr/IJoynrMessageSender.h"
#include "joynr/JoynrMessageFactory.h"
#include <string>

namespace joynr
{

class MessageRouter;

/**
  * The class JoynrMessageSender enables the exchange of JoynrMessages
  * between the clusterController and libJoynr. It is used by both.
  * It uses a JoynrMessage factory to create a JoynrMessage
  * and sends it via a <Middleware>MessagingStub.
  */

/*
  * JoynrMessageSender needs an Dispatcher, and Dispatcher needs a JoynrMessageSender.
  * This is the case, because the MessageSender needs access to the callerDirectory (via
  * the dispatcher) to store the requestReplyId to the caller). The Dispatcher needs the
  * JoynrMessageSender to send the replies.
  * To break this circle, the JoynrMessageSender is created without Dispatcher* and the Dispatcher
  * is later registered with the JoynrMessageSender. This is a temporary workaround, which can be
  * like this:
  *     Once the JoynrMessage does not have the Reply/RequestId in the header, the dispatcher
  *     will use the participantId to deliver the message to the caller. The caller will then
  *     take the reply/request-ID from the payload and handle it accordingly.
  *     Now the proxy can register the reply-caller with the Dispatcher,
  *     include the reply/requestId into the payload, and pass the payload to the JoynrMessageSender
  *     The MessageSender does not need to register anything with the dispatcher, and thus needs
  *     No reference to the dispatcher.
  */

class JOYNR_EXPORT JoynrMessageSender : public IJoynrMessageSender
{
public:
    JoynrMessageSender(QSharedPointer<MessageRouter> messagingRouter);

    virtual ~JoynrMessageSender();

    /*
      * registers Dispatcher. See above comment why this is necessary.
      */
    void registerDispatcher(IDispatcher* dispatcher);

    virtual void sendRequest(const std::string& senderParticipantId,
                             const std::string& receiverParticipantId,
                             const MessagingQos& qos,
                             const Request& request,
                             QSharedPointer<IReplyCaller> callback);
    /*
     * Prepares and sends a reply message (an answer to a request)
     */
    virtual void sendReply(const std::string& senderParticipantId,
                           const std::string& receiverParticipantId,
                           const MessagingQos& qos,
                           const Reply& reply);

    virtual void sendSubscriptionRequest(const std::string& senderParticipantId,
                                         const std::string& receiverParticipantId,
                                         const MessagingQos& qos,
                                         const SubscriptionRequest& subscriptionRequest);

    virtual void sendBroadcastSubscriptionRequest(
            const std::string& senderParticipantId,
            const std::string& receiverParticipantId,
            const MessagingQos& qos,
            const BroadcastSubscriptionRequest& subscriptionRequest);

    virtual void sendSubscriptionReply(const std::string& senderParticipantId,
                                       const std::string& receiverParticipantId,
                                       const MessagingQos& qos,
                                       const SubscriptionReply& subscriptionReply);

    virtual void sendSubscriptionStop(const std::string& senderParticipantId,
                                      const std::string& receiverParticipantId,
                                      const MessagingQos& qos,
                                      const SubscriptionStop& subscriptionStop);

    virtual void sendSubscriptionPublication(
            const std::string& senderParticipantId,
            const std::string& receiverParticipantId,
            const MessagingQos& qos,
            const SubscriptionPublication& subscriptionPublication);

private:
    DISALLOW_COPY_AND_ASSIGN(JoynrMessageSender);
    IDispatcher* dispatcher;
    QSharedPointer<MessageRouter> messageRouter;
    JoynrMessageFactory messageFactory;
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // JOYNRMESSAGESENDER_H
