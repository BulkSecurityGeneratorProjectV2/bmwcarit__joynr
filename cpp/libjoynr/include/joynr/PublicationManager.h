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
#ifndef PUBLICATIONMANAGER_H
#define PUBLICATIONMANAGER_H

#include <mutex>
#include <memory>
#include <vector>
#include <map>
#include <string>

#include <boost/optional.hpp>

#include "joynr/SubscriptionPublication.h"
#include "joynr/MulticastPublication.h"
#include "joynr/SubscriptionRequestInformation.h"
#include "joynr/BroadcastSubscriptionRequestInformation.h"
#include "joynr/BroadcastFilterParameters.h"

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/ReadWriteLock.h"
#include "joynr/ThreadSafeMap.h"
#include "joynr/SubscriptionReply.h"

#include "joynr/MessagingQos.h"
#include "joynr/IJoynrMessageSender.h"

namespace boost
{
namespace asio
{
class io_service;
} // namespace asio
} // namespace boost

namespace joynr
{

class SubscriptionRequest;
class BroadcastSubscriptionRequest;
class MulticastSubscriptionRequest;
class SubscriptionInformation;
class IPublicationSender;
class RequestCaller;
class UnicastBroadcastListener;
class SubscriptionQos;

namespace exceptions
{
class JoynrException;
class SubscriptionException;
} // namespace exceptions
class SubscriptionQos;

/**
  * @class PublicationManager
  * @brief Publication manager receives subscription requests and prepares publications,
  * which are send back to the subscription manager.
  * Responsible for deleting SubscriptionRequests and PublicationStates (the runnable notifies the
  * SubscriptionManager when it terminates - this triggeres the delete).
  */
class JOYNR_EXPORT PublicationManager
{
public:
    PublicationManager(boost::asio::io_service& ioService,
                       IJoynrMessageSender* messageSender,
                       int maxThreads = 1);
    PublicationManager(DelayedScheduler* scheduler, IJoynrMessageSender* messageSender);
    virtual ~PublicationManager();
    /**
     * @brief Adds the SubscriptionRequest and starts runnable to poll attributes.
     * @param requestCaller
     * @param subscriptionRequest
     * @param publicationSender
     */
    void add(const std::string& proxyParticipantId,
             const std::string& providerParticipantId,
             std::shared_ptr<RequestCaller> requestCaller,
             SubscriptionRequest& subscriptionRequest,
             IPublicationSender* publicationSender);

    /**
     * @brief Adds SubscriptionRequest when the Provider is not yet registered
     *   and there is no RequestCaller as yet.
     *
     * @param subscriptionRequest
     */
    void add(const std::string& proxyParticipantId,
             const std::string& providerParticipantId,
             SubscriptionRequest& subscriptionRequest);

    /**
     * @brief Adds the BroadcastSubscriptionRequest and starts runnable to poll attributes.
     * @param requestCaller
     * @param subscriptionRequest
     * @param publicationSender
     */
    void add(const std::string& proxyParticipantId,
             const std::string& providerParticipantId,
             std::shared_ptr<RequestCaller> requestCaller,
             BroadcastSubscriptionRequest& subscriptionRequest,
             IPublicationSender* publicationSender);

    /**
     * @brief Adds MulticastSubscriptionRequest
     * @param proxyParticipantId
     * @param providerParticipantId
     * @param subscriptionRequest
     * @param publicationSender
     */
    void add(const std::string& proxyParticipantId,
             const std::string& providerParticipantId,
             MulticastSubscriptionRequest& subscriptionRequest,
             IPublicationSender* publicationSender);

    /**
     * @brief Adds BroadcastSubscriptionRequest when the Provider is not yet registered
     *   and there is no RequestCaller as yet.
     *
     * @param subscriptionRequest
     */
    void add(const std::string& proxyParticipantId,
             const std::string& providerParticipantId,
             BroadcastSubscriptionRequest& subscriptionRequest);

