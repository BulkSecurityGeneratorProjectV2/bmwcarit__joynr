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
#ifndef REQUESTCALLERFACTORY_H
#define REQUESTCALLERFACTORY_H

#include <memory>

namespace joynr
{

class RequestCaller;

// Default definition of a RequestCallerFactoryHelper
// Specializations of this template appear in provider header files
template <class T>
class RequestCallerFactoryHelper
{
public:
    std::shared_ptr<RequestCaller> create(std::shared_ptr<T> provider)
    {
        notImplemented();
        return std::shared_ptr<RequestCaller>();
    }

    void notImplemented();
};

// Create a request caller for the given provider
class RequestCallerFactory
{
public:
    template <class T>
    static std::shared_ptr<RequestCaller> create(std::shared_ptr<T> provider)
    {
        return RequestCallerFactoryHelper<T>().create(provider);
    }
};

} // namespace joynr
#endif // REQUESTCALLERFACTORY_H
