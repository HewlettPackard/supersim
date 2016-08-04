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
#include "network/cube/util.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include <vector>

TEST(CubeUtil, computeTerminalAddress) {
  const std::vector<u32> widths({3, 2, 3});
  const u32 concentration = 2;

  std::vector<u32> address;

  Cube::computeTerminalAddress(0, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 0, 0, 0}));

  Cube::computeTerminalAddress(1, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 0, 0, 0}));

  Cube::computeTerminalAddress(8, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 1, 1, 0}));

  Cube::computeTerminalAddress(9, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 1, 1, 0}));

  Cube::computeTerminalAddress(16, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 2, 0, 1}));

  Cube::computeTerminalAddress(17, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 2, 0, 1}));

  Cube::computeTerminalAddress(20, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 1, 1, 1}));

  Cube::computeTerminalAddress(21, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 1, 1, 1}));
}

TEST(CubeUtil, computeTerminalId) {
  const std::vector<u32> widths({3, 2, 3});
  const u32 concentration = 2;

  std::vector<u32> address;

  address = {0, 0, 0, 0};
  ASSERT_EQ(0u, Cube::computeTerminalId(&address, widths, concentration));

  address = {1, 0, 0, 0};
  ASSERT_EQ(1u, Cube::computeTerminalId(&address, widths, concentration));

  address = {0, 1, 1, 0};
  ASSERT_EQ(8u, Cube::computeTerminalId(&address, widths, concentration));

  address = {1, 1, 1, 0};
  ASSERT_EQ(9u, Cube::computeTerminalId(&address, widths, concentration));

  address = {0, 2, 0, 1};
  ASSERT_EQ(16u, Cube::computeTerminalId(&address, widths, concentration));

  address = {1, 2, 0, 1};
  ASSERT_EQ(17u, Cube::computeTerminalId(&address, widths, concentration));

  address = {0, 1, 1, 1};
  ASSERT_EQ(20u, Cube::computeTerminalId(&address, widths, concentration));

  address = {1, 1, 1, 1};
  ASSERT_EQ(21u, Cube::computeTerminalId(&address, widths, concentration));
}

TEST(CubeUtil, computeRouterAddress) {
  const std::vector<u32> widths({3, 2, 3});

  std::vector<u32> address;

  Cube::computeRouterAddress(0, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 0, 0}));

  Cube::computeRouterAddress(1, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 0, 0}));

  Cube::computeRouterAddress(3, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 1, 0}));

  Cube::computeRouterAddress(5, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({2, 1, 0}));

  Cube::computeRouterAddress(6, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 0, 1}));

  Cube::computeRouterAddress(8, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({2, 0, 1}));

  Cube::computeRouterAddress(13, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 0, 2}));

  Cube::computeRouterAddress(17, widths, &address);
  ASSERT_EQ(address, std::vector<u32>({2, 1, 2}));
}

TEST(CubeUtil, computeRouterId) {
  const std::vector<u32> widths({3, 2, 3});

  std::vector<u32> address;

  address = {0, 0, 0};
  ASSERT_EQ(0u, Cube::computeRouterId(&address, widths));

  address = {1, 0, 0};
  ASSERT_EQ(1u, Cube::computeRouterId(&address, widths));

  address = {0, 1, 0};
  ASSERT_EQ(3u, Cube::computeRouterId(&address, widths));

  address = {2, 1, 0};
  ASSERT_EQ(5u, Cube::computeRouterId(&address, widths));

  address = {0, 0, 1};
  ASSERT_EQ(6u, Cube::computeRouterId(&address, widths));

  address = {2, 0, 1};
  ASSERT_EQ(8u, Cube::computeRouterId(&address, widths));

  address = {1, 0, 2};
  ASSERT_EQ(13u, Cube::computeRouterId(&address, widths));

  address = {2, 1, 2};
  ASSERT_EQ(17u, Cube::computeRouterId(&address, widths));
}
