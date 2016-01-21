package io.outofscope;

/*
 * #%L
 * io.joynr.tools.generator:generator-framework
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.generator.AbstractJoynrGenerator;

import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.xtext.generator.IFileSystemAccess;

public class OutOfScopeJoynGenerator extends AbstractJoynrGenerator {

    @Override
    public void doGenerate(Resource input, IFileSystemAccess fsa) {
        //do nothing
    }

    @Override
    public String getLanguageId() {
        return null;
    }

    @Override
    public void setParameters(Map<String, String> parameter) {
        //do nothing
    }

    @Override
    public Set<String> supportedParameters() {
        return new HashSet<String>();
    }
}
