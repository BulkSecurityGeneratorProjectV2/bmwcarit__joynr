# Joynr Javascript Developer Guide

## Conversion of Franca entries

### Place holders

Note that the following notations in the code examples below must be replaced by actual values from
Franca:

```
// "<Attribute>" the Franca name of the attribute
// "<AttributeType>" the Franca name of the attribute type
// "<broadcast>" the Franca name of the broadcast, starting with a lowercase letter
// "<Broadcast>" the Franca name of the broadcast, starting with capital letter
// "BroadcastFilter<Attribute>" Attribute is the Franca attributes name
// "<Filter>" the Franca name of the broadcast filter
// "<interface>" the Franca interface name, starting with a lowercase letter
// "<Interface>" the Franca interface name, starting with capital letter
// "<method>" the Franca method name, starting with a lowercase letter
// "<Method>" the Franca method name, starting with capital letter
// "<OutputType>" the Franca broadcast output type name
// "<Package>" the Franca package name
// "<ProviderDomain>" the provider domain name used by provider and client
// "<ReturnType>" the Franca return type name
```

### Package name
The Franca ```<Package>``` will be transformed to the Javascript module ```joynr.<Package>```.

### Type collection name

The Franca ```<TypeCollection>``` will be transformed to the Javascript
module ```joynr.<Package>.<TypeCollection>```.

### Complex type name

Any Franca complex type ```<TypeCollection>.<Type>``` will result in the creation of an object
```joynr.<Package>.<TypeCollection>.<Type>``` (see above).

The same ```<Type>``` will be used for all elements in the event that this type is used as an
element of other complex types, as a method input or output argument, or as a broadcast output
argument.

### Interface name

The Franca ```<Interface>``` will be used as a prefix to create the following JavaScript objects:

```
<Interface>Provider
<Interface>Proxy
```

### Attribute, Method and Broadcast names

In Javascript the names of attributes, methods and broadcasts within the same interface must be
unique, as each name will become a property of the Proxy object.

# Building a Javascript consumer application

A Javascript joynr consumer application must "require" or otherwise load the ```joynr.js``` module
and call its ```joynr.load()``` method with provisioning arguments in order to create a **joynr
object**.

Next, for all Franca interfaces that are to be used, a **proxy** must be created using the
```build()``` method of the ```joynr.proxyBuilder``` object, with the provider's domain passed as
an argument.

Once the proxy has been successfully created, the application can add any attribute or broadcast
subscriptions it needs, and then enter its event loop where it can call the interface methods.

## Required imports

The following Javascript modules must be made available using require or some other loading
mechanism:
```
// for each type <Type>
"import" js/joynr/<Package>/<Type>.js
// for each interface <Interface>
"import" js/joynr/<Package>/<Interface>Proxy.js
"import" js/joynr.js
"import" js/joynrprovisioning.common.js
"import" js/joynrprovisioning.consumer.js
```

## Base implementation

The Javascript application must load and initialize the joynr runtime environment prior to calling
any other Joynr API.

```javascript
joynr.load(provisioning).then(function(loadedJoynr) {
    joynr = loadedJoynr;

    // build one or more proxies and optionally set up event handlers
}).catch(function(error) {
    // error handling
});

```

## The discovery quality of service

The ```DiscoveryQos``` configures how the search for a provider will be handled. It has the
following members:

* **discoveryTimeoutMs**  Timeout for the discovery process (milliseconds) if no compatible
  provider was found within the given time. A timeout triggers a DiscoveryException or
  NoCompatibleProviderFoundException containing the versions of the discovered incompatible
  providers.
* **discoveryRetryDelayMs** The time to wait between discovery retries after encountering a
  discovery error.
* **arbitrationStrategy** The arbitration strategy (details see below)
* **cacheMaxAgeMs** Defines the maximum allowed age of cached entries (milliseconds); only younger
  entries will be considered. If no suitable providers are found, depending on the discoveryScope,
  a remote global lookup may be triggered.
