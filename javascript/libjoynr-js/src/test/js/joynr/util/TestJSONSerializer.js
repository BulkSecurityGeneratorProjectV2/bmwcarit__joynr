/*jslint es5: true, nomen: true */
/*global joynrTestRequire: true */

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

joynrTestRequire("joynr/util/TestJSONSerializer", [
    "joynr/util/JSONSerializer",
    "joynr/tests/testTypes/TestEnum"
], function(JSONSerializer, TestEnum) {

    describe("libjoynr-js.joynr.JSONSerializer.ensureCorrectSerialization", function() {

        it("Test enum serialization", function() {
            var fixture = TestEnum.ZERO, actual;

            actual = JSONSerializer.stringify(fixture);

            expect(actual).toBe("\"" + fixture.name + "\"");
        });

    });

});
/* jslint nomen: false */
