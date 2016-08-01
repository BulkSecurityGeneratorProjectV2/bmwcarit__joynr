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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/JoynrMessage.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/JoynrMessageSender.h"
#include "joynr/Dispatcher.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/SubscriptionStop.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/MetaTypeRegistrar.h"
#include "joynr/tests/testRequestInterpreter.h"
#include "tests/utils/MockObjects.h"
#include "utils/MockCallback.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include <string>
#include "joynr/LibjoynrSettings.h"
#include "joynr/tests/testTypes/TestEnum.h"
#include "joynr/tests/testRequestCaller.h"
#include "joynr/types/Localisation/GpsLocation.h"

using namespace ::testing;

using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

/**
  * Is an integration test. Tests from Dispatcher -> SubscriptionListener and RequestCaller
  */
class SubscriptionTest : public ::testing::Test {
public:
    SubscriptionTest() :
        mockMessageRouter(new MockMessageRouter()),
        mockCallback(new MockCallbackWithJoynrException<types::Localisation::GpsLocation>()),
        mockRequestCaller(new MockTestRequestCaller()),
        mockReplyCaller(new MockReplyCaller<types::Localisation::GpsLocation>(
                [this](const types::Localisation::GpsLocation& location) {
                    mockCallback->onSuccess(location);
                },
                [] (const exceptions::JoynrException& error){
                })),
        mockGpsLocationListener(new MockSubscriptionListenerOneType<types::Localisation::GpsLocation>()),
        mockTestEnumSubscriptionListener(new MockSubscriptionListenerOneType<tests::testTypes::TestEnum::Enum>()),
        gpsLocation1(1.1, 2.2, 3.3, types::Localisation::GpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 444),
        qos(2000),
        providerParticipantId("providerParticipantId"),
        proxyParticipantId("proxyParticipantId"),
        requestReplyId("requestReplyId"),
        messageFactory(),
        messageSender(mockMessageRouter),
        dispatcher(&messageSender),
        subscriptionManager(nullptr),
        provider(new MockTestProvider),
        publicationManager(nullptr),
        requestCaller(new joynr::tests::testRequestCaller(provider))
    {
    }

    void SetUp(){
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str()); //remove stored subscriptions
        subscriptionManager = new SubscriptionManager();
        publicationManager = new PublicationManager();
        dispatcher.registerPublicationManager(publicationManager);
        dispatcher.registerSubscriptionManager(subscriptionManager);
        InterfaceRegistrar::instance().registerRequestInterpreter<tests::testRequestInterpreter>(tests::ItestBase::INTERFACE_NAME());
        MetaTypeRegistrar::instance().registerMetaType<types::Localisation::GpsLocation>();
        MetaTypeRegistrar::instance().registerEnumMetaType<joynr::tests::testTypes::TestEnum>();
    }

    void TearDown(){

    }

protected:
    std::shared_ptr<MockMessageRouter> mockMessageRouter;
    std::shared_ptr<MockCallbackWithJoynrException<types::Localisation::GpsLocation> > mockCallback;

    std::shared_ptr<MockTestRequestCaller> mockRequestCaller;
    std::shared_ptr<MockReplyCaller<types::Localisation::GpsLocation> > mockReplyCaller;
    std::shared_ptr<MockSubscriptionListenerOneType<types::Localisation::GpsLocation> > mockGpsLocationListener;
    std::shared_ptr<MockSubscriptionListenerOneType<tests::testTypes::TestEnum::Enum> > mockTestEnumSubscriptionListener;

    types::Localisation::GpsLocation gpsLocation1;

    // create test data
    MessagingQos qos;
    std::string providerParticipantId;
    std::string proxyParticipantId;
    std::string requestReplyId;

    JoynrMessageFactory messageFactory;
    JoynrMessageSender messageSender;
    Dispatcher dispatcher;
    SubscriptionManager * subscriptionManager;
    std::shared_ptr<MockTestProvider> provider;
    PublicationManager* publicationManager;
    std::shared_ptr<joynr::tests::testRequestCaller> requestCaller;
private:
    DISALLOW_COPY_AND_ASSIGN(SubscriptionTest);
};


