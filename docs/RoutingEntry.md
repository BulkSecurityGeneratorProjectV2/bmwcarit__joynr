Routing Entry: Objectives and life cycle
======

## Overview
A Routing Entry collects joynr routing information without making a distinction
between providers and proxies. More specifically it is composed of:
 * **participantId**
   The unique identifier of a participant (provider or proxy) at joynr level.
 * **address**
   Defines how a participant (provider or proxy) is reachable from this runtime.
 * **isGloballyVisible**
   Whether the participantId is globally visible or not.
   *This information is required for the routing of multicast publications and for the handling
   subscriptions to multicasts. The details are out of the scope of this document.*
   * a provider is globally visible when it is registered globally
   * a proxy is globally visible when
     * it is connected to a global provider
     * it has discovered a local provider globally (DiscoveryScope: `GLOBAL`, depending on the
       arbitration strategy, this might also happen with DiscoveryScope `LOCAL_AND_GLOBAL` and
       `LOCAL_THEN_GLOBAL`)
 * **expiryDateMs**
   The expiryDate associated with it - once this date has passed, the entry can be garbage
   collected.
 * **isSticky**
   Defines whether this entry is protected against garbage collection and overwriting. Only
   provisioned entries, especially joynr internal providers, are sticky.

The first three elements are routing specific information, the others are only relevant for
bookkeeping operations.

