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

module.exports = function(config) {
  config.set({
      plugins: [
            // Karma will require() these plugins
            'karma-jasmine',
            'karma-chrome-launcher',
            'karma-phantomjs-launcher',
            'karma-requirejs',
            'karma-junit-reporter',
            'karma-verbose-reporter'
    ],

    // base path that will be used to resolve all patterns (eg. files, exclude)
    basePath: '../../../target',


    // frameworks to use
    // available frameworks: https://npmjs.org/browse/keyword/karma-adapter
    frameworks: ['jasmine', 'requirejs'],


    // list of files / patterns to load in the browser
    files: [
            {pattern: 'classes/lib/*.js', included: false},
            {pattern: 'jar-classes/*.js', included: false},
            {pattern: 'test-classes/global/*.js', included: false},
            {pattern: 'classes/global/*.js', included: false},
            {pattern: 'classes/joynr.js', included: false},
            {pattern: 'classes/libjoynr-deps.js', included: false},
            {pattern: 'classes/joynr/**/*.js', included: false},
            {pattern: 'test-classes/require.config.common.js', included: false},
            {pattern: 'test-classes/test/**/*.js', included: false},
            {pattern: 'test-classes/joynr/provisioning/*.js', included: false},
            {pattern: 'test-classes/joynr/vehicle/*.js', included: false},
            {pattern: 'test-classes/joynr/vehicle/radiotypes/*.js', included: false},
            {pattern: 'test-classes/joynr/tests/testTypes/*.js', included: false},
            {pattern: 'test-classes/joynr/types/TestTypes/*.js', included: false},
            {pattern: 'test-classes/joynr/datatypes/*.js', included: false},
            {pattern: 'test-classes/joynr/datatypes/exampleTypes/*.js', included: false},

            {pattern: 'test-classes/integration/IntegrationUtils.js', included: false},
            {pattern: 'test-classes/integration/LocalStorageSimulator.js', included: false},
            {pattern: 'test-classes/integration/provisioning_end2end_common.js', included: false},
            {pattern: 'test-classes/integration/TestEnd2EndCommProviderWorker.js', included: false},
            {pattern: 'test-classes/integration/TestEnd2EndDatatypesProviderWorker.js', included: false},
            {pattern: 'test-classes/integration/TestEnd2EndDatatypesTestData.js', included: false},
            {pattern: 'test-classes/integration/WorkerUtils.js', included: false},

            {pattern: 'test-classes/integration/LibJoynrTest.js', included: false},
            {pattern: 'test-classes/integration/End2EndCommTest.js', included: false},
            {pattern: 'test-classes/integration/End2EndDatatypesTest.js', included: false},

            'test-classes/test-system-integration.js'
    ],


    // list of files to exclude
    exclude: [
            'test-classes/joynr/**/*Test.js'
    ],


    // preprocess matching files before serving them to the browser
    // available preprocessors: https://npmjs.org/browse/keyword/karma-preprocessor
    preprocessors: {
    },


    // test results reporter to use
    // possible values: 'dots', 'progress'
    // available reporters: https://npmjs.org/browse/keyword/karma-reporter
    //reporters: ['progress', 'junit'],
    reporters: ['verbose', 'junit'],


    // web server port
    port: 9876,


    // enable / disable colors in the output (reporters and logs)
    colors: true,


    // level of logging
    // possible values: config.LOG_DISABLE || config.LOG_ERROR || config.LOG_WARN || config.LOG_INFO || config.LOG_DEBUG
    logLevel: config.LOG_INFO,
    //logLevel: config.LOG_DEBUG,


    // enable / disable watching file and executing tests whenever any file changes
    autoWatch: true,


    // start these browsers
    // available browser launchers: https://npmjs.org/browse/keyword/karma-launcher
    //browsers: ['Chrome'],
    browsers: ['PhantomJS'],


    // Continuous Integration mode
    // if true, Karma captures browsers, runs the tests and exits
    singleRun: false,

    // Concurrency level
    // how many browser should be started simultanous
    concurrency: Infinity,

    // outputDir is already located in 'target'
    junitReporter: {
      outputDir: 'jstd-test-results',
      outputFile: 'TestSystemIntegration.xml',
      suite: '',
      useBrowserName: false,
      nameFormatter: undefined,
      classNameFormatter: undefined,
      properties: {}
    }
  })
}