/**
  * Trigger:    The dispatcher receives a SubscriptionRequest.
  * Expected:   The PublicationManager creates a PublisherRunnable and polls
  *             the MockCaller for the attribute.
  */
TEST_F(SubscriptionTest, receive_subscriptionRequestAndPollAttribute) {

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    Semaphore semaphore(0);
    EXPECT_CALL(*mockRequestCaller, getLocation(_,_))
            .WillRepeatedly(
                DoAll(
                    Invoke(mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
                    ReleaseSemaphore(&semaphore)));

    std::string attributeName = "Location";
    Variant subscriptionQos = Variant::make<OnChangeWithKeepAliveSubscriptionQos>(OnChangeWithKeepAliveSubscriptionQos(
                500, // validity_ms
                1000, // minInterval_ms
                2000, // maxInterval_ms
                1000 // alertInterval_ms
    ));
    std::string subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    JoynrMessage msg = messageFactory.createSubscriptionRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                subscriptionRequest);

    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);
    dispatcher.receive(msg);

    // Wait for a call to be made to the mockRequestCaller
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
}


/**
  * Trigger:    The dispatcher receives a Publication.
  * Expected:   The SubscriptionManager retrieves the correct SubscriptionCallback and the
  *             Interpreter executes it correctly
  */
TEST_F(SubscriptionTest, receive_publication ) {

    // getType is used by the ReplyInterpreterFactory to create an interpreter for the reply
    // so this has to match with the type being passed to the dispatcher in the reply
    ON_CALL(*mockReplyCaller, getType()).WillByDefault(Return(std::string("GpsLocation")));

    // Use a semaphore to count and wait on calls to the mockGpsLocationListener
    Semaphore semaphore(0);
    EXPECT_CALL(*mockGpsLocationListener, onReceive(A<const types::Localisation::GpsLocation&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    //register the subscription on the consumer side
    std::string attributeName = "Location";
    Variant subscriptionQos = Variant::make<OnChangeWithKeepAliveSubscriptionQos>(OnChangeWithKeepAliveSubscriptionQos(
                500, // validity_ms
                1000, // minInterval_ms
                2000, // maxInterval_ms
                1000 // alertInterval_ms
    ));

    SubscriptionRequest subscriptionRequest;
    //construct a reply containing a GpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    std::vector<Variant> response;
    response.push_back(Variant::make<types::Localisation::GpsLocation>(gpsLocation1));
    subscriptionPublication.setResponseVariant(response);

    auto subscriptionCallback = std::make_shared<SubscriptionCallback<types::Localisation::GpsLocation>>(mockGpsLocationListener);

    // subscriptionRequest is an out param
    subscriptionManager->registerSubscription(
                attributeName,
                subscriptionCallback,
                subscriptionQos,
                subscriptionRequest);
    // incoming publication from the provider
    JoynrMessage msg = messageFactory.createSubscriptionPublication(
                providerParticipantId,
                proxyParticipantId,
                qos,
                subscriptionPublication);

    dispatcher.receive(msg);

    // Assert that only one subscription message is received by the subscription listener
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(1)));
}

/**
  * Trigger:    The dispatcher receives an enum Publication.
  * Expected:   The SubscriptionManager retrieves the correct SubscriptionCallback and the
  *             Interpreter executes it correctly
  */
