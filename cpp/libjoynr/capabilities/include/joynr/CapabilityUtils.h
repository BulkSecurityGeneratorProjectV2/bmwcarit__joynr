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

#include <vector>

namespace joynr
{
namespace types
{
class DiscoveryEntry;
class DiscoveryEntryWithMetaInfo;
} // namespace types

namespace util
{
types::DiscoveryEntryWithMetaInfo convert(bool isLocal, const types::DiscoveryEntry& entry);
std::vector<types::DiscoveryEntryWithMetaInfo> convert(
        bool isLocal,
        const std::vector<types::DiscoveryEntry>& entries);
} // namespace util

} // namespace joynr
