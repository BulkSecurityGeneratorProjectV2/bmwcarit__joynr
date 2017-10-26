/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef TESTS_MOCK_MOCKPUBLICATIONSENDER_H
#define TESTS_MOCK_MOCKPUBLICATIONSENDER_H

#include <gmock/gmock.h>

#include "joynr/IPublicationSender.h"

class MockPublicationSender : public joynr::IPublicationSender {
public:
    MOCK_METHOD4(
            sendSubscriptionPublicationMock,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionPublication& subscriptionPublication
            )
    );

    MOCK_METHOD4(
            sendSubscriptionReply,
            void(
                const std::string& senderParticipantId,
                const std::string& receiverParticipantId,
                const joynr::MessagingQos& qos,
                const joynr::SubscriptionReply& subscriptionReply
            )
    );

    void sendSubscriptionPublication(
        const std::string& senderParticipantId,
        const std::string& receiverParticipantId,
        const joynr::MessagingQos& qos,
        joynr::SubscriptionPublication&& subscriptionPublication
    ){
        sendSubscriptionPublicationMock(senderParticipantId,receiverParticipantId,qos,subscriptionPublication);
    }
};

#endif // TESTS_MOCK_MOCKPUBLICATIONSENDER_H
