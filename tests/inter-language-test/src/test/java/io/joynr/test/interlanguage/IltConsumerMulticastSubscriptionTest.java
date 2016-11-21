package io.joynr.test.interlanguage;

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

import static org.junit.Assert.fail;

import java.util.concurrent.Semaphore;

import org.junit.Assert;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.SubscriptionException;
import io.joynr.proxy.Future;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSingleEnumerationParameterBroadcastAdapter;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;

public class IltConsumerMulticastSubscriptionTest extends IltConsumerTest {
    private static final Logger LOG = LoggerFactory.getLogger(IltConsumerTest.class);

    volatile boolean subscribeBroadcastWithSingleEnumerationParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
    Semaphore callbackCalledSemaphore = new Semaphore(0);

    @Test
    public void doNotReceivePublicationsForOtherPartitions() {
        int minIntervalMs = 0;
        int maxIntervalMs = 10000;
        long validityMs = 60000;
        int alertAfterIntervalMs = 20000;
        int publicationTtlMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                                                                         .setMaxIntervalMs(maxIntervalMs)
                                                                                                         .setValidityMs(validityMs)
                                                                                                         .setAlertAfterIntervalMs(alertAfterIntervalMs)
                                                                                                         .setPublicationTtlMs(publicationTtlMs);

        String[] subscribeToPartitions = new String[]{ "partitions0", "partitions1" };
        String[] broadcastPartitions = new String[]{ "otherPartition" };

        try {
            BroadcastWithSingleEnumerationParameterBroadcastAdapter adapter = new BroadcastWithSingleEnumerationParameterBroadcastAdapter() {
                @Override
                public void onReceive(ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut) {
                    LOG.info(name.getMethodName() + " - callback - got broadcast");
                    subscribeBroadcastWithSingleEnumerationParameterCallbackResult = true;
                    subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
                    synchronized (callbackCalledSemaphore) {
                        callbackCalledSemaphore.notify();
                    }
                }

                @Override
                public void onError(SubscriptionException error) {
                    LOG.info(name.getMethodName() + " - callback - error");
                    subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
                    subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
                    synchronized (callbackCalledSemaphore) {
                        callbackCalledSemaphore.notify();
                    }
                }
            };

            Future<String> subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithSingleEnumerationParameterBroadcast(adapter,
                                                                                                                                 subscriptionQos,
                                                                                                                                 subscribeToPartitions);

            String subscriptionId = subscriptionIdFuture.get();
            LOG.info(name.getMethodName() + " - subscription successful");

            LOG.info(name.getMethodName() + " - Invoking fire method with not matching partitions");
            testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter(broadcastPartitions);

            synchronized (callbackCalledSemaphore) {
                callbackCalledSemaphore.wait(2000);
            }

            Assert.assertEquals(false, subscribeBroadcastWithSingleEnumerationParameterCallbackDone);

            LOG.info(name.getMethodName() + " - Invoking fire method with matching partitions");
            testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter(subscribeToPartitions);

            synchronized (callbackCalledSemaphore) {
                callbackCalledSemaphore.wait(1000);
            }

            Assert.assertEquals(true, subscribeBroadcastWithSingleEnumerationParameterCallbackDone);
            Assert.assertEquals(true, subscribeBroadcastWithSingleEnumerationParameterCallbackResult);
            LOG.info(name.getMethodName() + " - received expected broadcast");

            testInterfaceProxy.unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(subscriptionId);
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
        }
    }
}