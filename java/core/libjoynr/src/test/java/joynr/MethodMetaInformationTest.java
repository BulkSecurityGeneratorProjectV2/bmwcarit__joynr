/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package joynr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Method;

import org.junit.Test;

import io.joynr.Async;
import io.joynr.Sync;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;

public class MethodMetaInformationTest {

    @Test
    public void methodWithNoParametersShouldNotHaveCallbackIndexSet() {
        final Method method = getMethod(TestSyncInterface.class, "methodWithoutParameters");

        final MethodMetaInformation methodMetaInformation = new MethodMetaInformation(method);

        assertMethodMetaInformation(methodMetaInformation, method);
        assertCallbackIndexIsNotSet(methodMetaInformation);
    }

    @Test
    public void methodWithSingleInputParameterShouldNotHaveCallbackIndexSet() {
        final Method method = getMethod(TestSyncInterface.class, "methodWithSingleInputParameter", Integer.class);

        final MethodMetaInformation methodMetaInformation = new MethodMetaInformation(method);

        assertMethodMetaInformation(methodMetaInformation, method);
        assertCallbackIndexIsNotSet(methodMetaInformation);
    }

    @Test
    public void methodWithSingleCallbackShouldHaveCallbackIndexSet() {
        final Method method = getMethod(TestAsyncInterface.class, "methodWithoutParameters", Callback.class);

        final MethodMetaInformation methodMetaInformation = new MethodMetaInformation(method);

        assertMethodMetaInformation(methodMetaInformation, method);
        assertCallbackIndexIsSet(methodMetaInformation);
    }

    @Test
    public void methodWithSingleCallbackButWithoutAnnotationShouldNotHaveCallbackIndexSet() {
        final Method method = getMethod(TestAsyncInterface.class,
                                        "methodWithoutParametersAndNoAnnotation",
                                        Callback.class);

        final MethodMetaInformation methodMetaInformation = new MethodMetaInformation(method);

        assertMethodMetaInformation(methodMetaInformation, method);
        assertCallbackIndexIsNotSet(methodMetaInformation);
    }

    @Test
    public void methodWithSingleCallbackButWithoutProperAnnotationShouldNotHaveCallbackIndexSet() {
        final Method method = getMethod(TestAsyncInterface.class,
                                        "methodWithoutParametersWithoutProperAnnotation",
                                        Callback.class);

        final MethodMetaInformation methodMetaInformation = new MethodMetaInformation(method);

        assertMethodMetaInformation(methodMetaInformation, method);
        assertCallbackIndexIsNotSet(methodMetaInformation);
    }

    private Method getMethod(final Class interfaceClass, final String methodName, final Class<?>... parameterTypes) {
        try {
            return interfaceClass.getDeclaredMethod(methodName, parameterTypes);
        } catch (final NoSuchMethodException exception) {
            fail("Unexpected exception while getting method via reflection: " + exception.getMessage());
            return null;
        }
    }

    private void assertMethodMetaInformation(final MethodMetaInformation actualMethodMetaInformation,
                                             final Method expectedMethod) {
        assertNotNull(actualMethodMetaInformation);
        assertEquals(expectedMethod, actualMethodMetaInformation.getMethod());
        assertEquals(expectedMethod.getName(), actualMethodMetaInformation.getMethodName());
        assertNotNull(actualMethodMetaInformation.getClasses());
        assertEquals(expectedMethod.getParameterTypes().length, actualMethodMetaInformation.getClasses().length);
    }

    private void assertCallbackIndexIsNotSet(final MethodMetaInformation actualMethodMetaInformation) {
        assertEquals(-1, actualMethodMetaInformation.getCallbackIndex());
        assertNull(actualMethodMetaInformation.getCallbackAnnotation());
    }

    private void assertCallbackIndexIsSet(final MethodMetaInformation actualMethodMetaInformation) {
        assertNotEquals(-1, actualMethodMetaInformation.getCallbackIndex());
        assertNotNull(actualMethodMetaInformation.getCallbackAnnotation());
    }

    @Async
    interface TestAsyncInterface {
        Future<Void> methodWithoutParameters(@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback);

        Future<Void> methodWithoutParametersAndNoAnnotation(Callback<Void> callback);

        Future<Void> methodWithoutParametersWithoutProperAnnotation(@TestParamCustomAnnotation Callback<Void> callback);
    }

    @Sync
    interface TestSyncInterface {
        void methodWithoutParameters();

        void methodWithSingleInputParameter(Integer input);
    }

    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.PARAMETER)
    @interface TestParamCustomAnnotation {

    }
}
