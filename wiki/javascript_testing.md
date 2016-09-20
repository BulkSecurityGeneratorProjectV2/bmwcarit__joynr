# Joynr Javascript testing

## Testing environment

The libjoynr Javascript tests use the 
[Jasmine 2.4](http://jasmine.github.io/2.4/introduction.html) test
framework. All tests are run through the
[Karma](https://karma-runner.github.io) test runner. Additionally,
unit tests are also run as pure nodejs tests.

### Prerequisites

In order to run the tests, the following environment must be built:

```
cd <JOYNR_REPO>
mvn clean install -P javascript
cd javascript/libjoynr-js
```

### Running tests

The tests can then be run in automatic mode using

```
cd <JOYNR_REPO>
cd javascript/libjoynr-js
mvn clean install -DskipTests=false
```

## Karma test configuration

The karma test environment is configured using the following files:

```
# Unit test configuration
src/test/karma/karma.conf.js
src/test/karma/test-unit.js

# Integration test configuration
src/test/karma/karma.integration.conf.js
src/test/karma/test-integration.js

# System integration test configuration
src/test/karma/karma.system-integration.conf.js
src/test/karma/test-system-integration.js

# Intertab integration test configuration
src/test/karma/karma.intertab-integration.conf.js
src/test/karma/test-intertab-integration.js
```

The ```karma.*.conf.js``` files specify
* which files are made available (files to be tested, environment, test cases)
* which port is used for debugging
* which reporters are used (e.g. junit to create XML output)
* the output location of the test reports

The ```test-*.js``` files specify
* which of the provided files are to be run as tests by pattern filtering
  (currently selecting all provided files which match the pattern '\*Test.js')

### Selection of test cases

The test cases can be expanded or shrinked by

* reducing the number of files provided for loading in the browser through configuration change in
  the ```karma*.conf.js``` files - or -
* using a different filtering pattern in the  ```test-*.js``` files

If only a single-test should be run, the easiest way is to use the ```fit()``` (to enable just a
single test-case) or ```fdescribe()``` API (to enable all tests for that section) instead
of the regular ```it()``` or ```describe()``` ones. All other tests will be automatically
disabled. If used, remember to add those methods to a jslint global comment entry, otherwise
the jslint run will fail on that file.

If multiple tests should be run, then either the pattern (```TEST_REGEXP```) needs to be
modified or the list of loaded files must be explictly specified (instead of using wildcard
syntax), so that tests that should not be run are not loaded and consequently cannot be found
by the pattern match.

### Debugging

Karma tests can be debugged as follows:

In the required ```karma.<config>.js``` temporarily change the following entry:

* change ```browsers``` to the browser of your liking (e.g. ```Chrome``` or ```PhantomJS```)

Example:
```
    ...
    browsers: ['Chrome'],
    ...
```

The test can be run from command line provided the required modules are installed in the local
nodejs / npm environment. The required modules can be found in the ```devDependencies``` and
```dependencies``` sections of the npm configuration file ```src/main/resources/package.json```.
For the required npm and nodejs version please refer to what is included in the ```joynr-javascript```
docker container specified in ```docker/joynr-javascript/Dockerfile```.

Example for installation of the required modules:

```
cd <JOYNR_REPO>/javascript/libjoynr-js
npm install target/node-classes
ln -s target/node-classes/node_modules node_modules
```

Example command line for manual start:

```karma start src/test/karma/karma.<config>.js --single-run=false --debug```

If running a browser with visible UI just hit the debug button once the test has run once.
If using a headless browser (e.g. ```PhantomJS```) please run a browser (e.g. ```Google-Chrome```)
and load the following URI:

```
http://localhost:9876/debug.html
```

Activate the Browser console / debug window.

Browse the ```Files``` section or alike in order to locate the test-case in ```test-classes``` hierarchy
or the test source (e.g. in ```classes```). Set breakpoints as needed.
Reload the page (e.g. using ```Ctrl-R```). The tests should be executed again.

### Troubleshooting

Make sure that no conflicting Jetty instances are running since the test environment starts its own.

## Pure nodejs tests

### Selection of test cases

The configuration of the pure nodejs tests is part of the file ```/src/test/resources/node-run-unit-tests.js```.
The modules to be tested are part of the array argument passed to the last requirejs() call in that file.
In order to run a different selection of tests this list can be adapted accordingly.

Example:
```
requirejs([
    "tests/joynr/provider/ProviderOperationTest",
    "tests/joynr/provider/ProviderTest",
    ...
], function() {
    console.log("all tests modules loaded");

    loadingFinished = true;
    jasmine.execute();
});
```

### Running tests manually

In order to run the tests manually, execute the following commands

```
cd target/test-classes
chmod u+x node_run_unit_tests.sh
./node_run_unit_tests.sh
```
