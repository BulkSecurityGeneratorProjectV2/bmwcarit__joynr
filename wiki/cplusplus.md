# Joynr C++ Developer Guide

## FAQ

**Q1: My code throws "TypeId not known" exceptions when deserializing custom types.**

**A1: The type registration in C++ is realized through static initalization.
  This static initialization can be "optimized" away by the linker when you link against
  a static library containing the generated joynr code for your interfaces.**

  **There are two options:**

1. build a shared library
2. disable the linker optimization. For `ld` this can be done through
   `-Wl,--whole-archive <LIB_CONTAINING_GENERATED_CODE> -Wl,--no-whole-archive`.

## Conversion of Franca entries

### Place holders

Note that the following elements in the code examples below must be replaced by actual values from Franca:

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
The Franca ```<Package>``` will be transformed to the C++ namespace ```joynr::<Package>```.

### Type collection name
The Franca ```<TypeCollection>``` will be transformed to the C++ namespace ```joynr::<Package>::<TypeCollection>```.

### Complex type name

Any Franca complex type ```<TypeCollection>.<Type>``` will result in the creation of a ```class joynr::<Package>::<TypeCollection>::<Type>``` (see above).

The same ```<Type>``` will be used for all elements in the event that this type is used as an element of other complex types, as a method input or output argument, or as a broadcast output argument.

Getter and Setter methods will be created for any element of a struct type. Also a standard constructor, full arguments constructor and object argument constructor will be created automatically.

### Interface name

The Franca ```<Interface>``` will be used as a prefix to create the following C++ classes (the names of classes that serve as interfaces like in Java, are prefixed with the capital letter **I**):

```cpp
joynr::<Package>::I<Interface>
joynr::<Package>::I<Interface>Async
joynr::<Package>::I<Interface>Base
joynr::<Package>::I<Interface>Connector
joynr::<Package>::I<Interface>Subscription
joynr::<Package>::I<Interface>Sync
joynr::<Package>::<Interface>AbstractProvider
joynr::<Package>::Default<Interface>Provider
joynr::<Package>::<Interface>AsyncProxy
joynr::<Package>::<Interface>InProcessConnector
joynr::<Package>::<Interface>JoynrMessagingConnector
joynr::<Package>::<Interface><Broadcast>BroadcastFilter
joynr::<Package>::<Interface><Broadcast>BroadcastFilterParameters
joynr::<Package>::<Interface>Provider
joynr::<Package>::<Interface>Proxy
joynr::<Package>::<Interface>ProxyBase
joynr::<Package>::<Interface>RequestCaller
joynr::<Package>::<Interface>RequestInterpreter
joynr::<Package>::<Interface>SyncProxy
```

# Building a C++ consumer application

## Required include files

The following base includes are required for a C++ Consumer application:

```cpp
#include "joynr/JoynrRuntime.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/SubscriptionListener.h"
#include "joynr/OnChangeWithKeepAliveSubscriptionQos.h"
#include <cassert>
#include <limits>
#include "joynr/JsonSerializer.h
```

## The main program
The ```main()``` function in C++ should be structured as follows:

```cpp
int
main(int argc, char** argv)
{
    // creating the joynr runtime
    // creating the joynr proxy builder
    // creating the proxy / proxies
    // main application logic
    // shutting down
}
```

## Creating the runtime and proxy builder

The ```main()``` function must setup the configuration (provider domain etc.) and create the ```JoynrRuntime``` instance.

As a prerequisite, the **provider** and **consumer domain** need to be defined as shown below.

```cpp
    // setup providerDomain, pathToMessagingSettings, pathToMessagingSettings
    JoynrRuntime* runtime =
        JoynrRuntime::createRuntime(pathToLibJoynrSettings, pathToMessagingSettings);
    ProxyBuilder<<Package>::<Interface>Proxy>* proxyBuilder =
        runtime->createProxyBuilder<<Package>::<Interface>Proxy>(providerDomain);
```

Use the createRuntimeAsync static method of JoynrRuntime to create the runtime asynchronously:


