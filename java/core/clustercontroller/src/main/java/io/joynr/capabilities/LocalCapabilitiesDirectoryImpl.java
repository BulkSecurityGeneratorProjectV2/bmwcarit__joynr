package io.joynr.capabilities;

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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import javax.annotation.CheckForNull;
import javax.annotation.Nonnull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.base.Function;
import com.google.common.collect.Collections2;
import com.google.common.collect.Lists;
import com.google.common.collect.Sets;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.TransportReadyListener;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.runtime.GlobalAddressProvider;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderScope;

@Singleton
public class LocalCapabilitiesDirectoryImpl extends AbstractLocalCapabilitiesDirectory implements
TransportReadyListener {

    private static final Logger logger = LoggerFactory.getLogger(LocalCapabilitiesDirectoryImpl.class);

    private static final Set<DiscoveryScope> INCLUDE_LOCAL_SCOPES = new HashSet<>();
    static {
        INCLUDE_LOCAL_SCOPES.add(DiscoveryScope.LOCAL_ONLY);
        INCLUDE_LOCAL_SCOPES.add(DiscoveryScope.LOCAL_AND_GLOBAL);
        INCLUDE_LOCAL_SCOPES.add(DiscoveryScope.LOCAL_THEN_GLOBAL);
    }

    private static final Set<DiscoveryScope> INCLUDE_GLOBAL_SCOPES = new HashSet<>();
    static {
        INCLUDE_GLOBAL_SCOPES.add(DiscoveryScope.GLOBAL_ONLY);
        INCLUDE_GLOBAL_SCOPES.add(DiscoveryScope.LOCAL_AND_GLOBAL);
        INCLUDE_GLOBAL_SCOPES.add(DiscoveryScope.LOCAL_THEN_GLOBAL);
    }

    private DiscoveryEntryStore localDiscoveryEntryStore;
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesDirectoryClient;
    private DiscoveryEntryStore globalDiscoveryEntryCache;
    private static final long DEFAULT_DISCOVERYTIMEOUT = 30000;

    private MessageRouter messageRouter;

    private GlobalAddressProvider globalAddressProvider;

    private Address globalAddress;
    private Object globalAddressLock = new Object();

    private List<QueuedDiscoveryEntry> queuedDiscoveryEntries = new ArrayList<QueuedDiscoveryEntry>();

    static class QueuedDiscoveryEntry {
        private DiscoveryEntry discoveryEntry;
        private DeferredVoid deferred;

        public QueuedDiscoveryEntry(DiscoveryEntry discoveryEntry, DeferredVoid deferred) {
            this.discoveryEntry = discoveryEntry;
            this.deferred = deferred;
        }

        public DiscoveryEntry getDiscoveryEntry() {
            return discoveryEntry;
        }

        public DeferredVoid getDeferred() {
            return deferred;
        }
    }

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public LocalCapabilitiesDirectoryImpl(CapabilitiesProvisioning capabilitiesProvisioning,
                                          GlobalAddressProvider globalAddressProvider,
                                          DiscoveryEntryStore localDiscoveryEntryStore,
                                          DiscoveryEntryStore globalDiscoveryEntryCache,
                                          MessageRouter messageRouter,
                                          GlobalCapabilitiesDirectoryClient globalCapabilitiesDirectoryClient,
                                          ExpiredDiscoveryEntryCacheCleaner expiredDiscoveryEntryCacheCleaner) {
        this.globalAddressProvider = globalAddressProvider;
        // CHECKSTYLE:ON
        this.messageRouter = messageRouter;
        this.localDiscoveryEntryStore = localDiscoveryEntryStore;
        this.globalDiscoveryEntryCache = globalDiscoveryEntryCache;
        this.globalCapabilitiesDirectoryClient = globalCapabilitiesDirectoryClient;
        this.globalDiscoveryEntryCache.add(capabilitiesProvisioning.getDiscoveryEntries());
        expiredDiscoveryEntryCacheCleaner.scheduleCleanUpForCaches(
            new ExpiredDiscoveryEntryCacheCleaner.CleanupAction() {
                @Override
                public void cleanup(Set<DiscoveryEntry> expiredDiscoveryEntries) {
                    for (DiscoveryEntry discoveryEntry : expiredDiscoveryEntries) {
                        remove(discoveryEntry);
                    }
                }
            }, globalDiscoveryEntryCache, localDiscoveryEntryStore);
    }

    /**
     * Adds local capability to local and (depending on SCOPE) the global directory
     */
    @Override
    public Promise<DeferredVoid> add(final DiscoveryEntry discoveryEntry) {
        final DeferredVoid deferred = new DeferredVoid();

        if (localDiscoveryEntryStore.hasDiscoveryEntry(discoveryEntry)) {
            DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                         ArbitrationStrategy.HighestPriority,
                                                         DiscoveryQos.NO_MAX_AGE,
                                                         DiscoveryScope.LOCAL_AND_GLOBAL);
            if (discoveryEntry.getQos().getScope().equals(ProviderScope.LOCAL)
                    || globalDiscoveryEntryCache.lookup(discoveryEntry.getParticipantId(), discoveryQos.getCacheMaxAgeMs()) != null) {
                // in this case, no further need for global registration is required. Registration completed.
                deferred.resolve();
                return new Promise<>(deferred);
            }
            // in the other case, the global registration needs to be done
        } else {
            localDiscoveryEntryStore.add(discoveryEntry);
            notifyCapabilityAdded(discoveryEntry);
        }

        if (discoveryEntry.getQos().getScope().equals(ProviderScope.GLOBAL)) {
            registerGlobal(discoveryEntry, deferred);
        } else {
            deferred.resolve();
        }
        return new Promise<>(deferred);
    }

    private void registerGlobal(final DiscoveryEntry discoveryEntry, final DeferredVoid deferred) {
        synchronized (globalAddressLock) {
            try {
                globalAddress = globalAddressProvider.get();
            } catch (Exception e) {
                logger.debug("error getting global address", e);
                globalAddress = null;
            }

            if (globalAddress == null) {
                queuedDiscoveryEntries.add(new QueuedDiscoveryEntry(discoveryEntry, deferred));
                globalAddressProvider.registerGlobalAddressesReadyListener(this);
                return;
            }
        }

        final GlobalDiscoveryEntry globalDiscoveryEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry,
                                                                                                              globalAddress);
        if (globalDiscoveryEntry != null) {

            logger.info("starting global registration for " + globalDiscoveryEntry.getDomain() + " : "
                    + globalDiscoveryEntry.getInterfaceName());

            globalCapabilitiesDirectoryClient.add(new Callback<Void>() {

                @Override
                public void onSuccess(Void nothing) {
                    logger.info("global registration for " + globalDiscoveryEntry.getDomain() + " : "
                            + globalDiscoveryEntry.getInterfaceName() + " completed");
                    deferred.resolve();
                    globalDiscoveryEntryCache.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry,
                                                                                                      globalAddress));
                }

                @Override
                public void onFailure(JoynrRuntimeException exception) {
                    deferred.reject(new ProviderRuntimeException(exception.toString()));
                }
            }, globalDiscoveryEntry);
        }
    }

    @Override
    public void remove(final DiscoveryEntry discoveryEntry) {
        localDiscoveryEntryStore.remove(discoveryEntry.getParticipantId());
        notifyCapabilityRemoved(discoveryEntry);

        // Remove from the global capabilities directory if needed
        if (discoveryEntry.getQos().getScope() != ProviderScope.LOCAL) {

            Callback<Void> callback = new Callback<Void>() {

                @Override
                public void onSuccess(Void result) {
                    globalDiscoveryEntryCache.remove(discoveryEntry.getParticipantId());
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    //do nothing
                }
            };
            globalCapabilitiesDirectoryClient.remove(callback, Arrays.asList(discoveryEntry.getParticipantId()));
        }

        // Remove endpoint addresses
        messageRouter.removeNextHop(discoveryEntry.getParticipantId());
    }

    @Override
    public void lookup(final String[] domains,
                       final String interfaceName,
                       final DiscoveryQos discoveryQos,
                       final CapabilitiesCallback capabilitiesCallback) {
        DiscoveryScope discoveryScope = discoveryQos.getDiscoveryScope();
        Set<DiscoveryEntry> localDiscoveryEntries = getLocalEntriesIfRequired(discoveryScope, domains, interfaceName);
        Set<DiscoveryEntry> globalDiscoveryEntries = getGloballyCachedEntriesIfRequired(discoveryScope, domains, interfaceName, discoveryQos.getCacheMaxAgeMs());
        switch (discoveryScope) {
        case LOCAL_ONLY:
            capabilitiesCallback.processCapabilitiesReceived(localDiscoveryEntries);
            break;
        case LOCAL_THEN_GLOBAL:
            handleLocalThenGlobal(domains, interfaceName, discoveryQos, capabilitiesCallback, localDiscoveryEntries, globalDiscoveryEntries);
            break;
        case GLOBAL_ONLY:
            handleGlobalOnly(domains, interfaceName, discoveryQos, capabilitiesCallback, globalDiscoveryEntries);
            break;
        case LOCAL_AND_GLOBAL:
            handleLocalAndGlobal(domains, interfaceName, discoveryQos, capabilitiesCallback, localDiscoveryEntries, globalDiscoveryEntries);
            break;
        default:
            throw new IllegalStateException("Unknown or illegal DiscoveryScope value: " + discoveryScope);
        }
    }

    private void handleLocalThenGlobal(String[] domains,
                                       String interfaceName,
                                       DiscoveryQos discoveryQos,
                                       CapabilitiesCallback capabilitiesCallback,
                                       Set<DiscoveryEntry> localDiscoveryEntries,
                                       Set<DiscoveryEntry> globalDiscoveryEntries) {
        Set<String> domainsForGlobalLookup = new HashSet<>();
        Set<DiscoveryEntry> matchedDiscoveryEntries = new HashSet<>();
        for (String domainToMatch : domains) {
            boolean domainMatched = addEntriesForDomain(localDiscoveryEntries, matchedDiscoveryEntries, domainToMatch);
            domainMatched = domainMatched || addEntriesForDomain(globalDiscoveryEntries, matchedDiscoveryEntries, domainToMatch);
            if (!domainMatched) {
                domainsForGlobalLookup.add(domainToMatch);
            }
        }
        handleMissingGlobalEntries(interfaceName,
                                   discoveryQos,
                                   capabilitiesCallback,
                                   domainsForGlobalLookup,
                                   matchedDiscoveryEntries);
    }

    private void handleLocalAndGlobal(String[] domains,
                                      String interfaceName,
                                      DiscoveryQos discoveryQos,
                                      CapabilitiesCallback capabilitiesCallback,
                                      Set<DiscoveryEntry> localDiscoveryEntries,
                                      Set<DiscoveryEntry> globalDiscoveryEntries) {
        Set<String> domainsForGlobalLookup = new HashSet<>();
        Set<DiscoveryEntry> matchedDiscoveryEntries = new HashSet<>();
        for (String domainToMatch : domains) {
            addEntriesForDomain(localDiscoveryEntries, matchedDiscoveryEntries, domainToMatch);
            if (!addEntriesForDomain(globalDiscoveryEntries, matchedDiscoveryEntries, domainToMatch)) {
                domainsForGlobalLookup.add(domainToMatch);
            }
        }
        handleMissingGlobalEntries(interfaceName,
                                   discoveryQos,
                                   capabilitiesCallback,
                                   domainsForGlobalLookup,
                                   matchedDiscoveryEntries);
    }

    private void handleGlobalOnly(String[] domains,
                                  String interfaceName,
                                  DiscoveryQos discoveryQos,
                                  CapabilitiesCallback capabilitiesCallback,
                                  Set<DiscoveryEntry> globalDiscoveryEntries) {
        Set<String> domainsForGlobalLookup = Sets.newHashSet(domains);
        for (DiscoveryEntry discoveryEntry : globalDiscoveryEntries) {
            domainsForGlobalLookup.remove(discoveryEntry.getDomain());
        }
        handleMissingGlobalEntries(interfaceName,
                                   discoveryQos,
                                   capabilitiesCallback,
                                   domainsForGlobalLookup,
                                   globalDiscoveryEntries);
    }

    private void handleMissingGlobalEntries(String interfaceName,
                                            DiscoveryQos discoveryQos,
                                            CapabilitiesCallback capabilitiesCallback,
                                            Set<String> domainsForGlobalLookup,
                                            Set<DiscoveryEntry> matchedDiscoveryEntries) {
        if (domainsForGlobalLookup.isEmpty()) {
            capabilitiesCallback.processCapabilitiesReceived(matchedDiscoveryEntries);
        } else {
            asyncGetGlobalCapabilitities(domainsForGlobalLookup.toArray(new String[domainsForGlobalLookup.size()]),
                                         interfaceName,
                                         matchedDiscoveryEntries,
                                         discoveryQos.getDiscoveryTimeoutMs(),
                                         capabilitiesCallback);
        }
    }

    private boolean addEntriesForDomain(Collection<DiscoveryEntry> discoveryEntries, Collection<DiscoveryEntry> addTo, String domain) {
        boolean domainMatched = false;
        for (DiscoveryEntry discoveryEntry : discoveryEntries) {
            if (discoveryEntry.getDomain().equals(domain)) {
                addTo.add(discoveryEntry);
                domainMatched = true;
            }
        }
        return domainMatched;
    }

    private Set<DiscoveryEntry> getGloballyCachedEntriesIfRequired(DiscoveryScope discoveryScope,
                                                                   String[] domains,
                                                                   String interfaceName, long cacheMaxAge) {
        if (INCLUDE_GLOBAL_SCOPES.contains(discoveryScope)) {
            return new HashSet<DiscoveryEntry>(globalDiscoveryEntryCache.lookup(domains, interfaceName, cacheMaxAge));
        }
        return null;
    }

    private Set<DiscoveryEntry> getLocalEntriesIfRequired(DiscoveryScope discoveryScope, String[] domains, String interfaceName) {
        if (INCLUDE_LOCAL_SCOPES.contains(discoveryScope)) {
            return new HashSet<DiscoveryEntry>(localDiscoveryEntryStore.lookup(domains, interfaceName));
        }
        return null;
    }

    @Override
    @CheckForNull
    public void lookup(final String participantId,
                       final DiscoveryQos discoveryQos,
                       final CapabilityCallback capabilityCallback) {

        final DiscoveryEntry localDiscoveryEntry = localDiscoveryEntryStore.lookup(participantId,
                                                                                   discoveryQos.getCacheMaxAgeMs());

        DiscoveryScope discoveryScope = discoveryQos.getDiscoveryScope();
        switch (discoveryScope) {
        case LOCAL_ONLY:
            capabilityCallback.processCapabilityReceived(localDiscoveryEntry);
            break;
        case LOCAL_THEN_GLOBAL:
        case LOCAL_AND_GLOBAL:
            if (localDiscoveryEntry != null) {
                capabilityCallback.processCapabilityReceived(localDiscoveryEntry);
            } else {
                asyncGetGlobalCapabilitity(participantId, discoveryQos, capabilityCallback);
            }
            break;
        case GLOBAL_ONLY:
            asyncGetGlobalCapabilitity(participantId, discoveryQos, capabilityCallback);
            break;
        default:
            break;
        }
    }

    @Override
    @CheckForNull
    public DiscoveryEntry lookup(String participantId, DiscoveryQos discoveryQos) {
        final Future<DiscoveryEntry> lookupFuture = new Future<>();
        lookup(participantId, discoveryQos, new CapabilityCallback() {

            @Override
            public void processCapabilityReceived(DiscoveryEntry capability) {
                lookupFuture.onSuccess(capability);
            }

            @Override
            public void onError(Throwable e) {
                lookupFuture.onFailure(new JoynrRuntimeException(e));
            }
        });
        DiscoveryEntry retrievedCapabilitiyEntry = null;

        try {
            retrievedCapabilitiyEntry = lookupFuture.get();
        } catch (InterruptedException e1) {
            logger.error("interrupted while retrieving capability entry by participant ID", e1);
        } catch (ApplicationException e1) {
            // should not be reachable since ApplicationExceptions are not used internally
            logger.error("ApplicationException while retrieving capability entry by participant ID", e1);
        }
        return retrievedCapabilitiyEntry;
    }

    private void registerIncomingEndpoints(Collection<GlobalDiscoveryEntry> caps) {
        for (GlobalDiscoveryEntry ce : caps) {
            // TODO when are entries purged from the messagingEndpointDirectory?
            if (ce.getParticipantId() != null && ce.getAddress() != null) {
                Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(ce);
                messageRouter.addNextHop(ce.getParticipantId(), address);
            }
        }
    }

    private void asyncGetGlobalCapabilitity(final String participantId,
                                            DiscoveryQos discoveryQos,
                                            final CapabilityCallback capabilitiesCallback) {

        DiscoveryEntry cachedGlobalCapability = globalDiscoveryEntryCache.lookup(participantId,
                                                                                 discoveryQos.getCacheMaxAgeMs());

        if (cachedGlobalCapability != null) {
            capabilitiesCallback.processCapabilityReceived(cachedGlobalCapability);
        } else {
            globalCapabilitiesDirectoryClient.lookup(new Callback<GlobalDiscoveryEntry>() {

                @Override
                public void onSuccess(@CheckForNull GlobalDiscoveryEntry newGlobalDiscoveryEntry) {
                    if (newGlobalDiscoveryEntry != null) {
                        registerIncomingEndpoints(Lists.newArrayList(newGlobalDiscoveryEntry));
                        globalDiscoveryEntryCache.add(newGlobalDiscoveryEntry);
                        capabilitiesCallback.processCapabilityReceived(newGlobalDiscoveryEntry);
                    } else {
                        capabilitiesCallback.onError(new NullPointerException("Received capabilities are null"));
                    }
                }

                @Override
                public void onFailure(JoynrRuntimeException exception) {
                    capabilitiesCallback.onError(exception);

                }
            }, participantId, discoveryQos.getDiscoveryTimeoutMs());
        }

    }

    /**
     * mixes in the localDiscoveryEntries to global capabilities found by participantId
     */
    private void asyncGetGlobalCapabilitities(final String[] domains,
                                              final String interfaceName,
                                              Collection<DiscoveryEntry> localDiscoveryEntries2,
                                              long discoveryTimeout,
                                              final CapabilitiesCallback capabilitiesCallback) {

        final Collection<DiscoveryEntry> localDiscoveryEntries = localDiscoveryEntries2 == null ? new LinkedList<DiscoveryEntry>()
                : localDiscoveryEntries2;

        globalCapabilitiesDirectoryClient.lookup(new Callback<List<GlobalDiscoveryEntry>>() {

            @Override
            public void onSuccess(List<GlobalDiscoveryEntry> globalDiscoverEntries) {
                if (globalDiscoverEntries != null) {
                    registerIncomingEndpoints(globalDiscoverEntries);
                    globalDiscoveryEntryCache.add(globalDiscoverEntries);
                    Collection<DiscoveryEntry> allDisoveryEntries = new ArrayList<DiscoveryEntry>(globalDiscoverEntries.size()
                            + localDiscoveryEntries.size());
                    allDisoveryEntries.addAll(globalDiscoverEntries);
                    allDisoveryEntries.addAll(localDiscoveryEntries);
                    capabilitiesCallback.processCapabilitiesReceived(allDisoveryEntries);
                } else {
                    capabilitiesCallback.onError(new NullPointerException("Received capabilities are null"));
                }
            }

            @Override
            public void onFailure(JoynrRuntimeException exception) {
                capabilitiesCallback.onError(exception);
            }
        },
                                                 domains,
                                                 interfaceName,
                                                 discoveryTimeout);
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        if (unregisterAllRegisteredCapabilities) {
            Set<DiscoveryEntry> allDiscoveryEntries = localDiscoveryEntryStore.getAllDiscoveryEntries();

            List<DiscoveryEntry> discoveryEntries = new ArrayList<>(allDiscoveryEntries.size());

            for (DiscoveryEntry capabilityEntry : allDiscoveryEntries) {
                if (capabilityEntry.getQos().getScope() == ProviderScope.GLOBAL) {
                    discoveryEntries.add(capabilityEntry);
                }
            }

            if (discoveryEntries.size() > 0) {
                try {
                    Function<? super DiscoveryEntry, String> transfomerFct = new Function<DiscoveryEntry, String>() {

                        @Override
                        public String apply(DiscoveryEntry input) {
                            return input != null ? input.getParticipantId() : null;
                        }
                    };
                    Callback<Void> callback = new Callback<Void>() {

                        @Override
                        public void onFailure(JoynrRuntimeException error) {
                        }

                        @Override
                        public void onSuccess(Void result) {
                        }

                    };
                    globalCapabilitiesDirectoryClient.remove(callback, Lists.newArrayList(Collections2.transform(discoveryEntries,
                                                                                                                 transfomerFct)));
                } catch (DiscoveryException e) {
                    logger.debug("error removing discovery entries", e);
                }
            }
        }
    }

    @Override
    public Promise<Lookup1Deferred> lookup(String[] domains,
                                           String interfaceName,
                                           joynr.types.DiscoveryQos discoveryQos) {
        final Lookup1Deferred deferred = new Lookup1Deferred();
        CapabilitiesCallback callback = new CapabilitiesCallback() {
            @Override
            public void processCapabilitiesReceived(@CheckForNull Collection<DiscoveryEntry> capabilities) {
                if (capabilities == null) {
                    deferred.reject(new ProviderRuntimeException("Received capablities collection was null"));
                } else {
                    deferred.resolve(capabilities.toArray(new DiscoveryEntry[capabilities.size()]));
                }
            }

            @Override
            public void onError(Throwable e) {
                deferred.reject(new ProviderRuntimeException(e.toString()));
            }
        };
        DiscoveryScope discoveryScope = DiscoveryScope.valueOf(discoveryQos.getDiscoveryScope().name());
        lookup(domains, interfaceName, new DiscoveryQos(DEFAULT_DISCOVERYTIMEOUT,
                                                        ArbitrationStrategy.NotSet,
                                                        discoveryQos.getCacheMaxAge(),
                                                        discoveryScope), callback);

        return new Promise<>(deferred);
    }

    @Override
    public Promise<Lookup2Deferred> lookup(String participantId) {
        Lookup2Deferred deferred = new Lookup2Deferred();
        DiscoveryEntry discoveryEntry = lookup(participantId, DiscoveryQos.NO_FILTER);
        deferred.resolve(discoveryEntry);
        return new Promise<>(deferred);
    }

    @Override
    public io.joynr.provider.Promise<io.joynr.provider.DeferredVoid> remove(String participantId) {
        DeferredVoid deferred = new DeferredVoid();
        DiscoveryEntry entryToRemove = localDiscoveryEntryStore.lookup(participantId, Long.MAX_VALUE);
        if (entryToRemove != null) {
            remove(entryToRemove);
            deferred.resolve();
        } else {
            deferred.reject(new ProviderRuntimeException("Failed to remove participantId: " + participantId));
        }
        return new Promise<>(deferred);
    }

    @Override
    public Set<DiscoveryEntry> listLocalCapabilities() {
        return localDiscoveryEntryStore.getAllDiscoveryEntries();
    }

    @Override
    public void transportReady(@Nonnull Address address) {
        synchronized (globalAddressLock) {
            globalAddress = address;
        }
        for (QueuedDiscoveryEntry queuedDiscoveryEntry : queuedDiscoveryEntries) {
            registerGlobal(queuedDiscoveryEntry.getDiscoveryEntry(), queuedDiscoveryEntry.getDeferred());
        }
    }
}
