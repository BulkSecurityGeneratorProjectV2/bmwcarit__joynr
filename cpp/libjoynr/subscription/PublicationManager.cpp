/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include "joynr/PublicationManager.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <sstream>

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/CallContextStorage.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/IPublicationSender.h"
#include "joynr/IRequestInterpreter.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/MessagingQos.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/RequestCaller.h"
#include "joynr/Runnable.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionUtil.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/TimePoint.h"
#include "joynr/UnicastSubscriptionQos.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"
#include "joynr/exceptions/SubscriptionException.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class PublicationManager::PublisherRunnable : public Runnable
{
public:
    ~PublisherRunnable() override = default;
    PublisherRunnable(std::weak_ptr<PublicationManager> publicationManager,
                      const std::string& subscriptionId);

    void shutdown() override;

    // Calls PublicationManager::pollSubscription()
    void run() override;

private:
    DISALLOW_COPY_AND_ASSIGN(PublisherRunnable);
    std::weak_ptr<PublicationManager> publicationManager;
    std::string subscriptionId;
};

class PublicationManager::PublicationEndRunnable : public Runnable
{
public:
    ~PublicationEndRunnable() override = default;
    PublicationEndRunnable(std::weak_ptr<PublicationManager> publicationManager,
                           const std::string& subscriptionId);

    void shutdown() override;

    // Calls PublicationManager::removePublication()
    void run() override;

private:
    DISALLOW_COPY_AND_ASSIGN(PublicationEndRunnable);
    std::weak_ptr<PublicationManager> publicationManager;
    std::string subscriptionId;
};

//------ PublicationManager ----------------------------------------------------

PublicationManager::~PublicationManager()
{
    std::lock_guard<std::mutex> shutDownLocker(shutDownMutex);
    assert(shuttingDown);
}

void PublicationManager::shutdown()
{
    // saveSubscriptionRequestsMap will not store to file, as soon as shuttingDown is true
    // except extra parameter is provided
    {
        std::lock_guard<std::mutex> shutDownLocker(shutDownMutex);
        assert(!shuttingDown);
        if (shuttingDown) {
            return;
        }
        shuttingDown = true;
    }

    JOYNR_LOG_TRACE(logger(), "shutting down thread pool and scheduler ...");
    delayedScheduler->shutdown();

    JOYNR_LOG_TRACE(logger(), "saving subscriptionsMap...");
    bool finalSave = true;
    saveAttributeSubscriptionRequestsMap(finalSave);
    saveBroadcastSubscriptionRequestsMap(finalSave);

    // Remove all publications
    JOYNR_LOG_TRACE(logger(), "removing publications");

    while (subscriptionId2SubscriptionRequest.size() > 0) {
        auto subscriptionRequest = subscriptionId2SubscriptionRequest.begin();
        removeAttributePublication((subscriptionRequest->second)->getSubscriptionId(), false);
    }

    while (subscriptionId2BroadcastSubscriptionRequest.size() > 0) {
        auto broadcastRequest = subscriptionId2BroadcastSubscriptionRequest.begin();
        removeBroadcastPublication((broadcastRequest->second)->getSubscriptionId(), false);
    }
}

PublicationManager::PublicationManager(boost::asio::io_service& ioService,
                                       std::weak_ptr<IMessageSender> messageSender,
                                       bool enableSubscriptionStorage,
                                       std::uint64_t ttlUplift,
                                       int maxThreads)
        : messageSender(messageSender),
          publications(),
          subscriptionId2SubscriptionRequest(),
          subscriptionId2BroadcastSubscriptionRequest(),
          fileWriteLock(),
          delayedScheduler(std::make_shared<ThreadPoolDelayedScheduler>(maxThreads,
                                                                        "PubManager",
                                                                        ioService)),
          shutDownMutex(),
          shuttingDown(false),
          subscriptionRequestStorageFileName(),
          broadcastSubscriptionRequestStorageFileName(),
          queuedSubscriptionRequests(),
          queuedSubscriptionRequestsMutex(),
          queuedBroadcastSubscriptionRequests(),
          queuedBroadcastSubscriptionRequestsMutex(),
          currentScheduledPublications(),
          currentScheduledPublicationsMutex(),
          broadcastFilterLock(),
          ttlUplift(ttlUplift),
          enableSubscriptionStorage(enableSubscriptionStorage)
{
}

bool isSubscriptionExpired(const std::shared_ptr<SubscriptionQos> qos, int offset = 0)
{
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch()).count();
    return qos->getExpiryDateMs() != SubscriptionQos::NO_EXPIRY_DATE() &&
           qos->getExpiryDateMs() < (now + offset);
}

