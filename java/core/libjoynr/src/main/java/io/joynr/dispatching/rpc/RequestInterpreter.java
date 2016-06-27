package io.joynr.dispatching.rpc;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.dispatcher.rpc.ReflectionUtils;
import io.joynr.dispatching.RequestCaller;
import io.joynr.exceptions.JoynrException;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.provider.ProviderCallback;
import io.joynr.proxy.MethodSignature;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.MethodInvocationException;
import joynr.exceptions.ProviderRuntimeException;
import io.joynr.JoynrVersion;
import io.joynr.util.AnnotationUtil;
import joynr.types.Version;

public class RequestInterpreter {
    private static final Logger logger = LoggerFactory.getLogger(RequestInterpreter.class);

    @Inject
    public RequestInterpreter() {
    }

    // use for caching because creation of MethodMetaInformation is expensive
    // ConcurrentMap<ClassName, ConcurrentMap<MethodName, MethodMetaInformation>
    // >
    // TODO move methodMetaInformation to a central place, save metaInformations in a cache (e.g. with predefined max
    // size)
    private final ConcurrentMap<MethodSignature, Method> methodSignatureToMethodMap = new ConcurrentHashMap<MethodSignature, Method>();

    private Reply createReply(Request request, Object... response) {
        return new Reply(request.getRequestReplyId(), response);
    }

    public void execute(final ProviderCallback<Reply> callback, RequestCaller requestCaller, final Request request) {
        Promise<? extends AbstractDeferred> promise;
        try {
            promise = (Promise<?>) invokeMethod(requestCaller, request);
        } catch (MethodInvocationException | ProviderRuntimeException e) {
            callback.onFailure(e);
            return;
        } catch (Exception e) {
            JoynrVersion joynrVersion = AnnotationUtil.getAnnotation(requestCaller.getClass(), JoynrVersion.class);
            callback.onFailure(new MethodInvocationException(e, new Version(joynrVersion.major(), joynrVersion.minor())));
            return;
        }
        promise.then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException error) {
                callback.onFailure(error);
            }

            @Override
            public void onFulfillment(Object... values) {
                callback.onSuccess(createReply(request, values));
            }
        });

    }

    public Object invokeMethod(RequestCaller requestCaller, OneWayRequest request) {
        // A method is identified by its defining request caller, its name and the types of its arguments
        MethodSignature methodSignature = new MethodSignature(requestCaller,
                                                              request.getMethodName(),
                                                              request.getParamDatatypes());

        ensureMethodMetaInformationPresent(requestCaller, request, methodSignature);

        Method method = methodSignatureToMethodMap.get(methodSignature);

        Object[] params = null;
        try {
            if (method.getParameterTypes().length > 0) {
                // method with parameters
                params = request.getParams();
            }
            return method.invoke(requestCaller, params);
        } catch (IllegalAccessException e) {
            logger.error("RequestInterpreter: Received an RPC invocation for a non public method {}", request);
            JoynrVersion joynrVersion = AnnotationUtil.getAnnotation(requestCaller.getClass(), JoynrVersion.class);
            throw new MethodInvocationException(e, new Version(joynrVersion.major(), joynrVersion.minor()));

        } catch (InvocationTargetException e) {
            logger.debug("invokeMethod error", e);
            Throwable cause = e.getCause();
            logger.error("RequestInterpreter: Could not perform an RPC invocation: {}", cause == null ? e.toString()
                    : cause.getMessage());
            throw new ProviderRuntimeException(cause == null ? e.toString() : cause.toString());
        }
    }

    private void ensureMethodMetaInformationPresent(RequestCaller requestCaller,
                                                    OneWayRequest request,
                                                    MethodSignature methodSignature) {
        try {
            if (!methodSignatureToMethodMap.containsKey(methodSignature)) {
                Method method;
                method = ReflectionUtils.findMethodByParamTypeNames(methodSignature.getRequestCaller().getClass(),
                                                                    methodSignature.getMethodName(),
                                                                    methodSignature.getParameterTypeNames());
                methodSignatureToMethodMap.putIfAbsent(methodSignature, method);
            }
        } catch (NoSuchMethodException e) {
            logger.error("RequestInterpreter: Received an RPC invocation for a non existing method" + request, e);
            JoynrVersion joynrVersion = AnnotationUtil.getAnnotation(requestCaller.getClass(), JoynrVersion.class);
            throw new MethodInvocationException(e.toString(), new Version(joynrVersion.major(), joynrVersion.minor()));
        }
    }
}