    /**
     * @brief Stops the sending of publications
     *
     * @param subscriptionId
     */
    void stopPublication(const std::string& subscriptionId);

    /**
     * @brief Stops all publications for a provider
     *
     * @param providerId
     */
    void removeAllSubscriptions(const std::string& providerId);

    /**
     * @brief Called by the Dispatcher every time a provider is registered to check whether there
     * are already subscriptionRequests waiting.
     *
     * @param providerId
     * @param requestCaller
     * @param publicationSender
     */
    void restore(const std::string& providerId,
                 std::shared_ptr<RequestCaller> requestCaller,
                 IPublicationSender* publicationSender);

    /**
      * @brief Publishes an onChange message when an attribute value changes
      *
      * @param subscriptionId A subscription that was listening on the attribute
      * @param value The new attribute value
      */
    template <typename T>
    void attributeValueChanged(const std::string& subscriptionId, const T& value);

    /**
      * @brief Publishes an broadcast publication message when a broadcast occurs
      *
      * This method is virtual so that it can be overridden by a mock object.
      * @param subscriptionId A subscription that was listening on the broadcast
      * @param values The new broadcast values
      */
    template <typename... Ts>
    void broadcastOccurred(const std::string& subscriptionId, const Ts&... values);

    /**
      * @brief Publishes a multicast broadcast publication message
      *
      * This method is virtual so that it can be overridden by a mock object.
      * @param broadcastName The name of the broadcast
      * @param providerParticipantId The participantID of the provider
      * @param partitions list of partitions the broadcast applies to
      * @param values Broadcast's value
      */
    template <typename... Ts>
    void broadcastOccurred(const std::string& broadcastName,
                           const std::string& providerParticipantId,
                           const std::vector<std::string>& partitions,
                           const Ts&... values);

    template <typename BroadcastFilter, typename... Ts>
    void selectiveBroadcastOccurred(const std::string& subscriptionId,
                                    const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
                                    const Ts&... values);

    void loadSavedBroadcastSubscriptionRequestsMap(const std::string& fileName);
    void loadSavedAttributeSubscriptionRequestsMap(const std::string& fileName);

private:
    DISALLOW_COPY_AND_ASSIGN(PublicationManager);

    // Used for multicast publication
    IJoynrMessageSender* joynrMessageSender;

    // A class that groups together the information needed for a publication
    class Publication;

    // Information for each publication is keyed by subcriptionId
    ThreadSafeMap<std::string, std::shared_ptr<Publication>> publications;
    ThreadSafeMap<std::string, std::shared_ptr<SubscriptionRequestInformation>>
            subscriptionId2SubscriptionRequest;
    ThreadSafeMap<std::string, std::shared_ptr<BroadcastSubscriptionRequestInformation>>
            subscriptionId2BroadcastSubscriptionRequest;

    std::mutex fileWriteLock;
    // Publications are scheduled to run on a thread pool
    DelayedScheduler* delayedScheduler;

    // Support for clean shutdowns
    std::mutex shutDownMutex;
    bool shuttingDown;

    // Subscription persistence
    std::string subscriptionRequestStorageFileName;
    std::string broadcastSubscriptionRequestStorageFileName;

    // Queues all subscription requests that are either received by the
    // dispatcher or restored from the subscription storage file before
    // the corresponding provider is added
    std::multimap<std::string, std::shared_ptr<SubscriptionRequestInformation>>
            queuedSubscriptionRequests;
    std::mutex queuedSubscriptionRequestsMutex;

    // Queues all broadcast subscription requests that are either received by the
    // dispatcher or restored from the subscription storage file before
    // the corresponding provider is added
    std::multimap<std::string, std::shared_ptr<BroadcastSubscriptionRequestInformation>>
            queuedBroadcastSubscriptionRequests;
    std::mutex queuedBroadcastSubscriptionRequestsMutex;

    // Logging
    ADD_LOGGER(PublicationManager);

    // List of subscriptionId's of runnables scheduled with delay <= qos.getMinIntervalMs_ms()
    std::vector<std::string> currentScheduledPublications;
    std::mutex currentScheduledPublicationsMutex;