void PublicationManager::sendSubscriptionReply(std::weak_ptr<IPublicationSender> publicationSender,
                                               const std::string& fromParticipantId,
                                               const std::string& toParticipantId,
                                               std::int64_t expiryDateMs,
                                               const SubscriptionReply& subscriptionReply)
{
    MessagingQos messagingQos;
    if (expiryDateMs == SubscriptionQos::NO_EXPIRY_DATE()) {
        messagingQos.setTtl(INT64_MAX);
    } else {
        const auto timePoint = TimePoint::fromAbsoluteMs(expiryDateMs);
        messagingQos.setTtl(timePoint.relativeFromNow().count());
    }
    if (auto publicationSenderSharedPtr = publicationSender.lock()) {
        publicationSenderSharedPtr->sendSubscriptionReply(
                fromParticipantId, toParticipantId, messagingQos, subscriptionReply);
    }
}

void PublicationManager::sendSubscriptionReply(std::weak_ptr<IPublicationSender> publicationSender,
                                               const std::string& fromParticipantId,
                                               const std::string& toParticipantId,
                                               std::int64_t expiryDateMs,
                                               const std::string& subscriptionId)
{
    SubscriptionReply subscriptionReply;
    subscriptionReply.setSubscriptionId(subscriptionId);
    sendSubscriptionReply(
            publicationSender, fromParticipantId, toParticipantId, expiryDateMs, subscriptionReply);
}

void PublicationManager::sendSubscriptionReply(
        std::weak_ptr<IPublicationSender> publicationSender,
        const std::string& fromParticipantId,
        const std::string& toParticipantId,
        std::int64_t expiryDateMs,
        const std::string& subscriptionId,
        std::shared_ptr<exceptions::SubscriptionException> error)
{
    SubscriptionReply subscriptionReply;
    subscriptionReply.setSubscriptionId(subscriptionId);
    subscriptionReply.setError(std::move(error));
    sendSubscriptionReply(
            publicationSender, fromParticipantId, toParticipantId, expiryDateMs, subscriptionReply);
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             std::shared_ptr<RequestCaller> requestCaller,
                             SubscriptionRequest& subscriptionRequest,
                             std::weak_ptr<IPublicationSender> publicationSender)
{
    assert(requestCaller);
    auto requestInfo = std::make_shared<SubscriptionRequestInformation>(proxyParticipantId,
                                                                        providerParticipantId,
                                                                        CallContextStorage::get(),
                                                                        subscriptionRequest);
    handleAttributeSubscriptionRequest(requestInfo, requestCaller, publicationSender);
}

void PublicationManager::addSubscriptionCleanupIfNecessary(std::shared_ptr<Publication> publication,
                                                           std::shared_ptr<SubscriptionQos> qos,
                                                           const std::string& subscriptionId)
{
    if (qos->getExpiryDateMs() != SubscriptionQos::NO_EXPIRY_DATE()) {

        std::chrono::hours tolerance = std::chrono::hours(1);
        const TimePoint max = TimePoint::max() - tolerance;
        TimePoint publicationEnd;
        if (qos->getExpiryDateMs() > max.toMilliseconds() - static_cast<std::int64_t>(ttlUplift)) {
            publicationEnd = max;
        } else {
            publicationEnd = TimePoint::fromAbsoluteMs(qos->getExpiryDateMs() + ttlUplift);
        }
        publication->publicationEndRunnableHandle = delayedScheduler->schedule(
                std::make_shared<PublicationEndRunnable>(shared_from_this(), subscriptionId),
                publicationEnd.relativeFromNow());
        JOYNR_LOG_TRACE(logger(),
                        "publication will end in {}  ms",
                        publicationEnd.relativeFromNow().count());
    }
}

void PublicationManager::handleAttributeSubscriptionRequest(
        std::shared_ptr<SubscriptionRequestInformation> requestInfo,
        std::shared_ptr<RequestCaller> requestCaller,
        std::weak_ptr<IPublicationSender> publicationSender)
{
    const std::string& subscriptionId = requestInfo->getSubscriptionId();
    auto publication = std::make_shared<Publication>(publicationSender, requestCaller);

    if (publicationExists(subscriptionId)) {
        JOYNR_LOG_TRACE(logger(),
                        "Publication with id: {}  already exists. Updating...",
                        requestInfo->getSubscriptionId());
        removeAttributePublication(subscriptionId);
    }

    subscriptionId2SubscriptionRequest.insert(subscriptionId, requestInfo);
    // Make note of the publication
    publications.insert(subscriptionId, publication);

    saveAttributeSubscriptionRequestsMap();

    JOYNR_LOG_TRACE(logger(), "added subscription: {}", requestInfo->toString());

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Add an onChange publication if needed
        addOnChangePublication(subscriptionId, requestInfo, publication);

        // Schedule a runnable to remove the publication when it finishes
        const std::shared_ptr<SubscriptionQos> qos = requestInfo->getQos();

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            addSubscriptionCleanupIfNecessary(publication, qos, subscriptionId);
            {
                std::lock_guard<std::mutex> currentScheduledLocker(
                        currentScheduledPublicationsMutex);
                currentScheduledPublications.push_back(subscriptionId);
            }
            sendSubscriptionReply(publicationSender,
                                  requestInfo->getProviderId(),
                                  requestInfo->getProxyId(),
                                  qos->getExpiryDateMs(),
                                  subscriptionId);
            // sent at least once the current value
            delayedScheduler->schedule(
                    std::make_shared<PublisherRunnable>(shared_from_this(), subscriptionId));
        } else {
            JOYNR_LOG_WARN(logger(), "publication end is in the past");
            const TimePoint expiryDate = TimePoint::fromRelativeMs(60000) + ttlUplift;
            sendSubscriptionReply(publicationSender,
                                  requestInfo->getProviderId(),
                                  requestInfo->getProxyId(),
                                  expiryDate.toMilliseconds(),
                                  subscriptionId,
                                  std::make_shared<exceptions::SubscriptionException>(
                                          "publication end is in the past", subscriptionId));
        }
    }
}

