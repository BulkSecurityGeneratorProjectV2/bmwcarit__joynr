package io.joynr.capabilities;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;

import javax.annotation.CheckForNull;
import javax.annotation.Nonnull;
import javax.persistence.EntityManager;
import javax.persistence.EntityTransaction;
import javax.transaction.Transactional;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.Singleton;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import com.google.inject.persist.PersistService;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrCommunicationException;
import joynr.types.DiscoveryEntry;

/**
 * The CapabilitiesStore stores a list of provider channelIds and the interfaces
 * they offer.
 * Capability informations are stored in a concurrentHashMap. Using a in memory
 * database could be possible optimization.
 */
@Singleton
public class DiscoveryEntryStorePersisted implements DiscoveryEntryStore {

    private static final Logger logger = LoggerFactory.getLogger(DiscoveryEntryStorePersisted.class);
    protected Provider<EntityManager> entityManagerProvider;

    // Do not sychronize on a Boolean
    // Fixes FindBug warning: DL: Synchronization on Boolean
    private Object capsLock = new Object();

    @Inject
    public DiscoveryEntryStorePersisted(CapabilitiesProvisioning staticProvisioning,
                                        Provider<EntityManager> entityManagerProvider,
                                        PersistService persistService) {
        persistService.start();
        this.entityManagerProvider = entityManagerProvider;
        logger.debug("creating CapabilitiesStore {} with static provisioning", this);
        //add(staticProvisioning.getCapabilityEntries());
    }

    /*
     * (non-Javadoc)
     * @see io.joynr.capabilities.CapabilitiesStore#add(io.joynr.
     * capabilities .DiscoveryEntry)
     */
    @Override
    public synchronized void add(DiscoveryEntry discoveryEntry) {
        if (!(discoveryEntry instanceof GlobalDiscoveryEntryPersisted)) {
            return;
        }
        GlobalDiscoveryEntryPersisted globalDiscoveryEntry = (GlobalDiscoveryEntryPersisted) discoveryEntry;
        if (globalDiscoveryEntry.getDomain() == null || globalDiscoveryEntry.getInterfaceName() == null
                || globalDiscoveryEntry.getParticipantId() == null || globalDiscoveryEntry.getAddress() == null) {
            String message = "discoveryEntry being registered is not complete: " + discoveryEntry;
            logger.error(message);
            throw new JoynrCommunicationException(message);
        }

        EntityManager entityManager = entityManagerProvider.get();
        GlobalDiscoveryEntryPersisted discoveryEntryFound = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                               discoveryEntry.getParticipantId());

        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            if (discoveryEntryFound != null) {
                entityManager.merge(discoveryEntry);
            } else {
                entityManager.persist(discoveryEntry);
            }
            transaction.commit();
        } catch (Exception e) {
            if (transaction.isActive()) {
                transaction.rollback();
            }
            logger.error("unable to add discoveryEntry: {}, reason: {}", discoveryEntry, e.getMessage());
        } finally {
        }
    }

    @Override
    public void add(Collection<? extends DiscoveryEntry> entries) {
        if (entries != null) {
            for (DiscoveryEntry entry : entries) {
                add(entry);
            }
        }
    }

    @Override
    public boolean remove(String participantId) {
        boolean removedSuccessfully = false;

        synchronized (capsLock) {
            removedSuccessfully = removeCapabilityFromStore(participantId);
        }
        if (!removedSuccessfully) {
            logger.error("Could not find capability to remove with Id: {}", participantId);
        }
        return removedSuccessfully;
    }

    private boolean removeCapabilityFromStore(String participantId) {
        EntityManager entityManager = entityManagerProvider.get();
        GlobalDiscoveryEntryPersisted discoveryEntry = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                          participantId);

        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            entityManager.remove(discoveryEntry);
            transaction.commit();
        } catch (Exception e) {
            if (transaction.isActive()) {
                transaction.rollback();
            }
            logger.error("unable to remove capability: {} reason: {}", participantId, e.getMessage());
            return false;
        } finally {
        }
        return true;
    }

    @Override
    public void remove(Collection<String> participantIds) {
        for (String participantId : participantIds) {
            remove(participantId);
        }
    }

    @Override
    public Collection<DiscoveryEntry> lookup(final String[] domains, final String interfaceName) {
        return lookup(domains, interfaceName, DiscoveryQos.NO_MAX_AGE);
    }

    @SuppressWarnings("unchecked")
    @Override
    @Transactional
    public Collection<DiscoveryEntry> lookup(final String[] domains, final String interfaceName, long cacheMaxAge) {
        EntityManager entityManager = entityManagerProvider.get();
        String query = "from GlobalDiscoveryEntryPersisted where domain=:domain and interfaceName=:interfaceName";
        List<DiscoveryEntry> result = new ArrayList<>();
        for (String domain : domains) {
            List<DiscoveryEntry> capabilitiesList = entityManager.createQuery(query)
                                                                  .setParameter("domain", domain)
                                                                  .setParameter("interfaceName", interfaceName)
                                                                  .getResultList();
            result.addAll(capabilitiesList);
        }

        logger.debug("Capabilities found: {}", result.toString());
        return result;
    }

    @Override
    @CheckForNull
    @Transactional
    public DiscoveryEntry lookup(String participantId, long cacheMaxAge) {
        EntityManager entityManager = entityManagerProvider.get();
        return entityManager.find(GlobalDiscoveryEntryPersisted.class, participantId);
    }

    @Override
    @Transactional
    public HashSet<DiscoveryEntry> getAllDiscoveryEntries() {
        EntityManager entityManager = entityManagerProvider.get();
        List<GlobalDiscoveryEntryPersisted> allCapabilityEntries = entityManager.createQuery("Select discoveryEntry from GlobalDiscoveryEntryPersisted discoveryEntry",
                                                                                             GlobalDiscoveryEntryPersisted.class)
                                                                                .getResultList();

        return new HashSet<DiscoveryEntry>(allCapabilityEntries);

    }

    @Override
    public boolean hasDiscoveryEntry(@Nonnull DiscoveryEntry discoveryEntry) {
        if (discoveryEntry instanceof GlobalDiscoveryEntryPersisted) {
            EntityManager entityManager = entityManagerProvider.get();
            GlobalDiscoveryEntryPersisted searchingForDiscoveryEntry = (GlobalDiscoveryEntryPersisted) discoveryEntry;
            GlobalDiscoveryEntryPersisted foundCapability = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                               searchingForDiscoveryEntry.getParticipantId());
            return discoveryEntry.equals(foundCapability);
        } else {
            return false;
        }
    }
}