    // Read/write lock for broadcast filters
    mutable ReadWriteLock broadcastFilterLock;

    // PublisherRunnables are used to send publications via a ThreadPool
    class PublisherRunnable;

    // PublicationEndRunnables finish a publication
    class PublicationEndRunnable;

    // Functions called by runnables
    void pollSubscription(const std::string& subscriptionId);
    void removePublication(const std::string& subscriptionId);
    void removeAttributePublication(const std::string& subscriptionId,
                                    const bool updatePersistenceFile = true);
    void removeBroadcastPublication(const std::string& subscriptionId,
                                    const bool updatePersistenceFile = true);

    // Helper functions
    void sendSubscriptionReply(IPublicationSender* publicationSender,
                               const std::string& fromParticipantId,
                               const std::string& toParticipantId,
                               const std::int64_t& expiryDateMs,
                               const SubscriptionReply& subscriptionReply);
    void sendSubscriptionReply(IPublicationSender* publicationSender,
                               const std::string& fromParticipantId,
                               const std::string& toParticipantId,
                               const std::int64_t& expiryDateMs,
                               const std::string& subscriptionId);
    void sendSubscriptionReply(IPublicationSender* publicationSender,
                               const std::string& fromParticipantId,
                               const std::string& toParticipantId,
                               const std::int64_t& expiryDateMs,
                               const std::string& subscriptionId,
                               const std::shared_ptr<exceptions::SubscriptionException>& error);
    bool publicationExists(const std::string& subscriptionId) const;
    void createPublishRunnable(const std::string& subscriptionId);
    void saveAttributeSubscriptionRequestsMap();
    void saveBroadcastSubscriptionRequestsMap();

    void reschedulePublication(const std::string& subscriptionId, std::int64_t nextPublication);

    bool isPublicationAlreadyScheduled(const std::string& subscriptionId);

    /**
     * @brief getTimeUntilNextPublication determines the time to wait until the next publication
     * can be sent base on the QOS information.
     * @param subscriptionId
     * @param qos
     * @return  0 if publication can immediately be sent;
     *          amount of ms to wait, if interval was too short;
     *          -1 on error
     */
    std::int64_t getTimeUntilNextPublication(std::shared_ptr<Publication> publication,
                                             const std::shared_ptr<SubscriptionQos> qos);

    template <typename Map>
    void saveSubscriptionRequestsMap(const Map& map, const std::string& storageFilename);

    template <class RequestInformationType>
    void loadSavedSubscriptionRequestsMap(
            const std::string& storageFilename,
            std::mutex& mutex,
            std::multimap<std::string, std::shared_ptr<RequestInformationType>>&
                    queuedSubscriptions);

    bool isShuttingDown();
    std::int64_t getPublicationTtlMs(
            std::shared_ptr<SubscriptionRequest> subscriptionRequest) const;

    void sendPublication(std::shared_ptr<Publication> publication,
                         std::shared_ptr<SubscriptionInformation> subscriptionInformation,
                         std::shared_ptr<SubscriptionRequest> subscriptionRequest,
                         BaseReply&& value);

    void sendSubscriptionPublication(
            std::shared_ptr<Publication> publication,
            std::shared_ptr<SubscriptionInformation> subscriptionInformation,
            std::shared_ptr<SubscriptionRequest> request,
            SubscriptionPublication&& subscriptionPublication);

    void sendPublicationError(std::shared_ptr<Publication> publication,
                              std::shared_ptr<SubscriptionInformation> subscriptionInformation,
                              std::shared_ptr<SubscriptionRequest> subscriptionRequest,
                              std::shared_ptr<exceptions::JoynrRuntimeException> exception);

    void handleAttributeSubscriptionRequest(
            std::shared_ptr<SubscriptionRequestInformation> requestInfo,
            std::shared_ptr<RequestCaller> requestCaller,
            IPublicationSender* publicationSender);

