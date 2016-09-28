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

#ifndef LIBJOYNRWEBSOCKETRUNTIME_H
#define LIBJOYNRWEBSOCKETRUNTIME_H

#include <memory>
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Logger.h"
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"
#include "joynr/Settings.h"
#include "libjoynr/websocket/WebSocketSettings.h"

namespace joynr
{
class WebSocketPpClient;
class WebSocketLibJoynrMessagingSkeleton;

class LibJoynrWebSocketRuntime : public LibJoynrRuntime
{
    WebSocketSettings wsSettings;

public:
    explicit LibJoynrWebSocketRuntime(std::unique_ptr<Settings> settings);
    ~LibJoynrWebSocketRuntime() override;

protected:
    void startLibJoynrMessagingSkeleton(
            const std::shared_ptr<MessageRouter>& messageRouter) override;

private:
    DISALLOW_COPY_AND_ASSIGN(LibJoynrWebSocketRuntime);

    void onWebSocketError(const std::string& errorMessage);

    std::shared_ptr<WebSocketPpClient> websocket;
    ADD_LOGGER(LibJoynrWebSocketRuntime);
};

} // namespace joynr
#endif // LIBJOYNRWEBSOCKETRUNTIME_H
