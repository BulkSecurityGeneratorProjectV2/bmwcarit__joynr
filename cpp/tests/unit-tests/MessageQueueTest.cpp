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
#include <chrono>
#include <cstdint>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ImmutableMessage.h"
#include "joynr/MessageQueue.h"
#include "joynr/MutableMessage.h"
#include "joynr/PrivateCopyAssign.h"

#include "tests/JoynrTest.h"

using namespace joynr;

class MessageQueueTest : public ::testing::Test {
public:
    MessageQueueTest()
        : messageQueue(),
          expiryDate(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()) + std::chrono::milliseconds(100))
    {
    }

    ~MessageQueueTest() = default;

protected:
    MessageQueue<std::string> messageQueue;
    JoynrTimePoint expiryDate;

    void createAndQueueMessage(const JoynrTimePoint& expiryDate) {
        MutableMessage mutableMsg;
        mutableMsg.setExpiryDate(expiryDate);
        auto immutableMessage = mutableMsg.getImmutableMessage();
        auto recipient = immutableMessage->getRecipient();
        messageQueue.queueMessage(recipient, std::move(immutableMessage));
    }

private:
    DISALLOW_COPY_AND_ASSIGN(MessageQueueTest);
};

TEST_F(MessageQueueTest, initialQueueIsEmpty) {
    EXPECT_EQ(messageQueue.getQueueLength(), 0);
}

TEST_F(MessageQueueTest, addMultipleMessages) {
    createAndQueueMessage(expiryDate);
    EXPECT_EQ(1, messageQueue.getQueueLength());

    createAndQueueMessage(expiryDate);
    EXPECT_EQ(2, messageQueue.getQueueLength());

    createAndQueueMessage(expiryDate);
    EXPECT_EQ(3, messageQueue.getQueueLength());

    createAndQueueMessage(expiryDate);
    EXPECT_EQ(4, messageQueue.getQueueLength());
}

TEST_F(MessageQueueTest, removeExpiredMessages_AllMessagesExpired) {
    const JoynrTimePoint zeroTimepoint(std::chrono::milliseconds::zero());

    createAndQueueMessage(zeroTimepoint);
    createAndQueueMessage(zeroTimepoint);
    EXPECT_EQ(2, messageQueue.getQueueLength());

    messageQueue.removeOutdatedMessages();
    EXPECT_EQ(0, messageQueue.getQueueLength());
}

TEST_F(MessageQueueTest, removeExpiredMessages_SomeMessagesExpired) {
    const JoynrTimePoint zeroTimepoint(std::chrono::milliseconds::zero());

    createAndQueueMessage(zeroTimepoint);
    createAndQueueMessage(zeroTimepoint);
    createAndQueueMessage(expiryDate);
    createAndQueueMessage(expiryDate);
    EXPECT_EQ(4, messageQueue.getQueueLength());

    messageQueue.removeOutdatedMessages();
    EXPECT_EQ(2, messageQueue.getQueueLength());
}

TEST_F(MessageQueueTest, removeExpiredMessages_NoMessageExpired) {
    createAndQueueMessage(expiryDate);
    createAndQueueMessage(expiryDate);
    EXPECT_EQ(2, messageQueue.getQueueLength());

    messageQueue.removeOutdatedMessages();
    EXPECT_EQ(2, messageQueue.getQueueLength());
}