## Address types
An address may be of type
 * **WebSocketClientAddress**  
   the participantId is reachable from this runtime (cluster controller) using a connection to the
   specified websocket client (libjoynr runtime)
 * **WebSocketAddress**  
   the participantId is reachable from this (libjoynr) runtime using a connection to the specified
   websocket server (cluster controller)
 * **MqttAddress**/**ChannelAddress**  
   the participantId is reachable from this runtime (cluster controller) using a connection to the
   specified MQTT broker/Http Bounceproxy
 * **InProcessAddress**  
   the participantId is in the same runtime
 * **BrowserAddress**  
   NOT USED

## Operational constraints
1. Each participantId is unique
2. Each participantId has only one RoutingEntry at a time
3. Each RoutingEntry contains exactly one address
4. If no Routing Entry for a participantId exists, it is always allowed to create one
5. Sticky Routing Entries (provisioned routing entries) cannot be replaced or removed
6. If a Routing Entry for a participantId exists, it can be replaced only if (see also
   [section Precedence of address types](#precedence-of-address-types))
   1. the existing entry and the new entry contain the same address type **OR**
   2. the new entry is an InProcess entry **OR**
   3. the existing entry is a remote entry and the new one is also a remote entry **OR**
   *  libjoynr runtime:
      1. the existing entry is a WebSocket, WebSocketClient or remote entry (MqttAddress,
         ChannelAddress) and the new one is a WebSocket entry **OR**
      2. the existing entry is a remote entry and the new one is WebSocketClient entry
   *  cluster controller runtime:
      1. the existing entry is a WebSocket, or remote entry (MqttAddress, ChannelAddress) and the
         new one is a WebSocketClient entry **OR**
      2. the existing entry is a WebSocket entry and the new one is a remote entry
7. If a RoutingEntry is replaced, the expiryDateMs attribute and the isSticky flag are merged
8. If a Routing Entry for a participant exists and a new entry contains the same address and
   isGloballyVisible flag, only the expiryDateMs attribute and the isSticky flag are merged
9. If the expiryDateMs attribute and isSticky flag of two routing entries are merged,
   * the longest expiryDate from existing and new entry is kept.
   * the isSticky flag is set to true, if it is true in either old or new entry.

## Precedence of address types
**General rule:** `InProcessAddress > WebSocketClientAddress > MqttAddress/ChannelAddress`
(local address types have precedence over global address types) AND every address type can
be overwritten by a new address of the same type:
* **InProcessAddress** has precedence over all other address types:  
  Participants in the same runtime are most trustworthy and the routing information of those
  inprocess participants, especially joynr internal providers like routing or discovery provider
  in the cluster controller, must not be overwritten by any other address from outside.  
  This also allows the restart of a libjoynr runtime with consumer/proxy and provider in the same
  runtime (after a crash), even if the proxy is built before the provider is registered, see section
  [Possible scenarios](#possible-scenarios) below. In this case, the consumer discovers the old
  DiscoveryEntry of the provider and adds a routing entry for the in process provider with the
  cluster controller's WebSocketAddress. When the provider registers again, the WebSocketAddress
  in the libjoynr runtime is overwritten by the correct InProcessAddress of the provider and the
  routing works as expected.
* **WebSocketClientAddress** has precedence over global address types:  
  A WebSocketClientAddress addresses local (in terms of a cluster controller) clients. It must not
  be overwritten by a global address which could cause a loop, see the following sections.
* **MqttAddress** and **ChannelAddress** (global address types) have no precedence over other
  address types
* **Special case: WebSocketAddress**
  * In **libjoynr runtimes**, it has precedence over global addresses (MqttAddress, ChannelAddress)
    and WebSocketClientAddress:  
    `InProcessAddress > WebSocketAddress > WebSocketClientAddress > MqttAddress/ChannelAddress`  
    A libjoynr runtime has no direct connection to global participants. Messages for global
    participants are routed through the local cluster controller which is addressed by its
    WebSocketAddress. Global addresses and WebSocketClient addresses are not used in libjoynr
    runtimes, a libjoynr runtime has no global connection or clients.
  * In **cluster controller runtimes**, a WebSocketAddress must not overwrite other address types:  
    `InProcessAddress > WebSocketClientAddress > MqttAddress/ChannelAddress > WebSocketAddress`  
    In the cluster controller, a WebSocketAddress which addresses the cluster controller itself
    or even another cluster controller makes no sense becaue cluster controllers cannot be connected
    directly.

## Possible scenarios
1. **An inprocess provider (in a cluster controller runtime) registers locally**
   1. On first time registration, no RoutingEntry entry exists and a new InProcess entry is created.
   2. On provider (including its cluster controller runtime) restart if re-using its former
      participantId (default) an existing RoutingEntry gets replaced.
      1. A WebSocketClient entry could already exist if a local provider in a (websocket) libjoynr
         runtime has registered itself with a participantId of a joynr internal provider of the
         clustercontroller, e.g. RoutingProvider, (local) DiscoveryProvider or any custom provider
         in a cluster controller runtime.
2. **An inprocess provider (in a cluster controller runtime) registers globally**
   1. On first time registration, no RoutingEntry entry exists and a new InProcess entry is created.
   2. On provider (including its cluster controller runtime) restart if re-using its former
      participantId (default) an existing RoutingEntry gets replaced.
      1. see 1.ii.a.
      2. A global RoutingEntry (with global address) could already exist in case during local (in the
         same runtime or in a connected libjoynr runtime) proxy creation a still existing older
         Global Discovery Entry got discovered before the provider could re-register itself.
3. **A websocket provider (in a libjoynr runtime connected to a cluster controller) registers
   locally**
   1. On first time registration, no RoutingEntry entry exists and new ones are created:  
      An InProcess entry in the provider's runtime and a WebSocketClient entry in the cluster
      controller
   2. On provider restart if re-using its former participantId (default) any existing RoutingEntry
      gets replaced if it is not an InProcess entry (see above).
      1. A WebSocketClient RoutingEntry could already exist in the cluster controller in case the
         provider had just crashed without unregistering itself.
      2. A WebSocket RoutingEntry could already exist in the provider's libjoynr runtime if a local
         proxy for this provider is created in the same runtime before the provider has
         (re-)registered itself after the restart, i.e. the provider gets discovered via its old
         DiscoveryEntry from the previous run.
4. **A websocket provider (in a libjoynr runtime) registers globally**
   1. On first time registration, no RoutingEntry entry exists and new ones are created:  
      An InProcess entry in the provider's runtime and a WebSocketClient entry in the cluster
      controller
   2. On provider restart if re-using its former participantId (default) any existing RoutingEntry
      gets replaced if it is not an InProcess entry (see above).
      1. see 3.ii.a.
      2. see 3.ii.b.
      3. If the cluster controller had also been restarted, a global RoutingEntry (with global
         address) could already exist in the cluster controller in case during proxy creation (in the
         provider's runtime or any other runtime connected to the same cluster controller) a still
         existing older GlobalDiscoveryEntry got discovered before the provider could re-register
         itself.
3. **A provider unregisters**
   The RoutingEntry gets removed.
4. **A proxy is built for a local provider (a local provider is discovered)**
   No Routing entry exists for the proxy and the new one is created (a new proxy always gets a new
   participantId, so that a conflict is not possible)
5. **A proxy is build for a global provider (a global provider is discovered)**
   * No Routing entry exists for the proxy and new ones are created (a new proxy always gets a new
     participantId, so that a conflict is not possible):  
     An InProcess entry in the proxy's runtime and a WebSocketClient entry in the cluster
     controller if the proxy proxy's runtime is not a cluster controller runtime.
   * The global proxy creation also causes a GCD lookup and Routing Entries are getting generated in
     the cluster controller for the providers returned by the lookup request.  
     This can happen also for local providers in case a proxy is created with DiscoveryScope that can
     cause a GCD lookup (i.e. GLOBAL_ONLY, LOCAL_AND_GLOBAL, LOCAL_THEN_GLOBAL).
     1. If no RoutingEntry exists for a given provider participantId, it is created.
     2. If a RoutingEntry **with global address** already exists for a given provider participantId,
        it is getting replaced.
     3. If a RoutingEntry **with local address** already exists for a given provider participantId,
      **it is being kept**.
6. **A global proxy is added (from the replyTo address of an incoming request)**
   1. On first time request, no RoutingEntry exists and the new one is created. Furthermore the
      expiryDate is set according to the expiryDate of the request message.
   2. On any further request, the expiryDate of the existing RoutingEntry gets extended, if
      applicable.  
      This allows to garbage collect Routing Entries of no longer used global proxies.
   3. If a further request contains a different replyTo address, the existing routing entry is
      replaced with a new (global) routing entry with the new address.
7. **A RoutingEntry expires**
   The Routing Table cleanup job periodically checks for expired Routing Entries and removes them
   (if they are not sticky).