void PublicationManager::addOnChangePublication(
        const std::string& subscriptionId,
        std::shared_ptr<SubscriptionRequestInformation> request,
        std::shared_ptr<Publication> publication)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    if (SubscriptionUtil::isOnChangeSubscription(request->getQos())) {
        JOYNR_LOG_TRACE(logger(), "adding onChange subscription: {}", subscriptionId);

        // Create an attribute listener to listen for onChange events
        std::shared_ptr<SubscriptionAttributeListener> attributeListener =
                std::make_shared<SubscriptionAttributeListener>(subscriptionId, shared_from_this());

        // Register the attribute listener
        std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->registerAttributeListener(request->getSubscribeToName(), attributeListener);

        // Make note of the attribute listener so that it can be unregistered
        publication->attributeListener = std::move(attributeListener);
    }
}

void PublicationManager::addBroadcastPublication(
        const std::string& subscriptionId,
        std::shared_ptr<BroadcastSubscriptionRequestInformation> request,
        std::shared_ptr<PublicationManager::Publication> publication)
{
    JOYNR_LOG_TRACE(logger(), "adding broadcast subscription: {}", subscriptionId);

    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));

    // Create a broadcast listener to listen for broadcast events
    std::shared_ptr<UnicastBroadcastListener> broadcastListener =
            std::make_shared<UnicastBroadcastListener>(subscriptionId, shared_from_this());

    // Register the broadcast listener
    std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
    requestCaller->registerBroadcastListener(request->getSubscribeToName(), broadcastListener);

    // Make note of the attribute listener so that it can be unregistered
    publication->broadcastListener = std::move(broadcastListener);
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             SubscriptionRequest& subscriptionRequest)
{
    JOYNR_LOG_TRACE(logger(),
                    "Added subscription for non existing provider (adding subscriptionRequest "
                    "to queue).");
    auto requestInfo = std::make_shared<SubscriptionRequestInformation>(proxyParticipantId,
                                                                        providerParticipantId,
                                                                        CallContextStorage::get(),
                                                                        subscriptionRequest);
    {
        std::lock_guard<std::mutex> queueLocker(queuedSubscriptionRequestsMutex);
        queuedSubscriptionRequests.insert(
                std::make_pair(requestInfo->getProviderId(), requestInfo));
    }

    subscriptionId2SubscriptionRequest.insert(requestInfo->getSubscriptionId(), requestInfo);
    saveAttributeSubscriptionRequestsMap();
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             MulticastSubscriptionRequest& subscriptionRequest,
                             std::weak_ptr<IPublicationSender> publicationSender)
{
    // silently handle multicast subscription request
    sendSubscriptionReply(publicationSender,
                          providerParticipantId,
                          proxyParticipantId,
                          subscriptionRequest.getQos()->getExpiryDateMs(),
                          subscriptionRequest.getSubscriptionId());
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             std::shared_ptr<RequestCaller> requestCaller,
                             BroadcastSubscriptionRequest& subscriptionRequest,
                             std::weak_ptr<IPublicationSender> publicationSender)
{
    assert(requestCaller);
    auto requestInfo = std::make_shared<BroadcastSubscriptionRequestInformation>(
            proxyParticipantId, providerParticipantId, subscriptionRequest);

    handleBroadcastSubscriptionRequest(requestInfo, requestCaller, publicationSender);
}

