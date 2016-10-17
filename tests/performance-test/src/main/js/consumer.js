/*jslint node: true */

/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

var Promise = require("bluebird").Promise;
var consumerBase = require("./consumer.base.js");

//disable log
console.log = function() {};

consumerBase.initialize().then(function() {
    return consumerBase.echoString()
        .then(consumerBase.echoComplexStruct)
        .then(consumerBase.echoByteArray)
        .then(function(){
             return consumerBase.echoByteArray(1000);
        })
        .then(function() {
            console.log("SUCCEEDED");
            process.exit(0);
        })
        .then(consumerBase.shutdown);
}).catch(function(error) {
    console.log("Error while performing test: " + error);
    throw error;
});