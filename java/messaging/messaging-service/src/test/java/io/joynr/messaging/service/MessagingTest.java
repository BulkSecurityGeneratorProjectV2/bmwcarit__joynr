package io.joynr.messaging.service;

/*
 * #%L
 * joynr::java::messaging::messaging-service
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

import static com.jayway.restassured.RestAssured.given;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_EXPIRYDATENOTSET;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import io.joynr.common.ExpiryDate;
import io.joynr.messaging.datatypes.JoynrMessagingError;
import io.joynr.messaging.serialize.JoynrEnumSerializer;
import io.joynr.messaging.serialize.JoynrListSerializer;
import io.joynr.messaging.serialize.JoynrUntypedObjectDeserializer;
import io.joynr.messaging.serialize.NumberSerializer;

import java.io.IOException;
import java.util.UUID;

import joynr.JoynrMessage;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.Version;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.MapperFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.fasterxml.jackson.databind.SerializationFeature;
import com.fasterxml.jackson.databind.jsontype.TypeDeserializer;
import com.fasterxml.jackson.databind.jsontype.TypeResolverBuilder;
import com.fasterxml.jackson.databind.module.SimpleModule;
import com.fasterxml.jackson.databind.type.SimpleType;
import com.google.inject.servlet.ServletModule;
import com.jayway.restassured.http.ContentType;
import com.jayway.restassured.response.Response;
import com.sun.jersey.guice.spi.container.servlet.GuiceContainer;

@RunWith(MockitoJUnitRunner.class)
public class MessagingTest extends AbstractServiceInterfaceTest {

    private String serverUrl;

    @Mock
    MessagingService mock;

    private ObjectMapper objectMapper;

    @Override
    protected ServletModule getServletTestModule() {

        return new ServletModule() {

            @Override
            protected void configureServlets() {

                bind(MessagingService.class).toInstance(mock);
                bind(MessagingServiceRestAdapter.class);

                serve("/*").with(GuiceContainer.class);
            }

        };

    }

    private ObjectMapper getObjectMapper() {
        objectMapper = new ObjectMapper();
        objectMapper.configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false);
        objectMapper.configure(SerializationFeature.WRAP_ROOT_VALUE, false);
        // objectMapper.configure(SerializationFeature.ORDER_MAP_ENTRIES_BY_KEYS,
        // true);
        objectMapper.configure(SerializationFeature.WRITE_NULL_MAP_VALUES, true);
        objectMapper.configure(SerializationFeature.FAIL_ON_EMPTY_BEANS, false);
        objectMapper.configure(MapperFeature.SORT_PROPERTIES_ALPHABETICALLY, true);

        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        TypeResolverBuilder<?> joynrTypeResolverBuilder = objectMapper.getSerializationConfig()
                                                                      .getDefaultTyper(SimpleType.construct(Object.class));

        SimpleModule module = new SimpleModule("NonTypedModule", new Version(1, 0, 0, "", "", ""));
        module.addSerializer(Number.class, new NumberSerializer());
        module.addSerializer(new JoynrEnumSerializer());
        module.addSerializer(new JoynrListSerializer());

        TypeDeserializer typeDeserializer = joynrTypeResolverBuilder.buildTypeDeserializer(objectMapper.getDeserializationConfig(),
                                                                                           SimpleType.construct(Object.class),
                                                                                           null);

        module.addDeserializer(Object.class, new JoynrUntypedObjectDeserializer(typeDeserializer));
        objectMapper.registerModule(module);
        return objectMapper;
    }

    @Before
    public void setUp() throws Exception {
        super.setUp();

        serverUrl = String.format("%s", getServerUrlWithoutPath());

        objectMapper = getObjectMapper();
    }

    // @Test
    // public void testPostMessageWithMissingChannelId() throws
    // JsonProcessingException {
    //
    // String serializedMessage = createJoynrMessage();
    //
    // // TODO how is the call in this case???
    // Response response = //
    // given(). //
    // contentType(ContentType.JSON).
    // when().body(serializedMessage).
    // post(serverUrl + "/channels/null/message");
    //
    // assertEquals(201 /* Created */, response.getStatusCode());
    //
    // }

    @Test
    public void testPostMessageWithoutExpiryDate() throws IOException {

        JoynrMessage message = createJoynrMessage();
        message.setExpirationDate(ExpiryDate.fromAbsolute(0));
        String serializedMessage = objectMapper.writeValueAsString(message);

        Response response = //
        given(). //
               contentType(ContentType.JSON)
               .when()
               .body(serializedMessage)
               .post(serverUrl + "/channels/channel-123/message");

        assertEquals(400 /* Bad Request */, response.getStatusCode());

        JoynrMessagingError error = objectMapper.readValue(response.getBody().asString(), JoynrMessagingError.class);

        assertEquals(JOYNRMESSAGINGERROR_EXPIRYDATENOTSET.getCode(), error.getCode());
        assertEquals(JOYNRMESSAGINGERROR_EXPIRYDATENOTSET.getDescription(), error.getReason());
        Mockito.verify(mock, Mockito.never()).passMessageToReceiver("channel-123", message);
    }

    @Test
    public void testPostMessageExpiryDateExpired() throws IOException {

        JoynrMessage message = createJoynrMessage();
        message.setExpirationDate(ExpiryDate.fromAbsolute(1));
        String serializedMessage = objectMapper.writeValueAsString(message);

        Response response = //
        given(). //
               contentType(ContentType.JSON)
               .when()
               .body(serializedMessage)
               .post(serverUrl + "/channels/channel-123/message");

        assertEquals(400 /* Bad Request */, response.getStatusCode());

        JoynrMessagingError error = objectMapper.readValue(response.getBody().asString(), JoynrMessagingError.class);

        assertEquals(JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED.getCode(), error.getCode());
        assertEquals(JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED.getDescription() + ": 127.0.0.1", error.getReason());

        Mockito.verify(mock, Mockito.never()).passMessageToReceiver("channel-123", message);
    }

    @Test
    public void testPostMessageWithoutMessageReceivers() throws IOException {

        Mockito.when(mock.hasMessageReceiver("channel-123")).thenReturn(false);
        Mockito.when(mock.isAssignedForChannel("channel-123")).thenReturn(true);

        JoynrMessage message = createJoynrMessage();
        String serializedMessage = objectMapper.writeValueAsString(message);

        Response response = //
        given(). //
               contentType(ContentType.JSON)
               .when()
               .body(serializedMessage)
               .post(serverUrl + "/channels/channel-123/message");

        assertEquals(204 /* No Content */, response.getStatusCode());
        Mockito.verify(mock).hasMessageReceiver("channel-123");
        Mockito.verify(mock, Mockito.never()).passMessageToReceiver("channel-123", message);
    }

    @Test
    public void testPostMessage() throws JsonProcessingException {

        Mockito.when(mock.hasMessageReceiver("channel-123")).thenReturn(true);
        Mockito.when(mock.isAssignedForChannel("channel-123")).thenReturn(true);

        JoynrMessage message = createJoynrMessage();
        String serializedMessage = objectMapper.writeValueAsString(message);

        Response response = //
        given(). //
               contentType(ContentType.JSON)
               .when()
               .body(serializedMessage)
               .post(serverUrl + "/channels/channel-123/message");

        assertEquals(201 /* Created */, response.getStatusCode());
        assertEquals(serverUrl + "/messages/" + message.getId(), response.getHeader("Location"));
        assertEquals(message.getId(), response.getHeader("msgId"));
        Mockito.verify(mock).passMessageToReceiver("channel-123", message);
    }

    @Test
    public void testPostMessageChannelWasMigrated() throws JsonProcessingException {

        Mockito.when(mock.isAssignedForChannel("channel-123")).thenReturn(false);
        Mockito.when(mock.hasChannelAssignmentMoved("channel-123")).thenReturn(true);

        JoynrMessage message = createJoynrMessage();
        String serializedMessage = objectMapper.writeValueAsString(message);

        Response response = //
        given(). //
               contentType(ContentType.JSON)
               .when()
               .body(serializedMessage)
               .post(serverUrl + "/channels/channel-123/message");

        assertEquals(410 /* Gone */, response.getStatusCode());
        assertNull(response.getHeader("Location"));
        assertNull(response.getHeader("msgId"));
        Mockito.verify(mock, Mockito.never()).passMessageToReceiver("channel-123", message);
    }

    @Test
    public void testPostMessageChannelWasNotRegistered() throws JsonProcessingException {

        Mockito.when(mock.isAssignedForChannel("channel-123")).thenReturn(false);
        Mockito.when(mock.hasChannelAssignmentMoved("channel-123")).thenReturn(false);

        JoynrMessage message = createJoynrMessage();
        String serializedMessage = objectMapper.writeValueAsString(message);

        Response response = //
        given(). //
               contentType(ContentType.JSON)
               .when()
               .body(serializedMessage)
               .post(serverUrl + "/channels/channel-123/message");

        assertEquals(404 /* Not Found */, response.getStatusCode());
        assertNull(response.getHeader("Location"));
        assertNull(response.getHeader("msgId"));
        Mockito.verify(mock, Mockito.never()).passMessageToReceiver("channel-123", message);
    }

    private JoynrMessage createJoynrMessage() {
        JoynrMessage message = new JoynrMessage();
        message.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(50000));
        message.setPayload("payload-" + UUID.randomUUID().toString());
        return message;
    }
}
