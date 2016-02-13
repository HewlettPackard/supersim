/*
 * Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "allocator/CrSeparableAllocator.h"

#include <json/json.h>
#include <gtest/gtest.h>
#include <prim/prim.h>

#include "settings/settings.h"

#include "allocator/Allocator_TEST.h"

static void verify(u32 _numClients, u32 _numResources, const bool* _request,
                   const u64* _metadata, const bool* _grant) {
  // TODO(nic): find a good test for this
}

TEST(CrSeparableAllocator, lslp) {
  // create the allocator settings
  Json::Value arbSettings;
  arbSettings["type"] = "lslp";
  Json::Value allocSettings;
  allocSettings["resource_arbiter"] = arbSettings;
  allocSettings["client_arbiter"] = arbSettings;
  allocSettings["iterations"] = 3;
  allocSettings["slip_latch"] = true;
  allocSettings["type"] = "cr_separable";
  // printf("%s\n", toString(&allocSettings).c_str());

  // test
  AllocatorTest(allocSettings, verify, false);
}

TEST(CrSeparableAllocator, greater) {
  // create the allocator settings
  Json::Value arbSettings;
  arbSettings["type"] = "comparing";
  arbSettings["greater"] = true;
  Json::Value allocSettings;
  allocSettings["resource_arbiter"] = arbSettings;
  allocSettings["client_arbiter"] = arbSettings;
  allocSettings["iterations"] = 2;
  allocSettings["slip_latch"] = true;
  allocSettings["type"] = "cr_separable";
  // printf("%s\n", toString(&allocSettings).c_str());

  // test
  AllocatorTest(allocSettings, verify, false);
}

TEST(CrSeparableAllocator, lesser) {
  // create the allocator settings
  Json::Value arbSettings;
  arbSettings["type"] = "comparing";
  arbSettings["greater"] = false;
  Json::Value allocSettings;
  allocSettings["resource_arbiter"] = arbSettings;
  allocSettings["client_arbiter"] = arbSettings;
  allocSettings["iterations"] = 1;
  allocSettings["slip_latch"] = true;
  allocSettings["type"] = "cr_separable";
  // printf("%s\n", toString(&allocSettings).c_str());

  // test
  AllocatorTest(allocSettings, verify, false);
}

TEST(CrSeparableAllocator, random) {
  // create the allocator settings
  Json::Value arbSettings;
  arbSettings["type"] = "random";
  Json::Value allocSettings;
  allocSettings["resource_arbiter"] = arbSettings;
  allocSettings["client_arbiter"] = arbSettings;
  allocSettings["iterations"] = 1;
  allocSettings["slip_latch"] = true;
  allocSettings["type"] = "cr_separable";
  // printf("%s\n", toString(&allocSettings).c_str());

  // test
  AllocatorTest(allocSettings, verify, false);
}