void PublicationManager::handleBroadcastSubscriptionRequest(
        std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo,
        std::shared_ptr<RequestCaller> requestCaller,
        std::weak_ptr<IPublicationSender> publicationSender)
{
    const std::string& subscriptionId = requestInfo->getSubscriptionId();

    auto publication = std::make_shared<Publication>(publicationSender, requestCaller);

    if (publicationExists(subscriptionId)) {
        JOYNR_LOG_TRACE(logger(),
                        "Publication with id: {}  already exists. Updating...",
                        requestInfo->getSubscriptionId());
        removeBroadcastPublication(subscriptionId);
    }

    subscriptionId2BroadcastSubscriptionRequest.insert(subscriptionId, requestInfo);

    // Make note of the publication
    publications.insert(subscriptionId, publication);
    JOYNR_LOG_TRACE(logger(), "added subscription: {}", requestInfo->toString());

    saveBroadcastSubscriptionRequestsMap();

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Add an onChange publication if needed
        addBroadcastPublication(subscriptionId, requestInfo, publication);

        // Schedule a runnable to remove the publication when it finishes
        const std::shared_ptr<SubscriptionQos> qos = requestInfo->getQos();

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            addSubscriptionCleanupIfNecessary(publication, qos, subscriptionId);
            sendSubscriptionReply(publicationSender,
                                  requestInfo->getProviderId(),
                                  requestInfo->getProxyId(),
                                  qos->getExpiryDateMs(),
                                  subscriptionId);
        } else {
            JOYNR_LOG_WARN(logger(), "publication end is in the past");
            const TimePoint expiryDate = TimePoint::fromRelativeMs(60000) + ttlUplift;
            sendSubscriptionReply(publicationSender,
                                  requestInfo->getProviderId(),
                                  requestInfo->getProxyId(),
                                  expiryDate.toMilliseconds(),
                                  subscriptionId,
                                  std::make_shared<exceptions::SubscriptionException>(
                                          "publication end is in the past", subscriptionId));
        }
    }
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             BroadcastSubscriptionRequest& subscriptionRequest)
{
    JOYNR_LOG_TRACE(logger(),
                    "Added broadcast subscription for non existing provider (adding "
                    "subscriptionRequest to queue).");
    auto requestInfo = std::make_shared<BroadcastSubscriptionRequestInformation>(
            proxyParticipantId, providerParticipantId, subscriptionRequest);
    {
        std::lock_guard<std::mutex> queueLocker(queuedBroadcastSubscriptionRequestsMutex);
        queuedBroadcastSubscriptionRequests.insert(
                std::make_pair(requestInfo->getProviderId(), requestInfo));
    }

    subscriptionId2BroadcastSubscriptionRequest.insert(
            requestInfo->getSubscriptionId(), requestInfo);
    saveBroadcastSubscriptionRequestsMap();
}

void PublicationManager::removeAllSubscriptions(const std::string& providerId)
{
    JOYNR_LOG_TRACE(logger(), "Removing all subscriptions for provider id= {}", providerId);

    // Build lists of subscriptionIds to remove
    std::vector<std::string> publicationsToRemove;
    {
        auto callback = [&publicationsToRemove, &providerId](auto&& map) {
            for (auto&& requestInfo : map) {
                std::string subscriptionId = requestInfo.second->getSubscriptionId();

                if ((requestInfo.second)->getProviderId() == providerId) {
                    publicationsToRemove.push_back(subscriptionId);
                }
            }
        };
        subscriptionId2SubscriptionRequest.applyReadFun(callback);
    }

    std::vector<std::string> broadcastsToRemove;
    {
        auto callback = [&broadcastsToRemove, &providerId](auto&& map) {
            for (auto& requestInfo : map) {
                std::string subscriptionId = requestInfo.second->getSubscriptionId();

                if ((requestInfo.second)->getProviderId() == providerId) {
                    broadcastsToRemove.push_back(subscriptionId);
                }
            }
        };
        subscriptionId2BroadcastSubscriptionRequest.applyReadFun(callback);
    }

    // Remove each publication
    for (const std::string& subscriptionId : publicationsToRemove) {
        JOYNR_LOG_TRACE(logger(),
                        "Removing subscription providerId= {}, subscriptionId = {}",
                        providerId,
                        subscriptionId);
        removeAttributePublication(subscriptionId);
    }

    // Remove each broadcast
    for (const std::string& subscriptionId : broadcastsToRemove) {
        JOYNR_LOG_TRACE(logger(),
                        "Removing subscription providerId= {}, subscriptionId = {}",
                        providerId,
                        subscriptionId);
        removeBroadcastPublication(subscriptionId);
    }
}

void PublicationManager::stopPublication(const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(logger(), "stopPublication: {}", subscriptionId);
    removePublication(subscriptionId);
}

bool PublicationManager::publicationExists(const std::string& subscriptionId) const
{
    return publications.contains(subscriptionId);
}

void PublicationManager::restore(const std::string& providerId,
                                 std::shared_ptr<RequestCaller> requestCaller,
                                 std::weak_ptr<IPublicationSender> publicationSender)
{
    JOYNR_LOG_TRACE(logger(), "restore: entering ...");

    {
        std::lock_guard<std::mutex> queueLocker(queuedSubscriptionRequestsMutex);
        std::multimap<std::string, std::shared_ptr<SubscriptionRequestInformation>>::iterator
                queuedSubscriptionRequestsIterator = queuedSubscriptionRequests.find(providerId);
        while (queuedSubscriptionRequestsIterator != queuedSubscriptionRequests.end()) {
            std::shared_ptr<SubscriptionRequestInformation> requestInfo(
                    queuedSubscriptionRequestsIterator->second);
            queuedSubscriptionRequests.erase(queuedSubscriptionRequestsIterator);
            if (!isSubscriptionExpired(requestInfo->getQos())) {
                JOYNR_LOG_TRACE(logger(),
                                "Restoring subscription for provider: {} {}",
                                providerId,
                                requestInfo->toString());
                handleAttributeSubscriptionRequest(requestInfo, requestCaller, publicationSender);
            }
            queuedSubscriptionRequestsIterator = queuedSubscriptionRequests.find(providerId);
        }
    }

    {
        std::lock_guard<std::mutex> queueLocker(queuedBroadcastSubscriptionRequestsMutex);
        std::multimap<std::string,
                      std::shared_ptr<BroadcastSubscriptionRequestInformation>>::iterator
                queuedBroadcastSubscriptionRequestsIterator =
                        queuedBroadcastSubscriptionRequests.find(providerId);
        while (queuedBroadcastSubscriptionRequestsIterator !=
               queuedBroadcastSubscriptionRequests.end()) {
            std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo(
                    queuedBroadcastSubscriptionRequestsIterator->second);
            queuedBroadcastSubscriptionRequests.erase(queuedBroadcastSubscriptionRequestsIterator);
            if (!isSubscriptionExpired(requestInfo->getQos())) {
                JOYNR_LOG_TRACE(logger(),
                                "Restoring subscription for provider: {}  {}",
                                providerId,
                                requestInfo->toString());
                handleBroadcastSubscriptionRequest(requestInfo, requestCaller, publicationSender);
            }
            queuedBroadcastSubscriptionRequestsIterator =
                    queuedBroadcastSubscriptionRequests.find(providerId);
        }
    }
}