```cpp
    auto onRuntimeCreated = [](std::unique_ptr<JoynrRuntime> createdRuntime) {
        // Process the created runtime here
    };

    auto onErrorCallback = [](exceptions::JoynrRuntimeException& exception) {
        // Process the error here
    };

    JoynrRuntime::createRuntimeAsync(
        pathToLibJoynrSettings,
        onRuntimeCreated,
        onErrorCallback,
        pathToMessagingSettings);
```

## The discovery quality of service

The class ```DiscoveryQos``` configures how the search for a provider will be handled. It has the following members:

* **discoveryTimeoutMs**  Timeout for discovery process (milliseconds), afterwards triggers
   DiscoveryException if no provider was found or NoCompatibleProviderFoundException containing the
   versions of the discovered incompatible providers
* **cacheMaxAgeMs** Defines the maximum allowed age of cached entries (milliseconds), only younger
   entries will be considered. If no suitable providers are found, then depending on the
   discoveryScope, a remote global lookup may be triggered.
* **arbitrationStrategy** The arbitration strategy (see below)
* **customParameters** special parameters, that must match, e.g. keyword (see below)
* **retryIntervalMs** The time to wait between discovery retries after encountering a discovery error.
* **discoveryScope** default: LOCAL_AND_GLOBAL (details see below)

The **discoveryScope** defines, whether a suitable provider will be searched only in the local
capabilities directory or also in the global one.

Available values are as follows:

* **LOCAL_ONLY** Only entries from local capabilities directory will be searched
* **LOCAL_THEN_GLOBAL** Entries will be taken from local capabilities directory, unless no such
   entries exist, in which case global entries will be looked up at as well.
* **LOCAL_AND_GLOBAL** Entries will be taken from local capabilities directory and from global
   capabilities directory.
* **GLOBAL_ONLY** Only the global entries will be looked at.

Whenever global entries are involved, they are first searched in the local cache. In case no global
entries are found in the cache, a remote lookup is triggered.

The enumeration ```ArbitrationStrategy``` defines special options to select a Provider:

* **LAST_SEEN** The participant that was last refreshed (i.e. with the most current last seen date)
 will be selected
* **HIGHEST_PRIORITY** Entries will be considered according to priority
* **KEYWORD** Only entries that have a matching keyword will be considered
* **FIXED_PARTICIPANT** select provider which matches the participantId provided as custom parameter
   in DiscoveryQos (see below), if existing
* **LOCAL_ONLY** (not implemented yet, will throw DiscoveryException)

**Default arbitration strategy:** LAST_SEEN

The priority is set by the provider through the call ```providerQos.setPriority()```.

Class ```DiscoveryQos``` also provides keys for the key-value pair for the custom Parameters of
discoveryScope:

* **KEYWORD_PARAMETER**

Example for **KEYWORD** arbitration strategy:

```cpp
discoveryQos.addCustomParameter(DiscoveryQos::KEYWORD_PARAMETER(), "keyword");
```

Example for **FIXED_PARTICIPANT** arbitration strategy:

```cpp
discoveryQos.addCustomParameter("fixedParticipantId", "participantId");
```

Example for the creation of a DiscoveryQos class object:

```cpp
DiscoveryQos discoveryQos;

discoveryQos.setDiscoveryTimeoutMs(10000); // optional, default 30000
discoveryQos.setCacheMaxAgeMs(0); // optional, default 0
discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY); // default HP
discoveryQos.addCustomParameter(key, value); // optional, default none
discoveryQos.setProviderMustSupportOnChange(true); // optional, default false
discoveryQos.setRetryIntervalMs(1000); // optional, default 1000
```

## The message quality of service

The ```MesssagingQos``` class defines the roundtrip timeout for RPC requests in milliseconds
and allows definition of additional custom message headers.

If no specific setting is given, the default roundtrip timeout is 60 seconds.
The keys of custom message headers may contain ascii alphanumeric or hyphen.
The values of custom message headers may contain alphanumeric, space, semi-colon, colon,
comma, plus, ampersand, question mark, hyphen, dot, star, forward slash and back slash.
If a key or value is invalid, the API method called to introduce the custom message
header throws a std::invalid_argument exception.

Example:

