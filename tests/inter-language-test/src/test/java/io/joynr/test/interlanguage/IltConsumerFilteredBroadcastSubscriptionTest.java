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

import java.io.StringWriter;
import java.io.PrintWriter;

import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithFilteringBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithFilteringBroadcastFilterParameters;
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;

import org.junit.Test;
import static org.junit.Assert.fail;

import edu.umd.cs.findbugs.annotations.SuppressWarnings;
import io.joynr.exceptions.SubscriptionException;
import io.joynr.proxy.Future;

public class IltConsumerFilteredBroadcastSubscriptionTest extends IltConsumerTest {
    private static final Logger LOG = LoggerFactory.getLogger(IltConsumerTest.class);

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithFilteringCallbackDone = false;
    volatile boolean subscribeBroadcastWithFilteringCallbackResult = false;

    @SuppressWarnings("checkstyle:methodlength")
    @Test
    public void callSubscribeBroadcastWithFiltering() {
        Future<String> subscriptionIdFuture;
        String subscriptionId;
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
        boolean result;

        LOG.info(name.getMethodName() + "");

        try {
            BroadcastWithFilteringBroadcastFilterParameters filterParameters = new BroadcastWithFilteringBroadcastFilterParameters();
            String stringOfInterst = "fireBroadcast";
            filterParameters.setStringOfInterest(stringOfInterst);

            String[] stringArrayOfInterest = IltUtil.createStringArray();
            String json;
            try {
                LOG.info(name.getMethodName() + " - objectMapper is " + objectMapper);
                LOG.info(name.getMethodName() + " - objectMapper stringArrayOfInterest " + stringArrayOfInterest);
                json = objectMapper.writeValueAsString(stringArrayOfInterest);
            } catch (JsonProcessingException je) {
                fail(name.getMethodName() + " - FAILED - got exception when serializing stringArrayOfInterest"
                        + je.getMessage());
                return;
            }
            filterParameters.setStringArrayOfInterest(json);

            ExtendedTypeCollectionEnumerationInTypeCollection enumerationOfInterest = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
            try {
                json = objectMapper.writeValueAsString(enumerationOfInterest);
            } catch (JsonProcessingException je) {
                fail(name.getMethodName() + " - FAILED - got exception when serializing enumerationOfInterest"
                        + je.getMessage());
                return;
            }
            filterParameters.setEnumerationOfInterest(json);

            StructWithStringArray structWithStringArrayOfInterest = IltUtil.createStructWithStringArray();
            try {
                json = objectMapper.writeValueAsString(structWithStringArrayOfInterest);
            } catch (JsonProcessingException je) {
                fail(name.getMethodName()
                        + " - FAILED - got exception when serializing structWithStringArrayOfInterest"
                        + je.getMessage());
                return;
            }
            filterParameters.setStructWithStringArrayOfInterest(json);

            StructWithStringArray[] structWithStringArrayArrayOfInterest = IltUtil.createStructWithStringArrayArray();
            try {
                json = objectMapper.writeValueAsString(structWithStringArrayArrayOfInterest);
            } catch (JsonProcessingException je) {
                fail(name.getMethodName()
                        + " - FAILED - got exception when serializing structWithStringArrayArrayOfInterest"
                        + je.getMessage());
                return;
            }
            filterParameters.setStructWithStringArrayArrayOfInterest(json);

            subscriptionIdFuture = testInterfaceProxy.subscribeToBroadcastWithFilteringBroadcast(new BroadcastWithFilteringBroadcastAdapter() {
                                                                                                     @Override
                                                                                                     public void onReceive(String stringOut,
                                                                                                                           String[] stringArrayOut,
                                                                                                                           ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut,
                                                                                                                           StructWithStringArray structWithStringArrayOut,
                                                                                                                           StructWithStringArray[] structWithStringArrayArrayOut) {

                                                                                                         LOG.info(name.getMethodName()
                                                                                                                 + " - callback - got broadcast");

                                                                                                         if (!IltUtil.checkStringArray(stringArrayOut)) {
                                                                                                             subscribeBroadcastWithFilteringCallbackResult = false;
                                                                                                         } else if (enumerationOut != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
                                                                                                             LOG.info(name.getMethodName()
                                                                                                                     + " - callback - invalid content");
                                                                                                             subscribeBroadcastWithFilteringCallbackResult = false;
                                                                                                         } else if (!IltUtil.checkStructWithStringArray(structWithStringArrayOut)) {
                                                                                                             LOG.info(name.getMethodName()
                                                                                                                     + " - callback - invalid content");
                                                                                                             subscribeBroadcastWithFilteringCallbackResult = false;
                                                                                                         } else if (!IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
                                                                                                             LOG.info(name.getMethodName()
                                                                                                                     + " - callback - invalid content");
                                                                                                             subscribeBroadcastWithFilteringCallbackResult = false;
                                                                                                         } else {
                                                                                                             LOG.info(name.getMethodName()
                                                                                                                     + " - callback - content OK");
                                                                                                             subscribeBroadcastWithFilteringCallbackResult = true;
                                                                                                         }
                                                                                                         subscribeBroadcastWithFilteringCallbackDone = true;
                                                                                                     }

                                                                                                     @Override
                                                                                                     public void onError(SubscriptionException error) {
                                                                                                         LOG.info(name.getMethodName()
                                                                                                                 + " - callback - error");
                                                                                                         subscribeBroadcastWithFilteringCallbackResult = false;
                                                                                                         subscribeBroadcastWithFilteringCallbackDone = true;
                                                                                                     }
                                                                                                 },
                                                                                                 subscriptionQos,
                                                                                                 filterParameters);
            subscriptionId = subscriptionIdFuture.get(10000);
            LOG.info(name.getMethodName() + " - subscription successful, subscriptionId = " + subscriptionId);
            LOG.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            LOG.info(name.getMethodName() + " - Wait done, invoking fire method");
            String stringArg = "fireBroadcast";
            testInterfaceProxy.methodToFireBroadcastWithFiltering(stringArg);
            LOG.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithFilteringCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeBroadcastWithFilteringCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithFilteringCallbackResult) {
                LOG.info(name.getMethodName() + " - callback got called and received expected publication");
                result = true;
            } else {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
                result = false;
            }

            // get out, if first test run failed
            if (result == false) {
                return;
            }

            // reset counter for 2nd test
            subscribeBroadcastWithFilteringCallbackResult = false;
            subscribeBroadcastWithFilteringCallbackDone = false;

            LOG.info(name.getMethodName() + " - invoking fire method with wrong stringArg");
            stringArg = "doNotfireBroadcast";
            testInterfaceProxy.methodToFireBroadcastWithFiltering(stringArg);
            LOG.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithFilteringCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeBroadcastWithFilteringCallbackDone) {
                LOG.info(name.getMethodName() + " - callback did not get called in time (expected)");
                result = true;
            } else {
                fail(name.getMethodName() + " - FAILED - callback got called unexpectedly");
                result = false;
            }

            // try to unsubscribe
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
                LOG.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: " + e.getMessage());
                result = false;
            }

            if (!result) {
                LOG.info(name.getMethodName() + " - FAILED");
            } else {
                LOG.info(name.getMethodName() + " - OK");
            }
            return;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            StringWriter stringWriter = new StringWriter();
            PrintWriter printWriter = new PrintWriter(stringWriter);
            e.printStackTrace(printWriter);
            printWriter.flush();

            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + stringWriter.toString());
            return;
        }
    }
}
