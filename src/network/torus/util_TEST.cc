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
#include "network/torus/util.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include <vector>

TEST(TorusUtil, computeInputPortDim) {
}

TEST(TorusUtil, computeAddress) {
  const std::vector<u32> widths({3, 2, 3});
  const u32 concentration = 2;

  std::vector<u32> address;

  Torus::computeAddress(0, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 0, 0, 0}));

  Torus::computeAddress(1, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 0, 0, 0}));

  Torus::computeAddress(8, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 1, 1, 0}));

  Torus::computeAddress(9, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 1, 1, 0}));

  Torus::computeAddress(16, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 2, 0, 1}));

  Torus::computeAddress(17, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 2, 0, 1}));

  Torus::computeAddress(20, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({0, 1, 1, 1}));

  Torus::computeAddress(21, widths, concentration, &address);
  ASSERT_EQ(address, std::vector<u32>({1, 1, 1, 1}));
}

TEST(TorusUtil, computeId) {
  const std::vector<u32> widths({3, 2, 3});
  const u32 concentration = 2;

  std::vector<u32> address;

  address = {0, 0, 0, 0};
  ASSERT_EQ(0u, Torus::computeId(address, widths, concentration));

  address = {1, 0, 0, 0};
  ASSERT_EQ(1u, Torus::computeId(address, widths, concentration));

  address = {0, 1, 1, 0};
  ASSERT_EQ(8u, Torus::computeId(address, widths, concentration));

  address = {1, 1, 1, 0};
  ASSERT_EQ(9u, Torus::computeId(address, widths, concentration));

  address = {0, 2, 0, 1};
  ASSERT_EQ(16u, Torus::computeId(address, widths, concentration));

  address = {1, 2, 0, 1};
  ASSERT_EQ(17u, Torus::computeId(address, widths, concentration));

  address = {0, 1, 1, 1};
  ASSERT_EQ(20u, Torus::computeId(address, widths, concentration));

  address = {1, 1, 1, 1};
  ASSERT_EQ(21u, Torus::computeId(address, widths, concentration));
}