void PublicationManager::loadSavedAttributeSubscriptionRequestsMap(const std::string& fileName)
{
    JOYNR_LOG_TRACE(logger(), "Loading stored AttributeSubscriptionrequests.");

    // update reference file
    if (fileName != subscriptionRequestStorageFileName) {
        subscriptionRequestStorageFileName = std::move(fileName);
    }

    loadSavedSubscriptionRequestsMap<SubscriptionRequestInformation>(
            subscriptionRequestStorageFileName,
            queuedSubscriptionRequestsMutex,
            queuedSubscriptionRequests);
}

void PublicationManager::loadSavedBroadcastSubscriptionRequestsMap(const std::string& fileName)
{
    JOYNR_LOG_TRACE(logger(), "Loading stored BroadcastSubscriptionrequests.");

    // update reference file
    if (fileName != broadcastSubscriptionRequestStorageFileName) {
        broadcastSubscriptionRequestStorageFileName = std::move(fileName);
    }

    loadSavedSubscriptionRequestsMap<BroadcastSubscriptionRequestInformation>(
            broadcastSubscriptionRequestStorageFileName,
            queuedBroadcastSubscriptionRequestsMutex,
            queuedBroadcastSubscriptionRequests);
}

// This function assumes that subscriptionList is a copy that is exclusively used by this function
void PublicationManager::saveBroadcastSubscriptionRequestsMap(bool saveOnShutdown)
{
    JOYNR_LOG_TRACE(logger(), "Saving active broadcastSubscriptionRequests to file.");

    saveSubscriptionRequestsMap(subscriptionId2BroadcastSubscriptionRequest,
                                broadcastSubscriptionRequestStorageFileName,
                                saveOnShutdown);
}

void PublicationManager::saveAttributeSubscriptionRequestsMap(bool finalSave)
{
    JOYNR_LOG_TRACE(logger(), "Saving active attribute subscriptionRequests to file.");

    saveSubscriptionRequestsMap(
            subscriptionId2SubscriptionRequest, subscriptionRequestStorageFileName, finalSave);
}

template <typename Map>
void PublicationManager::saveSubscriptionRequestsMap(const Map& map,
                                                     const std::string& storageFilename,
                                                     bool finalSave)
{
    if (!enableSubscriptionStorage) {
        return;
    }

    if (!finalSave && isShuttingDown()) {
        JOYNR_LOG_TRACE(logger(), "Abort saving, because we are already shutting down.");
        return;
    }

    if (storageFilename.empty()) {
        JOYNR_LOG_TRACE(logger(), "Won't save since no storage file was specified.");
        return;
    }

    std::vector<typename Map::mapped_type> subscriptionVector;
    subscriptionVector.reserve(map.size());

    auto callback = [&subscriptionVector](auto&& map) {
        for (auto&& entry : map) {
            subscriptionVector.push_back(entry.second);
        }
    };

    map.applyReadFun(callback);

    try {
        joynr::util::saveStringToFile(
                storageFilename, joynr::serializer::serializeToJson(subscriptionVector));
    } catch (const std::invalid_argument& ex) {
        JOYNR_LOG_ERROR(logger(), "serializing subscription map to JSON failed: {}", ex.what());
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger(), ex.what());
    }
}