TEST_F(SubscriptionTest, receive_enumPublication ) {

    // getType is used by the ReplyInterpreterFactory to create an interpreter for the reply
    // so this has to match with the type being passed to the dispatcher in the reply
    ON_CALL(*mockReplyCaller, getType()).WillByDefault(Return(std::string("TestEnum")));

    // Use a semaphore to count and wait on calls to the mockTestEnumSubscriptionListener
    Semaphore semaphore(0);
    EXPECT_CALL(*mockTestEnumSubscriptionListener, onReceive(A<const joynr::tests::testTypes::TestEnum::Enum&>()))
            .WillRepeatedly(ReleaseSemaphore(&semaphore));

    //register the subscription on the consumer side
    std::string attributeName = "testEnum";
    Variant subscriptionQos = Variant::make<OnChangeWithKeepAliveSubscriptionQos>(OnChangeWithKeepAliveSubscriptionQos(
                500, // validity_ms
                1000, // minInterval_ms
                2000, // maxInterval_ms
                1000 // alertInterval_ms
    ));

    SubscriptionRequest subscriptionRequest;
    //construct a reply containing a GpsLocation
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    std::vector<Variant> response;
    response.push_back(Variant::make<joynr::tests::testTypes::TestEnum::Enum>(tests::testTypes::TestEnum::ZERO));
    subscriptionPublication.setResponseVariant(response);

    auto subscriptionCallback = std::make_shared<SubscriptionCallback<joynr::tests::testTypes::TestEnum::Enum>>(mockTestEnumSubscriptionListener);

    // subscriptionRequest is an out param
    subscriptionManager->registerSubscription(
                attributeName,
                subscriptionCallback,
                subscriptionQos,
                subscriptionRequest);
    // incoming publication from the provider
    JoynrMessage msg = messageFactory.createSubscriptionPublication(
                providerParticipantId,
                proxyParticipantId,
                qos,
                subscriptionPublication);

    dispatcher.receive(msg);

    // Assert that only one subscription message is received by the subscription listener
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(1)));
}

/**
  * Precondition: Dispatcher receives a SubscriptionRequest for a not(yet) existing Provider.
  * Trigger:    The provider is registered.
  * Expected:   The PublicationManager registers the provider and notifies the PublicationManager
  *             to restore waiting Subscriptions
  */
TEST_F(SubscriptionTest, receive_RestoresSubscription) {

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    Semaphore semaphore(0);
    EXPECT_CALL(
            *mockRequestCaller,
            getLocation(A<std::function<void(const types::Localisation::GpsLocation&)>>(),
                        A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>())
    )
            .WillOnce(DoAll(
                    Invoke(mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
                    ReleaseSemaphore(&semaphore)
            ));
    std::string attributeName = "Location";
    Variant subscriptionQos = Variant::make<OnChangeWithKeepAliveSubscriptionQos>(OnChangeWithKeepAliveSubscriptionQos(
                500, // validity_ms
                1000, // minInterval_ms
                2000, // maxInterval_ms
                1000 // alertInterval_ms
    ));
    std::string subscriptionId = "SubscriptionID";

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    JoynrMessage msg = messageFactory.createSubscriptionRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                subscriptionRequest);
    // first received message with subscription request

    dispatcher.receive(msg);
    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(15)));
    //Try to acquire a semaphore for up to 15 seconds. Acquireing the semaphore will only work, if the mockRequestCaller has been called
    //and will be much faster than waiting for 1s to make sure it has been called
}

TEST_F(SubscriptionTest, sendPublication_attributeWithSingleArrayParam) {

    std::string subscriptionId = "SubscriptionID";
    Variant subscriptionQos =
            Variant::make<OnChangeSubscriptionQos>(OnChangeSubscriptionQos(
                800, // validity_ms
                0 // minInterval_ms
    ));

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    Semaphore semaphore(0);

    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName("listOfStrings");
    subscriptionRequest.setQos(subscriptionQos);

    EXPECT_CALL(
            *provider,
            getListOfStrings(A<std::function<void(const std::vector<std::string> &)>>(),
                    A<std::function<void(const joynr::exceptions::ProviderRuntimeException&)>>())
    )
            .WillOnce(DoAll(
                    Invoke(provider.get(), &MockTestProvider::invokeListOfStringsOnSuccess),
                    ReleaseSemaphore(&semaphore)
            ));

    auto mockMessageRouter = std::make_shared<MockMessageRouter>();
    JoynrMessageSender* joynrMessageSender = new JoynrMessageSender(mockMessageRouter);

    /* ensure the serialization succeeds and the first publication is send to the proxy */
    EXPECT_CALL(*mockMessageRouter, route(
                     AllOf(
                         A<JoynrMessage>(),
                         Property(&JoynrMessage::getHeaderFrom, Eq(providerParticipantId)),
                         Property(&JoynrMessage::getHeaderTo, Eq(proxyParticipantId))),
                     _
                     ));

    publicationManager->add(
                proxyParticipantId,
                providerParticipantId,
                requestCaller,
                subscriptionRequest,
                joynrMessageSender);

    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(15)));

    std::vector<std::string> listOfStrings;
    listOfStrings.push_back("1");
    listOfStrings.push_back("2");

    /* ensure the value change leads to another publication */
    Mock::VerifyAndClear(mockMessageRouter.get());
    EXPECT_CALL(*mockMessageRouter, route(
                     AllOf(
                         A<JoynrMessage>(),
                         Property(&JoynrMessage::getHeaderFrom, Eq(providerParticipantId)),
                         Property(&JoynrMessage::getHeaderTo, Eq(proxyParticipantId))),
                     _
                     ));


    provider->listOfStringsChanged(listOfStrings);

    delete joynrMessageSender;
}

