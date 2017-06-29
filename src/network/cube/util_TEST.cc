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
#include "network/cube/util.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include <vector>

#include "event/Simulator.h"
#include "test/TestSetup_TEST.h"

TEST(CubeUtil, computeNumRoutersAndTerminals) {
  TestSetup ts(1, 1, 1, 0xDEAFBEEF);

  for (u32 test = 0; test < 1000; test++) {
    std::vector<u32> widths;
    u32 dims = gSim->rnd.nextU64(1, 10);
    for (u32 dim = 0; dim < dims; dim++) {
      widths.push_back(gSim->rnd.nextU64(2, 6));
    }
    u32 concentration = gSim->rnd.nextU64(1, 256);

    // determine expected results
    u32 expRouters = std::accumulate(
        std::begin(widths), std::end(widths), 1, std::multiplies<double>());
    u32 expTerminals = expRouters * concentration;

    // compare against util functions
    ASSERT_EQ(Cube::computeNumRouters(widths), expRouters);
    ASSERT_EQ(Cube::computeNumTerminals(widths, concentration), expTerminals);
  }
}

TEST(CubeUtil, translateInterfaceIdToAddress) {
  const std::vector<u32> widths({3, 2, 3});
  const u32 concentration = 2;

  std::vector<u32> address;

  Cube::translateInterfaceIdToAddress(0, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 0, 0, 0}));

  Cube::translateInterfaceIdToAddress(1, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 0, 0, 0}));

  Cube::translateInterfaceIdToAddress(8, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 1, 1, 0}));

  Cube::translateInterfaceIdToAddress(9, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 1, 1, 0}));

  Cube::translateInterfaceIdToAddress(16, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 2, 0, 1}));

  Cube::translateInterfaceIdToAddress(17, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 2, 0, 1}));

  Cube::translateInterfaceIdToAddress(20, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 1, 1, 1}));

  Cube::translateInterfaceIdToAddress(21, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 1, 1, 1}));
}

TEST(CubeUtil, translateInterfaceAddressToId) {
  const std::vector<u32> widths({3, 2, 3});
  const u32 concentration = 2;

  std::vector<u32> address;

  address = {0, 0, 0, 0};
  ASSERT_EQ(0u, Cube::translateInterfaceAddressToId(&address, widths,
                                                    concentration));

  address = {1, 0, 0, 0};
  ASSERT_EQ(1u, Cube::translateInterfaceAddressToId(&address, widths,
                                                    concentration));

  address = {0, 1, 1, 0};
  ASSERT_EQ(8u, Cube::translateInterfaceAddressToId(&address, widths,
                                                    concentration));

  address = {1, 1, 1, 0};
  ASSERT_EQ(9u, Cube::translateInterfaceAddressToId(&address, widths,
                                                    concentration));

  address = {0, 2, 0, 1};
  ASSERT_EQ(16u, Cube::translateInterfaceAddressToId(&address, widths,
                                                     concentration));

  address = {1, 2, 0, 1};
  ASSERT_EQ(17u, Cube::translateInterfaceAddressToId(&address, widths,
                                                     concentration));

  address = {0, 1, 1, 1};
  ASSERT_EQ(20u, Cube::translateInterfaceAddressToId(&address, widths,
                                                     concentration));

  address = {1, 1, 1, 1};
  ASSERT_EQ(21u, Cube::translateInterfaceAddressToId(&address, widths,
                                                     concentration));
}

TEST(CubeUtil, translateRouterIdToAddress) {
  const std::vector<u32> widths({3, 2, 3});

  std::vector<u32> address;

  Cube::translateRouterIdToAddress(0, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 0, 0}));

  Cube::translateRouterIdToAddress(1, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 0, 0}));

  Cube::translateRouterIdToAddress(3, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 1, 0}));

  Cube::translateRouterIdToAddress(5, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({2, 1, 0}));

  Cube::translateRouterIdToAddress(6, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 0, 1}));

  Cube::translateRouterIdToAddress(8, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({2, 0, 1}));

  Cube::translateRouterIdToAddress(13, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 0, 2}));

  Cube::translateRouterIdToAddress(17, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({2, 1, 2}));
}

TEST(CubeUtil, translateRouterAddressToId) {
  const std::vector<u32> widths({3, 2, 3});

  std::vector<u32> address;

  address = {0, 0, 0};
  ASSERT_EQ(0u, Cube::translateRouterAddressToId(&address, widths));

  address = {1, 0, 0};
  ASSERT_EQ(1u, Cube::translateRouterAddressToId(&address, widths));

  address = {0, 1, 0};
  ASSERT_EQ(3u, Cube::translateRouterAddressToId(&address, widths));

  address = {2, 1, 0};
  ASSERT_EQ(5u, Cube::translateRouterAddressToId(&address, widths));

  address = {0, 0, 1};
  ASSERT_EQ(6u, Cube::translateRouterAddressToId(&address, widths));

  address = {2, 0, 1};
  ASSERT_EQ(8u, Cube::translateRouterAddressToId(&address, widths));

  address = {1, 0, 2};
  ASSERT_EQ(13u, Cube::translateRouterAddressToId(&address, widths));

  address = {2, 1, 2};
  ASSERT_EQ(17u, Cube::translateRouterAddressToId(&address, widths));
}
