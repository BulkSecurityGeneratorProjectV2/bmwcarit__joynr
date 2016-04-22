package io.joynr.test.interlanguage;

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
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;

import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;

import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.TestInterface.MethodWithAnonymousErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterfaceAbstractProvider;
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.BaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.BaseStructWithoutElements;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedEnumerationWithPartlyDefinedValues;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedErrorEnumTc;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.MapStringString;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class IltProvider extends TestInterfaceAbstractProvider {
    protected Byte attributeUInt8;
    protected Double attributeDouble;
    protected Boolean attributeBooleanReadonly;
    protected String attributeStringNoSubscriptions;
    protected Byte attributeInt8readonlyNoSubscriptions;
    protected String[] attributeArrayOfStringImplicit;
    protected Enumeration attributeEnumeration;
    protected ExtendedEnumerationWithPartlyDefinedValues attributeExtendedEnumerationReadonly;
    protected BaseStruct attributeBaseStruct;
    protected ExtendedExtendedBaseStruct attributeExtendedExtendedBaseStruct;
    protected MapStringString attributeMapStringString;

    private static final Logger logger = LoggerFactory.getLogger(IltProvider.class);

    public IltProvider() {
        // default uses a priority that is the current time,
        // causing arbitration to the last started instance
        providerQos.setPriority(System.currentTimeMillis());
    }

    @Override
    public Promise<Deferred<Byte>> getAttributeUInt8() {
        Deferred<Byte> deferred = new Deferred<Byte>();
        deferred.resolve(attributeUInt8);
        return new Promise<Deferred<Byte>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeUInt8(Byte attributeUInt8) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeUInt8 = attributeUInt8;
        attributeUInt8Changed(attributeUInt8);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Double>> getAttributeDouble() {
        Deferred<Double> deferred = new Deferred<Double>();
        deferred.resolve(attributeDouble);
        return new Promise<Deferred<Double>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeDouble(Double attributeDouble) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeDouble = attributeDouble;
        attributeDoubleChanged(attributeDouble);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Boolean>> getAttributeBooleanReadonly() {
        Deferred<Boolean> deferred = new Deferred<Boolean>();
        // since there is no setter, set non-default value here
        attributeBooleanReadonly = true;
        deferred.resolve(attributeBooleanReadonly);
        return new Promise<Deferred<Boolean>>(deferred);
    }

    @Override
    public Promise<Deferred<String>> getAttributeStringNoSubscriptions() {
        Deferred<String> deferred = new Deferred<String>();
        deferred.resolve(attributeStringNoSubscriptions);
        return new Promise<Deferred<String>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeStringNoSubscriptions(String attributeStringNoSubscriptions) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeStringNoSubscriptions = attributeStringNoSubscriptions;
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Byte>> getAttributeInt8readonlyNoSubscriptions() {
        Deferred<Byte> deferred = new Deferred<Byte>();
        attributeInt8readonlyNoSubscriptions = -128;
        deferred.resolve(attributeInt8readonlyNoSubscriptions);
        return new Promise<Deferred<Byte>>(deferred);
    }

    @Override
    public Promise<Deferred<String[]>> getAttributeArrayOfStringImplicit() {
        Deferred<String[]> deferred = new Deferred<String[]>();
        deferred.resolve(attributeArrayOfStringImplicit);
        return new Promise<Deferred<String[]>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeArrayOfStringImplicit(String[] attributeArrayOfStringImplicit) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeArrayOfStringImplicit = attributeArrayOfStringImplicit;
        attributeArrayOfStringImplicitChanged(attributeArrayOfStringImplicit);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Enumeration>> getAttributeEnumeration() {
        Deferred<Enumeration> deferred = new Deferred<Enumeration>();
        deferred.resolve(attributeEnumeration);
        return new Promise<Deferred<Enumeration>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeEnumeration(Enumeration attributeEnumeration) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeEnumeration = attributeEnumeration;
        attributeEnumerationChanged(attributeEnumeration);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<ExtendedEnumerationWithPartlyDefinedValues>> getAttributeExtendedEnumerationReadonly() {
        Deferred<ExtendedEnumerationWithPartlyDefinedValues> deferred = new Deferred<ExtendedEnumerationWithPartlyDefinedValues>();
        // since there is no setter, hardcode a non-standard value here
        attributeExtendedEnumerationReadonly = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        deferred.resolve(attributeExtendedEnumerationReadonly);
        return new Promise<Deferred<ExtendedEnumerationWithPartlyDefinedValues>>(deferred);
    }

    @Override
    public Promise<Deferred<BaseStruct>> getAttributeBaseStruct() {
        Deferred<BaseStruct> deferred = new Deferred<BaseStruct>();
        deferred.resolve(attributeBaseStruct);
        return new Promise<Deferred<BaseStruct>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeBaseStruct(BaseStruct attributeBaseStruct) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeBaseStruct = attributeBaseStruct;
        attributeBaseStructChanged(attributeBaseStruct);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<ExtendedExtendedBaseStruct>> getAttributeExtendedExtendedBaseStruct() {
        Deferred<ExtendedExtendedBaseStruct> deferred = new Deferred<ExtendedExtendedBaseStruct>();
        deferred.resolve(attributeExtendedExtendedBaseStruct);
        return new Promise<Deferred<ExtendedExtendedBaseStruct>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeExtendedExtendedBaseStruct(ExtendedExtendedBaseStruct attributeExtendedExtendedBaseStruct) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeExtendedExtendedBaseStruct = attributeExtendedExtendedBaseStruct;
        attributeExtendedExtendedBaseStructChanged(attributeExtendedExtendedBaseStruct);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<Boolean>> getAttributeWithException() {
        Deferred<Boolean> deferred = new Deferred<Boolean>();
        deferred.reject(new ProviderRuntimeException("Exception from getAttributeWithException"));
        return new Promise<Deferred<Boolean>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeWithException(Boolean attributeWithException) {
        DeferredVoid deferred = new DeferredVoid();
        deferred.reject(new ProviderRuntimeException("Exception from setAttributeWithException"));
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodWithoutParameters
     *
     * no output possible, but may reject instead
     */
    @Override
    public Promise<DeferredVoid> methodWithoutParameters() {
        logger.warn("*******************************************");
        logger.warn("* IltProvider.methodWithoutParameters called");
        logger.warn("*******************************************");
        DeferredVoid deferred = new DeferredVoid();
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodWithoutInputParameter
     *
     * return fixed output (true) or reject
     */
    @Override
    public Promise<MethodWithoutInputParameterDeferred> methodWithoutInputParameter() {
        logger.warn("************************************************");
        logger.warn("* IltProvider.methodWithoutInputParameter called");
        logger.warn("************************************************");
        MethodWithoutInputParameterDeferred deferred = new MethodWithoutInputParameterDeferred();
        deferred.resolve(true);
        return new Promise<MethodWithoutInputParameterDeferred>(deferred);
    }

    /*
     * methodWithoutOutputParameter
     *
     * can only resolve or reject since there is no output parameter
     */
    @Override
    public Promise<DeferredVoid> methodWithoutOutputParameter(Boolean booleanArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.methodWithoutOutputParameter called");
        logger.warn("*************************************************");
        DeferredVoid deferred = new DeferredVoid();

        if (booleanArg != false) {
            logger.warn("methodWithoutOutputParameter: invalid argument booleanArg");
            deferred.reject(new ProviderRuntimeException("methodWithoutOutputParameter: received wrong argument"));
            return new Promise<DeferredVoid>(deferred);
        }
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodWithSinglePrimitiveParameters
     *
     * returns the integer parameter as string
     */
    @Override
    public Promise<MethodWithSinglePrimitiveParametersDeferred> methodWithSinglePrimitiveParameters(Short uInt16Arg) {
        logger.warn("********************************************************");
        logger.warn("* IltProvider.methodWithSinglePrimitiveParameters called");
        logger.warn("********************************************************");
        MethodWithSinglePrimitiveParametersDeferred deferred = new MethodWithSinglePrimitiveParametersDeferred();

        // check input parameters
        // if (Short.toUnsignedInt(uInt16Arg) != 65535) {
        if (uInt16Arg != 32767) {
            logger.warn("methodWithSinglePrimitiveParameters: invalid argument uInt16Arg");
            deferred.reject(new ProviderRuntimeException("methodWithSinglePrimitiveParameters: received wrong argument"));
            return new Promise<MethodWithSinglePrimitiveParametersDeferred>(deferred);
        }

        // send back the input converted to a string
        deferred.resolve(new Integer(Short.toUnsignedInt(uInt16Arg)).toString());
        return new Promise<MethodWithSinglePrimitiveParametersDeferred>(deferred);
    }

    /*
     * methodWithMultiplePrimitiveParameters
     *
     * the 'float' of France is delivered as Double here, just return it as 'double'
     * and return the integer argument as string
     */
    @Override
    public Promise<MethodWithMultiplePrimitiveParametersDeferred> methodWithMultiplePrimitiveParameters(Integer int32Arg,
                                                                                                        Float floatArg,
                                                                                                        Boolean booleanArg) {
        logger.warn("**********************************************************");
        logger.warn("* IltProvider.methodWithMultiplePrimitiveParameters called");
        logger.warn("**********************************************************");
        MethodWithMultiplePrimitiveParametersDeferred deferred = new MethodWithMultiplePrimitiveParametersDeferred();

        // check input parameters
        if (int32Arg != 2147483647 || !IltUtil.cmpFloat(floatArg, 47.11f) || booleanArg != false) {
            logger.warn("methodWithMultiplePrimitiveParameters: invalid argument int32Arg, floatArg or booleanArg");
            deferred.reject(new ProviderRuntimeException("methodWithMultiplePrimitiveParameters: received wrong argument"));
            return new Promise<MethodWithMultiplePrimitiveParametersDeferred>(deferred);
        }

        // prepare output parameters
        Double doubleOut = (double) floatArg;
        String stringOut = int32Arg.toString();
        deferred.resolve(doubleOut, stringOut);
        return new Promise<MethodWithMultiplePrimitiveParametersDeferred>(deferred);
    }

    /*
     * methodWithSingleArrayParameters
     *
     * Return an array with the stringified double entries
     */
    @Override
    public Promise<MethodWithSingleArrayParametersDeferred> methodWithSingleArrayParameters(Double[] doubleArrayArg) {
        logger.warn("****************************************************");
        logger.warn("* IltProvider.methodWithSingleArrayParameters called");
        logger.warn("****************************************************");
        MethodWithSingleArrayParametersDeferred deferred = new MethodWithSingleArrayParametersDeferred();

        // check input parameter
        if (!IltUtil.checkDoubleArray(doubleArrayArg)) {
            logger.warn("methodWithMultiplePrimitiveParameters: invalid argument doubleArrayArg");
            deferred.reject(new ProviderRuntimeException("methodWithSingleArrayParameters: received wrong argument"));
            return new Promise<MethodWithSingleArrayParametersDeferred>(deferred);
        }

        // prepare output parameter
        String[] stringArrayOut = IltUtil.createStringArray();
        deferred.resolve(stringArrayOut);
        return new Promise<MethodWithSingleArrayParametersDeferred>(deferred);
    }

    /*
     * methodWithMultipleArrayParameters
     *
     * return the byte array as int64array
     * return the string list as list of string arrays with 1 element each, where this element
     * refers to the one from input
     */
    @Override
    public Promise<MethodWithMultipleArrayParametersDeferred> methodWithMultipleArrayParameters(String[] stringArrayArg,
                                                                                                Byte[] int8ArrayArg,
                                                                                                ExtendedInterfaceEnumerationInTypeCollection[] enumArrayArg,
                                                                                                StructWithStringArray[] structWithStringArrayArrayArg) {
        logger.warn("******************************************************");
        logger.warn("* IltProvider.methodWithMultipleArrayParameters called");
        logger.warn("******************************************************");
        MethodWithMultipleArrayParametersDeferred deferred = new MethodWithMultipleArrayParametersDeferred();

        if (!IltUtil.checkStringArray(stringArrayArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid stringArrayArg"));
            return new Promise<MethodWithMultipleArrayParametersDeferred>(deferred);
        }

        if (!IltUtil.checkByteArray(int8ArrayArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid int8ArrayArg"));
            return new Promise<MethodWithMultipleArrayParametersDeferred>(deferred);
        }

        if (!IltUtil.checkExtendedInterfaceEnumerationInTypeCollectionArray(enumArrayArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid enumArrayArg"));
            return new Promise<MethodWithMultipleArrayParametersDeferred>(deferred);
        }

        if (!IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleArrayParameters: invalid structWithStringArrayArrayArg"));
            return new Promise<MethodWithMultipleArrayParametersDeferred>(deferred);
        }

        Long[] uInt64ArrayOut = IltUtil.createUInt64Array();
        StructWithStringArray[] structWithStringArrayArrayOut = new StructWithStringArray[2];
        structWithStringArrayArrayOut[0] = IltUtil.createStructWithStringArray();
        structWithStringArrayArrayOut[1] = IltUtil.createStructWithStringArray();

        deferred.resolve(uInt64ArrayOut, structWithStringArrayArrayOut);
        return new Promise<MethodWithMultipleArrayParametersDeferred>(deferred);
    }

    /*
     * methodWithSingleEnumParameters
     *
     * return fixed value for now
     */
    @Override
    public Promise<MethodWithSingleEnumParametersDeferred> methodWithSingleEnumParameters(ExtendedEnumerationWithPartlyDefinedValues enumerationArg) {
        logger.warn("***************************************************");
        logger.warn("* IltProvider.methodWithSingleEnumParameters called");
        logger.warn("***************************************************");
        MethodWithSingleEnumParametersDeferred deferred = new MethodWithSingleEnumParametersDeferred();

        // check input parameter
        if (enumerationArg != ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
            logger.warn("methodWithSingleEnumParameters: invalid argument enumerationArg");
            deferred.reject(new ProviderRuntimeException("methodWithSingleEnumParameters: received wrong argument"));
            return new Promise<MethodWithSingleEnumParametersDeferred>(deferred);
        }

        // prepare output parameter
        ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        deferred.resolve(enumerationOut);
        return new Promise<MethodWithSingleEnumParametersDeferred>(deferred);
    }

    /*
     * methodWithMultipleEnumParameters
     *
     * return fixed values for now
     */
    @Override
    public Promise<MethodWithMultipleEnumParametersDeferred> methodWithMultipleEnumParameters(Enumeration enumerationArg,
                                                                                              ExtendedTypeCollectionEnumerationInTypeCollection extendedEnumerationArg) {
        logger.warn("*****************************************************");
        logger.warn("* IltProvider.methodWithMultipleEnumParameters called");
        logger.warn("*****************************************************");
        MethodWithMultipleEnumParametersDeferred deferred = new MethodWithMultipleEnumParametersDeferred();

        // check input parameters
        if (enumerationArg != joynr.interlanguagetest.Enumeration.ENUM_0_VALUE_3
                || extendedEnumerationArg != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
            logger.warn("methodWithMultipleEnumParameters: invalid argument enumerationArg or extendedEnumerationArg");
            deferred.reject(new ProviderRuntimeException("methodWithMultipleEnumParameters: received wrong argument"));
            return new Promise<MethodWithMultipleEnumParametersDeferred>(deferred);
        }

        // prepare output parameters
        ExtendedEnumerationWithPartlyDefinedValues extendedEnumerationOut = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        Enumeration enumerationOut = Enumeration.ENUM_0_VALUE_1;
        deferred.resolve(extendedEnumerationOut, enumerationOut);
        return new Promise<MethodWithMultipleEnumParametersDeferred>(deferred);
    }

    /*
     * methodWithSingleStructParameters
     */
    @Override
    public Promise<MethodWithSingleStructParametersDeferred> methodWithSingleStructParameters(ExtendedBaseStruct extendedBaseStructArg) {
        logger.warn("*****************************************************");
        logger.warn("* IltProvider.methodWithSingleStructParameters called");
        logger.warn("*****************************************************");
        MethodWithSingleStructParametersDeferred deferred = new MethodWithSingleStructParametersDeferred();

        if (!IltUtil.checkExtendedBaseStruct(extendedBaseStructArg)) {
            logger.error("methodWithSingleStructParameters: invalid parameter extendedBaseStructArg");
            deferred.reject(new ProviderRuntimeException("methodWithSingleStructParameters: invalid parameter extendedBaseStructArg"));
            return new Promise<MethodWithSingleStructParametersDeferred>(deferred);
        }

        // prepare output parameters
        ExtendedStructOfPrimitives extendedStructOfPrimitivesOut = IltUtil.createExtendedStructOfPrimitives();
        deferred.resolve(extendedStructOfPrimitivesOut);
        return new Promise<MethodWithSingleStructParametersDeferred>(deferred);
    }

    /*
     * methodWithMultipleStructParameters
     */
    @Override
    public Promise<MethodWithMultipleStructParametersDeferred> methodWithMultipleStructParameters(ExtendedStructOfPrimitives extendedStructOfPrimitivesArg,
                                                                                                  BaseStruct baseStructArg) {
        MethodWithMultipleStructParametersDeferred deferred = new MethodWithMultipleStructParametersDeferred();
        logger.warn("*******************************************************");
        logger.warn("* IltProvider.methodWithMultipleStructParameters called");
        logger.warn("*******************************************************");

        // check input parameter
        if (!IltUtil.checkExtendedStructOfPrimitives(extendedStructOfPrimitivesArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleStructParameters: invalid parameter extendedStructOfPrimitivesArg"));
            return new Promise<MethodWithMultipleStructParametersDeferred>(deferred);
        }

        if (!IltUtil.checkBaseStruct(baseStructArg)) {
            deferred.reject(new ProviderRuntimeException("methodWithMultipleStructParameters: invalid parameter baseStructArg"));
            return new Promise<MethodWithMultipleStructParametersDeferred>(deferred);
        }

        // set output values
        BaseStructWithoutElements baseStructWithoutElementsOut = IltUtil.createBaseStructWithoutElements();
        ExtendedExtendedBaseStruct extendedExtendedBaseStructOut = IltUtil.createExtendedExtendedBaseStruct();

        deferred.resolve(baseStructWithoutElementsOut, extendedExtendedBaseStructOut);
        return new Promise<MethodWithMultipleStructParametersDeferred>(deferred);
    }

    /*
     * overloadedMethod (1)
     */
    @Override
    public Promise<OverloadedMethod1Deferred> overloadedMethod() {
        logger.warn("*****************************************");
        logger.warn("* IltProvider.overloadedMethod called (1)");
        logger.warn("*****************************************");
        OverloadedMethod1Deferred deferred = new OverloadedMethod1Deferred();
        String stringOut = "TestString 1";
        deferred.resolve(stringOut);
        return new Promise<OverloadedMethod1Deferred>(deferred);
    }

    /*
     * overloadedMethod (2)
     */
    @Override
    public Promise<OverloadedMethod1Deferred> overloadedMethod(Boolean booleanArg) {
        logger.warn("*****************************************");
        logger.warn("* IltProvider.overloadedMethod called (2)");
        logger.warn("*****************************************");
        OverloadedMethod1Deferred deferred = new OverloadedMethod1Deferred();
        if (booleanArg != false) {
            logger.warn("overloadedMethod_2: invalid argument booleanArg");
            deferred.reject(new ProviderRuntimeException("overloadedMethod_2: invalid parameter baseStructArg"));
            return new Promise<OverloadedMethod1Deferred>(deferred);
        }
        String stringOut = "TestString 2";
        deferred.resolve(stringOut);
        return new Promise<OverloadedMethod1Deferred>(deferred);
    }

    /*
     * overloadedMethod (3)
     */
    @Override
    public Promise<OverloadedMethod2Deferred> overloadedMethod(ExtendedExtendedEnumeration[] enumArrayArg,
                                                               Long int64Arg,
                                                               BaseStruct baseStructArg,
                                                               Boolean booleanArg) {
        logger.warn("*****************************************");
        logger.warn("* IltProvider.overloadedMethod called (3)");
        logger.warn("*****************************************");
        OverloadedMethod2Deferred deferred = new OverloadedMethod2Deferred();

        // check input parameter
        if (int64Arg != 1L || booleanArg != false) {
            logger.warn("overloadedMethod_3: invalid argument int64Arg or booleanArg");
            deferred.reject(new ProviderRuntimeException("overloadedMethod_3: invalid parameter int64Arg or booleanArg"));
            return new Promise<OverloadedMethod2Deferred>(deferred);
        }

        // check enumArrayArg
        if (!IltUtil.checkExtendedExtendedEnumerationArray(enumArrayArg)) {
            deferred.reject(new ProviderRuntimeException("overloadedMethod_3: invalid parameter enumArrayArg"));
            return new Promise<OverloadedMethod2Deferred>(deferred);
        }

        // check baseStructArg
        if (!IltUtil.checkBaseStruct(baseStructArg)) {
            logger.warn("overloadedMethod_3: invalid argument baseStructArg");
            deferred.reject(new ProviderRuntimeException("overloadedMethod_3: invalid parameter baseStructArg"));
            return new Promise<OverloadedMethod2Deferred>(deferred);
        }

        // setup output parameter
        Double doubleOut = 0d;
        String[] stringArrayOut = IltUtil.createStringArray();
        ExtendedBaseStruct extendedBaseStructOut = IltUtil.createExtendedBaseStruct();

        deferred.resolve(doubleOut, stringArrayOut, extendedBaseStructOut);
        return new Promise<OverloadedMethod2Deferred>(deferred);
    }

    /*
     * overloadedMethodWithSelector (1)
     */
    @Override
    public Promise<OverloadedMethodWithSelector1Deferred> overloadedMethodWithSelector() {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.overloadedMethodWithSelector called");
        logger.warn("*************************************************");
        OverloadedMethodWithSelector1Deferred deferred = new OverloadedMethodWithSelector1Deferred();
        String stringOut = "Return value from overloadedMethodWithSelector 1";
        deferred.resolve(stringOut);
        return new Promise<OverloadedMethodWithSelector1Deferred>(deferred);
    }

    /*
     * overloadedMethodWithSelector (2)
     */
    @Override
    public Promise<OverloadedMethodWithSelector1Deferred> overloadedMethodWithSelector(Boolean booleanArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.overloadedMethodWithSelector called");
        logger.warn("*************************************************");
        OverloadedMethodWithSelector1Deferred deferred = new OverloadedMethodWithSelector1Deferred();

        // check input parameter
        if (booleanArg != false) {
            logger.warn("overloadedMethodWithSelector: invalid argument booleanArg");
            deferred.reject(new ProviderRuntimeException("overloadedMethodWithSelector: invalid parameter booleanArg"));
            return new Promise<OverloadedMethodWithSelector1Deferred>(deferred);
        }

        // setup output parameter
        String stringOut = "Return value from overloadedMethodWithSelector 2";
        deferred.resolve(stringOut);
        return new Promise<OverloadedMethodWithSelector1Deferred>(deferred);
    }

    /*
     * overloadedMethodWithSelector (3)
     */
    @Override
    public Promise<OverloadedMethodWithSelector2Deferred> overloadedMethodWithSelector(ExtendedExtendedEnumeration[] enumArrayArg,
                                                                                       Long int64Arg,
                                                                                       BaseStruct baseStructArg,
                                                                                       Boolean booleanArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.overloadedMethodWithSelector called");
        logger.warn("*************************************************");
        OverloadedMethodWithSelector2Deferred deferred = new OverloadedMethodWithSelector2Deferred();

        /* check input */
        if (!IltUtil.checkExtendedExtendedEnumerationArray(enumArrayArg)) {
            deferred.reject(new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare enumArrayArg"));
            return new Promise<OverloadedMethodWithSelector2Deferred>(deferred);
        }

        if (int64Arg != 1L) {
            deferred.reject(new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare int64Arg"));
            return new Promise<OverloadedMethodWithSelector2Deferred>(deferred);
        }

        if (!IltUtil.checkBaseStruct(baseStructArg)) {
            deferred.reject(new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare baseStructArg"));
            return new Promise<OverloadedMethodWithSelector2Deferred>(deferred);
        }

        if (booleanArg != false) {
            deferred.reject(new ProviderRuntimeException("overloadedMethodWithSelector: failed to compare booleanArg"));
            return new Promise<OverloadedMethodWithSelector2Deferred>(deferred);
        }

        /* prepare output */
        Double doubleOut = 1.1d;

        String[] stringArrayOut = IltUtil.createStringArray();
        ExtendedBaseStruct extendedBaseStructOut = IltUtil.createExtendedBaseStruct();

        deferred.resolve(doubleOut, stringArrayOut, extendedBaseStructOut);
        return new Promise<OverloadedMethodWithSelector2Deferred>(deferred);
    }

    /*
     * methodWithStringsAndSpecifiedStringOutLength
     */
    @Override
    public Promise<MethodWithStringsAndSpecifiedStringOutLengthDeferred> methodWithStringsAndSpecifiedStringOutLength(String stringArg,
                                                                                                                      Integer int32StringLengthArg) {
        logger.warn("*****************************************************************");
        logger.warn("* IltProvider.methodWithStringsAndSpecifiedStringOutLength called");
        logger.warn("*****************************************************************");
        MethodWithStringsAndSpecifiedStringOutLengthDeferred deferred = new MethodWithStringsAndSpecifiedStringOutLengthDeferred();
        StringBuilder s = new StringBuilder();
        if (int32StringLengthArg > 1024 * 1024) {
            deferred.reject(new ProviderRuntimeException("methodWithStringsAndSpecifiedStringOutLength: Maximum length exceeded"));
            return new Promise<MethodWithStringsAndSpecifiedStringOutLengthDeferred>(deferred);
        }
        for (int i = 0; i < int32StringLengthArg; i++) {
            s.append("A");
        }
        deferred.resolve(s.toString());
        return new Promise<MethodWithStringsAndSpecifiedStringOutLengthDeferred>(deferred);
    }

    /*
     * methodWithoutErrorEnum
     */
    @Override
    public Promise<DeferredVoid> methodWithoutErrorEnum(String wantedExceptionArg) {
        logger.warn("*******************************************");
        logger.warn("* IltProvider.methodWithoutErrorEnum called");
        logger.warn("*******************************************");
        DeferredVoid deferred = new DeferredVoid();

        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            deferred.reject(new ProviderRuntimeException("Exception from methodWithoutErrorEnum"));
        } else {
            deferred.resolve();
        }
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodWithAnonymousErrorEnum
     */
    @Override
    public Promise<MethodWithAnonymousErrorEnumDeferred> methodWithAnonymousErrorEnum(String wantedExceptionArg) {
        logger.warn("*************************************************");
        logger.warn("* IltProvider.methodWithAnonymousErrorEnum called");
        logger.warn("*************************************************");
        MethodWithAnonymousErrorEnumDeferred deferred = new MethodWithAnonymousErrorEnumDeferred();

        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            deferred.reject(new ProviderRuntimeException("Exception from methodWithAnonymousErrorEnum"));
        } else if (wantedExceptionArg.equals("ApplicationException")) {
            deferred.reject(MethodWithAnonymousErrorEnumErrorEnum.ERROR_3_1_NTC);
        } else {
            deferred.resolve();
        }
        return new Promise<MethodWithAnonymousErrorEnumDeferred>(deferred);
    }

    /*
     * methodWithExistingErrorEnum
     */
    @Override
    public Promise<MethodWithExistingErrorEnumDeferred> methodWithExistingErrorEnum(String wantedExceptionArg) {
        logger.warn("************************************************");
        logger.warn("* IltProvider.methodWithExistingErrorEnum called");
        logger.warn("************************************************");
        MethodWithExistingErrorEnumDeferred deferred = new MethodWithExistingErrorEnumDeferred();
        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            deferred.reject(new ProviderRuntimeException("Exception from methodWithExistingErrorEnum"));
        } else if (wantedExceptionArg.equals("ApplicationException_1")) {
            deferred.reject(ExtendedErrorEnumTc.ERROR_2_3_TC2);
        } else if (wantedExceptionArg.equals("ApplicationException_2")) {
            deferred.reject(ExtendedErrorEnumTc.ERROR_1_2_TC_2);
        } else {
            deferred.resolve();
        }
        return new Promise<MethodWithExistingErrorEnumDeferred>(deferred);
    }

    /*
     * methodWithExtendedErrorEnum
     */
    @Override
    public Promise<MethodWithExtendedErrorEnumDeferred> methodWithExtendedErrorEnum(String wantedExceptionArg) {
        logger.warn("************************************************");
        logger.warn("* IltProvider.methodWithExtendedErrorEnum called");
        logger.warn("************************************************");
        MethodWithExtendedErrorEnumDeferred deferred = new MethodWithExtendedErrorEnumDeferred();
        if (wantedExceptionArg.equals("ProviderRuntimeException")) {
            deferred.reject(new ProviderRuntimeException("Exception from methodWithExtendedErrorEnum"));
        } else if (wantedExceptionArg.equals("ApplicationException_1")) {
            deferred.reject(MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC);
        } else if (wantedExceptionArg.equals("ApplicationException_2")) {
            deferred.reject(MethodWithExtendedErrorEnumErrorEnum.ERROR_2_1_TC2);
        } else {
            deferred.resolve();
        }
        return new Promise<MethodWithExtendedErrorEnumDeferred>(deferred);
    }

    /*
     * methodToFireBroadcastWithSinglePrimitiveParameter
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithSinglePrimitiveParameter() {
        logger.warn("**********************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSinglePrimitiveParameter called");
        logger.warn("**********************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        String stringOut = "boom";
        fireBroadcastWithSinglePrimitiveParameter(stringOut);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithMultiplePrimitiveParameters
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithMultiplePrimitiveParameters() {
        logger.warn("*************************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultiplePrimitiveParameters called");
        logger.warn("*************************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        Double doubleOut = 1.1d;
        String stringOut = "boom";
        fireBroadcastWithMultiplePrimitiveParameters(doubleOut, stringOut);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithSingleArrayParameter
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithSingleArrayParameter() {
        logger.warn("******************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSingleArrayParameter called");
        logger.warn("******************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        String[] stringArrayOut = IltUtil.createStringArray();
        fireBroadcastWithSingleArrayParameter(stringArrayOut);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithMultipleArrayParameters
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithMultipleArrayParameters() {
        logger.warn("*********************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultipleArrayParameters called");
        logger.warn("*********************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        Long[] uInt64ArrayOut = IltUtil.createUInt64Array();
        StructWithStringArray[] structWithStringArrayArrayOut = IltUtil.createStructWithStringArrayArray();
        fireBroadcastWithMultipleArrayParameters(uInt64ArrayOut, structWithStringArrayArrayOut);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithSingleEnumerationParameter
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithSingleEnumerationParameter() {
        logger.warn("************************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSingleEnumerationParameter called");
        logger.warn("************************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        fireBroadcastWithSingleEnumerationParameter(enumerationOut);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithMultipleEnumerationParameters
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithMultipleEnumerationParameters() {
        logger.warn("***************************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultipleEnumerationParameters called");
        logger.warn("***************************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        deferred.resolve();
        ExtendedEnumerationWithPartlyDefinedValues extendedEnumerationOut = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
        Enumeration enumerationOut = Enumeration.ENUM_0_VALUE_1;
        fireBroadcastWithMultipleEnumerationParameters(extendedEnumerationOut, enumerationOut);
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithSingleStructParameter
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithSingleStructParameter() {
        logger.warn("*******************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithSingleStructParameter called");
        logger.warn("*******************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        ExtendedStructOfPrimitives extendedStructOfPrimitivesOut = IltUtil.createExtendedStructOfPrimitives();
        fireBroadcastWithSingleStructParameter(extendedStructOfPrimitivesOut);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithMultipleStructParameters
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithMultipleStructParameters() {
        logger.warn("**********************************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithMultipleStructParameters called");
        logger.warn("**********************************************************************");
        DeferredVoid deferred = new DeferredVoid();
        BaseStructWithoutElements baseStructWithoutElementsOut = IltUtil.createBaseStructWithoutElements();
        ExtendedExtendedBaseStruct extendedExtendedBaseStructOut = IltUtil.createExtendedExtendedBaseStruct();
        fireBroadcastWithMultipleStructParameters(baseStructWithoutElementsOut, extendedExtendedBaseStructOut);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    /*
     * methodToFireBroadcastWithFiltering
     */
    @Override
    public Promise<DeferredVoid> methodToFireBroadcastWithFiltering(String stringArg) {
        logger.warn("*******************************************************");
        logger.warn("* IltProvider.methodToFireBroadcastWithFiltering called");
        logger.warn("*******************************************************");
        DeferredVoid deferred = new DeferredVoid();

        // take the stringArg as input for the filtering
        String stringOut = stringArg;
        String[] stringArrayOut = IltUtil.createStringArray();
        ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
        StructWithStringArray structWithStringArrayOut = IltUtil.createStructWithStringArray();
        StructWithStringArray[] structWithStringArrayArrayOut = IltUtil.createStructWithStringArrayArray();

        fireBroadcastWithFiltering(stringOut,
                                   stringArrayOut,
                                   enumerationOut,
                                   structWithStringArrayOut,
                                   structWithStringArrayArrayOut);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<Deferred<MapStringString>> getAttributeMapStringString() {
        Deferred<MapStringString> deferred = new Deferred<MapStringString>();
        deferred.resolve(attributeMapStringString);
        return new Promise<Deferred<MapStringString>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> setAttributeMapStringString(MapStringString attributeMapStringString) {
        DeferredVoid deferred = new DeferredVoid();
        this.attributeMapStringString = attributeMapStringString;
        attributeMapStringStringChanged(attributeMapStringString);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<MethodWithSingleMapParametersDeferred> methodWithSingleMapParameters(MapStringString mapArg) {
        MethodWithSingleMapParametersDeferred deferred = new MethodWithSingleMapParametersDeferred();
        if (mapArg == null) {
            deferred.resolve(null);
        } else {
            MapStringString mapOut = new MapStringString();
            Iterator<Map.Entry<String, String>> iterator = mapArg.entrySet().iterator();
            while (iterator.hasNext()) {
                Map.Entry<String, String> entry = (Entry<String, String>) iterator.next();
                mapOut.put(entry.getValue(), entry.getKey());
            }
            deferred.resolve(mapOut);
        }
        return new Promise<MethodWithSingleMapParametersDeferred>(deferred);
    }
}