template <class RequestInformationType>
void PublicationManager::loadSavedSubscriptionRequestsMap(
        const std::string& storageFilename,
        std::mutex& queueMutex,
        std::multimap<std::string, std::shared_ptr<RequestInformationType>>& queuedSubscriptions)
{
    static_assert(std::is_base_of<SubscriptionRequest, RequestInformationType>::value,
                  "loadSavedSubscriptionRequestsMap can only be used for subclasses of "
                  "SubscriptionRequest");

    if (!enableSubscriptionStorage) {
        return;
    }

    if (storageFilename.empty()) {
        JOYNR_LOG_TRACE(logger(), "Won't load since no file was specified for loading.");
        return;
    }

    std::string jsonString;
    try {
        jsonString = joynr::util::loadStringFromFile(storageFilename);
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_INFO(logger(), ex.what());
    }

    if (jsonString.empty()) {
        return;
    }

    std::lock_guard<std::mutex> queueLocker(queueMutex);

    // Deserialize the JSON into the array of subscription requests
    std::vector<std::shared_ptr<RequestInformationType>> subscriptionVector;
    try {
        joynr::serializer::deserializeFromJson(subscriptionVector, jsonString);
    } catch (const std::invalid_argument& e) {
        std::string errorMessage("could not deserialize subscription requests from'" + jsonString +
                                 "' - error: " + e.what());
        JOYNR_LOG_FATAL(logger(), errorMessage);
        return;
    }

    // Loop through the saved subscriptions
    for (auto& requestInfo : subscriptionVector) {
        if (isSubscriptionExpired(requestInfo->getQos())) {
            JOYNR_LOG_TRACE(logger(), "Removing subscription Request: {}", requestInfo->toString());
        } else {
            queuedSubscriptions.emplace(requestInfo->getProviderId(), std::move(requestInfo));
        }
    }
}

void PublicationManager::removeAttributePublication(const std::string& subscriptionId,
                                                    const bool updatePersistenceFile)
{
    JOYNR_LOG_TRACE(logger(), "removePublication: {}", subscriptionId);

    std::shared_ptr<Publication> publication = publications.take(subscriptionId);
    std::shared_ptr<SubscriptionRequestInformation> request =
            subscriptionId2SubscriptionRequest.take(subscriptionId);

    if (publication && request) {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Delete the onChange publication if needed
        removeOnChangePublication(subscriptionId, request, publication);
    }

    if (updatePersistenceFile) {
        saveAttributeSubscriptionRequestsMap();
    }
}

void PublicationManager::removeBroadcastPublication(const std::string& subscriptionId,
                                                    const bool updatePersistenceFile)
{
    JOYNR_LOG_TRACE(logger(), "removeBroadcast: {}", subscriptionId);

    std::shared_ptr<Publication> publication = publications.take(subscriptionId);

    std::shared_ptr<BroadcastSubscriptionRequestInformation> request =
            subscriptionId2BroadcastSubscriptionRequest.take(subscriptionId);

    if (publication && request) {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Remove listener
        std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->unregisterBroadcastListener(
                request->getSubscribeToName(), publication->broadcastListener);
        publication->broadcastListener = nullptr;

        removePublicationEndRunnable(publication);
    }

    if (updatePersistenceFile) {
        saveBroadcastSubscriptionRequestsMap();
    }
}

void PublicationManager::removeOnChangePublication(
        const std::string& subscriptionId,
        std::shared_ptr<SubscriptionRequestInformation> request,
        std::shared_ptr<Publication> publication)
{
    assert(request);
    assert(publication);
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    JOYNR_LOG_TRACE(logger(), "Removing onChange publication for id = {}", subscriptionId);
    // to silence unused-variable compiler warnings
    std::ignore = subscriptionId;

    if (SubscriptionUtil::isOnChangeSubscription(request->getQos())) {
        // Unregister and delete the attribute listener
        std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->unregisterAttributeListener(
                request->getSubscribeToName(), publication->attributeListener);
        publication->attributeListener = nullptr;
    }
    removePublicationEndRunnable(publication);
}

// This function assumes a write lock is alrady held for the publication}
void PublicationManager::removePublicationEndRunnable(std::shared_ptr<Publication> publication)
{
    assert(publication);
    if (publication->publicationEndRunnableHandle != DelayedScheduler::INVALID_RUNNABLE_HANDLE &&
        !isShuttingDown()) {
        JOYNR_LOG_TRACE(logger(),
                        "Unscheduling PublicationEndRunnable with handle: {}",
                        publication->publicationEndRunnableHandle);
        delayedScheduler->unschedule(publication->publicationEndRunnableHandle);
        publication->publicationEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE;
    }
}

bool PublicationManager::isShuttingDown()
{
    std::lock_guard<std::mutex> shuwDownLocker(shutDownMutex);
    return shuttingDown;
}

std::int64_t PublicationManager::getPublicationTtlMs(
        std::shared_ptr<SubscriptionRequest> subscriptionRequest) const
{
    assert(subscriptionRequest);
    // Get publication ttl only if subscriptionQos is a UnicastSubscritpionQos
    auto qos = subscriptionRequest->getQos();
    if (auto unicastQos = std::dynamic_pointer_cast<UnicastSubscriptionQos>(qos)) {
        return unicastQos->getPublicationTtlMs();
    }
    JOYNR_LOG_WARN(
            logger(), "Attempted to get publication ttl out of an invalid Qos: returing default.");
    return UnicastSubscriptionQos::DEFAULT_PUBLICATION_TTL_MS();
}