TEST_F(MessageQueueTest, queueDequeueMessages) {
    // add messages to the queue
    MutableMessage mutableMsg1;
    const std::string recipient1("TEST1");
    mutableMsg1.setRecipient(recipient1);
    mutableMsg1.setExpiryDate(expiryDate);
    auto immutableMsg1 = mutableMsg1.getImmutableMessage();
    messageQueue.queueMessage(recipient1, std::move(immutableMsg1));

    MutableMessage mutableMsg2;
    const std::string recipient2("TEST2");
    mutableMsg2.setRecipient(recipient2);
    mutableMsg2.setExpiryDate(expiryDate);
    auto immutableMsg2 = mutableMsg2.getImmutableMessage();
    messageQueue.queueMessage(recipient2, std::move(immutableMsg2));
    EXPECT_EQ(messageQueue.getQueueLength(), 2);

    // get messages from queue
    auto item = messageQueue.getNextMessageFor(recipient1);
    compareMutableImmutableMessage(mutableMsg1, item);
    EXPECT_EQ(messageQueue.getQueueLength(), 1);

    item = messageQueue.getNextMessageFor(recipient2);
    compareMutableImmutableMessage(mutableMsg2, item);
    EXPECT_EQ(messageQueue.getQueueLength(), 0);
}

TEST_F(MessageQueueTest, queueDequeueMultipleMessagesForOneParticipant) {
    // add messages to the queue
    MutableMessage mutableMessage;
    const std::string participantId("TEST");
    mutableMessage.setRecipient(participantId);
    mutableMessage.setExpiryDate(expiryDate);
    messageQueue.queueMessage(participantId, mutableMessage.getImmutableMessage());
    messageQueue.queueMessage(participantId, mutableMessage.getImmutableMessage());
    EXPECT_EQ(messageQueue.getQueueLength(), 2);

    // get messages from queue
    auto item = messageQueue.getNextMessageFor(participantId);
    compareMutableImmutableMessage(mutableMessage, item);
    EXPECT_EQ(messageQueue.getQueueLength(), 1);

    item = messageQueue.getNextMessageFor(participantId);
    compareMutableImmutableMessage(mutableMessage, item);
    EXPECT_EQ(messageQueue.getQueueLength(), 0);
}

TEST_F(MessageQueueTest, dequeueInvalidParticipantId) {
    EXPECT_EQ(messageQueue.getNextMessageFor("TEST"), nullptr);
}

class MessageQueueWithLimitTest : public ::testing::Test
{
public:
    MessageQueueWithLimitTest() { }
    ~MessageQueueWithLimitTest() = default;

protected:
    std::shared_ptr<ImmutableMessage> createMessage(const JoynrTimePoint& expiryDate, const std::string& recipient, const std::string& payload = "") {
        MutableMessage mutableMsg;
        mutableMsg.setExpiryDate(expiryDate);
        mutableMsg.setRecipient(recipient);
        mutableMsg.setPayload(payload);
        return mutableMsg.getImmutableMessage();
    }

    void createAndQueueMessage(MessageQueue<std::string>& queue, const JoynrTimePoint& expiryDate, const std::string& recipient, const std::string& payload = "") {
        queue.queueMessage(recipient, createMessage(expiryDate, recipient, payload));
    }

    std::string payloadAsString(std::shared_ptr<ImmutableMessage> message) {
        const smrf::ByteArrayView& byteArrayView = message->getUnencryptedBody();
        return std::string(reinterpret_cast<const char*>(byteArrayView.data()), byteArrayView.size());
    }

private:
    DISALLOW_COPY_AND_ASSIGN(MessageQueueWithLimitTest);
};

JoynrTimePoint getExpiryDateFromNow(long long offset)
{
    return (std::chrono::time_point_cast<std::chrono::milliseconds>( std::chrono::system_clock::now()) + std::chrono::milliseconds(offset));
}

