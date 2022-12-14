// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

namespace doris_udf {
class FunctionContext;
struct IntVal;
struct BigIntVal;
struct StringVal;
} // namespace doris_udf

namespace doris {

class HashFunctions {
public:
    static void init();
    static doris_udf::IntVal murmur_hash3_32(doris_udf::FunctionContext* ctx, int num_children,
                                             const doris_udf::StringVal* inputs);
    static doris_udf::BigIntVal murmur_hash3_64(doris_udf::FunctionContext* ctx, int num_children,
                                                const doris_udf::StringVal* inputs);
};

} // namespace doris
