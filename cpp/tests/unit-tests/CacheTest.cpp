/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "joynr/Cache.h"
#include "joynr/joynrlogging.h"

using ::testing::Property;
using ::testing::Eq;
using ::testing::ByRef;
using ::testing::NotNull;
using ::testing::_;
using namespace ::testing;
using namespace joynr;

/*
 * This tests the Cache class.
 */

static const std::string defaultCacheKey("greeting");
static const std::string defaultCacheValue("Hello");

class CacheTest : public ::testing::Test {
public:

    CacheTest():logger(joynr_logging::Logging::getInstance()->getLogger("TST", "CacheTest")){}

    void SetUp() {
        std::string* strPtr = new std::string(defaultCacheValue);
        cache.insert(defaultCacheKey, strPtr);
    }

    void TearDown() {
        cache.clear();
        assert(cache.size() == 0);
    }

protected:
    joynr_logging::Logger* logger;
    // cache with default maxCost 100
    Cache<std::string, std::string> cache;
};

TEST_F(CacheTest, checkContains) {
    EXPECT_TRUE(cache.contains(defaultCacheKey));
}

TEST_F(CacheTest, checkObject) {
    std::string* stringPtr = cache.object(defaultCacheKey);
    EXPECT_EQ(*stringPtr, defaultCacheValue);
}

TEST_F(CacheTest, checkInsert) {
    cache.setCacheCapacity(2);
    cache.insert("who", new std::string("World"));
    ASSERT_EQ(cache.size(), 2);
    ASSERT_EQ(*cache.object("who"), "World");
}

TEST_F(CacheTest, checkInsertOverMaxCost) {
    cache.setCacheCapacity(2);
    cache.insert("who", new std::string("World"));
    cache.insert("who1", new std::string("CarIT"));
    ASSERT_EQ(cache.size(), 2);
    ASSERT_EQ(*(cache.object("who1")), "CarIT");
}

TEST_F(CacheTest, checkRemove) {
    cache.setCacheCapacity(3);
    cache.insert("who", new std::string("World"));
    cache.insert("who1", new std::string("CarIT"));
    ASSERT_EQ(cache.size(), 3);
    cache.remove("who1");
    ASSERT_EQ(cache.size(), 2);
    ASSERT_EQ(*cache.object("who"), "World");
}

TEST_F(CacheTest, checkKeys) {
    cache.setCacheCapacity(2);
    cache.insert("who", new std::string("World"));
    ASSERT_EQ(cache.size(), 2);
    ASSERT_EQ(cache.keys().at(1), "who");
}
