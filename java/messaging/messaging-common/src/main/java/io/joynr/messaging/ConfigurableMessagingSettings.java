package io.joynr.messaging;

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

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * Storage class for network configuration used to send messages.
 */

@Singleton
public class ConfigurableMessagingSettings implements MessagingSettings {
    // public static final String PROPERTY_JOYNR_MESSAGING_CHANNELID = "joynr.messaging.channelId"; //NOT USUALLY SET BY
    // THE APPLICATION!
    // public static final String PROPERTY_JOYNR_MESSAGING_CAPABILITIESDIRECTORYURL =
    // "joynr.messaging.capabilitiesDirectoryUrl";
    // public static final String PROPERTY_BOUNCE_PROXY_URL = "joynr.messaging.bounceProxyUrl";

    public static final String PROPERTY_CC_CONNECTION_TYPE = "joynr.messaging.cc.connectiontype";

    public static final String PROPERTY_ARBITRATION_MINIMUMRETRYDELAY = "joynr.arbitration.minimumretrydelay";
    public static final String PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID = "joynr.messaging.capabilitiesdirectoryparticipantid";
    public static final String PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID = "joynr.messaging.capabilitiesdirectorychannelid";
    public static final String PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN = "joynr.messaging.discoverydirectoriesdomain";
    public static final String PROPERTY_DISCOVERY_REQUEST_TIMEOUT = "joynr.discovery.requesttimeout";
    public static final String PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS = "joynr.discovery.provider.defaultexpirytimems";

    public static final String PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID = "joynr.messaging.domainaccesscontrollerparticipantid";
    public static final String PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID = "joynr.messaging.domainaccesscontrollerchannelid";

    public static final String PROPERTY_CREATE_CHANNEL_RETRY_INTERVAL_MS = "joynr.messaging.createchannelretryintervalms";
    public static final String PROPERTY_DELETE_CHANNEL_RETRY_INTERVAL_MS = "joynr.messaging.deletechannelretryintervalms";
    public static final String PROPERTY_SEND_MSG_RETRY_INTERVAL_MS = "joynr.messaging.sendmsgretryintervalms";
    public static final String PROPERTY_LONG_POLL_RETRY_INTERVAL_MS = "joynr.messaging.longpollretryintervalms";
    public static final String PROPERTY_MAX_RETRY_COUNT = "joynr.messaging.maxretriescount";
    public static final String PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE = "joynr.discovery.participantids_persistence_file";
    public static final String DEFAULT_PARTICIPANTIDS_PERSISTENCE_FILE = "joynr_participantIds.properties";

    public static final String PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS = "joynr.messaging.maximumparallelsends";
    public static final String PROPERTY_HOSTS_FILENAME = "joynr.messaging.hostsfilename";

    public static final String PROPERTY_MAX_MESSAGE_SIZE = "joynr.messaging.maxmessagesize";

    public static final String PROPERTY_MESSAGING_MAXIMUM_TTL_MS = "joynr.messaging.maxttlms";

    private final BounceProxyUrl bounceProxyUrl;
    private final long createChannelRetryIntervalMs;
    private final long deleteChannelRetryIntervalMs;
    private final long sendMsgRetryIntervalMs;
    private final long longPollRetryIntervalMs;
    private final int maxRetriesCount;
    private long discoveryRequestTimeoutMs;
    private int maximumParallelSends;

    @Inject
    // CHECKSTYLE:OFF
    public ConfigurableMessagingSettings(@Named(MessagingPropertyKeys.BOUNCE_PROXY_URL) String bounceProxyUrl,
                                         @Named(PROPERTY_MAX_RETRY_COUNT) int maxRetriesCount,
                                         @Named(PROPERTY_CREATE_CHANNEL_RETRY_INTERVAL_MS) long createChannelRetryIntervalMs,
                                         @Named(PROPERTY_DELETE_CHANNEL_RETRY_INTERVAL_MS) long deleteChannelRetryIntervalMs,
                                         @Named(PROPERTY_SEND_MSG_RETRY_INTERVAL_MS) long sendMsgRetryIntervalMs,
                                         @Named(PROPERTY_LONG_POLL_RETRY_INTERVAL_MS) long longPollRetryIntervalMs,
                                         @Named(PROPERTY_DISCOVERY_REQUEST_TIMEOUT) long discoveryRequestTimeoutMs,
                                         @Named(PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maximumParallelSends) {
        // CHECKSTYLE:ON
        this.maxRetriesCount = maxRetriesCount;
        this.discoveryRequestTimeoutMs = discoveryRequestTimeoutMs;
        this.maximumParallelSends = maximumParallelSends;
        this.bounceProxyUrl = new BounceProxyUrl(bounceProxyUrl);
        this.createChannelRetryIntervalMs = createChannelRetryIntervalMs;
        this.deleteChannelRetryIntervalMs = deleteChannelRetryIntervalMs;
        this.sendMsgRetryIntervalMs = sendMsgRetryIntervalMs;
        this.longPollRetryIntervalMs = longPollRetryIntervalMs;

    }

    public int getMaximumParallelSends() {
        return maximumParallelSends;
    }

    @Override
    public BounceProxyUrl getBounceProxyUrl() {
        return bounceProxyUrl;
    }

    @Override
    public long getCreateChannelRetryIntervalMs() {
        return createChannelRetryIntervalMs;
    }

    @Override
    public long getDeleteChannelRetryIntervalMs() {
        return deleteChannelRetryIntervalMs;
    }

    @Override
    public long getSendMsgRetryIntervalMs() {
        return sendMsgRetryIntervalMs;
    }

    @Override
    public long getLongPollRetryIntervalMs() {
        return longPollRetryIntervalMs;
    }

    @Override
    public String toString() {
        return "MessagingSettings [bounceProxyUrl=" + bounceProxyUrl + ", createChannelRetryIntervalMs="
                + createChannelRetryIntervalMs + ", sendMsgRetryIntervalMs=" + sendMsgRetryIntervalMs + "]";
    }

    @Override
    public int getMaxRetriesCount() {
        return maxRetriesCount;
    }

    public long getDiscoveryRequestTimeoutMs() {
        return discoveryRequestTimeoutMs;
    }

}