    void handleBroadcastSubscriptionRequest(
            std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo,
            std::shared_ptr<RequestCaller> requestCaller,
            IPublicationSender* publicationSender);

    void addOnChangePublication(const std::string& subscriptionId,
                                std::shared_ptr<SubscriptionRequestInformation> request,
                                std::shared_ptr<Publication> publication);

    void addBroadcastPublication(const std::string& subscriptionId,
                                 std::shared_ptr<BroadcastSubscriptionRequestInformation> request,
                                 std::shared_ptr<Publication> publication);

    void removeOnChangePublication(const std::string& subscriptionId,
                                   std::shared_ptr<SubscriptionRequestInformation> request,
                                   std::shared_ptr<Publication> publication);

    void removePublicationEndRunnable(std::shared_ptr<Publication> publication);

    template <typename BroadcastFilter, typename... Ts>
    bool processFilterChain(const std::string& subscriptionId,
                            const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
                            const Ts&... broadcastValues);
};

} // namespace joynr

#include "joynr/SubscriptionAttributeListener.h"
#include "joynr/UnicastBroadcastListener.h"

namespace joynr
{

//------ HelperClasses ---------------------------------------------------------

class PublicationManager::Publication
{
public:
    Publication(IPublicationSender* publicationSender,
                std::shared_ptr<RequestCaller> requestCaller);
    // This class is not responsible for deleting the PublicationSender or AttributeListener
    ~Publication() = default;

    std::int64_t timeOfLastPublication;
    IPublicationSender* sender;
    std::shared_ptr<RequestCaller> requestCaller;
    SubscriptionAttributeListener* attributeListener;
    UnicastBroadcastListener* broadcastListener;
    std::recursive_mutex mutex;
    DelayedScheduler::RunnableHandle publicationEndRunnableHandle;

private:
    DISALLOW_COPY_AND_ASSIGN(Publication);
};

template <typename T>
void PublicationManager::attributeValueChanged(const std::string& subscriptionId, const T& value)
{
    JOYNR_LOG_DEBUG(logger, "attributeValueChanged for onChange subscription {}", subscriptionId);

    // See if the subscription is still valid
    if (!publicationExists(subscriptionId)) {
        JOYNR_LOG_ERROR(logger,
                        "attributeValueChanged called for non-existing subscription {}",
                        subscriptionId);
        return;
    }

    std::shared_ptr<SubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2SubscriptionRequest.value(subscriptionId));

    std::shared_ptr<Publication> publication(publications.value(subscriptionId));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        if (!isPublicationAlreadyScheduled(subscriptionId)) {
            std::int64_t timeUntilNextPublication =
                    getTimeUntilNextPublication(publication, subscriptionRequest->getQos());

            if (timeUntilNextPublication == 0) {
                // Send the publication
                BaseReply replyValue;
                replyValue.setResponse(value);
                sendPublication(publication,
                                subscriptionRequest,
                                subscriptionRequest,
                                std::move(replyValue));
            } else {
                reschedulePublication(subscriptionId, timeUntilNextPublication);
            }
        }
    }
}

template <typename... Ts>
void PublicationManager::broadcastOccurred(const std::string& broadcastName,
                                           const std::string& providerParticipantId,
                                           const std::vector<std::string>& partitions,
                                           const Ts&... values)
{
    MulticastPublication publication;
    std::string multicastID =
            util::createMulticastId(providerParticipantId, broadcastName, partitions);
    publication.setMulticastId(multicastID);
    publication.setResponse(values...);
    MessagingQos mQos;
    joynrMessageSender->sendMulticast(providerParticipantId, publication, mQos);
}

