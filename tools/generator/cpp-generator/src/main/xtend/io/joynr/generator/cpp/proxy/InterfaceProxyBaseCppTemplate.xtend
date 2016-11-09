package io.joynr.generator.cpp.proxy
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
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.InterfaceTemplate
import io.joynr.generator.templates.util.AttributeUtil
import io.joynr.generator.templates.util.InterfaceUtil
import io.joynr.generator.templates.util.NamingUtil
import io.joynr.generator.cpp.util.InterfaceSubscriptionUtil

class InterfaceProxyBaseCppTemplate extends InterfaceTemplate {
	@Inject	extension JoynrCppGeneratorExtensions
	@Inject extension TemplateBase
	@Inject extension InterfaceSubscriptionUtil
	@Inject private extension NamingUtil
	@Inject private extension AttributeUtil
	@Inject private extension InterfaceUtil

	override generate()
'''
«val serviceName =  francaIntf.joynrName»
«val className = serviceName + "ProxyBase"»
«warning()»

#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«className».h"
#include "joynr/ConnectorFactory.h"
#include "joynr/ISubscriptionListener.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«serviceName»InProcessConnector.h"
#include "«getPackagePathWithJoynrPrefix(francaIntf, "/")»/«serviceName»JoynrMessagingConnector.h"

«getNamespaceStarter(francaIntf)»
«className»::«className»(
		std::shared_ptr<const joynr::system::RoutingTypes::Address> messagingAddress,
		joynr::ConnectorFactory* connectorFactory,
		joynr::IClientCache *cache,
		const std::string &domain,
		const joynr::MessagingQos &qosSettings,
		bool cached
) :
		joynr::ProxyBase(connectorFactory, cache, domain, qosSettings, cached),
		messagingAddress(messagingAddress),
		connector()
{
}

void «className»::handleArbitrationFinished(
		const std::string &providerParticipantId,
		bool useInProcessConnector
) {
	connector = connectorFactory->create<«getPackagePathWithJoynrPrefix(francaIntf, "::")»::I«serviceName»Connector>(
				domain,
				proxyParticipantId,
				providerParticipantId,
				qosSettings,
				cache,
				cached,
				useInProcessConnector
	);

	joynr::ProxyBase::handleArbitrationFinished(providerParticipantId, useInProcessConnector);
}

«FOR attribute: getAttributes(francaIntf).filter[attribute | attribute.notifiable]»
	«var attributeName = attribute.joynrName»
	«produceUnsubscribeFromAttributeSignature(attribute, className)»
	{
		if (!connector){
			JOYNR_LOG_WARN(logger, "proxy cannot subscribe to «className».«attributeName», \
					 because the communication end partner is not (yet) known");
			return;
		}
		connector->unsubscribeFrom«attributeName.toFirstUpper»(subscriptionId);
	}

	«produceUpdateAttributeSubscriptionSignature(attribute, className)» {
		if (!connector){
			std::string errorMsg = "proxy cannot subscribe to «className».«attributeName», \
					 because the communication end partner is not (yet) known";
			JOYNR_LOG_WARN(logger, errorMsg);
			auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorMsg);
			auto future = std::make_shared<Future<std::string>>();
			future->onError(error);
			subscriptionListener->onError(*error);
			return future;
		}
		return connector->subscribeTo«attributeName.toFirstUpper»(
					subscriptionListener,
					subscriptionQos,
					subscriptionId);
	}

	«produceSubscribeToAttributeSignature(attribute, className)» {
		if (!connector){
			std::string errorMsg = "proxy cannot subscribe to «className».«attributeName», \
					 because the communication end partner is not (yet) known";
			JOYNR_LOG_WARN(logger, errorMsg);
			auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorMsg);
			auto future = std::make_shared<Future<std::string>>();
			future->onError(error);
			subscriptionListener->onError(*error);
			return future;
		}
		return connector->subscribeTo«attributeName.toFirstUpper»(
					subscriptionListener,
					subscriptionQos);
	}

«ENDFOR»

«FOR broadcast: francaIntf.broadcasts»
	«var broadcastName = broadcast.joynrName»
	«produceUnsubscribeFromBroadcastSignature(broadcast, className)»
	{
		if (!connector){
			JOYNR_LOG_WARN(logger, "proxy cannot unsubscribe from «className».«broadcastName» broadcast, \
					 because the communication end partner is not (yet) known");
			return;
		}
		connector->unsubscribeFrom«broadcastName.toFirstUpper»Broadcast(subscriptionId);
	}

	«produceSubscribeToBroadcastSignature(broadcast, francaIntf, className)» {
		std::string errorMsg;
		if (!connector){
			errorMsg = "proxy cannot subscribe to «className».«broadcastName» broadcast, \
				because the communication end partner is not (yet) known";
		}

		«IF !broadcast.selective»
			try {
				util::validatePartitions(partitions, true);
			} catch (std::invalid_argument exception) {
				errorMsg = "invalid argument:\n" + std::string(exception.what());
			}
		«ENDIF»

		if (!errorMsg.empty()) {
			JOYNR_LOG_WARN(logger, errorMsg);
			auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorMsg);
			auto future = std::make_shared<Future<std::string>>();
			future->onError(error);
			subscriptionListener->onError(*error);
			return future;
		}

		«IF broadcast.selective»
			return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
					filterParameters,
					subscriptionListener,
					subscriptionQos);
		«ELSE»
			return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
					subscriptionListener,
					subscriptionQos,
					partitions);
		«ENDIF»
	}

	«produceUpdateBroadcastSubscriptionSignature(broadcast, francaIntf, className)» {
		std::string errorMsg;
		if (!connector){
			errorMsg = "proxy cannot subscribe to «className».«broadcastName» broadcast, \
				because the communication end partner is not (yet) known";
		}

		«IF !broadcast.selective»
			try {
				util::validatePartitions(partitions, true);
			} catch (std::invalid_argument exception) {
				errorMsg = "invalid argument:\n" + std::string(exception.what());
			}
		«ENDIF»
		if (!errorMsg.empty()) {
			JOYNR_LOG_WARN(logger, errorMsg);
			auto error = std::make_shared<exceptions::JoynrRuntimeException>(errorMsg);
			auto future = std::make_shared<Future<std::string>>();
			future->onError(error);
			subscriptionListener->onError(*error);
			return future;
		}

		«IF broadcast.selective»
			return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
						subscriptionId,
						filterParameters,
						subscriptionListener,
						subscriptionQos);
		«ELSE»
			return connector->subscribeTo«broadcastName.toFirstUpper»Broadcast(
						subscriptionId,
						subscriptionListener,
						subscriptionQos,
						partitions);
		«ENDIF»
	}
«ENDFOR»

«getNamespaceEnder(francaIntf)»
'''
}
