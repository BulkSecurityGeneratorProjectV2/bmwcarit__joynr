/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#ifndef IREPLYCALLER_H
#define IREPLYCALLER_H

#include "joynr/ITimeoutListener.h"
#include <QString>
#include <cassert>

namespace joynr {

/**
 * @brief This interface is required by the JoynrMessageSender to initiate the
 * request and return the response to its caller.
 * IReplyCaller is in /common, because the Directory needs to be aware of IReplyCaller, to call
 * timeOut() when an Object of Type IReplyCaller is removed because ttl has passed.
 * As Directory is in Common, it cannot have a dependency to libjoynr (where IReplyCaller was previously
 * situated). In C++ there is no language mechanism to require a template to inherit from a specific
 * class (like ITimeOutListener), so Directory has to be aware of IReplyCaller directly
 */
class IReplyCaller : virtual public ITimeoutListener {
public:

    virtual ~IReplyCaller() { }

    /**
      * Every Replycaller should have a returnValue<T> method.
      * This method is not part of the interface, to allow the interface to be untemplated
      **/

    /**
     * @brief This method will be called by the directory when
     * a time out occurs.
     * After this method is called, the directory will remove
     * this object from its directory.
     */
    virtual void timeOut() = 0;

    /**
     * @brief This method should return the name of the type that this reply caller should return.
     * This is required by the ReplyInterpreterFactory to create an interpreter for the type in
     * Dispatcher::handleReplyReceived.
     *
     * @return QString The name of the type, e.g. "int".
     */
    virtual QString getTypeName() const = 0;
    virtual int getTypeId() const = 0;
};



} // namespace joynr
#endif //IREPLYCALLER_H
