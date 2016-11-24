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

define([
    "joynr/datatypes/exampleTypes/ComplexRadioStation",
    "joynr/datatypes/exampleTypes/ComplexStruct",
    "joynr/datatypes/exampleTypes/Country",
    "joynr/datatypes/exampleTypes/ExtendedComplexStruct",
    "joynr/tests/testTypes/TestEnum"
], function(ComplexRadioStation, ComplexStruct, Country, ExtendedComplexStruct, TestEnum) {

    beforeEach(function() {});

    describe("Enum type", function() {
        it("equals", function() {
            var fixture = TestEnum.ZERO;
            expect(fixture.equals(TestEnum.ZERO)).toBeTruthy();
            expect(fixture.equals(TestEnum.ONE)).toBeFalsy();
            expect(fixture.equals(TestEnum.TWO)).toBeFalsy();
        });
    });

    describe("Compound type", function() {
        function testEqualsWithPrimitiveParams(fixture) {
            expect(fixture.equals("")).toBeFalsy();
            expect(fixture.equals(1)).toBeFalsy();
            expect(fixture.equals(true)).toBeFalsy();
            expect(fixture.equals(undefined)).toBeFalsy();
            expect(fixture.equals(null)).toBeFalsy();
        }

        function testEqualsForComplexStruct(settings, Constructor) {
            var fixture = new Constructor(settings);
            testEqualsWithPrimitiveParams(fixture);
            expect(fixture.equals(new Constructor())).toBeFalsy();
            expect(fixture.equals(fixture)).toBeTruthy();
            expect(fixture.equals(new Constructor(settings))).toBeTruthy();

            settings.num32 = 31;
            expect(fixture.equals(new Constructor(settings))).toBeFalsy();
            settings.num32 = 32;

            settings.num64 = 63;
            expect(fixture.equals(new Constructor(settings))).toBeFalsy();
            settings.num64 = 64;

            settings.data = [ 1
            ];
            expect(fixture.equals(new Constructor(settings))).toBeFalsy();
            settings.data = [];

            settings.str = "newString";
            expect(fixture.equals(new Constructor(settings))).toBeFalsy();
            settings.str = "string";

            return fixture;
        }

        it("equals of basic struct", function() {
            var settings = {
                num32 : 32,
                num64 : 64,
                data : [],
                str : "string"
            };
            testEqualsForComplexStruct(settings, ComplexStruct);
        });

        it("equals of struct containing enum member", function() {
            var settings = {
                name : "name",
                station : "station",
                source : Country.GERMANY
            };
            var fixture = new ComplexRadioStation(settings);
            testEqualsWithPrimitiveParams(fixture);
            expect(fixture.equals(Country.GERMANY)).toBeFalsy();

            expect(fixture.equals(new ComplexRadioStation())).toBeFalsy();
            expect(fixture.equals(fixture)).toBeTruthy();
            expect(fixture.equals(new ComplexRadioStation(settings))).toBeTruthy();

            settings.name = "newName";
            expect(fixture.equals(new ComplexRadioStation(settings))).toBeFalsy();
            settings.name = "name";

            settings.source = Country.ITALY;
            expect(fixture.equals(new ComplexRadioStation(settings))).toBeFalsy();
            settings.source = Country.GERMANY;

        });

        it("equals of hierarchic struct having struct as member", function() {
            var settings = {
                num32 : 32,
                num64 : 64,
                data : [],
                str : "string",
                structMember : new ExtendedComplexStruct()
            };
            var fixture = testEqualsForComplexStruct(settings, ExtendedComplexStruct);

            settings.structMember = new ExtendedComplexStruct({
                num32 : 0
            });

            expect(fixture.equals(new ExtendedComplexStruct(settings))).toBeFalsy();

            settings.structMember = new ExtendedComplexStruct();
            expect(fixture.equals(new ExtendedComplexStruct(settings))).toBeTruthy();
        });
    });

});
