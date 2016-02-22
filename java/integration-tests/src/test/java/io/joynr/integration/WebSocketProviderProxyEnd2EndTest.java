package io.joynr.integration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import java.util.Properties;

import com.google.inject.AbstractModule;
import com.google.inject.Key;
import com.google.inject.name.Names;

import io.joynr.messaging.websocket.JoynrWebSocketEndpoint;
import io.joynr.messaging.websocket.WebSocketEndpointFactory;
import io.joynr.proxy.ProxyBuilder;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.tests.testProxy;
import org.junit.After;
import org.junit.Before;

import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.runtime.CCWebSocketRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import io.joynr.servlet.ServletUtil;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

/**
 *
 */
public class WebSocketProviderProxyEnd2EndTest extends ProviderProxyEnd2EndTest {

    private static final long CONST_DEFAULT_TEST_TIMEOUT = 10000;
    private JoynrRuntime ccJoynrRuntime;
    private Properties webSocketConfig;
    private Injector injectorCC;

    @Before
    @Override
    public void baseSetup() throws Exception {
        final int port = ServletUtil.findFreePort();
        webSocketConfig = new Properties();
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "" + port);
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        webSocketConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");

        //init websocket properties before libjoynr runtimes are created
        super.baseSetup();
    }

    @Override
    @After
    public void tearDown() throws InterruptedException {
        super.tearDown();
        ccJoynrRuntime.shutdown(false);
    }

    private JoynrRuntime createClusterController(Properties webSocketConfig) {
        Properties ccConfig = new Properties();
        ccConfig.putAll(webSocketConfig);
        ccConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_CC_CONNECTION_TYPE, "WEBSOCKET");
        injectorCC = new JoynrInjectorFactory(ccConfig, Modules.override(new CCWebSocketRuntimeModule())
                                                               .with(new AtmosphereMessagingModule())).getInjector();
        return injectorCC.getInstance(JoynrRuntime.class);
    }

    @Override
    protected JoynrRuntime getRuntime(final Properties joynrConfig, final Module... modules) {
        if (ccJoynrRuntime == null) {
            ccJoynrRuntime = createClusterController(webSocketConfig);
        }
        joynrConfig.putAll(webSocketConfig);
        joynrConfig.setProperty(ConfigurableMessagingSettings.PROPERTY_CC_CONNECTION_TYPE, "WEBSOCKET");
        Module modulesWithRuntime = Modules.override(modules)
                                           .with(Modules.override(new LibjoynrWebSocketRuntimeModule())
                                                        .with(new AbstractModule() {
                                                            @Override
                                                            protected void configure() {
                                                                //shorten reconnect delay to speed up tests
                                                                bind(long.class).annotatedWith(Names.named(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_RECONNECT_DELAY))
                                                                                .toInstance(100L);
                                                            }
                                                        }));
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             modulesWithRuntime).createApplication(DummyJoynrApplication.class);
        dummyApplications.add(application);
        return application.getRuntime();
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void testWebsocketReconnect() throws InterruptedException {
        int result;
        ProxyBuilder<testProxy> proxyBuilder = consumerRuntime.getProxyBuilder(domain, testProxy.class);
        testProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        //Test rpc before connection is lost
        result = proxy.addNumbers(6, 3, 2);
        assertEquals(11, result);

        //simulate losing connection
        WebSocketEndpointFactory ccWebSocketServerFactory = injectorCC.getInstance(WebSocketEndpointFactory.class);
        WebSocketAddress serverAddress = injectorCC.getInstance(Key.get(WebSocketAddress.class,
                                                                        Names.named(WebsocketModule.WEBSOCKET_SERVER_ADDRESS)));
        JoynrWebSocketEndpoint ccWebSocketServer = ccWebSocketServerFactory.create(serverAddress);

        ccWebSocketServer.shutdown();
        Thread.sleep(1000);
        ccWebSocketServer.start();
        Thread.sleep(1000);
        result = proxy.addNumbers(7, 8, 1);
        assertEquals(16, result);
    }
}