void PublicationManager::sendPublicationError(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        std::shared_ptr<exceptions::JoynrRuntimeException> exception)
{
    assert(publication);
    assert(subscriptionInformation);
    assert(request);
    assert(exception);
    JOYNR_LOG_TRACE(logger(), "sending subscription error");
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    subscriptionPublication.setError(std::move(exception));
    sendSubscriptionPublication(
            publication, subscriptionInformation, request, std::move(subscriptionPublication));
    JOYNR_LOG_TRACE(logger(), "sent subscription error");
}

void PublicationManager::sendSubscriptionPublication(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        SubscriptionPublication&& subscriptionPublication)
{
    assert(publication);
    assert(subscriptionInformation);
    assert(request);

    MessagingQos mQos;

    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    // Set the TTL
    mQos.setTtl(getPublicationTtlMs(request));

    std::weak_ptr<IPublicationSender> publicationSender = publication->sender;

    if (auto publicationSenderSharedPtr = publicationSender.lock()) {
        publicationSenderSharedPtr->sendSubscriptionPublication(
                subscriptionInformation->getProviderId(),
                subscriptionInformation->getProxyId(),
                mQos,
                std::move(subscriptionPublication));
        // Make note of when this publication was sent
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        publication->timeOfLastPublication = now;

        {
            std::lock_guard<std::mutex> currentScheduledLocker(currentScheduledPublicationsMutex);
            util::removeAll(currentScheduledPublications, request->getSubscriptionId());
        }
        JOYNR_LOG_TRACE(logger(), "sent publication @ {}", now);
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "publication could not be sent because publicationSender is not available");
    }
}

void PublicationManager::sendPublication(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        BaseReply&& response)
{
    assert(publication);
    assert(subscriptionInformation);
    assert(request);

    JOYNR_LOG_TRACE(logger(), "sending subscription reply");
    SubscriptionPublication subscriptionPublication(std::move(response));
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    sendSubscriptionPublication(
            publication, subscriptionInformation, request, std::move(subscriptionPublication));
    JOYNR_LOG_TRACE(logger(), "sent subscription reply");
}

void PublicationManager::pollSubscription(const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(logger(), "pollSubscription {}", subscriptionId);

    if (isShuttingDown()) {
        return;
    }

    // Get the subscription details
    std::shared_ptr<Publication> publication = publications.value(subscriptionId);
    std::shared_ptr<SubscriptionRequestInformation> subscriptionRequest =
            subscriptionId2SubscriptionRequest.value(subscriptionId);

    if (publication && subscriptionRequest) {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // See if the publication is needed
        const std::shared_ptr<SubscriptionQos> qos = subscriptionRequest->getQos();
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        std::int64_t publicationInterval = SubscriptionUtil::getPeriodicPublicationInterval(qos);

        // check if the subscription qos needs a periodic publication
        if (publicationInterval > 0) {
            std::int64_t timeSinceLast = now - publication->timeOfLastPublication;
            // publish only if not published in the current interval
            if (timeSinceLast < publicationInterval) {
                JOYNR_LOG_TRACE(
                        logger(),
                        "no publication necessary. publicationInterval: {}, timeSinceLast {}",
                        publicationInterval,
                        timeSinceLast);

                std::int64_t delayUntilNextPublication = publicationInterval - timeSinceLast;
                assert(delayUntilNextPublication >= 0);
                delayedScheduler->schedule(
                        std::make_shared<PublisherRunnable>(shared_from_this(), subscriptionId),
                        std::chrono::milliseconds(delayUntilNextPublication));
                return;
            }
        }

        std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
        const std::string& interfaceName = requestCaller->getInterfaceName();
        std::shared_ptr<IRequestInterpreter> requestInterpreter =
                InterfaceRegistrar::instance().getRequestInterpreter(interfaceName);
        if (!requestInterpreter) {
            JOYNR_LOG_ERROR(
                    logger(),
                    "requestInterpreter not found for interface {} while polling subscriptionId {}",
                    interfaceName,
                    subscriptionId);
            return;
        }

        // Get the value of the attribute
        std::string attributeGetter(
                util::attributeGetterFromName(subscriptionRequest->getSubscribeToName()));

        std::function<void(Reply && )> onSuccess =
                [publication, publicationInterval, qos, subscriptionRequest, this, subscriptionId](
                        Reply&& response) {
            sendPublication(
                    publication, subscriptionRequest, subscriptionRequest, std::move(response));

            // Reschedule the next poll
            if (publicationInterval > 0 && (!isSubscriptionExpired(qos))) {
                JOYNR_LOG_TRACE(
                        logger(), "rescheduling runnable with delay: {}", publicationInterval);
                delayedScheduler->schedule(
                        std::make_shared<PublisherRunnable>(shared_from_this(), subscriptionId),
                        std::chrono::milliseconds(publicationInterval));
            }
        };

        std::function<void(const std::shared_ptr<exceptions::JoynrException>&)> onError =
                [publication, publicationInterval, qos, subscriptionRequest, this, subscriptionId](
                        const std::shared_ptr<exceptions::JoynrException>& exception) {

            std::shared_ptr<exceptions::JoynrRuntimeException> runtimeError =
                    std::dynamic_pointer_cast<exceptions::JoynrRuntimeException>(exception);
            assert(runtimeError);
            sendPublicationError(
                    publication, subscriptionRequest, subscriptionRequest, std::move(runtimeError));

            // Reschedule the next poll
            if (publicationInterval > 0 && (!isSubscriptionExpired(qos))) {
                JOYNR_LOG_TRACE(
                        logger(), "rescheduling runnable with delay: {}", publicationInterval);
                delayedScheduler->schedule(
                        std::make_shared<PublisherRunnable>(shared_from_this(), subscriptionId),
                        std::chrono::milliseconds(publicationInterval));
            }
        };

        JOYNR_LOG_TRACE(logger(), "run: executing requestInterpreter= {}", attributeGetter);
        Request dummyRequest;
        dummyRequest.setMethodName(attributeGetter);

        CallContextStorage::set(subscriptionRequest->getCallContext());
        requestInterpreter->execute(
                std::move(requestCaller), dummyRequest, std::move(onSuccess), std::move(onError));
        CallContextStorage::invalidate();
    }
}

