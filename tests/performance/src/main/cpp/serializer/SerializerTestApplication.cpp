/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

#include <tuple>

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/for_each.hpp>

#include "SerializerPerformanceTest.h"

int main()
{
    // run serialization and deserialization for the following payload types:
    using namespace generator;
    using Generators = std::tuple<String, ByteArray, ComplexStruct>;

    std::size_t length = 100;
    auto fun = [length](auto generator) {
        using Generator = decltype(generator);
        SerializerPerformanceTest<Generator> test(length);

        test.runSerializationBenchmark();
        test.runDeSerializationBenchmark();

        test.runFullMessageSerializationBenchmark();
        test.runFullMessageDeSerializationBenchmark();
    };

    boost::fusion::for_each(Generators(), fun);

    return 0;
}