```cpp
long ttl_ms = 60000;
MessagingQos messagingQos(ttl_ms);
// optional custom headers
std::unordered_map<std::string, std::string> customHeaders;
customHeaders.emplace("key1", "value1");
...
customHeaders.emplace("keyN", "valueN");
messagingQos.putAllCustomMessageHeaders(customHeaders);
...
std::string anotherKey("anotherKey");
std::string anotherValue("anotherValue");
messagingQos.putCustomMessageHeader(anotherKey, anotherValue);
```

## Creating a proxy

The consumer application instance must create one **proxy** per used Franca interface in order to be able to
* call its **methods** (RPC) either **synchronously** or **asynchronously**
* **subscribe** or **unsubscribe** to its **attributes** or **update** a subscription
* **subscribe** or **unsubscribe** to its **broadcasts** or **update** a subscription

In case no suitable provider can be found during discovery, a ```DiscoveryException``` is thrown.

```cpp
    DiscoveryQos discoveryQos;
    MessagingQos messagingQos;

    // setup discoveryQos, messagingQos attributes

    ProxyBuilder<<Package>::<Interface>Proxy>* proxyBuilder =
        runtime->createProxyBuilder<<Package>::<Interface>Proxy>(providerDomain);

    try {
        <Package>::<Interface>Proxy* proxy = proxyBuilder->setMessagingQos(messagingQos)
            ->setCached(false)
            ->setDiscoveryQos(discoveryQos)
            ->build();

        // call methods, subscribe to broadcasts etc.
        // enter some event loop
    } catch(joynr::exceptions::DiscoveryException& e) {
        // error handling
    }
```

Use the buildAsync method of ProxyBuilder to create a proxy asynchronously:

```cpp
    auto onSuccess = [](std::unique_ptr<<Package>::<Interface>Proxy> proxy) {
        // Process the created proxy here
    }

    auto onError = [](const exceptions::DiscoveryException& exception) {
        // Handle the exception here
    }

    proxyBuilder->setMessagingQos(messagingQos)
        ->setCached(false)
        ->setDiscoveryQos(discoveryQos)
        ->buildAsync(onSuccess, onError);
```

## Synchronous Remote procedure calls
While the provider executes the call asynchronously in any case, the consumer will wait until the call is finished, i.e. the thread will be blocked.
Note that the message order on Joynr RPCs will not be preserved.

```cpp
// for any Franca type named "<Type>" used
#include "joynr/<Package>/<TypeCollection>/<Type>.h"

    try {
        <ReturnType1> retval1;
        ...
        <ReturnTypeN> retvalN;

        <interface>Proxy-><method>([retval1, ..., retvalN,][inputVal1, ..., inputValN);
    } catch (joynr::exceptions::JoynrRuntimeException& e) {
        // error handling
    } catch (joynr::exceptions::ApplicationException& e) {
        // If error enumerations are defined for the called method, ApplicationExceptions must also
        // be caught. The ApplicationException serves as container for the actual error enumeration
        // which can be retrieved by calling e.getError().
    }
```

## Asynchronous Remote Procedure calls
Using asynchronous method calls allows the current thread to continue its work. For this purpose a
callback has to be provided for the API call in order to receive the result and error respectively.
Note the current thread will still be blocked until the Joynr message is internally set up and
serialized. It will then be enqueued and handled by a Joynr Middleware thread.
The message order on Joynr RPCs will not be preserved.
If no return type exists, the term ```Void``` is used instead.

```cpp
#include "joynr/<Package>/<TypeCollection>/<Type>.h"

std::shared_ptr<joynr::Future<<ReturnType1> [, ..., <ReturnTypeN>]> >
    future(new joynr::Future<<ReturnType1> [, ..., <ReturnTypeN>]>());
<ReturnType1> retval1;
...
<ReturnTypeN> retvalN;

// optional callback functions
std::function<void(const <ReturnType1> retval1 [, ..., const <ReturnTypeN> retvalN])> onSuccess =
    [] (const <ReturnType1> retval1 [, ..., const <ReturnTypeN> retvalN]) {
        // special handling
    };
std::function<void(const joynr::exceptions::JoynrException&)> onError =
    [] (const joynr::exceptions::JoynrException& error) {
        // error handling
    };

future = <interface>Proxy-><method>(... arguments ..., [onSuccess [, onError]]);
try {
    std::uint16_t optionalTimeoutMs = 500;
    future->get([optionalTimeoutMs, ]retval1 [, ..., retvalN ]);
} catch (const joynr::exceptions::JoynrRuntimeException& e) {
    // error handling
} catch (const joynr::exceptions::ApplicationException& e) {
    // If error enumerations are defined for the called method, ApplicationExceptions must also
    // be caught. The ApplicationException serves as container for the actual error enumeration
    // which can be retrieved by calling e.getError().
} catch (const joynr::exceptions::JoynrTimeOutException& e) {
    // handle timeout
}
```

