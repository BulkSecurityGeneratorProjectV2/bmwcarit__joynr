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

#ifndef LIBJOYNRWEBSOCKETRUNTIME_H
#define LIBJOYNRWEBSOCKETRUNTIME_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/Logger.h"
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"
#include "libjoynr/websocket/WebSocketSettings.h"
#include "libjoynr/websocket/WebSocketClient.h"

namespace joynr
{
class WebSocketLibJoynrMessagingSkeleton;

class LibJoynrWebSocketRuntime : public LibJoynrRuntime
{
    WebSocketSettings wsSettings;

public:
    explicit LibJoynrWebSocketRuntime(Settings* settings);
    ~LibJoynrWebSocketRuntime() override;

protected:
    WebSocketLibJoynrMessagingSkeleton* wsLibJoynrMessagingSkeleton;

    void startLibJoynrMessagingSkeleton(MessageRouter& messageRouter) override;

private:
    DISALLOW_COPY_AND_ASSIGN(LibJoynrWebSocketRuntime);

    void onWebSocketError(const std::string& errorMessage);

    std::shared_ptr<WebSocketClient> websocket;
    ADD_LOGGER(LibJoynrWebSocketRuntime);
};

} // namespace joynr
#endif // LIBJOYNRWEBSOCKETRUNTIME_H
