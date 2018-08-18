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
package io.joynr.proxy;

import java.lang.reflect.Method;
import java.util.Set;

import javax.annotation.CheckForNull;

import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.DispatcherUtils;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RpcAsyncRequestReplyCaller;
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import joynr.MethodMetaInformation;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.ApplicationException;
import joynr.types.DiscoveryEntryWithMetaInfo;

//TODO maybe Connector should not be a dynamic proxy. ProxyInvocationHandler could call execute...Method() directly.
final class JoynrMessagingConnectorInvocationHandler implements ConnectorInvocationHandler {
    private static final Logger logger = LoggerFactory.getLogger(JoynrMessagingConnectorInvocationHandler.class);

    private final Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries;
    private final String fromParticipantId;

    private final MessagingQos qosSettings;

    private final RequestReplyManager requestReplyManager;
    private final ReplyCallerDirectory replyCallerDirectory;

    private final SubscriptionManager subscriptionManager;
    private final StatelessAsyncIdCalculator statelessAsyncIdCalculator;

    JoynrMessagingConnectorInvocationHandler(Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                             String fromParticipantId,
                                             MessagingQos qosSettings,
                                             RequestReplyManager requestReplyManager,
                                             ReplyCallerDirectory replyCallerDirectory,
                                             SubscriptionManager subscriptionManager,
                                             StatelessAsyncIdCalculator statelessAsyncIdCalculator) {
        this.toDiscoveryEntries = toDiscoveryEntries;
        this.fromParticipantId = fromParticipantId;

        this.qosSettings = qosSettings;

        this.requestReplyManager = requestReplyManager;
        this.replyCallerDirectory = replyCallerDirectory;
        this.subscriptionManager = subscriptionManager;

        this.statelessAsyncIdCalculator = statelessAsyncIdCalculator;
    }

    @SuppressWarnings("unchecked")
    @Override
    public Future<?> executeAsyncMethod(Object proxy, Method method, Object[] params, Future<?> future) {

        if (method == null) {
            throw new IllegalArgumentException("Method cannot be null");
        }
        if (toDiscoveryEntries.size() > 1) {
            throw new JoynrIllegalStateException("You can't execute async methods for multiple participants.");
        }
        if (toDiscoveryEntries.isEmpty()) {
            throw new JoynrIllegalStateException("You must have exactly one participant to be able to execute an async method.");
        }

        MethodMetaInformation methodMetaInformation = JoynrMessagingConnectorFactory.ensureMethodMetaInformationPresent(method);
        if (methodMetaInformation.getCallbackAnnotation() == null) {
            throw new JoynrIllegalStateException("All async methods need to have a annotated callback parameter.");
        }

        int callbackIndex = methodMetaInformation.getCallbackIndex();
        ICallback callback = (ICallback) params[callbackIndex];

        Object[] paramsWithoutCallback = new Object[params.length - 1];
        copyArrayWithoutElement(params, paramsWithoutCallback, callbackIndex);
        Class<?>[] paramDatatypes = method.getParameterTypes();
        Class<?>[] paramDatatypesWithoutCallback = new Class<?>[paramDatatypes.length - 1];
        copyArrayWithoutElement(paramDatatypes, paramDatatypesWithoutCallback, callbackIndex);

        Request request = new Request(method.getName(), paramsWithoutCallback, paramDatatypesWithoutCallback);
        String requestReplyId = request.getRequestReplyId();

        @SuppressWarnings("rawtypes")
        RpcAsyncRequestReplyCaller<?> callbackWrappingReplyCaller = new RpcAsyncRequestReplyCaller(proxy,
                                                                                                   requestReplyId,
                                                                                                   callback,
                                                                                                   future,
                                                                                                   method,
                                                                                                   methodMetaInformation);

        ExpiryDate expiryDate = DispatcherUtils.convertTtlToExpirationDate(qosSettings.getRoundTripTtl_ms());

        replyCallerDirectory.addReplyCaller(requestReplyId, callbackWrappingReplyCaller, expiryDate);
        requestReplyManager.sendRequest(fromParticipantId, toDiscoveryEntries.iterator().next(), request, qosSettings);
        return future;
    }

