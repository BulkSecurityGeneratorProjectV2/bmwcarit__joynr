# joynr JEE inter-language-test

The `io.joynr.tests.inter-language-test-jee` project provides an inter-language
test environment using JEE applications.

As of now, the following features are supported:

* Synchronous method calls
* Synchronous Getter / Setter calls for attributes

## Installation

If you're building from source, then you can build and install the artifacts to
your local maven repo with:

```
cd <repository>/tests/inter-language-test-jee
mvn install
```

## Usage

This section describes the general usage of the joynr inter-language-test-jee
and provides simple examples of the usage where possible.

### Configuration

The configuration is included in the files
`src/main/java/io/joynr/test/interlanguage/jee/JoynrConfigurationProvider.java`
in the inter-language-test-jee-provider and inter-language-test-jee-consumer
directories.

A `@Singleton` EJB which has no business interface (i.e. does not
implement any interfaces) provides methods annotated with:

* `@JoynrProperties`
  * The method decorated with this annotation must return a `Properties`
    object which contains the joynr properties which the application
    wants to set.
* `@JoynrLocalDomain`
  * The method decorated with this annotation must return a string value
    which is the local domain used by the application to register all
    providers with.

Additionally, the method must be annotated with
`javax.enterprise.inject.Produces`.

#### Mandatory Properties

* `MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT` - this property needs
  to be set to the context root of your deployed application, with `/messaging`
  added to the end. E.g.: `/myapp/root/messaging`.
* `MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH` - this property needs to
  be set to the URL under which the application server you are running on can be
  reached, e.g. `https://myapp.mycompany.net`.
* `MessagingpropertyKeys.CHANNELID` - this property should be set to the
  application's unique DNS entry, e.g. `myapp.mycompany.net`. This is important,
  so that all nodes of the cluster are identified by the same channel ID.
* `MqttModule.PROPERTY_KEY_MQTT_BROKER_URI` - use this to configure the URL for
  connecting to the MQTT broker being used for communication.
  E.g. `tcp://mqtt.mycompany.net:1883`.

#### Optional Properties

