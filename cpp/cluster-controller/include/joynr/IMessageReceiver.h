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
#ifndef IMESSAGERECEIVER_H
#define IMESSAGERECEIVER_H

#include <memory>
#include <string>
#include "joynr/system/RoutingTypes/Address.h"

namespace joynr
{

/**
  * @class IMessageReceiver
  * @brief Interface for message receiver
  *
  * This interface describes basic messaging functionality available in
  * the Joynr framework. Especially, it offers a method to notify about
  * received messages.
  */
class IMessageReceiver
{

public:
    virtual ~IMessageReceiver() = default;

    /**
      * Returns the serialized (json) receive address
      */
    virtual const std::string& getGlobalClusterControllerAddress() const = 0;

    /**
      * Starts processing incomming messages. This method must be called
      * after creation in order to receive incoming messages.
      */
    virtual void startReceiveQueue() = 0;

    /**
      * Check if the receiver is connected
      */
    virtual bool isConnected() = 0;

    /**
      * Stops receiving messages over the specified channel.
      * The channel remains on the server.
      */
    virtual void stopReceiveQueue() = 0;

    /**
      * Update the settings of the Message Receiver
      */
    virtual void updateSettings() = 0;

    /**
      * Will try to delete the channel from the server. Returns true if successfull, false if not.
      */
    virtual bool tryToDeleteChannel() = 0;

    virtual void registerReceiveCallback(
            std::function<void(const std::string&)> onTextMessageReceived) = 0;
};

} // namespace joynr
#endif // IMESSAGERECEIVER_H