/**
  * Precondition: A provider is registered and there is at least one subscription for it.
  * Trigger:    The request caller is removed from the dispatcher
  * Expected:   The PublicationManager stops all subscriptions for this provider
  */
TEST_F(SubscriptionTest, removeRequestCaller_stopsPublications) {

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    Semaphore semaphore(0);
    EXPECT_CALL(*mockRequestCaller, getLocation(_,_))
            .WillRepeatedly(
                DoAll(
                    Invoke(mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
                    ReleaseSemaphore(&semaphore)));

    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);
    Variant subscriptionQos = Variant::make<OnChangeWithKeepAliveSubscriptionQos>(OnChangeWithKeepAliveSubscriptionQos(
                1200, // validity_ms
                10, // minInterval_ms
                100, // maxInterval_ms
                1100 // alertInterval_ms
    ));
    std::string subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    std::string attributeName = "Location";
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    JoynrMessage msg = messageFactory.createSubscriptionRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                subscriptionRequest);
    // first received message with subscription request
    dispatcher.receive(msg);
    // wait for two requests from the subscription
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    // remove the request caller
    dispatcher.removeRequestCaller(providerParticipantId);
    // assert that less than 2 requests happen in the next 300 milliseconds
    semaphore.waitFor(std::chrono::milliseconds(300));
    ASSERT_FALSE(semaphore.waitFor(std::chrono::milliseconds(300)));
}

/**
  * Precondition: A provider is registered and there is at least one subscription for it.
  * Trigger:    A subscription stop message is received
  * Expected:   The PublicationManager stops the publications for this provider
  */
TEST_F(SubscriptionTest, stopMessage_stopsPublications) {

    // Use a semaphore to count and wait on calls to the mockRequestCaller
    Semaphore semaphore(0);
    EXPECT_CALL(*mockRequestCaller, getLocation(_,_))
            .WillRepeatedly(
                DoAll(
                    Invoke(mockRequestCaller.get(), &MockTestRequestCaller::invokeLocationOnSuccessFct),
                    ReleaseSemaphore(&semaphore)));

    dispatcher.addRequestCaller(providerParticipantId, mockRequestCaller);
    std::string attributeName = "Location";
    Variant subscriptionQos = Variant::make<OnChangeWithKeepAliveSubscriptionQos>(OnChangeWithKeepAliveSubscriptionQos(
                1200, // validity_ms
                10, // minInterval_ms
                500, // maxInterval_ms
                1100 // alertInterval_ms
    ));
    std::string subscriptionId = "SubscriptionID";
    SubscriptionRequest subscriptionRequest;
    subscriptionRequest.setSubscriptionId(subscriptionId);
    subscriptionRequest.setSubscribeToName(attributeName);
    subscriptionRequest.setQos(subscriptionQos);

    JoynrMessage msg = messageFactory.createSubscriptionRequest(
                proxyParticipantId,
                providerParticipantId,
                qos,
                subscriptionRequest);
    // first received message with subscription request
    dispatcher.receive(msg);

    // wait for two requests from the subscription
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));
    ASSERT_TRUE(semaphore.waitFor(std::chrono::seconds(1)));

    SubscriptionStop subscriptionStop;
    subscriptionStop.setSubscriptionId(subscriptionRequest.getSubscriptionId());
    // receive a subscription stop message
    msg = messageFactory.createSubscriptionStop(
                proxyParticipantId,
                providerParticipantId,
                qos,
                subscriptionStop);
    dispatcher.receive(msg);

    ASSERT_FALSE(semaphore.waitFor(std::chrono::seconds(1)));
}