## Quality of Service settings for subscriptions

### SubscriptionQos

The abstract class ```SubscriptionQos``` has the following members:

* **expiryDateMs** Absolute Time until notifications will be send (milliseconds)
* **publicationTtlMs** Lifespan of a notification (milliseconds), it will be deleted afterwards

### PeriodicSubscriptionQos

The class ```PeriodicSubscriptionQos``` inherits from ```SubscriptionQos``` and has the following additional members:

* **periodMs** defines how long to wait before sending an update even if the value did not change
* **alertAfterIntervalMs** Timeout for notifications, afterwards a missed publication notification will be raised (milliseconds)

This class can be used for subscriptions to attributes.

Note that updates will be send only based on the specified interval and not based on changes of the attribute.

### OnChangeSubscriptionQos

The class ```OnChangeSubscriptionQos``` inherits from ```SubscriptionQos``` and has the following additional members:

* **minIntervalMs** Minimum time to wait between successive notifications (milliseconds)

This class should be used for subscriptions to broadcasts. It can also be used for subscriptions
to attributes if no periodic update is required.

### OnchangeWithKeepAliveSubscriptionQos

The class ```OnChangeWithKeepAliveSubscriptionQos``` inherits from ```OnChangeSubscriptionQos``` and has the following additional members:

* **maxIntervalMs** Maximum time to wait between notifications, if value has not changed
* **alertAfterIntervalMs** Timeout for notifications, afterwards a missed publication notification will be raised (milliseconds)

This class can be used for subscriptions to attributes. Updates will then be sent based both periodically and after a change (i.e. this acts like a combination of ```PeriodicSubscriptionQos``` and ```OnChangeSubscriptionQos```).

Using it for subscriptions to broadcasts is theoretically possible because of inheritance but makes no sense (in this case the additional members will be ignored).


## Subscribing to an attribute

Attribute subscription - depending on the subscription quality of service settings used - informs
the application either periodically and / or on change of an attribute about the current value.

The **subscriptionId** can be retrieved via the callback (onSubscribed) and via the future returned
by the subscribeTo call. It can be used later to update the subscription or to unsubscribe from it.
The subscriptionId will be available when the subscription is successfully registered at the
provider. If the subscription failed, a SubscriptionException will be returned via the callback
(onError) and thrown by future.get(subscriptionId).

To receive the subscription, a **callback** has to be provided which is done providing a listener
class as outlined below. Since the callback is called by a communication middleware thread, it
should not be blocked, wait for user interaction, or do larger computation. The callback methods
(onReceive, onSubscribed, onError) are optional. Only the required methods have to be implemented.

```cpp
// for any Franca type named "<Type>" used
#include "joynr/<Package>/<TypeCollection>/<Type>.h"

#include "joynr/SubscriptionListener.h"
#include "joynr/exceptions/SubscriptionException.h"
#include "joynr/exceptions/JoynrException.h"

class <AttributeType>Listener : public SubscriptionListener<<AttributeType>>
{
    public:
        // Constructor
        <AttributeType>Listener()
        {
        }

        // Destructor
        ~<AttributeType>Listener()
        {
        }

        // Gets called on every received publication
        void onReceive(const <AttributeType>& value)
        {
            // handle publication
        }

        // Gets called when the subscription is successfully registered at the provider
        void onSubscribed(const std::string& subscriptionId)
        {
            // save the subscriptionId for updating the subscription or unsubscribing from it
            // the subscriptionId can also be taken from the future returned by the subscribeTo call
        }

        // Gets called on every error that is detected on the subscription
        void onError(const joynr::exceptions::JoynrRuntimeException& error)
        {
            // handle error, e.g.:
            // - SubscriptionException if the subscription registration failed at the provider
            // - PublicationMissedException if a periodic subscription publication does not arrive
            //   in time
        }
};

auto listener = std::make_shared<ISubscriptionListener<AttributeType>>();

auto qos = std::make_shared<<QosClass>>();
// define details of qos by calling its setters here

std::shared_ptr<Future<std::string>> subscriptionIdFuture = proxy->subscribeTo<Attribute>(
    listener,
    qos
);

...

// get the subscriptionId from the Future when needed
std::string subscriptionId;
try {
    std::uint16_t optionalTimeoutMs = 500;
    subscriptionIdFuture->get([optionalTimeoutMs, ]subscriptionId);
} catch (const jonyr::exceptions::SubscriptionException& e) {
    // handle subscription error
} catch (const joynr::exceptions::JoynrTimeOutException& e) {
    // handle timeout
}
```