* `MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS` - enables the
  [HiveMQ](http://www.hivemq.com) specific 'shared subscription' feature, which allows
  clustering of JEE applications using just MQTT for communication. Set this to `true`
  to enable the feature. Defaults to `false`.
* `JeeIntegrationPropertyKeys.JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY` -
  set this property to `true` if you want to use the HTTP Bridge functionality. In this
  configuration incoming messages are communicated via HTTP and can then be load-balanced
  accross a cluster via, e.g. nginx, and outgoing messages are communicated directly
  via MQTT. If you activate this mode, then you must also provide an endpoint registry
  (see next property).
* `JeeIntegrationPropertyKeys.JEE_INTEGRATION_ENDPOINTREGISTRY_URI` -
  this property needs to point to the endpoint registration service's URL with which the
  JEE Integration will register itself for its channel's topic.
  E.g. `http://endpointregistry.mycompany.net:8080`.
* `MessagingPropertyKeys.DISCOVERYDIRECTORYURL` and
  `MessagingPropertyKeys.DOMAINACCESSCONTROLLERURL` - configure the addresses for the
  discovery directory and domain access control services.
* `MessagingPropertyKeys.PERSISTENCE_FILE` - if you are deploying multiple joynr-enabled
  applications to the same container instance, then you will need to set a different filename
  for this property for each application. E.g.: `"my-app-joynr.properties"` for one and
  `"my-other-app-joynr.properties"` for another. Failing to do so can result in unexpected
  behaviour, as one app will be using the persisted properties and IDs of the other app.

You are principally free to provide any other valid joynr properties via these
configuration methods. See the [official joynr documentation](./JavaSettings.md)
for details.

#### Example

An example of a configuration EJB, which uses `servlet` (http) as primary global transport
(default is `mqtt`, `longpolling` is not supported in joynr JEE), is:

```
@Singleton
public class JoynrConfigurationProvider {

  @Produces
  @JoynrProperties
  public Properties joynrProperties() {
    Properties joynrProperties = new Properties();
    joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT,
        "/inter-language-test-jee-provider/messaging");
    joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH,
        "http://localhost:8080");
    joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID,
        "io.joynr.test.interlanguage.jee.provider");
    joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI,
        "tcp://localhost:1883");
    joynrProperties.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL,
        "http://localhost:8383/bounceproxy/");
    joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT,
         "servlet");
    joynrProperties.setProperty(MessagingPropertyKeys.DISCOVERYDIRECTORYURL,
        "http://localhost:8383/discovery/channels/discoverydirectory_channelid/");
    joynrProperties.setProperty(MessagingPropertyKeys.DOMAINACCESSCONTROLLERURL,
        "http://localhost:8383/discovery/channels/discoverydirectory_channelid/");

    return joynrProperties;
  }

  @Produces
  @JoynrLocalDomain
  public String joynrLocalDomain() {
    return "joynr-inter-language-test-domain";
  }
}
```


### JEE Container configuration

Configure your container runtime with a `ManagedScheduledExecutorService`
resource which has the name `'concurrent/joynrMessagingScheduledExecutor'`.

For example for Glassfish/Payara:
`asadmin create-managed-scheduled-executor-service --corepoolsize=10 concurrent/joynrMessagingScheduledExecutor`

Note the `--corepoolsize=10` option. The default will only create one thread,
which can lead to blocking, so you should use at least 10 threads. Depending on your
load, you can experiment with higher values to enable more concurrency when
communicating joynr messages.

## Example

Under `tests/inter-language-test-jee` you can find the test applications.
It uses an `InterLanguageTest.fidl` file based on the one used for the
standard inter-language-test without JEE, but implements it as a JEE provider
application and a separate JEE consumer application.

The project is sub-divided into one multi-module parent project and three subprojects:

```
 - inter-language-test-jee
   |- inter-language-test-jee-api
   |- inter-language-test-jee-provider
   |- inter-language-test-jee-consumer
```

In order to build the project, change to the `inter-language-test-jee` directory and call `mvn install`.

The following describes running the example on [Payara 4.1](http://www.payara.fish). First,
install the application server and you will also need to install an MQTT broker, e.g.
[Mosquitto](http://mosquitto.org).

Start the MQTT broker, and make sure it's accepting traffic on `1883`.

Then start up the Payara server by changing to the Payara install directory and executing
`bin/asadmin start-domain`. Follow the instructions above for configuring the required
managed executor service.

When using primaryglobaltransport=mqtt (default), you need a connection pool for the database which
shall be used by the backend to persist data.
For this example, we'll create a database
on the JavaDB (based on Derby) database which is installed as part of Payara / Glassfish:

    create-jdbc-connection-pool \
        --datasourceclassname org.apache.derby.jdbc.ClientDataSource \
        --restype javax.sql.XADataSource \
        --property portNumber=1527:password=APP:user=APP:serverName=\
        localhost:databaseName=joynr-discovery-directory:connectionAttributes=\; \
        create\\=true JoynrPool

Next, create a datasource resource pointing to that database connection. Here's an
example of what that would look like when using the connection pool created above:

`bin/asadmin create-jdbc-resource --connectionpoolid JoynrPool joynr/DiscoveryDirectoryDS`
`bin/asadmin create-jdbc-resource --connectionpoolid JoynrPool joynr/DomainAccessControllerDS`

After this, you can start the database:

`bin/asadmin start-database`

Next, fire up the joynr backend services:
- When using primaryglobaltransport=mqtt, deploy the required war files:
    - `bin/asadmin deploy <joynr home>/tests/inter-language-test-jee/target/discovery-jee.war`
    - `bin/asadmin deploy <joynr home>/tests/inter-language-test-jee/target/accesscontrol-jee.war`
- When using primaryglobaltransport=longpolling, change to the `inter-language-test-jee`
directory and execute `mvn -N -Pbackend-services-http jetty:run`.

Depending on whether only consumer, only provider or both should run as JEE applications,
deploy the required WAR files:

- `bin/asadmin deploy <joynr home>/tests/inter-language-test-jee/inter-language-test-jee-provider/target/inter-language-test-jee-provider.war`
- `bin/asadmin deploy <joynr home>/tests/inter-language-test-jee/inter-language-test-jee-consumer/target/inter-language-test-jee-consumer.war`

Make sure that any involved external embedded or standalone cluster controller is configured
correctly (especially because the Payara server is running on port 8080 and the Discovery and
Bounce proxy service is running inside a Jetty on port 8383 which differs from standard
configuration used for other tests).

In case just the JEE provider should be tested, then once the provider application has started
successfully, you can start up the external consumer as normal. Note that an external consumer
might start test cases which are not supported by the JEE interlanguage test environment.
The resulting failure can be ignored.

If the JEE consumer application should be used, then once the consumer application and the JEE or
external provider application have started up successfully, you can use an HTTP client
(e.g. `curl` on the command line or [Paw](https://luckymarmot.com/paw) on Mac OS X) to trigger
the start of the JUnit test cases on the consumer side.

- `curl http://localhost:8080/inter-language-test-jee-consumer/start-tests'
  - This starts the tests, and you should see some output similar to Surefire JUnit XML reports,
    but in condensed JSON format.

Example JSON output (pretty-printed):
```
{
  "testSuiteResults": [
    {
      "name": <nameOfTestSuite>,
      "tests": <numberOfTests>,
      "errors": <numberOfErrors>,
      "skipped": <numberOfSkippedTests>,
      "testCaseResults" : [
        {
          "classname": "io.joynr.test.interlanguage.jee.<TestSuiteClass>",
          "name": "<nameOfTestCase>,
          "status": "ok",
          "time": "<timeUsed>"
        },
        ...
        {
          "classname": "io.joynr.test.interlanguage.jee.<TestSuiteClass>",
          "name": "<nameOfTestCase>,
          "status": "failed",
          "time": "<timeUsed>",
          "failure": {
            "message": "<failureMessage>",
            "text": "<detailedInfoIfAny",
            "type": "<typeIfAny>"
          }
        },
        ...
      ]
    }
  ]
}
```

Note that by definition the order of JSON elements in a JSON object is undefined;
only JSON arrays have a defined order - so your output might look a bit different.
