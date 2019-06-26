/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.tools.generator.gradle

import com.android.build.gradle.internal.dsl.BaseAppModuleExtension
import org.gradle.api.Plugin
import org.gradle.api.Project
import java.io.File

class JoynrGeneratorGradlePlugin : Plugin<Project> {

    companion object {
        const val JOYNR_GENERATOR_EXTENSION_NAME = "joynrGenerator"
        const val JOYNR_GENERATE_TASK_NAME = "joynrGenerate"
        const val CLEAN_TASK_NAME = "clean"
        private const val COMPILE_TASK = "compile"
        const val TASK_NAME_MATCHER_STRING = "^$COMPILE_TASK\\w*"
        private const val DEFAULT_OUTPUT_PATH = "app/build/generated/source/fidl/"
    }

    override fun apply(project: Project) {
        val extension = project.extensions.create(
            JOYNR_GENERATOR_EXTENSION_NAME,
            JoynrGeneratorPluginExtension::class.java,
            project
        )

        if (extension.outputPath.orNull == null) {
            extension.outputPath.set(DEFAULT_OUTPUT_PATH)
        }

        val joynrGeneratorArgumentHandler = JoynrGeneratorArgumentHandler(
            project.logger,
            extension.modelPath, extension.outputPath, extension.generationLanguage,
            extension.rootGenerator, extension.generationId, extension.skip,
            extension.addVersionTo, extension.extraParameters
        )

        val task = project.tasks.create(
            JOYNR_GENERATE_TASK_NAME,
            JoynrGeneratorTask::class.java
        ) { generatorTask ->
            generatorTask.joynrGeneratorHandler =
                JoynrGeneratorHandler(project.logger, joynrGeneratorArgumentHandler)
        }

        project.tasks.findByName(CLEAN_TASK_NAME)?.let {
            it.doLast {
                joynrGeneratorArgumentHandler.setClean(true)
                val generatorHandler =
                    JoynrGeneratorHandler(project.logger, joynrGeneratorArgumentHandler)
                generatorHandler.execute()
            }
        }

        project.afterEvaluate {
            project.tasks.forEach {
                when {
                    it.name.matches(TASK_NAME_MATCHER_STRING.toRegex()) -> {
                        // add joynr generate task as a dependency for the appropriate tasks, so we
                        // automatically generate code when developers build project with a correctly
                        // configured joynr generator
                        it.dependsOn(project.tasks.getByName(JOYNR_GENERATE_TASK_NAME))
                    }

                    else -> {
                        // do nothing
                    }
                }
            }
        }

        // Register our task with the variant's sources
        val android : BaseAppModuleExtension? = project.extensions.getByName("android") as? BaseAppModuleExtension
        val outputDir = File("${project.rootDir}/${extension.outputPath.get()}")
        project.logger.info("----> ${outputDir.path}")
        android?.applicationVariants?.all {variant ->
            variant.registerJavaGeneratingTask(task, outputDir)
        }
    }

}
