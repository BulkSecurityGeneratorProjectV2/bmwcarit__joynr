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
#ifndef IDBUSSTUBWRAPPER_H
#define IDBUSSTUBWRAPPER_H

#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrCommonExport.h"

#include <CommonAPI/CommonAPI.h>

#include "joynr/joynrlogging.h"

#include <QString>
#include <thread>
#include <chrono>

namespace joynr
{

template <template <class...> class _ProxyClass>
class JOYNRCOMMON_EXPORT IDbusStubWrapper
{
public:
    IDbusStubWrapper(QString serviceAddress)
            : serviceAddress(serviceAddress),
              proxy(NULL),
              logger(NULL),
              proxyEvent(NULL),
              proxyEventSubscription()
    {
    }

    virtual ~IDbusStubWrapper()
    {
        proxyEvent->unsubscribe(proxyEventSubscription);
    }

    bool isProxyAvailable()
    {
        return proxy && proxy->isAvailable();
    }

    void proxyEventListener(const CommonAPI::AvailabilityStatus& status)
    {
        switch (status) {
        case CommonAPI::AvailabilityStatus::UNKNOWN:
            logAvailabilityStatus("UNKNOWN");
            break;
        case CommonAPI::AvailabilityStatus::AVAILABLE:
            logAvailabilityStatus("AVAILABLE");
            break;
        case CommonAPI::AvailabilityStatus::NOT_AVAILABLE:
            logAvailabilityStatus("NOT_AVAILABLE");
            break;
        }
    }

    void printCallStatus(const CommonAPI::CallStatus& status, const QString& method)
    {
        switch (status) {
        case CommonAPI::CallStatus::SUCCESS:
            logCallStatus(method, "SUCCESS");
            break;
        case CommonAPI::CallStatus::OUT_OF_MEMORY:
            logCallStatus(method, "OUT_OF_MEMORY");
            break;
        case CommonAPI::CallStatus::NOT_AVAILABLE:
            logCallStatus(method, "NOT_AVAILABLE");
            break;
        case CommonAPI::CallStatus::CONNECTION_FAILED:
            logCallStatus(method, "CONNECTION_FAILED");
            break;
        case CommonAPI::CallStatus::REMOTE_ERROR:
            logCallStatus(method, "REMOTE_ERROR");
            break;
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(IDbusStubWrapper);

    void logCallStatus(const QString method, const QString status)
    {
        LOG_INFO(logger, "Call status " + serviceAddress + "->" + method + ": " + status);
    }

    void logAvailabilityStatus(const QString status)
    {
        LOG_INFO(logger, "Status dbus proxy on address " + serviceAddress + ": " + status);
    }

protected:
    QString serviceAddress;
    std::shared_ptr<_ProxyClass<>> proxy;
    joynr_logging::Logger* logger;

    // event handling subscritpion
    CommonAPI::ProxyStatusEvent* proxyEvent;
    CommonAPI::ProxyStatusEvent::Subscription proxyEventSubscription;

    void init()
    {
        // get proxy
        auto factory = CommonAPI::Runtime::load("DBus")->createFactory();
        proxy = factory->buildProxy<_ProxyClass>(serviceAddress.toStdString());

        auto callBack =
                std::bind(&IDbusStubWrapper::proxyEventListener, this, std::placeholders::_1);

        proxyEvent = &(proxy->getProxyStatusEvent());
        proxyEventSubscription = proxyEvent->subscribe(callBack);

        // wait until proxy is available or timeout (1000ms)
        int8_t retries = 0;
        int8_t max_retries = 10;
        int8_t retry_delay = 100;
        while (!isProxyAvailable() && retries < max_retries) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay));
            retries++;
        }

        // if proxy not available log and exit
        if (!isProxyAvailable()) {
            LOG_ERROR(logger,
                      QString("Could not connect to proxy within %1ms!")
                              .arg(max_retries * retry_delay));
            assert(false);
        }
    }

    void logMethodCall(const QString& method)
    {
        LOG_INFO(logger, "Call method " + serviceAddress + "-> " + method);
    }
};

} // namespace joynr
#endif // IDBUSSTUBWRAPPER_H
