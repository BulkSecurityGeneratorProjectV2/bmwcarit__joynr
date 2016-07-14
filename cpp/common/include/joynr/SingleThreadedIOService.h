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
#ifndef SINGLETHREADEDIOSERVICE_H
#define SINGLETHREADEDIOSERVICE_H

#include <thread>

#include <boost/asio/io_service.hpp>

class SingleThreadedIOService
{
public:
    SingleThreadedIOService()
            : ioService(),
              ioServiceWork(ioService),
              ioServiceThread(&runIOService, std::ref(ioService))
    {
    }

    ~SingleThreadedIOService()
    {
        ioService.stop();

        if (ioServiceThread.joinable()) {
            ioServiceThread.join();
        }
    }

    boost::asio::io_service& getIOService()
    {
        return ioService;
    }

private:
    static void runIOService(boost::asio::io_service& ioService)
    {
        ioService.run();
    }

private:
    boost::asio::io_service ioService;
    boost::asio::io_service::work ioServiceWork;
    std::thread ioServiceThread;
};

#endif