void PublicationManager::removePublication(const std::string& subscriptionId)
{
    if (subscriptionId2SubscriptionRequest.contains(subscriptionId)) {
        removeAttributePublication(subscriptionId);
    } else if (subscriptionId2BroadcastSubscriptionRequest.contains(subscriptionId)) {
        removeBroadcastPublication(subscriptionId);
    }
}

bool PublicationManager::isPublicationAlreadyScheduled(const std::string& subscriptionId)
{
    std::lock_guard<std::mutex> currentScheduledLocker(currentScheduledPublicationsMutex);
    return util::vectorContains(currentScheduledPublications, subscriptionId);
}

std::int64_t PublicationManager::getTimeUntilNextPublication(
        std::shared_ptr<Publication> publication,
        const std::shared_ptr<SubscriptionQos> qos)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    // Check the last publication time against the min interval
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch()).count();
    std::int64_t minInterval = SubscriptionUtil::getMinInterval(qos);

    std::int64_t timeSinceLast = now - publication->timeOfLastPublication;

    if (minInterval > 0 && timeSinceLast < minInterval) {
        return minInterval - timeSinceLast;
    }

    return 0;
}

void PublicationManager::reschedulePublication(const std::string& subscriptionId,
                                               std::int64_t nextPublication)
{
    if (nextPublication > 0) {
        std::lock_guard<std::mutex> currentScheduledLocker(currentScheduledPublicationsMutex);

        // Schedule a publication so that the change is not forgotten
        if (!util::vectorContains(currentScheduledPublications, subscriptionId)) {
            JOYNR_LOG_TRACE(logger(), "rescheduling runnable with delay: {}", nextPublication);
            currentScheduledPublications.push_back(subscriptionId);
            delayedScheduler->schedule(
                    std::make_shared<PublisherRunnable>(shared_from_this(), subscriptionId),
                    std::chrono::milliseconds(nextPublication));
        }
    }
}

//------ PublicationManager::Publication ---------------------------------------

PublicationManager::Publication::Publication(std::weak_ptr<IPublicationSender> publicationSender,
                                             std::shared_ptr<RequestCaller> requestCaller)
        : timeOfLastPublication(0),
          sender(publicationSender),
          requestCaller(std::move(requestCaller)),
          attributeListener(nullptr),
          broadcastListener(nullptr),
          mutex(),
          publicationEndRunnableHandle(DelayedScheduler::INVALID_RUNNABLE_HANDLE)
{
}

//------ PublicationManager::PublisherRunnable ---------------------------------

PublicationManager::PublisherRunnable::PublisherRunnable(
        std::weak_ptr<PublicationManager> publicationManager,
        const std::string& subscriptionId)
        : Runnable(),
          publicationManager(std::move(publicationManager)),
          subscriptionId(subscriptionId)
{
}

void PublicationManager::PublisherRunnable::shutdown()
{
}

void PublicationManager::PublisherRunnable::run()
{
    if (auto publicationManagerSharedPtr = publicationManager.lock()) {
        publicationManagerSharedPtr->pollSubscription(subscriptionId);
    }
}

//------ PublicationManager::PublicationEndRunnable ----------------------------

PublicationManager::PublicationEndRunnable::PublicationEndRunnable(
        std::weak_ptr<PublicationManager> publicationManager,
        const std::string& subscriptionId)
        : Runnable(), publicationManager(publicationManager), subscriptionId(subscriptionId)
{
}

void PublicationManager::PublicationEndRunnable::shutdown()
{
}

void PublicationManager::PublicationEndRunnable::run()
{
    if (auto publicationManagerSharedPtr = publicationManager.lock()) {
        std::shared_ptr<Publication> publication =
                publicationManagerSharedPtr->publications.value(subscriptionId);
        publicationManagerSharedPtr->removePublication(subscriptionId);

        if (publication) {
            std::lock_guard<std::recursive_mutex> lock((publication->mutex));
            publication->publicationEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE;
        }
    }
}

} // namespace joynr