    private void copyArrayWithoutElement(Object[] fromArray, Object[] toArray, int removeIndex) {
        System.arraycopy(fromArray, 0, toArray, 0, removeIndex);
        System.arraycopy(fromArray, removeIndex + 1, toArray, removeIndex, toArray.length - removeIndex);
    }

    @Override
    public void executeStatelessAsyncMethod(Method method,
                                            Object[] args,
                                            String interfaceName,
                                            StatelessAsyncCallback statelessAsyncCallback) {
        if (method == null) {
            throw new IllegalArgumentException("Method cannot be null");
        }
        if (toDiscoveryEntries.size() > 1) {
            throw new JoynrIllegalStateException("You can't execute stateless async methods for multiple participants.");
        }
        if (toDiscoveryEntries.isEmpty()) {
            throw new JoynrIllegalStateException("You must have exactly one participant to be able to execute a stateless async method.");
        }

        MessageIdCallback messageIdCallback = null;
        int messageIdCallbackIndex = -1;
        for (int index = 0; index < args.length; index++) {
            if (args[index] instanceof MessageIdCallback) {
                messageIdCallback = (MessageIdCallback) args[index];
                messageIdCallbackIndex = index;
            }
        }
        if (messageIdCallback == null) {
            throw new JoynrIllegalStateException("Stateless async method calls must have a MessageIdCallback as one of their arguments.");
        }
        Object[] paramsWithoutMessageIdCallback = new Object[args.length - 1];
        copyArrayWithoutElement(args, paramsWithoutMessageIdCallback, messageIdCallbackIndex);
        Class<?>[] paramDatatypes = method.getParameterTypes();
        Class<?>[] paramDatatypesWithoutMessageIdCallback = new Class<?>[paramDatatypes.length - 1];
        copyArrayWithoutElement(paramDatatypes, paramDatatypesWithoutMessageIdCallback, messageIdCallbackIndex);

        Request request = new Request(method.getName(),
                                      paramsWithoutMessageIdCallback,
                                      paramDatatypesWithoutMessageIdCallback,
                                      statelessAsyncIdCalculator.calculateStatelessCallbackRequestReplyId(method),
                                      statelessAsyncIdCalculator.calculateStatelessCallbackId(interfaceName,
                                                                                              statelessAsyncCallback));
        requestReplyManager.sendRequest(statelessAsyncIdCalculator.calculateParticipantId(interfaceName,
                                                                                          statelessAsyncCallback),
                                        toDiscoveryEntries.iterator().next(),
                                        request,
                                        qosSettings);
        messageIdCallback.accept(request.getRequestReplyId());
    }

    @CheckForNull
    @Override
    public Object executeSyncMethod(Method method, Object[] args) throws ApplicationException {

        // TODO does a method with 0 args pass in an empty args array, or null for args?
        if (method == null) {
            throw new IllegalArgumentException("Method cannot be null");
        }
        if (toDiscoveryEntries.size() > 1) {
            throw new JoynrIllegalStateException("You can't execute sync methods for multiple participants.");
        }
        if (toDiscoveryEntries.isEmpty()) {
            throw new JoynrIllegalStateException("You must have exactly one participant to be able to execute a sync method.");
        }

        MethodMetaInformation methodMetaInformation = JoynrMessagingConnectorFactory.ensureMethodMetaInformationPresent(method);

        Request request = new Request(method.getName(), args, method.getParameterTypes());
        Reply reply;
        String requestReplyId = request.getRequestReplyId();

        SynchronizedReplyCaller synchronizedReplyCaller = new SynchronizedReplyCaller(fromParticipantId,
                                                                                      requestReplyId,
                                                                                      request);
        ExpiryDate expiryDate = DispatcherUtils.convertTtlToExpirationDate(qosSettings.getRoundTripTtl_ms());
        replyCallerDirectory.addReplyCaller(requestReplyId, synchronizedReplyCaller, expiryDate);
        reply = (Reply) requestReplyManager.sendSyncRequest(fromParticipantId,
                                                            toDiscoveryEntries.iterator().next(),
                                                            request,
                                                            synchronizedReplyCaller,
                                                            qosSettings);
        if (reply.getError() == null) {
            if (method.getReturnType().equals(void.class)) {
                return null;
            }
            Object response = RpcUtils.reconstructReturnedObject(method, methodMetaInformation, reply.getResponse());
            logger.debug("REQUEST returns successful: requestReplyId: {}, method {}, response: {}",
                         requestReplyId,
                         method.getName(),
                         response);
            return response;
        } else if (reply.getError() instanceof ApplicationException) {
            logger.debug("REQUEST returns error: requestReplyId: {}, method {}, response: {}",
                         requestReplyId,
                         method.getName(),
                         reply.getError());
            throw (ApplicationException) reply.getError();
        } else {
            logger.debug("REQUEST returns error: requestReplyId: {}, method {}, response: {}",
                         requestReplyId,
                         method.getName(),
                         reply.getError());
            throw (JoynrRuntimeException) reply.getError();
        }

    }

