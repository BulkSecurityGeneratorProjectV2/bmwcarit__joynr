/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.types;

import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;

import org.junit.Test;

import joynr.tests.AnonymousVersionedStruct;
import joynr.tests.AnonymousVersionedStruct1;
import joynr.tests.AnonymousVersionedStruct2;
import joynr.tests.InterfaceVersionedStruct;
import joynr.tests.InterfaceVersionedStruct1;
import joynr.tests.InterfaceVersionedStruct2;
import joynr.tests.MultipleVersionsTypeCollection.VersionedStruct;
import joynr.tests.MultipleVersionsTypeCollection.VersionedStruct1;
import joynr.tests.MultipleVersionsTypeCollection.VersionedStruct2;

public class MultipleVersionsTest {

    @Test
    public void testInstantiationOfTypesWithVersionInPackageName() {
        joynr.tests.v1.AnonymousVersionedStruct structVersion1AnonymousTypeCollection = new joynr.tests.v1.AnonymousVersionedStruct();
        joynr.tests.v2.AnonymousVersionedStruct structVersion2AnonymousTypeCollection = new joynr.tests.v2.AnonymousVersionedStruct();
        joynr.tests.v1.MultipleVersionsTypeCollection.VersionedStruct structVersion1NamedTypeCollection = new joynr.tests.v1.MultipleVersionsTypeCollection.VersionedStruct();
        joynr.tests.v2.MultipleVersionsTypeCollection.VersionedStruct structVersion2NamedTypeCollection = new joynr.tests.v2.MultipleVersionsTypeCollection.VersionedStruct();
        joynr.tests.v1.InterfaceVersionedStruct structVersion1Interface = new joynr.tests.v1.InterfaceVersionedStruct();
        joynr.tests.v2.InterfaceVersionedStruct structVersion2Interface = new joynr.tests.v2.InterfaceVersionedStruct();

        assertNotNull(structVersion1AnonymousTypeCollection.getFlag1());
        assertNotNull(structVersion2AnonymousTypeCollection.getFlag2());
        assertNotNull(structVersion1NamedTypeCollection.getFlag1());
        assertNotNull(structVersion2NamedTypeCollection.getFlag2());
        assertNotNull(structVersion1Interface.getFlag());
        assertNotNull(structVersion2Interface.getFlag1());
        assertNotNull(structVersion2Interface.getFlag2());

        assertNotEquals(structVersion1AnonymousTypeCollection, structVersion2AnonymousTypeCollection);
        assertNotEquals(structVersion1AnonymousTypeCollection, structVersion1NamedTypeCollection);
        assertNotEquals(structVersion1AnonymousTypeCollection, structVersion2NamedTypeCollection);
        assertNotEquals(structVersion1AnonymousTypeCollection, structVersion1Interface);
        assertNotEquals(structVersion1AnonymousTypeCollection, structVersion2Interface);

        assertNotEquals(structVersion2AnonymousTypeCollection, structVersion1NamedTypeCollection);
        assertNotEquals(structVersion2AnonymousTypeCollection, structVersion2NamedTypeCollection);
        assertNotEquals(structVersion2AnonymousTypeCollection, structVersion1Interface);
        assertNotEquals(structVersion2AnonymousTypeCollection, structVersion2Interface);

        assertNotEquals(structVersion1NamedTypeCollection, structVersion2NamedTypeCollection);
        assertNotEquals(structVersion1NamedTypeCollection, structVersion1Interface);
        assertNotEquals(structVersion1NamedTypeCollection, structVersion2Interface);

        assertNotEquals(structVersion2NamedTypeCollection, structVersion1Interface);
        assertNotEquals(structVersion2NamedTypeCollection, structVersion2Interface);

        assertNotEquals(structVersion1Interface, structVersion2Interface);
    }

    @Test
    public void testInstantiationOfTypesWithVersionInTypeName() {
        AnonymousVersionedStruct1 structVersion1AnonymousTypeCollection = new AnonymousVersionedStruct1();
        AnonymousVersionedStruct2 structVersion2AnonymousTypeCollection = new AnonymousVersionedStruct2();
        VersionedStruct1 structVersion1NamedTypeCollection = new VersionedStruct1();
        VersionedStruct2 structVersion2NamedTypeCollection = new VersionedStruct2();
        InterfaceVersionedStruct1 structVersion1Interface = new InterfaceVersionedStruct1();
        InterfaceVersionedStruct2 structVersion2Interface = new InterfaceVersionedStruct2();

        assertNotNull(structVersion1AnonymousTypeCollection.getFlag1());
        assertNotNull(structVersion2AnonymousTypeCollection.getFlag2());
        assertNotNull(structVersion1NamedTypeCollection.getFlag1());
        assertNotNull(structVersion2NamedTypeCollection.getFlag2());
        assertNotNull(structVersion1Interface.getFlag());
        assertNotNull(structVersion2Interface.getFlag1());
        assertNotNull(structVersion2Interface.getFlag2());

        assertNotEquals(structVersion1AnonymousTypeCollection, structVersion2AnonymousTypeCollection);
        assertNotEquals(structVersion1AnonymousTypeCollection, structVersion1NamedTypeCollection);
        assertNotEquals(structVersion1AnonymousTypeCollection, structVersion2NamedTypeCollection);
        assertNotEquals(structVersion1AnonymousTypeCollection, structVersion1Interface);
        assertNotEquals(structVersion1AnonymousTypeCollection, structVersion2Interface);

        assertNotEquals(structVersion2AnonymousTypeCollection, structVersion1NamedTypeCollection);
        assertNotEquals(structVersion2AnonymousTypeCollection, structVersion2NamedTypeCollection);
        assertNotEquals(structVersion2AnonymousTypeCollection, structVersion1Interface);
        assertNotEquals(structVersion2AnonymousTypeCollection, structVersion2Interface);

        assertNotEquals(structVersion1NamedTypeCollection, structVersion2NamedTypeCollection);
        assertNotEquals(structVersion1NamedTypeCollection, structVersion1Interface);
        assertNotEquals(structVersion1NamedTypeCollection, structVersion2Interface);

        assertNotEquals(structVersion2NamedTypeCollection, structVersion1Interface);
        assertNotEquals(structVersion2NamedTypeCollection, structVersion2Interface);

        assertNotEquals(structVersion1Interface, structVersion2Interface);
    }

    @Test
    public void testInstantiationOfTypesWithoutVersioning() {
        AnonymousVersionedStruct unversionedStructAnonymousTypeCollection = new AnonymousVersionedStruct();
        VersionedStruct versionedStructNamedTypeCollection = new VersionedStruct();
        InterfaceVersionedStruct versionedStructInterface = new InterfaceVersionedStruct();

        assertNotNull(unversionedStructAnonymousTypeCollection.getFlag2());
        assertNotNull(versionedStructNamedTypeCollection.getFlag2());
        assertNotNull(versionedStructInterface.getFlag1());
        assertNotNull(versionedStructInterface.getFlag2());

        assertNotEquals(unversionedStructAnonymousTypeCollection, versionedStructNamedTypeCollection);
        assertNotEquals(unversionedStructAnonymousTypeCollection, versionedStructInterface);
        assertNotEquals(versionedStructNamedTypeCollection, versionedStructInterface);
    }

}
