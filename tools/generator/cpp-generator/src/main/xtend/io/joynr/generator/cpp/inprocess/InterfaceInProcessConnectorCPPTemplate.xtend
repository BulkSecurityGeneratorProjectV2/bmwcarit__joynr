package io.joynr.generator.cpp.inprocess
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
import com.google.inject.assistedinject.Assisted
import io.joynr.generator.cpp.util.CppInterfaceUtil
import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.BroadcastUtil
import io.joynr.generator.templates.util.MethodUtil
import io.joynr.generator.templates.util.NamingUtil
import org.franca.core.franca.FInterface

class InterfaceInProcessConnectorCPPTemplate extends InterfaceTemplate{

	@Inject private extension TemplateBase
	@Inject private extension CppStdTypeUtil
	@Inject private extension CppInterfaceUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension MethodUtil
	@Inject private extension BroadcastUtil
	@Inject private extension JoynrCppGeneratorExtensions

	@Inject
	new(@Assisted FInterface francaIntf) {
		super(francaIntf)
	}

	override  generate()
'''
«var interfaceName = francaIntf.joynrName»
«warning()»
#include <cassert>
#include <functional>
#include <tuple>

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»InProcessConnector.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«interfaceName»RequestCaller.h"
«FOR datatype: getAllComplexTypes(francaIntf)»
	«IF isCompound(datatype) || isMap(datatype)»
		#include «getIncludeOf(datatype)»
	«ENDIF»
«ENDFOR»

#include "joynr/InProcessAddress.h"
#include "joynr/ISubscriptionManager.h"
#include "joynr/PublicationManager.h"
#include "joynr/SubscriptionCallback.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/Util.h"
#include "joynr/Future.h"
#include "joynr/TypeUtil.h"
#include "joynr/SubscriptionUtil.h"
#include "joynr/exceptions/JoynrException.h"

«getNamespaceStarter(francaIntf)»

«val className = interfaceName + "InProcessConnector"»
INIT_LOGGER(«className»);

«className»::«className»(
			joynr::ISubscriptionManager* subscriptionManager,
			joynr::PublicationManager* publicationManager,
			joynr::InProcessPublicationSender* inProcessPublicationSender,
			const std::string& proxyParticipantId,
			const std::string& providerParticipantId,
			std::shared_ptr<joynr::InProcessAddress> address
) :
	proxyParticipantId(proxyParticipantId),
	providerParticipantId(providerParticipantId),
	address(address),
	subscriptionManager(subscriptionManager),
	publicationManager(publicationManager),
	inProcessPublicationSender(inProcessPublicationSender)
{
}

bool «className»::usesClusterController() const{
	return false;
}

«FOR attribute : getAttributes(francaIntf)»
	«val returnType = attribute.typeName»
	«val attributeName = attribute.joynrName»
	«val setAttributeName = "set" + attribute.joynrName.toFirstUpper»
	«IF attribute.readable»
		«val getAttributeName = "get" + attribute.joynrName.toFirstUpper»
		«produceSyncGetterSignature(attribute, className)»
		{
			assert(address);
			std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(caller);
			std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
			assert(«francaIntf.interfaceCaller»);

			auto future = std::make_shared<joynr::Future<«returnType»>>();

			std::function<void(const «returnType»& «attributeName»)> onSuccess =
					[future] (const «returnType»& «attributeName») {
						future->onSuccess(«attributeName»);
					};

			std::function<void(const exceptions::ProviderRuntimeException&)> onError =
					[future] (const exceptions::ProviderRuntimeException& error) {
						future->onError(error);
					};

			//see header for more information
			«francaIntf.interfaceCaller»->«getAttributeName»(onSuccess, onError);
			future->get(«attributeName»);
		}

		«produceAsyncGetterSignature(attribute, className)»
		{
			assert(address);
			std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(caller);
			std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
			assert(«francaIntf.interfaceCaller»);

			auto future = std::make_shared<joynr::Future<«returnType»>>();

			std::function<void(const «returnType»& «attributeName»)> onSuccessWrapper =
					[future, onSuccess] (const «returnType»& «attributeName») {
						future->onSuccess(«attributeName»);
						if (onSuccess) {
							onSuccess(«attributeName»);
						}
					};

			std::function<void(const exceptions::ProviderRuntimeException&)> onErrorWrapper =
					[future, onError] (const exceptions::ProviderRuntimeException& error) {
						future->onError(error);
						if (onError) {
							onError(error);
						}
					};

			//see header for more information
			«francaIntf.interfaceCaller»->«getAttributeName»(onSuccessWrapper, onErrorWrapper);
			return future;
		}

	«ENDIF»
	«IF attribute.writable»
		«produceAsyncSetterSignature(attribute, className)»
		{
			assert(address);
			std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(caller);
			std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
			assert(«francaIntf.interfaceCaller»);

			auto future = std::make_shared<joynr::Future<void>>();
			std::function<void()> onSuccessWrapper =
					[future, onSuccess] () {
						future->onSuccess();
						if (onSuccess) {
							onSuccess();
						}
					};

			std::function<void(const exceptions::ProviderRuntimeException&)> onErrorWrapper =
					[future, onError] (const exceptions::ProviderRuntimeException& error) {
						future->onError(error);
						if (onError) {
							onError(error);
						}
					};

			//see header for more information
			JOYNR_LOG_ERROR(logger, "#### WARNING ##### «interfaceName»InProcessConnector::«setAttributeName»(Future) is synchronous.");
			«francaIntf.interfaceCaller»->«setAttributeName»(«attributeName», onSuccessWrapper, onErrorWrapper);
			return future;
		}

		«produceSyncSetterSignature(attribute, className)»
		{
			assert(address);
			std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
			assert(caller);
			std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
			assert(«francaIntf.interfaceCaller»);

			auto future = std::make_shared<joynr::Future<void>>();
			std::function<void()> onSuccess =
					[future] () {
						future->onSuccess();
					};

			std::function<void(const exceptions::ProviderRuntimeException&)> onError =
					[future] (const exceptions::ProviderRuntimeException& error) {
						future->onError(error);
					};

			//see header for more information
			«francaIntf.interfaceCaller»->«setAttributeName»(«attributeName», onSuccess, onError);
			return future->get();
		}

	«ENDIF»
	«IF attribute.notifiable»
		std::string «className»::subscribeTo«attributeName.toFirstUpper»(
				std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
				const joynr::SubscriptionQos& subscriptionQos,
				std::string& subscriptionId)
		{
			joynr::SubscriptionRequest subscriptionRequest;
			subscriptionRequest.setSubscriptionId(subscriptionId);
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		std::string «className»::subscribeTo«attributeName.toFirstUpper»(
				std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
				const joynr::SubscriptionQos& subscriptionQos)
		{
			joynr::SubscriptionRequest subscriptionRequest;
			return subscribeTo«attributeName.toFirstUpper»(subscriptionListener, subscriptionQos, subscriptionRequest);
		}

		std::string «className»::subscribeTo«attributeName.toFirstUpper»(
				std::shared_ptr<joynr::ISubscriptionListener<«returnType»> > subscriptionListener,
				const joynr::SubscriptionQos& subscriptionQos,
				joynr::SubscriptionRequest& subscriptionRequest)
		{
			«IF isEnum(attribute.type)»
				std::ignore = subscriptionListener;
				std::ignore = subscriptionQos;
				std::ignore = subscriptionRequest;
				// TODO support enum return values in C++ client
				JOYNR_LOG_FATAL(logger, "enum return values are currently not supported in C++ client (attribute name: «interfaceName».«attributeName»)");
				assert(false);
				// Visual C++ requires a return value
				return std::string();
			«ELSE»
				JOYNR_LOG_DEBUG(logger, "Subscribing to «attributeName».");
				assert(subscriptionManager != nullptr);
				std::string attributeName("«attributeName»");
				auto subscriptionCallback = std::make_shared<
						joynr::SubscriptionCallback<«returnType»>
				>(subscriptionListener);
				subscriptionManager->registerSubscription(
						attributeName,
						subscriptionCallback,
						SubscriptionUtil::getVariant(subscriptionQos),
						subscriptionRequest);
				JOYNR_LOG_DEBUG(logger, "Registered subscription: {}", subscriptionRequest.toString());
				assert(address);
				std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
				assert(caller);
				std::shared_ptr<«interfaceName»RequestCaller> requestCaller = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
				std::string subscriptionId(subscriptionRequest.getSubscriptionId());

				if(!caller) {
					assert(publicationManager != nullptr);
					/**
					* Provider not registered yet
					* Dispatcher will call publicationManger->restore when a new provider is added to activate
					* subscriptions for that provider
					*/
					publicationManager->add(proxyParticipantId, providerParticipantId, subscriptionRequest);
				} else {
					publicationManager->add(proxyParticipantId, providerParticipantId, caller, subscriptionRequest, inProcessPublicationSender);
				}
				return subscriptionId;
			«ENDIF»
		}

		void «className»::unsubscribeFrom«attributeName.toFirstUpper»(
				std::string& subscriptionId
		) {
			«IF isEnum(attribute.type)»
				std::ignore = subscriptionId;
				// TODO support enum return values in C++ client
				JOYNR_LOG_FATAL(logger, "enum return values are currently not supported in C++ client (attribute name: «interfaceName».«attributeName»)");
				assert(false);
			«ELSE»
				JOYNR_LOG_DEBUG(logger, "Unsubscribing. Id={}", subscriptionId);
				assert(publicationManager != nullptr);
				JOYNR_LOG_DEBUG(logger, "Stopping publications by publication manager.");
				publicationManager->stopPublication(subscriptionId);
				assert(subscriptionManager != nullptr);
				JOYNR_LOG_DEBUG(logger, "Unregistering attribute subscription.");
				subscriptionManager->unregisterSubscription(subscriptionId);
			«ENDIF»
		}

	«ENDIF»
«ENDFOR»

«FOR method: getMethods(francaIntf)»
«var methodname = method.joynrName»
«var outputParameters = method.commaSeparatedOutputParameterTypes»
«var inputParamList = method.commaSeperatedUntypedInputParameterList»
«var outputTypedConstParamList = method.commaSeperatedTypedConstOutputParameterList»
«var outputUntypedParamList = method.commaSeperatedUntypedOutputParameterList»

«produceSyncMethodSignature(method, className)»
{
	assert(address);
	std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
	assert(caller);
	std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
	assert(«francaIntf.interfaceCaller»);
	auto future = std::make_shared<joynr::Future<«outputParameters»>>();

	std::function<void(«outputTypedConstParamList»)> onSuccess =
			[future] («outputTypedConstParamList») {
				future->onSuccess(
						«outputUntypedParamList»
				);
			};

	std::function<void(const exceptions::JoynrException&)> onError =
			[future] (const exceptions::JoynrException& error) {
				future->onError(error);
			};

	«francaIntf.interfaceCaller»->«methodname»(«IF !method.inputParameters.empty»«inputParamList», «ENDIF»onSuccess, onError);
	future->get(«method.commaSeperatedUntypedOutputParameterList»);
}

«produceAsyncMethodSignature(francaIntf, method, className)»
{
	assert(address);
	std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
	assert(caller);
	std::shared_ptr<«interfaceName»RequestCaller> «francaIntf.interfaceCaller» = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
	assert(«francaIntf.interfaceCaller»);
	auto future = std::make_shared<joynr::Future<«outputParameters»>>();

	std::function<void(«outputTypedConstParamList»)> onSuccessWrapper =
			[future, onSuccess] («outputTypedConstParamList») {
				future->onSuccess(«outputUntypedParamList»);
				if (onSuccess)
				{
					onSuccess(«outputUntypedParamList»);
				}
			};

	std::function<void(const exceptions::JoynrException&)> onErrorWrapper =
			[future, onRuntimeError«IF method.hasErrorEnum», onApplicationError«ENDIF»] (const exceptions::JoynrException& error) {
				future->onError(error);
				«produceApplicationRuntimeErrorSplitForOnErrorWrapper(francaIntf, method)»
			};

	«francaIntf.interfaceCaller»->«methodname»(«IF !method.inputParameters.empty»«inputParamList», «ENDIF»onSuccessWrapper, onErrorWrapper);
	return future;
}

«ENDFOR»

«FOR broadcast: francaIntf.broadcasts»
	«val returnTypes = broadcast.commaSeparatedOutputParameterTypes»
	«val broadcastName = broadcast.joynrName»

	«IF isSelective(broadcast)»
		std::string «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				const «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				const joynr::OnChangeSubscriptionQos& subscriptionQos
	«ELSE»
		std::string «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				const joynr::OnChangeSubscriptionQos& subscriptionQos
	«ENDIF»
	) {
		JOYNR_LOG_DEBUG(logger, "Subscribing to «broadcastName».");
		assert(subscriptionManager != nullptr);
		std::string broadcastName("«broadcastName»");
		joynr::BroadcastSubscriptionRequest subscriptionRequest;
		«IF isSelective(broadcast)»
			subscriptionRequest.setFilterParameters(filterParameters);
		«ENDIF»
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(
					subscriptionListener,
					subscriptionQos,
					subscriptionRequest);
	}

	«IF isSelective(broadcast)»
		std::string «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				const «interfaceName.toFirstUpper»«broadcastName.toFirstUpper»BroadcastFilterParameters& filterParameters,
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				const joynr::OnChangeSubscriptionQos& subscriptionQos,
				std::string& subscriptionId
	«ELSE»
		std::string «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
				std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
				const joynr::OnChangeSubscriptionQos& subscriptionQos,
				std::string& subscriptionId
	«ENDIF»
	) {
		joynr::BroadcastSubscriptionRequest subscriptionRequest;
		«IF isSelective(broadcast)»
			subscriptionRequest.setFilterParameters(filterParameters);
		«ENDIF»
		subscriptionRequest.setSubscriptionId(subscriptionId);
		return subscribeTo«broadcastName.toFirstUpper»Broadcast(
					subscriptionListener,
					subscriptionQos,
					subscriptionRequest);
	}

	std::string «className»::subscribeTo«broadcastName.toFirstUpper»Broadcast(
			std::shared_ptr<joynr::ISubscriptionListener<«returnTypes» > > subscriptionListener,
			const joynr::OnChangeSubscriptionQos& subscriptionQos,
			joynr::BroadcastSubscriptionRequest& subscriptionRequest
	) {
		JOYNR_LOG_DEBUG(logger, "Subscribing to «broadcastName».");
		assert(subscriptionManager != nullptr);
		std::string broadcastName("«broadcastName»");

		auto subscriptionCallback = std::make_shared<
				joynr::SubscriptionCallback<«returnTypes»>
		>(subscriptionListener);
		subscriptionManager->registerSubscription(
					broadcastName,
					subscriptionCallback,
					Variant::make<OnChangeSubscriptionQos>(subscriptionQos),
					subscriptionRequest);
		JOYNR_LOG_DEBUG(logger, "Registered broadcast subscription: {}", subscriptionRequest.toString());
		assert(address);
		std::shared_ptr<joynr::RequestCaller> caller = address->getRequestCaller();
		assert(caller);
		std::shared_ptr<«interfaceName»RequestCaller> requestCaller = std::dynamic_pointer_cast<«interfaceName»RequestCaller>(caller);
		std::string subscriptionId(subscriptionRequest.getSubscriptionId());

		if(!caller) {
			assert(publicationManager != nullptr);
			/**
			* Provider not registered yet
			* Dispatcher will call publicationManger->restore when a new provider is added to activate
			* subscriptions for that provider
			*/
			publicationManager->add(proxyParticipantId, providerParticipantId, subscriptionRequest);
		} else {
			publicationManager->add(
						proxyParticipantId,
						providerParticipantId,
						caller,
						subscriptionRequest,
						inProcessPublicationSender);
		}
		return subscriptionId;
	}

	void «className»::unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(
			std::string& subscriptionId
	) {
		JOYNR_LOG_DEBUG(logger, "Unsubscribing broadcast. Id={}", subscriptionId);
		assert(publicationManager != nullptr);
		JOYNR_LOG_DEBUG(logger, "Stopping publications by publication manager.");
		publicationManager->stopPublication(subscriptionId);
		assert(subscriptionManager != nullptr);
		JOYNR_LOG_DEBUG(logger, "Unregistering broadcast subscription.");
		subscriptionManager->unregisterSubscription(subscriptionId);
	}
«ENDFOR»
«getNamespaceEnder(francaIntf)»
'''

	def getInterfaceCaller(FInterface serviceInterface){
		serviceInterface.joynrName.toFirstLower + "Caller"
	}
}