## Updating an attribute subscription

The ```subscribeTo``` method can also be used to update an existing subscription, when the
**subscriptionId** is given as additional parameter as follows:

```cpp
std::shared_ptr<Future<std::string>> subscriptionIdFuture = proxy->subscribeTo<Attribute>(
    listener,
    qos,
    subscriptionId
);
```

## Unsubscribing from an attribute

Unsubscribing from an attribute subscription also requires the **subscriptionId** returned by the
earlier subscribeTo call.

```cpp
proxy->unsubscribeFrom<Attribute>(subscriptionId);
```

## Subscribing to a broadcast unconditionally

Broadcast subscription informs the application in case a broadcast is fired from provider side and
returns the output values via callback.

The **subscriptionId** can be retrieved via the callback (onSubscribed) and via the future returned
by the subscribeTo call. It can be used later to update the subscription or to unsubscribe from it.
The subscriptionId will be available when the subscription is successfully registered at the
provider. If the subscription failed, a SubscriptionException will be returned via the callback
(onError) and thrown by future.get(subscriptionId).

To receive the subscription, a **callback** has to be provided which is done providing a listener
class as outlined below. Since the callback is called by a communication middleware thread, it
should not be blocked, wait for user interaction, or do larger computation. The callback methods
(onReceive, onSubscribed, onError) are optional. Only the required methods have to be implemented.

```cpp
// for any Franca type named "<Type>" used
#include "joynr/<Package>/<TypeCollection>/<Type>.h"

#include "joynr/SubscriptionListener.h"
#include "joynr/exceptions/SubscriptionException.h"
#include "joynr/exceptions/JoynrException.h"

class <Broadcast>Listener : public SubscriptionListener<<OutputType1>[, ... <OutputTypeN>]>
{
    public:
        // Constructor
        <Broadcast>Listener()
        {
        }

        // Destructor
        ~<Broadcast>Listener()
        {
        }

        // Gets called on every received publication
        void onReceive(<OutputType1> value1[, ... <OutputTypeN> valueN])
        {
            // handle broadcast
        }

        // Gets called when the subscription is successfully registered at the provider
        void onSubscribed(const std::string& subscriptionId)
        {
            // save the subscriptionId for updating the subscription or unsubscribing from it
            // the subscriptionId can also be taken from the future returned by the subscribeTo call
        }

        // Gets called on every error that is detected on the subscription
        void onError(const joynr::exceptions::JoynrRuntimeException& error)
        {
            // handle error
        }
};

auto listener = std::make_shared<ISubscriptionListener<OutputType1>[, ... <OutputTypeN>]>();

auto qos = std::make_shared<OnChangeSubscriptionQos>();
// define details of qos by calling its setters here

std::shared_ptr<Future<std::string>> subscriptionIdFuture = proxy->subscribeTo<Broadcast>Broadcast(
    listener,
    qos
);

...

// get the subscriptionId from the Future when needed
std::string subscriptionId;
try {
    std::uint16_t optionalTimeoutMs = 500;
    subscriptionIdFuture->get([optionalTimeoutMs, ]subscriptionId);
} catch (const jonyr::exceptions::SubscriptionException& e) {
    // handle subscription error
} catch (const joynr::exceptions::JoynrTimeOutException& e) {
    // handle timeout
}
```

