package io.joynr.generator.cpp.provider
/*
 * !!!
 *
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
 *
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
 */

import com.google.inject.Inject
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import java.util.Map
import org.franca.core.franca.FMethod

class InterfaceRequestCallerCppTemplate extends InterfaceTemplate {

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension JoynrCppGeneratorExtensions
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension InterfaceUtil
	@Inject private extension MethodUtil

	override generate()
'''
«var interfaceName = francaIntf.joynrName»
«warning()»
#include <functional>

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»RequestCaller.h"
«FOR datatype: getRequiredIncludesFor(francaIntf)»
	#include «datatype»
«ENDFOR»
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»Provider.h"
«IF !francaIntf.methods.empty || !francaIntf.attributes.empty»
	#include "joynr/TypeUtil.h"
«ENDIF»

«getNamespaceStarter(francaIntf)»
«interfaceName»RequestCaller::«interfaceName»RequestCaller(std::shared_ptr<«getPackagePathWithJoynrPrefix(francaIntf, "::")»::«interfaceName»Provider> provider)
	: joynr::RequestCaller(provider->getInterfaceName()),
	  provider(provider)
{
}

INIT_LOGGER(«interfaceName»RequestCaller);

«IF !francaIntf.attributes.empty»
	// attributes
«ENDIF»
«FOR attribute : francaIntf.attributes»
	«var attributeName = attribute.joynrName»
	«val returnType = attribute.typeName»
	«IF attribute.readable»
		void «interfaceName»RequestCaller::get«attributeName.toFirstUpper»(
				std::function<void(
						const «returnType»& «attributeName»
				)> onSuccess,
				std::function<void(
						const exceptions::ProviderRuntimeException&
				)> onError
		) {
			try {
				provider->get«attributeName.toFirstUpper»(onSuccess, onError);
			} catch (const exceptions::ProviderRuntimeException& e) {
				std::string message = "Could not perform «interfaceName»RequestCaller::get«attributeName.toFirstUpper», caught exception: " +
									e.getTypeName() + ":" + e.getMessage();
				onError(e);
			} catch (const exceptions::JoynrException& e) {
				std::string message = "Could not perform «interfaceName»RequestCaller::get«attributeName.toFirstUpper», caught exception: " +
									e.getTypeName() + ":" + e.getMessage();
				onError(exceptions::ProviderRuntimeException("caught exception: " + e.getTypeName() + ":" +
															e.getMessage()));
			}
		}
	«ENDIF»
	«IF attribute.writable»
		void «interfaceName»RequestCaller::set«attributeName.toFirstUpper»(
				const «returnType»& «attributeName»,
				std::function<void()> onSuccess,
				std::function<void(
						const exceptions::ProviderRuntimeException&
				)> onError
		) {
			try {
				provider->set«attributeName.toFirstUpper»(«attributeName», onSuccess, onError);
			} catch (const exceptions::ProviderRuntimeException& e) {
				std::string message = "Could not perform «interfaceName»RequestCaller::set«attributeName.toFirstUpper», caught exception: " +
									e.getTypeName() + ":" + e.getMessage();
				onError(e);
			} catch (const exceptions::JoynrException& e) {
				std::string message = "Could not perform «interfaceName»RequestCaller::set«attributeName.toFirstUpper», caught exception: " +
									e.getTypeName() + ":" + e.getMessage();
				onError(exceptions::ProviderRuntimeException("caught exception: " + e.getTypeName() + ":" +
															e.getMessage()));
			}
		}
	«ENDIF»

«ENDFOR»
«val methodToErrorEnumName = francaIntf.methodToErrorEnumName»
«IF !francaIntf.methods.empty»
	// methods
«ENDIF»
«FOR method : francaIntf.methods»
	«val outputTypedParamList = method.commaSeperatedTypedConstOutputParameterList»
	«val inputTypedParamList = method.commaSeperatedTypedConstInputParameterList»
	«val inputUntypedParamList = getCommaSeperatedUntypedInputParameterList(method)»
	«val methodName = method.joynrName»
	void «interfaceName»RequestCaller::«methodName»(
			«IF !method.inputParameters.empty»
				«inputTypedParamList»«IF !method.fireAndForget»,«ENDIF»
			«ENDIF»
			«IF !method.fireAndForget»
				«IF method.outputParameters.empty»
					std::function<void()> onSuccess,
				«ELSE»
					std::function<void(
							«outputTypedParamList»
					)> onSuccess,
				«ENDIF»
				std::function<void(
						const exceptions::JoynrException&
				)> onError
			«ENDIF»
	) {
		«IF !method.fireAndForget»
			«IF method.hasErrorEnum»
				«val errorTypeName = getErrorTypeName(method, methodToErrorEnumName)»
				std::function<void (const «errorTypeName»::«nestedEnumName»&)> onErrorWrapper =
						[onError] (const «errorTypeName»::«nestedEnumName»& errorEnum) {
							std::string typeName = «errorTypeName»::getTypeName();
							std::string name = «errorTypeName»::getLiteral(errorEnum);
							onError(exceptions::ApplicationException(typeName + "::" + name, Variant::make<«errorTypeName»::«nestedEnumName»>(errorEnum), name, typeName));
					};
			«ELSE»
			std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onErrorWrapper =
					[onError] (const joynr::exceptions::ProviderRuntimeException& error) {
						onError(error);
					};
			«ENDIF»
		«ENDIF»
		try {
			provider->«methodName»(
					«IF !method.inputParameters.empty»«inputUntypedParamList»«IF !method.fireAndForget»,«ENDIF»«ENDIF»
					«IF !method.fireAndForget»
						onSuccess,
						onErrorWrapper
					«ENDIF»
			);
		// ApplicationExceptions should not be created by the application itself to ensure
		// serializability. They are treated as JoynrExceptions. They can only be handled correctly
		// if the constructor is used properly (with the appropriate literal of the reported error
		// enumeration).
		} catch (const exceptions::ProviderRuntimeException& e) {
			std::string message = "Could not perform «interfaceName»RequestCaller::«methodName.toFirstUpper», caught exception: " +
								e.getTypeName() + ":" + e.getMessage();
			«IF method.fireAndForget»
				JOYNR_LOG_ERROR(logger, message);
			«ELSE»
				onError(e);
			«ENDIF»
		} catch (const exceptions::JoynrException& e) {
			std::string message = "Could not perform «interfaceName»RequestCaller::«methodName.toFirstUpper», caught exception: " +
								e.getTypeName() + ":" + e.getMessage();
			«IF method.fireAndForget»
				JOYNR_LOG_ERROR(logger, message);
			«ELSE»
				onError(exceptions::ProviderRuntimeException("caught exception: " + e.getTypeName() + ":" +
															e.getMessage()));
			«ENDIF»
		}
	}

«ENDFOR»

void «interfaceName»RequestCaller::registerAttributeListener(const std::string& attributeName, joynr::IAttributeListener* attributeListener)
{
	provider->registerAttributeListener(attributeName, attributeListener);
}

void «interfaceName»RequestCaller::unregisterAttributeListener(const std::string& attributeName, joynr::IAttributeListener* attributeListener)
{
	provider->unregisterAttributeListener(attributeName, attributeListener);
}

void «interfaceName»RequestCaller::registerBroadcastListener(const std::string& broadcastName, joynr::IBroadcastListener* broadcastListener)
{
	provider->registerBroadcastListener(broadcastName, broadcastListener);
}

void «interfaceName»RequestCaller::unregisterBroadcastListener(const std::string& broadcastName, joynr::IBroadcastListener* broadcastListener)
{
	provider->unregisterBroadcastListener(broadcastName, broadcastListener);
}

«getNamespaceEnder(francaIntf)»
'''

def getErrorTypeName(FMethod method, Map<FMethod, String> methodToErrorEnumName) {
	if (method.errors != null) {
		val packagePath = getPackagePathWithJoynrPrefix(method.errors, "::")
		packagePath + "::" + methodToErrorEnumName.get(method)
	} else{
		buildPackagePath(method.errorEnum, "::", true) + "::" + method.errorEnum.joynrName
	}
}
}
