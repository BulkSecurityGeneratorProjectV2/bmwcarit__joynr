package io.joynr.joynrandroidruntime;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.joynrandroidruntime.messaging.AndroidLongPollingMessagingModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.JoynrRuntimeImpl;
import io.joynr.runtime.PropertyLoader;

import java.io.File;
import java.util.Properties;

import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;

import com.google.inject.Injector;

public class InitRuntimeTask extends AsyncTask<Object, String, JoynrRuntime> {

    public static final long INIT_TIMEOUT = 30000;
    private UILogger uiLogger;
    private Context applicationContext;
    private Properties joynrConfig;

    public InitRuntimeTask(Context applicationContext, UILogger uiLogger) {
        this(PropertyLoader.loadProperties("res/raw/demo.properties"), applicationContext, uiLogger);
    }

    public InitRuntimeTask(Properties joynrConfig, Context applicationContext, UILogger uiLogger) {
        this.joynrConfig = joynrConfig;
        this.applicationContext = applicationContext;
        this.uiLogger = uiLogger;
    }

    @Override
    protected JoynrRuntime doInBackground(Object... params) {
        try {
            Log.d("JAS", "starting joynr runtime");
            publishProgress("Starting joynr runtime...\n");

            // create/make persistence file absolute
            File appWorkingDir = applicationContext.getFilesDir();
            String persistenceFileName = appWorkingDir.getPath()
                    + File.separator
                    + joynrConfig.getProperty(MessagingPropertyKeys.PERSISTENCE_FILE,
                                              MessagingPropertyKeys.DEFAULT_PERSISTENCE_FILE);
            joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, persistenceFileName);

            // create/make participant ID persistence file absolute
            String participantIdPersistenceFileName = appWorkingDir.getPath()
                    + File.separator
                    + joynrConfig.getProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE,
                                              ConfigurableMessagingSettings.DEFAULT_PARTICIPANTIDS_PERSISTENCE_FILE);
            joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE,
                                    participantIdPersistenceFileName);

            publishProgress("Properties loaded\n");

            joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_REQUEST_TIMEOUT, "120000");

            Injector injectorA = new JoynrInjectorFactory(joynrConfig, new AndroidLongPollingMessagingModule()).createChildInjector();

            JoynrRuntimeImpl runtime = injectorA.getInstance(JoynrRuntimeImpl.class);
            if (runtime != null) {
                Log.d("JAS", "joynr runtime started");
            } else {
                Log.e("JAS", "joynr runtime not started");
            }
            publishProgress("joynr runtime started.\n");

            return runtime;

        } catch (Exception e) {
            e.printStackTrace();
            publishProgress(e.getMessage());
        }
        return null;
    }

    protected void onProgressUpdate(String... progress) {
        uiLogger.logText(progress);
    }

};
