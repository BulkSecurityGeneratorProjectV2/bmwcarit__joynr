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
#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "joynr/ContentWithDecayTime.h"
#include "joynr/JoynrMessage.h"

using namespace joynr;

TEST(ContentWithDecayTimeTest, messageWithDecayTime)
{
    using namespace std::chrono_literals;
    JoynrMessage message;
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    JoynrTimePoint decaytime(std::chrono::milliseconds(now + 2000));
    ContentWithDecayTime<JoynrMessage> mwdt =  ContentWithDecayTime<JoynrMessage>(message, decaytime);
    EXPECT_TRUE(!mwdt.isExpired());
    EXPECT_GT(mwdt.getRemainingTtl(), 1500ms);
    EXPECT_LT(mwdt.getRemainingTtl(), 2500ms);
    EXPECT_EQ(decaytime, mwdt.getDecayTime());
    EXPECT_EQ(message, mwdt.getContent());
    std::this_thread::sleep_for(1s);
    EXPECT_GT( mwdt.getRemainingTtl(), 500ms);
    EXPECT_LT( mwdt.getRemainingTtl(), 1500ms);
    EXPECT_TRUE(!mwdt.isExpired());

    std::this_thread::sleep_for(1500ms);
    EXPECT_TRUE(mwdt.isExpired());
    EXPECT_LT(mwdt.getRemainingTtl(), 0ms);
}
