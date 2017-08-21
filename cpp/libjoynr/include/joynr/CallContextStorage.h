/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

#ifndef CALLCONTEXTSTORAGE_H
#define CALLCONTEXTSTORAGE_H

#include "joynr/CallContext.h"

namespace joynr
{

class CallContextStorage
{
public:
    static void set(const CallContext& callContext);
    static void set(CallContext&& callContext);
    static const CallContext& get();
    static void invalidate();

private:
    static thread_local CallContext callContext;
};

} // namespace joynr

#endif // CALLCONTEXTSTORAGE_H