TEST_F(MessageQueueWithLimitTest, testAddingMessages)
{
    constexpr std::uint64_t messageQueueLimit = 4;
    MessageQueue<std::string> messageQueue(messageQueueLimit);

    const int messageCount = 5;
    // Keep in mind that message 1 expires later than message 3. This is done in order to check
    // if removal deletes the message with lowest ttl and not the first inserted message.
    const JoynrTimePoint expiryDate[messageCount] = {
        getExpiryDateFromNow(300),
        getExpiryDateFromNow(200),
        getExpiryDateFromNow(100),
        getExpiryDateFromNow(500),
        getExpiryDateFromNow(600)
    };

    const std::string recipient[messageCount] = {
        "TEST1",
        "TEST2",
        "TEST3",
        "TEST4",
        "TEST5"};

    for (int i=0; i < messageCount; i++) {
        this->createAndQueueMessage(messageQueue, expiryDate[i], recipient[i]);
    }

    EXPECT_EQ(messageQueue.getQueueLength(), messageQueueLimit);

    // Check if the message with the lowest TTL (message3) was removed.
    EXPECT_EQ(messageQueue.getNextMessageFor(recipient[0])->getRecipient(), recipient[0]);
    EXPECT_EQ(messageQueue.getNextMessageFor(recipient[1])->getRecipient(), recipient[1]);
    EXPECT_EQ(messageQueue.getNextMessageFor(recipient[2]), nullptr);
    EXPECT_EQ(messageQueue.getNextMessageFor(recipient[3])->getRecipient(), recipient[3]);
    EXPECT_EQ(messageQueue.getNextMessageFor(recipient[4])->getRecipient(), recipient[4]);
}

TEST_F(MessageQueueWithLimitTest, testPerKeyQueueLimit_lowestTtlRemoved)
{
    const std::string recipient1("recipient1");
    const std::string recipient2("recipient2");
    const std::string msgRecipient1Payload1("payload1.1");
    const std::string msgRecipient1Payload2("payload1.2");
    const std::string msgRecipient2Payload1("payload2.1");

    constexpr std::uint64_t messageQueueLimit = 10;
    constexpr std::uint64_t perKeyQueueLimit = 1;

    MessageQueue<std::string> queue(messageQueueLimit, perKeyQueueLimit);
    createAndQueueMessage(queue, getExpiryDateFromNow(1000), recipient1, msgRecipient1Payload1);
    createAndQueueMessage(queue, getExpiryDateFromNow(2000), recipient1, msgRecipient1Payload2);
    createAndQueueMessage(queue, getExpiryDateFromNow(1000), recipient2, msgRecipient2Payload1);

    EXPECT_EQ(2, queue.getQueueLength());

    auto recipient1Message = queue.getNextMessageFor(recipient1);
    auto recipient2Message = queue.getNextMessageFor(recipient2);

    EXPECT_NE(nullptr, recipient1Message);
    EXPECT_NE(nullptr, recipient2Message);

    // When the per-key queue is full, the entry with the lowest TTL shall be removed.
    EXPECT_EQ(msgRecipient1Payload2, payloadAsString(recipient1Message));
    EXPECT_EQ(msgRecipient2Payload1, payloadAsString(recipient2Message));
}

TEST_F(MessageQueueWithLimitTest, testPerKeyQueueLimit_overallQueueIsFull)
{
    const std::string recipient1("recipient1");
    const std::string recipient2("recipient2");
    const std::string msgRecipient1Payload1("payload1.1");
    const std::string msgRecipient1Payload2("payload1.2");
    const std::string msgRecipient2Payload1("payload2.1");

    constexpr std::uint64_t messageQueueLimit = 2;
    constexpr std::uint64_t perKeyQueueLimit = 2;

    MessageQueue<std::string> queue(messageQueueLimit, perKeyQueueLimit);
    createAndQueueMessage(queue, getExpiryDateFromNow(1000), recipient1, msgRecipient1Payload1);
    createAndQueueMessage(queue, getExpiryDateFromNow(100), recipient2, msgRecipient2Payload1);

    // The overall queue is full. However, there is still space in the per-key-queue. Therefore
    // the message for recipient 2 must be removed.
    createAndQueueMessage(queue, getExpiryDateFromNow(2000), recipient1, msgRecipient1Payload2);

    EXPECT_EQ(2, queue.getQueueLength());

    auto recipient1Message1 = queue.getNextMessageFor(recipient1);
    auto recipient1Message2 = queue.getNextMessageFor(recipient1);
    auto recipient2Message1 = queue.getNextMessageFor(recipient2);

    EXPECT_NE(nullptr, recipient1Message1);
    EXPECT_NE(nullptr, recipient1Message2);
    EXPECT_EQ(nullptr, recipient2Message1);

    EXPECT_EQ(msgRecipient1Payload2, payloadAsString(recipient1Message1));
    EXPECT_EQ(msgRecipient1Payload1, payloadAsString(recipient1Message2));
}

