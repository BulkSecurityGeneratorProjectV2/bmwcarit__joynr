# Starting a standalone Java Global Capabilities Directory

```
cd $JOYNR_REPOSITORY
mvn clean install
cd java/backend-services/capabilities-directory
java -jar target/deploy/capabilities-directory-jar-with-dependencies.jar
```

This will connect to a MQTT broker on localhost:1883.

The MQTT broker address can also be specified on the command
line by overriding a joynr property as shown below:

```
java -Djoynr.messaging.mqtt.brokerUri="tcp://somehost:1883" \
     -jar target/deploy/capabilities-directory-jar-with-dependencies.jar
```

# Shutting down the standalone Java Global Capabilities Directory

The application will shutdown if a network connection is attempted on localhost:9999 (e.g. via
telnet etc.). If required, the port can be modified using the additional command line argument
```
-Djoynr.capabilitiesdirectorylauncher.shutdownport=<port>
```

# Additional configuration for multiple backends

## Configure own GBID of the GCD instance (=GBID of the backend of the GCD instance)

The GCD needs to know the GBID (backend) it is responsible for.

Add `-Djoynr.gcd.gbid="ownGbid"` or set the environment variable `joynr_gcd_gbid` before starting
the GCD service.

DEFAULT: `joynrdefaultgbid`

## Configure list of known GBIDs that shall be accepted by the GCD instance

Add `-Djoynr.gcd.valid.gbids="gbid1,gbid2" or set the environment variable `joynr_gcd_valid_gbids`
before starting the GCD service.

DEFAULT: `["joynrdefaultgbid"]`

The GCD will only accept GBIDs that are in its list of valid GBIDs. Add, lookup and remove
attempts with other GBIDs are rejected with `DiscoveryError.UNKNOWN_GBID` or
`DiscoveryError.INVALID_GBID`.

