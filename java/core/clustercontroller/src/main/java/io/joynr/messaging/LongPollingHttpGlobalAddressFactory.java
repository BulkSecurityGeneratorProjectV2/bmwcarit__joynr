package io.joynr.messaging;

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

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.messaging.http.HttpGlobalAddressFactory;
import io.joynr.messaging.http.operation.ChannelCreatedListener;
import io.joynr.messaging.http.operation.LongPollingMessageReceiver;

@Singleton
public class LongPollingHttpGlobalAddressFactory extends HttpGlobalAddressFactory implements ChannelCreatedListener {

    private String myChannelId;
    private String messagingEndpointUrl;

    @Inject
    public LongPollingHttpGlobalAddressFactory(@Named(MessagingPropertyKeys.CHANNELID) String myChannelId,
                                               LongPollingMessageReceiver longPollingMessageReceiver) {
        longPollingMessageReceiver.registerChannelCreatedListener(this);
        this.myChannelId = myChannelId;
    }

    @Override
    protected String getMyChannelId() {
        return myChannelId;
    }

    @Override
    protected String getMessagingEndpointUrl() {
        if (messagingEndpointUrl == null) {
            throw new JoynrDelayMessageException("bounceproxy channel for long polling not yet created");
        }
        return messagingEndpointUrl;
    }

    @Override
    public void channelCreated(String channelUrl) {
        this.messagingEndpointUrl = channelUrl;
    }

}