    @Override
    public void executeOneWayMethod(Method method, Object[] args) {
        // TODO does a method with 0 args pass in an empty args array, or null for args?
        if (method == null) {
            throw new IllegalArgumentException("Method cannot be null");
        }
        if (toDiscoveryEntries.isEmpty()) {
            throw new JoynrIllegalStateException("You must have at least one participant to be able to execute an oneWayMethod.");
        }

        logger.debug("ONEWAYREQUEST call proxy: method: {}, params: {}, proxy participantId: {},"
                + " provider discovery entries: {}", method.getName(), args, fromParticipantId, toDiscoveryEntries);

        OneWayRequest request = new OneWayRequest(method.getName(), args, method.getParameterTypes());
        requestReplyManager.sendOneWayRequest(fromParticipantId, toDiscoveryEntries, request, qosSettings);
    }

    @Override
    public void executeSubscriptionMethod(UnsubscribeInvocation unsubscribeInvocation) {

        if (toDiscoveryEntries.isEmpty()) {
            throw new JoynrIllegalStateException("You must have at least one participant to be able to execute a subscription method.");
        }

        subscriptionManager.unregisterSubscription(fromParticipantId,
                                                   toDiscoveryEntries,
                                                   unsubscribeInvocation.getSubscriptionId(),
                                                   qosSettings);
    }

    @Override
    public void executeSubscriptionMethod(AttributeSubscribeInvocation attributeSubscription) {
        if (toDiscoveryEntries.isEmpty()) {
            throw new JoynrIllegalStateException("You must have at least one participant to be able to execute a subscription method.");
        }

        logger.debug("SUBSCRIPTION call proxy: subscriptionId: {}, attribute: {}, qos: {},"
                + " proxy participantId: {}, provider discovery entries: {}",
                     attributeSubscription.getSubscriptionId(),
                     attributeSubscription.getSubscriptionName(),
                     attributeSubscription.getQos(),
                     fromParticipantId,
                     toDiscoveryEntries);
        subscriptionManager.registerAttributeSubscription(fromParticipantId, toDiscoveryEntries, attributeSubscription);
    }

    @Override
    public void executeSubscriptionMethod(BroadcastSubscribeInvocation broadcastSubscription) {

        if (toDiscoveryEntries.isEmpty()) {
            throw new JoynrIllegalStateException("You must have at least one participant to be able to execute a subscription method.");
        }

        logger.debug("SUBSCRIPTION call proxy: subscriptionId: {}, broadcast: {}, qos: {},"
                + " proxy participantId: {}, provider discovery entries: {}",
                     broadcastSubscription.getSubscriptionId(),
                     broadcastSubscription.getBroadcastName(),
                     broadcastSubscription.getQos(),
                     fromParticipantId,
                     toDiscoveryEntries);
        subscriptionManager.registerBroadcastSubscription(fromParticipantId, toDiscoveryEntries, broadcastSubscription);
    }

    @Override
    public void executeSubscriptionMethod(MulticastSubscribeInvocation multicastSubscription) {
        subscriptionManager.registerMulticastSubscription(fromParticipantId, toDiscoveryEntries, multicastSubscription);
    }
}