template <typename... Ts>
void PublicationManager::broadcastOccurred(const std::string& subscriptionId, const Ts&... values)
{
    JOYNR_LOG_DEBUG(logger,
                    "broadcastOccurred for subscription {}.  Number of values: ",
                    subscriptionId,
                    sizeof...(Ts));

    // See if the subscription is still valid
    if (!publicationExists(subscriptionId)) {
        JOYNR_LOG_ERROR(logger,
                        "broadcastOccurred called for non-existing subscription {}",
                        subscriptionId);
        return;
    }

    std::shared_ptr<BroadcastSubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2BroadcastSubscriptionRequest.value(subscriptionId));
    std::shared_ptr<Publication> publication(publications.value(subscriptionId));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Only proceed if publication can immediately be sent
        std::int64_t timeUntilNextPublication =
                getTimeUntilNextPublication(publication, subscriptionRequest->getQos());

        if (timeUntilNextPublication == 0) {
            // Send the publication
            BaseReply replyValues;
            replyValues.setResponse(values...);
            sendPublication(
                    publication, subscriptionRequest, subscriptionRequest, std::move(replyValues));
        } else {
            if (timeUntilNextPublication > 0) {
                JOYNR_LOG_DEBUG(logger,
                                "Omitting broadcast publication for subscription {} because of too "
                                "short interval. Next publication possible in {} ms",
                                subscriptionId,
                                timeUntilNextPublication);
            } else {
                JOYNR_LOG_DEBUG(
                        logger,
                        "Omitting broadcast publication for subscription {} because of error.",
                        subscriptionId);
            }
        }
    }
}

template <typename BroadcastFilter, typename... Ts>
void PublicationManager::selectiveBroadcastOccurred(
        const std::string& subscriptionId,
        const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
        const Ts&... values)
{

    JOYNR_LOG_DEBUG(logger,
                    "selectiveBroadcastOccurred for subscription {}.  Number of values: ",
                    subscriptionId,
                    sizeof...(Ts));

    // See if the subscription is still valid
    if (!publicationExists(subscriptionId)) {
        JOYNR_LOG_ERROR(logger,
                        "broadcastOccurred called for non-existing subscription {}",
                        subscriptionId);
        return;
    }

    std::shared_ptr<BroadcastSubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2BroadcastSubscriptionRequest.value(subscriptionId));
    std::shared_ptr<Publication> publication(publications.value(subscriptionId));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Only proceed if publication can immediately be sent
        std::int64_t timeUntilNextPublication =
                getTimeUntilNextPublication(publication, subscriptionRequest->getQos());

        if (timeUntilNextPublication == 0) {
            // Execute broadcast filters
            if (processFilterChain(subscriptionId, filters, values...)) {
                // Send the publication
                BaseReply replyValues;
                replyValues.setResponse(values...);
                sendPublication(publication,
                                subscriptionRequest,
                                subscriptionRequest,
                                std::move(replyValues));
            }
        } else {
            if (timeUntilNextPublication > 0) {
                JOYNR_LOG_DEBUG(logger,
                                "Omitting broadcast publication for subscription {} because of too "
                                "short interval. Next publication possible in {} ms",
                                subscriptionId,
                                timeUntilNextPublication);
            } else {
                JOYNR_LOG_DEBUG(
                        logger,
                        "Omitting broadcast publication for subscription {} because of error.",
                        subscriptionId);
            }
        }
    }
}

template <typename BroadcastFilter, typename... Ts>
bool PublicationManager::processFilterChain(
        const std::string& subscriptionId,
        const std::vector<std::shared_ptr<BroadcastFilter>>& filters,
        const Ts&... broadcastValues)
{
    bool success = true;

    std::shared_ptr<BroadcastSubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2BroadcastSubscriptionRequest.value(subscriptionId));

    const boost::optional<BroadcastFilterParameters>& filterParameters =
            subscriptionRequest->getFilterParameters();
    const BroadcastFilterParameters& bfp =
            (filterParameters) ? *filterParameters : BroadcastFilterParameters();
    for (auto filterIt = filters.begin(); success && (filterIt != filters.cend()); ++filterIt) {
        success = success && (*filterIt)->filterForward(broadcastValues..., bfp);
    }
    return success;
}

} // namespace joynr
#endif // PUBLICATIONMANAGER_H
