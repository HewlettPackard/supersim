/*
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership. You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "allocator/WavefrontAllocator.h"

#include <json/json.h>
#include <gtest/gtest.h>
#include <prim/prim.h>

#include "settings/settings.h"

#include "allocator/Allocator_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(WavefrontAllocator, sequential) {
  // create the allocator settings
  Json::Value allocSettings;
  allocSettings["scheme"] = "sequential";
  allocSettings["type"] = "wavefront";

  // test
  AllocatorTest(allocSettings, nullptr, false);
  AllocatorLoadBalanceTest(allocSettings);
}

TEST(WavefrontAllocator, random) {
  // create the allocator settings
  Json::Value allocSettings;
  allocSettings["scheme"] = "random";
  allocSettings["type"] = "wavefront";

  // test
  AllocatorTest(allocSettings, nullptr, false);
  AllocatorLoadBalanceTest(allocSettings);
}