* **discoveryScope** The discovery scope (details see below)
* **providerMustSupportOnChange** If set to true, select only providers which support onChange
  subscriptions (set by the provider in its providerQos settings)
* **additionalParameters** special application-specific parameters that must match, e.g. a keyword

The enumeration **discoveryScope** defines options to decide whether a suitable provider will be
searched in the local capabilities directory or in the global one.

Available values are as follows:

* **LOCAL\_ONLY** Only entries from local capability directory will be searched
* **LOCAL\_THEN\_GLOBAL** Entries will be taken from local capabilities directory, unless no such
  entries exist, in which case global entries will be considered as well.
* **LOCAL\_AND\_GLOBAL** Entries will be taken from local capabilities directory and from global
  capabilities directory.
* **GLOBAL\_ONLY** Only the global entries will be looked at.

**Default discovery scope:** ```LOCAL_THEN_GLOBAL```

Whenever global entries are involved, they are first searched in the local cache. In case no global
entries are found in the cache, a remote lookup is triggered.

The **arbitration strategy** defines how the results of the scoped lookup will be sorted
and / or filtered. The arbitration strategy is a function with one parameter (the array of
capability entries found). It can either be selected from the predefined arbitration strategies
in ArbitrationStrategyCollection or provided as user-defined function. If this user-defined
function `myFunction` needs additional filter criteria like the arbitration strategy *Keyword*,
the result of `myFunction.bind(myParam)` has to be used as arbitration strategy.

**Predefined arbitration strategies:**
* **ArbitrationStrategyCollection.LastSeen** The participant that was last refreshed (i.e. with the
  most current last seen date) will be selected
* **ArbitrationStrategyCollection.Nothing** use DefaultArbitrator which picks the first discovered
   entry with compatible version
* **ArbitrationStrategyCollection.HighestPriority** Highest priority provider will be selected
* **ArbitrationStrategyCollection.Keyword** Only a Provider that has keyword set will be selected

**Default arbitration strategy:** ```ArbitrationStrategyCollection.LastSeen```

The priority used by the arbitration strategy *HighestPriority* is set by the provider in its
providerQos settings.

Example for setting up a ```DiscoveryQos``` object:
```javascript
// additionalParameters unclear
// there is currently no ArbitrationConstants in Javascript like in Java
// { "keyword" : "someKeyword" }
// { "fixedParticipantId" : "someParticipantId" }
// { }

var discoveryQos = new joynr.proxy.DiscoveryQos({
    discoveryTimeoutMs : 30000,
    discoveryRetryDelayMs : 1000,
    arbitrationStrategy : ArbitrationStrategyCollection.LastSeen,
    cacheMaxAgeMs : 0,
    discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL,
    providerMustSupportOnChange : false,
    // additional parameters are used for arbitration strategy Keyword (key: "keyword")
    // or can be used for custom arbitration strategies
    additionalParameters : {
        "key1": "value1",
        ...
        "keyN": "valueN"
    }
});
```

Missing parameters will be replaced by the default settings.

## The message quality of service

