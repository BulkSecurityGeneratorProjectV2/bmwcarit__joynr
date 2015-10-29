package io.joynr.generator.cpp.defaultProvider
/*
 * !!!
 *
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import io.joynr.generator.templates.util.NamingUtil
import java.io.File
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FModel

class DefaultInterfaceProviderGenerator {

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension NamingUtil

	@Inject
	DefaultInterfaceProviderHTemplate defaultProviderHTemplate;

	@Inject
	DefaultInterfaceProviderCppTemplate defaultProviderCppTemplate;

	def doGenerate(FModel fModel,
		IFileSystemAccess sourceFileSystem,
		IFileSystemAccess headerFileSystem,
		String sourceContainerPath,
		String headerContainerPath
	){

		for(serviceInterface: fModel.interfaces){
			val sourcepath = sourceContainerPath + getPackageSourceDirectory(serviceInterface) + File::separator
			val headerpath = headerContainerPath + getPackagePathWithJoynrPrefix(serviceInterface, File::separator) + File::separator
			val serviceName = serviceInterface.joynrName;

			generateFile(
				headerFileSystem,
				headerpath + "Default" + serviceName + "Provider.h",
				defaultProviderHTemplate,
				serviceInterface
			);

			generateFile(
				sourceFileSystem,
				sourcepath + "Default" + serviceName + "Provider.cpp",
				defaultProviderCppTemplate,
				serviceInterface
			);
		}
	}
}