TEST_F(MessageQueueWithLimitTest, testMessageQueueLimitBytes)
{
    const std::string recipient1("recipient1");
    const std::string recipient2("recipient2");

    constexpr std::uint64_t messageQueueLimit = 0;
    constexpr std::uint64_t perKeyQueueLimit = 0;

    std::string payload(1000, 'x');
    std::uint64_t sizeOfSingleMessage = createMessage(getExpiryDateFromNow(1000), recipient1, payload)->getMessageSize();

    // set limit so that 2 messages safely fit into the queue, but 3 messages exceed it
    std::uint64_t messageQueueLimitBytes = sizeOfSingleMessage * 3 - 1;
    MessageQueue<std::string> queue(messageQueueLimit, perKeyQueueLimit, messageQueueLimitBytes);

    createAndQueueMessage(queue, getExpiryDateFromNow(1000), recipient1, payload);
    EXPECT_EQ(1, queue.getQueueLength());
    EXPECT_EQ(queue.getQueueSizeBytes(), sizeOfSingleMessage);
    EXPECT_LE(queue.getQueueSizeBytes(), messageQueueLimitBytes);

    // the following 2nd message is expected to be discarded when the
    // 3rd message is queued since it has the lowest expiry date
    createAndQueueMessage(queue, getExpiryDateFromNow(100), recipient2, payload);
    EXPECT_EQ(2, queue.getQueueLength());
    EXPECT_EQ(queue.getQueueSizeBytes(), sizeOfSingleMessage * 2);
    EXPECT_LE(queue.getQueueSizeBytes(), messageQueueLimitBytes);

    createAndQueueMessage(queue, getExpiryDateFromNow(2000), recipient1, payload);
    EXPECT_EQ(2, queue.getQueueLength());
    EXPECT_EQ(queue.getQueueSizeBytes(), sizeOfSingleMessage * 2);
    EXPECT_LE(queue.getQueueSizeBytes(), messageQueueLimitBytes);

    auto recipient1Message1 = queue.getNextMessageFor(recipient1);
    EXPECT_EQ(1, queue.getQueueLength());
    EXPECT_EQ(queue.getQueueSizeBytes(), sizeOfSingleMessage);
    EXPECT_NE(nullptr, recipient1Message1);

    auto recipient1Message2 = queue.getNextMessageFor(recipient1);
    EXPECT_EQ(0, queue.getQueueLength());
    EXPECT_EQ(0, queue.getQueueSizeBytes());
    EXPECT_NE(nullptr, recipient1Message2);

    auto recipient2Message1 = queue.getNextMessageFor(recipient2);
    EXPECT_EQ(nullptr, recipient2Message1);
}

TEST_F(MessageQueueWithLimitTest, testMessageQueueHandlesTooLargeMessage)
{
    const std::string recipient1("recipient1");
    constexpr std::uint64_t messageQueueLimit = 0;
    constexpr std::uint64_t perKeyQueueLimit = 0;
    std::string payload(1000, 'x');
    std::uint64_t sizeOfSingleMessage = createMessage(getExpiryDateFromNow(1000), recipient1, payload)->getMessageSize();
    std::uint64_t messageQueueLimitBytes = sizeOfSingleMessage - 1;

    MessageQueue<std::string> queue(messageQueueLimit, perKeyQueueLimit, messageQueueLimitBytes);

    createAndQueueMessage(queue, getExpiryDateFromNow(1000), recipient1, payload);

    EXPECT_EQ(0, queue.getQueueLength());
}