The ```MesssagingQos``` object defines the **roundtrip timeout in milliseconds** for
**RPC requests** (getter/setter/method calls) and unsubscribe requests and it allows
definition of additional custom message headers.
The ttl for subscription requests is calculated from the ```expiryDateMs```
in the [SubscriptionQos](#subscription-quality-of-service) settings.
The ttl of internal joynr messages (e.g. provider discovery) can be set via the
internalMessagingQosValue in the [provisioning settings](JavaScriptTutorial.md#provisioning)

If no specific setting is given, the default roundtrip timeout is 60 seconds.
The keys of custom message headers may contain ascii alphanumeric or hyphen.
The values of custom message headers may contain alphanumeric, space, semi-colon, colon,
comma, plus, ampersand, question mark, hyphen, dot, star, forward slash and back slash.
If a key or value is invalid, the API method called to introduce the custom message
header throws an Error.

Example:

```javascript
var messagingQos = new joynr.messaging.MessagingQos({
    ttl: 60000,
    // optional custom headers
    customHeaders: {
        "key1": "value1",
        ...
        "keyN": "valueN"
    }
});

// optional
messagingQos.putCustomHeader("anotherKey", "anotherValue");
```

## Building a proxy

Proxy creation is necessary before services from a provider can be called:
* call its **methods** (RPC) **asynchronously**
* **subscribe** or **unsubscribe** to its **attributes** or **update** a subscription
* **subscribe** or **unsubscribe** to its **broadcasts** or **update** a subscription

The ProxyBuilder.build call requires the provider's domain. Optionally, **messagingQos** and
**discoveryQos** settings can be specified if the default settings are not suitable.

In case no suitable provider can be found during discovery, a `DiscoveryException` or
`NoCompatibleProviderFoundException` is thrown.

```javascript
var domain = "<ProviderDomain>";

var messagingQos, discoveryQos;
// setup messagingQos, discoveryQos

joynr.proxyBuilder.build(<Interface>Proxy, {
    domain: domain,
    discoveryQos: discoveryQos, // optional
    messagingQos: messagingQos  // optional
}).then(function(<interface>Proxy) {
    // subscribe to attributes (optional)

    // subscribe to broadcasts (optional)

    // call methods or setup event handlers which call methods
}).catch(function(error) {
    // handle error
});
```

## Method calls (RPC)

In Javascript all method calls are asynchronous. Since the local proxy method returns a Promise,
the reaction to the resolving or rejecting of the Promise can be immediately defined.
Note that the message order on Joynr RPCs will not be preserved; if calling order is required,
then the subsequent dependent call should be made in the following then() call.
```javascript
<interface>Proxy.<method>(... optional arguments ...).then(function(response) {
	// call successful, handle response value
}).catch(function(error) {
    // call failed, execute error handling
    // The following objects are used to receive error details from provider side:
    // - joynr.exceptions.ApplicationException (its member 'error' holds the error enumeration value,
    //   wrt. error enumeration value range please refer to the Franca specification of the method)
    // - joynr.exceptions.ProviderRuntimeException (with embedded 'detailMessage')
});
```

## Subscription quality of service

A subscription quality of service setting is required for subscriptions to broadcasts or attribute
changes. The following sections cover the 4 quality of service objects available.

### SubscriptionQos

```SubscriptionQos``` has the following members:

* **expiryDateMs** Absolute Time until notifications will be send (in milliseconds)
* **publicationTtlMs** Lifespan of a notification (in milliseconds), the notification will be
  deleted afterwards
  Known Issue: subscriptionQos passed when subscribing to a non-selective broadcast are ignored.
  The API will be changed in the future: proxy subscribe calls will no longer take a
  subscriptionQos; instead the publication TTL will be settable on the provider side.

```javascript
var subscriptionQos = new joynr.proxy.SubscriptionQos({
    expiryDateMs : 0,
    publicationTtlMs : 10000
});
```

The default values are as follows:

```
{
    expiryDateMs: SubscriptionQos.NO_EXPIRY_DATE,  // 0
    publicationTtlMs : SubscriptionQos.DEFAULT_PUBLICATION_TTL // 10000
}
```

### PeriodicSubscriptionQos

```PeriodicSubscriptionQos``` has the following additional members:

* **periodMs** defines how long to wait before sending an update even if the value did not change
* **alertAfterIntervalMs** Timeout for notifications, afterwards a missed publication notification
  will be sent (milliseconds)

This object can be used for subscriptions to attributes.

Note that updates will be sent only based on the specified interval, and not as a result of an
attribute change.

```javascript
var subscriptionQosPeriodic = new joynr.proxy.PeriodicSubscriptionQos({
    periodMs : 60000,
    alertAfterIntervalMs : 0
});
```
The default values are as follows:
```
{
    periodMs: PeriodicSubscriptionQos.DEFAULT_PERIOD_MS // 60000
    alertAfterIntervalMs: PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL // 0
}
```

### OnchangeSubscriptionQos

The object ```OnChangeSubscriptionQos``` inherits from ```SubscriptionQos``` and has the following
additional members:

* **minIntervalMs** Minimum time to wait between successive notifications (milliseconds)

This object should be used for subscriptions to broadcasts. It can also be used for subscriptions
to attributes if no periodic update is required.

Example:
```javascript
var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
    minIntervalMs : 1000
});
```
The default is as follows:
```
{
    minIntervalMs: OnChangeSubscriptionQos.DEFAULT_MIN_INTERVAL_MS // 1000
}
```

### OnchangeWithKeepAliveSubscriptionQos

The object ```OnChangeWithKeepAliveSubscriptionQos``` inherits from ```OnChangeSubscriptionQos```
and has the following additional members:

* **maxIntervalMs** Maximum time to wait between notifications, if value has not changed
* **alertAfterIntervalMs** Timeout for notifications, afterwards a missed publication notification
  will be sent (milliseconds)

This object can be used for subscriptions to attributes. Updates will then be sent both
periodically and after a change (i.e. this acts like a combination of PeriodicSubscriptionQos
and OnChangeSubscriptionQos).

Using it for subscriptions to broadcasts is theoretically possible because of inheritance but
makes no sense (in this case the additional members will be ignored).

Example:
```javascript
var subscriptionQosOnChangeWithKeepAlive = new joynr.proxy.OnChangeWithKeepAliveSubscriptionQos({
    maxIntervalMs : 60000,
    alertAfterIntervalMs: 0
});
```
The default is as follows:
```
{
    maxIntervalMs : OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS // 60000
    alertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL // 0
}
```

## Subscribing to an attribute

Attribute subscription - depending on the subscription quality of service settings used - informs
an application either periodically and / or on change of an attribute about the current value.

The **subscriptionId** is returned asynchronously after the subscription is successfully registered
at the provider. It can be used later to update the subscription or to unsubscribe from it.
If the subscription failed, a SubscriptionException will be returned via the callback (onError) and
via the Promise.

To receive the subscription, **callback functions** (onReceive, onSubscribed, onError) have to be
provided as outlined below.

```javascript
<interface>Proxy.<Attribute>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully registered at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }
}).then(function(subscriptionId) {
    // subscription successful, store subscriptionId for later use
}).catch(function(error) {
    // handle error case
});
```

## Updating an attribute subscription

The ```subscribe()``` method can also be used to update an existing subscription, by passing the
**subscriptionId** as an additional parameter as follows:

```javascript
// subscriptionId from earlier subscribe call
<interface>Proxy.<Attribute>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    subscriptionId: subscriptionId,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully updated at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }
}).then(function(subscriptionId) {
    // subscription update successful, the subscriptionId should be the same as before
}).catch(function(error) {
    // handle error case
});
```

## Unsubscribing from an attribute

Unsubscribing from an attribute subscription requires the **subscriptionId** returned by the
earlier subscribe call.

```javascript
<interface>Proxy.<Attribute>.unsubscribe({
    subscriptionId: subscriptionId
}).then(function() {
    // handle success case
}).catch(function(error) {
    // handle error case
});
```

## Subscribing to a (non-selective) broadcast

A Broadcast subscription informs the application in case a broadcast is fired by a provider.
The output values are returned via a callback function.

A broadcast is selective only if it is declared with the `selective` keyword in Franca, otherwise it
is non-selective.

Non-selective broadcast subscriptions can be passed optional **partitions**. A partition is a
hierarchical list of strings similar to a URL path. Subscribing to a partition will cause only those
broadcasts to be sent to the consumer that match the partition. Note that the partition is set when
subscribing on the consumer side, and must match the partition set on the provider side when the
broadcast is performed.

Example: a consumer could set a partition of "europe", "germany", "munich" to receive broadcasts for
Munich only. The matching provider would use the same partition when sending the broadcast.

The **subscriptionId** is returned asynchronously after the subscription is successfully registered
at the provider. It can be used later to update the subscription or to unsubscribe from it.
If the subscription failed, a SubscriptionException will be returned via the callback (onError) and
via the Promise.

To receive the subscription, **callback functions** (onReceive, onSubscribed, onError) have to be
provided as outlined below.

```javascript
<interface>Proxy.<Broadcast>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully registered at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }

    // optional parameter for multicast subscriptions (subscriptions to non selective broadcasts)
    partitions: [partitionLevel1,
                 ...
                 partitionLevelN]
}).then(function(subscriptionId) {
    // subscription successful, store subscriptionId for later use
}).catch(function(error) {
    // handle error case
});
```
The [partition syntax is explained in the multicast concept](../docs/multicast.md#partitions)

## Updating a (non-selective) broadcast subscription

The ```subscribe()``` method can also be used to update an existing subscription, when the
**subscriptionId** is passed as an additional parameter as follows:
```javascript
<interface>Proxy.<Broadcast>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    subscriptionId: subscriptionId,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully updated at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }

    // optional parameter for multicast subscriptions (subscriptions to non selective broadcasts)
    partitions: [partitionLevel1,
                 ...
                 partitionLevelN]
}).then(function(subscriptionId) {
    // subscription update successful, the subscriptionId should be the same as before
}).catch(function(error) {
    // handle error case
});

```

## Subscribing to a broadcast with filter parameters

Broadcast subscription with a **filter** informs the application in case a **selected broadcast
which matches filter criteria** is fired from the provider side. The output values are returned
via callback.


The **subscriptionId** is returned asynchronously after the subscription is successfully registered
at the provider. It can be used later to update the subscription or to unsubscribe from it.
If the subscription failed, a SubscriptionException will be returned via the callback (onError) and
via the Promise.

To receive the subscription, **callback functions** (onReceive, onSubscribed, onError) have to be
provided as outlined below.

In addition to the normal broadcast subscription, the filter parameters for this broadcast must be
created and initialized as additional parameters to the ```subscribe``` method. These filter
parameters are used to receive only those broadcasts matching the provided filter criteria.

```javascript
var fParam = <interface>Proxy.<broadcast>.createFilterParameters();
// for each parameter
fParam.set<Parameter>(parameterValue);
<interface>Proxy.<Broadcast>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    filterParameters : fParam,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully registered at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }
}).then(function(subscriptionId) {
    // subscription successful, store subscriptionId for later use
}).catch(function(error) {
    // handle error case
});
```

## Updating a broadcast subscription with filter parameters

The **subscribeTo** method can also be used to update an existing subscription, by passing the
**subscriptionId** as an additional parameter as follows:


```javascript
var fParam = <interface>Proxy.<broadcast>.createFilterParameters();
// for each parameter
fParam.set<Parameter>(parameterValue);
<interface>Proxy.<Broadcast>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    subscriptionId: subscriptionId,
    filterParameters : fParam,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully updated at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }
}).then(function(subscriptionId) {
    // subscription update successful, the subscriptionId should be the same as before
}).catch(function(error) {
    // handle error case
});
```

## Unsubscribing from a broadcast

Unsubscribing from a broadcast subscription requires the **subscriptionId** returned asynchronously
by the earlier subscribe call.

```javascript
<interface>Proxy.<Broadcast>.unsubscribe({
    subscriptionId : subscriptionId,
}).then(function() {
    // call successful
}).catch(function(error) {
    // handle error case
});

```

# Building a Javascript Provider application

A Javascript joynr **provider** application must "require" or otherwise load the ```joynr.js```
module and call its ```joynr.load()``` method with provisioning arguments in order to create a
joynr object.

Next, for all Franca interfaces that are being implemented, a **providerBuilder** must be created
using the ```build()``` method of the ```joynr.providerBuilder``` object supplying the providers
domain as argument.

Upon successful creation, the application can register all **Capabilities** it provides, and enter
its event loop where it can handle calls from the proxy side.

## Required imports

The following Javascript modules must be made "required" or otherwise loaded:
```
// for each type <Type>
"import" js/joynr/<Package>/<Type>.js
// for each interface <Interface>
"import" js/joynr/<Package>/<Interface>Proxy.js
"import" js/joynr.js
"import" js/joynrprovisioning.common.js
"import" js/joynrprovisioning.provider.js
```

## The Provider quality of service

The ```ProviderQos``` has the following members:

* **customParameters** e.g. the key-value for the arbitration strategy Keyword during discovery
* **priority** the priority used for arbitration strategy HighestPriority during discovery
* **scope** the scope (see below), used in discovery
* **supportsOnChangeSubscriptions** whether the provider supports subscriptions on changes

The **scope** can be
* **LOCAL** The provider will be registered in the local capability directory
* **GLOBAL** The provider will be registered in the local and global capability directory

Example:
```javascript
var providerQos = new joynr.types.ProviderQos({
    customParameters: [],
    priority : 100,
    scope: joynr.types.ProviderScope.GLOBAL,
    supportsOnChangeSubscriptions : true
});
```

## Provider
A provider application must load joynr and when this has been successfully finished, it can
register a Provider implementation for each Franca interface it implements.
It is also possible to unregister that implementation again, e.g. on shutdown.

While the implementation is registered, the provider will respond to any method calls from outside,
can report any value changes via publications to subscribed consumers, and may fire broadcasts, as
defined in the Franca interface.
```javascript
$(function() {
    // for each <Interface> where a Provider should be registered for later
    var <interface>provider = null;
    var <interface>ProviderImpl = new <Interface>ProviderImpl();

    var provisioning = {};
    provisioning.channelId = "someChannel";

    joynr.load(provisioning).then(function(loadedJoynr) {
        joynr = loadedJoynr;

        // when applications starts up:
        // register <Interface>provider
        ...
        // main loop here
        ...
        // when application ends:
        // unregister <Interface>provider
    }).catch(function(error){
        if (error) {
            throw error;
        }
    });
})();
```

## Register a Provider implementation
When registering a provider implementation for a specific Franca interface, the object implementing
the interface, the provider's domain and the provider's quality of service settings are passed as
parameters.

```javascript
var <interface>ProviderQos;
<interface>Provider = joynr.providerBuilder.build(<Interface>Provider, <interface>ProviderImpl);

// for any filter of a broadcast with filter
<interface>Provider.<broadcast>.addBroadcastFilter(new <Filter>BroadcastFilter());

// setup <interface>ProviderQos
joynr.registration.registerProvider(
    domain,
    <interface>Provider,
    <interface>ProviderQos
).then(function() {
    // registration successful
}).catch(function() {
    // registration failed
});
```

## Unregister a provider
Unregistering a previously registered provider requires the provider's domain and the object that
represents the provider implementation.

```javascript
// provider should have been set and registered previously
joynr.registration.unregisterProvider(
    domain,
    <Interface>provider
).then(function() {
    // unregistration successful
}).catch(function() {
    // unregistration failed
});
```

## The Provider implementation for an interface
The function implementing the interface must provide code for all its methods and a getter function
for every attribute.
```javascript
var <Interface>ProviderImpl =
    function <Interface>ProviderImpl() {
        var self = this;

        // define <method> handler

        // define internal representation of <attribute> and
        // getter handlers per <attribute>
        // wrappers to fire broadcasts
    };
```

## Method handler
Each handler for a Franca method for a specific interface is implemented as a function object
member of **this**. The parameters are provided as objects. The implementation can be done
either asynchronously or synchronously.

### Synchronous implementation
```javascript
this.<method> = function(parameters) {
    // handle method, return returnValue of type <returnType> with
    // return returnValue;
    // - or -
    // throw errorEnumerationValue;
    // (wrt. error enumeration value range please refer to the Franca specification of the method)
    // - or -
    // throw new joynr.exceptions.ProviderRuntimeException({ detailMessage: "reason" });
};
```

### Asynchronous implementation
```javascript
this.<method> = function(parameters) {
    var result = new Promise(function(resolve, reject) {
        // handle method, then either return the value
        // of type <returnType> with
        // resolve(returnValue);
        // - or -
        // reject(errorEnumerationValue);
        // (wrt. error enumeration value range please refer to the Franca specification of the method)
        // - or -
        // reject(new ProviderRuntimeException({ detailMessage: "reason" }));
    });

    // handle method, return returnValue of type <returnType>
    return result;
};
```

## Attribute handler
For each Franca attribute of an interface, a member of **this** named after the
```<attribute>``` has to be created which consists of an object which includes a getter function
as attribute that returns the current value of the ```<attribute>```. Also an internal
representation of the Franca attribute value has to be created and properly intialized.
```javascript
// for each <attribute> of the <interface> provide an internal representation
// and a getter
var internal<Attribute> = <initialValue>;

this.<attribute> = {
    get: function() {
        return attributeValue;
    }
};
```

## Attribute change broadcast
The provider implementation must inform about any change of an attribute which is not done via a
remote setter call by calling valueChanged on the given attribute. If an attribute setter is called
remotely, an attribute change publication will be triggered automatically.
```javascript
self.<attribute>.valueChanged(newValue);
```
## Sending a broadcast
For each Franca broadcast, a member of **this** named after the ```<broadcast>```
has to be created which consists of an empty object.

```javascript
this.<broadcast> = {};
```

The broadcast can then later be fired using
```javascript
this.fire<Broadcast> = function() {
    var outputParameters;
    outputParameters = self.<broadcast>.createBroadcastOutputParameters();
    // foreach output parameter of the broadcast
    outputParameters.set<Parameter>(value);

    // optional: the partitions to be used for the broadcast
    // Note: wildcards are only allowed on consumer side
    var partitions = [partitionLevel1,
                      ...
                      partitionLevelN];

    self.<broadcast>.fire(outputParameters[, partitions]);
}
```
The [partition syntax is explained in the multicast concept](../docs/multicast.md#partitions)

## Selective (filtered) broadcasts

In contrast to unfiltered broadcasts, to realize selective (filtered) broadcasts, the filter logic
has to be implemented and registered by the provider. If multiple filters are registered on the
same provider and broadcast, all filters are applied in a chain and the broadcast is only
delivered if all filters in the chain return true.

### The broadcast filter object
A broadcast filter object implements a filtering function called ```filter()``` which returns a
boolean value indicating whether the broadcast should be delivered. The input parameters of the
```filter()``` method consist of the output parameters of the broadcast and the filter parameters
used by the consumer on subscription.

```javascript
(function(undefined) {
    var <Filter>BroadcastFilter = function <Filter>BroadcastFilter() {
        if (!(this instanceof <Filter>BroadcastFilter)) {
            return new <Filter>BroadcastFilter();
        }

        Object.defineProperty(this, 'filter', {
            enumerable: false,
            value: function(broadcastOutputParameters, filterParameters) {
                // Parameter value can be evaluated by calling getter functions, e.g.
                // broadcastOutputParameters.get<OutputParameter>()
                // filterParameters can be evaluated by using properties, e.g.
                // filterParameters.<property>
                //
                // Evaluate whether the broadcastOutputParameters fulfill
                // the filterParameter here, then return true, if this is
                // the case and the publication should be done, false
                // otherwise.

                return <booleanValue>;
            };
        });
    };

    return <Filter>BroadcastFilter;
}());
```