## Updating an unconditional broadcast subscription

The subscribeTo method can also be used to update an existing subscription, when the
**subscriptionId** is given as additional parameter as follows:

```cpp
std::shared_ptr<Future<std::string>> subscriptionIdFuture =
    <interface>Proxy.subscribeTo<Broadcast>Broadcast(
            listener,
            qos,
            subscriptionId
);
```

## Subscribing to a broadcast with filter parameters

Selective Broadcasts use filter logic implemented by the provider and filter parameters set by the
consumer to send only those broadcasts from the provider to the consumer that pass the filter. The
broadcast output values are passed to the consumer via callback.

The **subscriptionId** can be retrieved via the callback (onSubscribed) and via the future returned
by the subscribeTo call (see section
[Subscribing to a broadcast unconditionally](#subscribing-to-a-broadcast-unconditionally)).

To receive the subscription, a **callback** has to be provided (cf. section
[Subscribing to a broadcast unconditionally](#subscribing-to-a-broadcast-unconditionally)).

In addition to the normal broadcast subscription, the **filter parameters** for this broadcast must
be created and initialized as additional parameters to the ```subscribeTo``` method. These filter
parameters are used to receive only those broadcasts matching the provided filter criteria.

```cpp
// for any Franca type named "<Type>" used
#include "joynr/<Package>/<TypeCollection>/<Type>.h"

#include "joynr/ISubscriptionListener.h"
#include "joynr/SubscriptionListener.h"
#include "joynr/OnChangeSubscriptionQos.h"
...
// for class <Broadcast>Listener please refer to
// section "subscribing to a broadcast unconditionally"
...
auto listener = std::make_shared<ISubscriptionListener<OutputType1>[, ... <OutputTypeN>]>();

auto qos = std::make_shared<OnChangeSubscriptionQos>();
// define details of qos by calling its setters here

// create filterparams instance on stack
<Package>::<Interface><Broadcast>BroadcastFilterParams <broadcast>BroadcastFilterParams;
// set filter attributes by calling setters on the
// filter parameter instance

std::shared_ptr<Future<std::string>> subscriptionIdFuture =
    proxy->subscribeTo<Broadcast>Broadcast(
        <broadcast>BroadcastFilterParams,
        listener,
        qos
    );
// to retrieve the subscriptionId, please refer to section "subscribing to a broadcast unconditionally"
```

## Updating a broadcast subscription with filter parameters

The subscribeTo method can also be used to update an existing subscription, when the **subscriptionId** is given as additional parameter as follows:

```cpp
std::shared_ptr<Future<std::string>> subscriptionIdFuture =
    <interface>Proxy.subscribeTo<Broadcast>Broadcast(
            <broadcast>BroadcastFilterParams,
            listener,
            qos,
            subscriptionId
);
```

## Unsubscribing from a broadcast

Unsubscribing from a broadcast subscription requires the **subscriptionId** returned by the earlier subscribe call.

```cpp
proxy->unsubscribeFrom<Broadcast>Broadcast(subscriptionId);
```

## Shutting down
On shutdown of the application, the consumer should unsubscribe from any attributes and broadcasts it was subscribed to; delete any allocations and terminate the instance.

```cpp
// for each attribute subscribed to
proxy->unsubscribeFrom<Attribute>(subscriptionTo<Attribute>Id);

// for each broadcast subscribed to
proxy->unsubscribeFrom<Broadcast>Broadcast(subscriptionTo<Broadcast>Id);

delete proxy;
delete proxyBuilder;
delete runtime;
```

# Building a C++ Provider application

The C++ Provider mainly consists of the following classes:

* A  generic **Provider Application Class**
* One **Provider Class** for each Franca interface to be supported

## The MyProviderApplication program

The provider application class is used to register a provider class for each Franca interface to be supported.

### Required includes files

```cpp
#include "joynr/JoynrRuntime.h"
#include "My<Interface>Provider.h"
#include "<Broadcast>BroadcastFilter.h"
```

### The main function
The class can theoretically serve multiple Franca interfaces.

For each Franca interface implemented, the providing application creates an instance of ```My<Interface>Provider```, which implements the service for that particular interface, and registers it as a capability at the Joynr Middleware.

The example below shows the code for one interface:

```cpp
using namespace joynr;

int
main(int argc, char** argv)
{
    // creating the runtime
    // register any provider
    // main application logic
    // shutting down
}
```

### Creating the runtime

```cpp
    // setup pathToLibJoynrSettings, pathToMessagingSettings
    JoynrRuntime* runtime =
        JoynrRuntime::createRuntime(pathToLibJoynrSettings, pathToMessagingSettings);
```

### Registering provider
For each interface a specific provider class instance must be registered. From that time on, the provider will be reachable from outside and react on incoming requests (e.g. method RPC etc.). It can be found by consumers through Discovery.
Any specific broadcast filters must be added prior to registry.

```cpp
    // create instance of provider class
    std::shared_ptr<My<Interface>Provider> provider(new My<Interface>Provider());

    // create filter instance for each broadcast filter
    std::shared_ptr<<Broadcast>BroadcastFilter> <broadcast>BroadcastFilter(
            new <Broadcast>BroadcastFilter());
    provider->addBroadcastFilter(<broadcast>BroadcastFilter);

    runtime->registerProvider<<Package>::<Interface>Provider>(
        providerDomain, provider);
```

### Shutting down
On exit of the application it should cleanly unregister any providers the application had registered earlier and free resources.

```cpp
    // for each provider class
    runtime->unregisterProvider<Package>::<Interface>Provider>(
        providerDomain, provider);

    delete runtime;
```

## The My&lt;Interface>Provider class

The provider class implements the **attributes**, **methods** and **broadcasts** of a particular Franca interface.

### Required imports
The following Joynr C++ include files are required:

```cpp
#include "My<Interface>Provider.h"
```

### The Provider quality of service

The ```ProviderQos``` has the following members:

* **customParameters** e.g. the key-value for the arbitration strategy Keyword during discovery
* **providerVersion** the version of the provider
* **priority** the priority used for arbitration strategy HighestPriority during discovery
* **scope** the Provider scope (see below), used in discovery
* **supportsOnChangeSubscriptions** whether the provider supports subscriptions on changes

The **ProviderScope** can be
* **LOCAL** The provider will be registered in the local capability directory
* **GLOBAL** The provider will be registered in the local and global capability directory

Example:

```cpp
types::ProviderQos providerQos;
providerQos.setCustomParameters(customParameters);
providerQos.setProviderVersion(1);
providerQos.setPriority(100);
providerQos.setScope(ProviderScope.GLOBAL);
providerQos.setSupportsOnChangeSubscriptions(true);
```

### The base class
The provider class must extend the generated class ```joynr::<Package>::Default<Interface>Provider```  and implement getter and setter methods for each Franca attribute and a method for each method of the Franca interface. In order to send broadcasts the generated code of the super class ```joynr::<Interface>Provider``` can be used.

```cpp
#include "My<Interface>Provider.h"

using namespace joynr;

My<Interface>Provider::My<Interface>Provider()
    : Default<Interface>Provider()
{
    // call setters to configure inherited providerQos
    providerQos.setPriority(<priorityValue>);
    ...
}

My<Interface>Provider::~My<Interface>Provider()
{
}
...
// foreach Franca interface "<Attribute>" provide a getter method
// foreach Franca interface "<Attribute>" provide a setter method
// foreach Franca method provide an implementation
...
}
```

### Providing attribute getters and setters
The getter methods return the current value of an attribute. Since the current thread is blocked while the getter runs, activity should be kept as short as possible.

```cpp
// for any Franca type named "<Type>" used
#include "joynr/<Package>/<TypeCollection>/<Type>.h"
...
void My<Interface>Provider::get<Attribute>(
    std::function<void(const <AttributeType>&)> onSuccess,
    std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
)
{
    onSuccess(<AttributeValue>);
}

void My<Interface>Provider::set<Attribute>(
    const <AttributeType>& <Attribute>,
    std::function<void()> onSuccess,
    std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
)
{
    // handle and store the new Value
    ...
    // if attribute is notifiable (not marked as noSubscriptions in the Franca model),
    // inform subscribers about the value change
    <Attribute>Changed(<Attribute>);
    ...
    onSuccess();
}
```

### Implementing a Franca RPC method
The provider should always implement RPC calls asynchronously in order to not block the main thread longer than required. Also it needs to take care not to overload the server, e.g. it must not accept unlimited amount of RPC requests causing background activity. After exceeding a limit, further calls should be rejected until the number of outstanding activities falls below the limit again.

```cpp
// for any Franca type named "<Type>" used
#include "joynr/<Package>/<TypeCollection>/<Type>.h"
...
void My<Interface>Provider::<method>(
    ... input parameters ...   // optional
    std::function<void(
        const <ReturnType1>& returnValue1
        ...
        const <ReturnTypeN>& returnValueN
    )> onSuccess,
    std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
)
{
    // handle request
    ...
    onSuccess(returnValue1, ..., returnValueN);
}
```
If errors are modelled in Franca for the method, the onError function is replaced by:
```cpp
std::function<void (const joynr::<Package>::<Interface>::<Error>::Enum& errorEnum)> onError
```

### Firing a broadcast
Firing a broadcast blocks the current thread until the message is serialized.

```cpp
// for any Franca type named "<Type>" used
#include "joynr/<Package>/<TypeCollection>/<Type>.h"
...
void My<Interface>Provider::fire<Broadcast>Event()
{
    <OutputValueType1> outputValue1;
    ...
    <OutputValueTypeN> outputValueN;
    ...
    // setup outputValue(s)
    ...
    // use method provided by generators to send the broadcast
    fire<Broadcast>(outputValue1, ... , outputValueN);
}
```

## Selective (filtered) broadcasts
In contrast to unfiltered broadcasts, to realize selective (filtered) broadcasts, the filter logic has to be implemented and registered by the provider. If multiple filters are registered on the same provider and broadcast, all filters are applied in a chain and the broadcast is only delivered if all filters in the chain return true.
### The broadcast filter classes

A broadcast filter class implements a filtering function called ```filter()``` which returns a boolean value indicating whether the broadcast should be delivered. The input parameters of the ```filter()``` method reflect the output values of the broadcast.

```cpp
#include "joynr/<Package>/<Interface><Broadcast>BroadcastFilter.h"
// for any Franca type named "<Type>" used
#include "joynr/<Package>/<TypeCollection>/<Type>.h"
...
class <Filter>Filter: public <Package>::<Interface><Broadcast>BroadcastFilter
{
    public:
        <Broadcast>Filter();

        virtual bool filter(
            const joynr::<Package>::<OutputValueType1>& outputValue1,
            ...
            const joynr::<Package>::<OutputValueTypeN>& outputValueN,
            const joynr::<Package>::<Interface><Broadcast>BroadcastFilterParameters& filterParameters
        );
}
```

## The include file for the My&lt;Interface&gt;Provider class

The include file for the provider class contains
* prototypes for **getters** (and optionally **setters**) for each Franca attribute
* prototypes for each Franca **method**
* prototypes for each Franca **broadcast**
* **constructor** and **destructor**

```cpp
#ifndef MY_<INTERFACE>_PROVIDER_H
#define MY_<INTERFACE>_PROVIDER_H

#include "joynr/<Package>/Default<Interface>Provider.h"

class My<Interface>Provider : public joynr::<Package>::Default<Interface>Provider
{
public:
    My<Interface>Provider();
    ~My<Interface>Provider();

    // for each attribute
    void get<Attribute>(
        std::function<void(const <AttributeType>& result)> onSuccess,
        std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError);

    // for each attribute which are NOT readonly
    void set<Attribute>(
        const <AttributeType>& <attribute>,
        std::function<void()> onSuccess,
        std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError);

    // for each method
    void <method>(
        ... input parameters ...   // optional
        std::function<void(
            const <ReturnType1>& returnValue1
            ...
            const <ReturnTypeN>& returnValueN
        )> onSuccess,
        std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    );

    // for each broadcast
    void fire<Broadcast>Event();

private:
    My<Interface>Provider(const My<Interface>Provider&);
    void operator=(const My<Interface>Provider&);
}
#endif
```
For methods which are modelled with error enumerations, the onError function is replaced by:
```cpp
std::function<void (const joynr::<Package>::<Interface>::<Error>::Enum& errorEnum)> onError
```
