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
#include "network/foldedclos/util.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include <vector>

TEST(FoldedClos, translateInterfaceIdToAddress) {
  std::vector<u32> act;
  std::vector<u32> exp;
  u32 id;

  const u32 halfRadix = 3;
  const u32 numLevels = 4;
  const u32 rowRouters = 27;

  id = 0;
  exp = {0, 0, 0, 0};
  FoldedClos::translateInterfaceIdToAddress(halfRadix, numLevels, rowRouters,
                                            id, &act);
  ASSERT_EQ(act, exp);

  id = 1;
  exp = {1, 0, 0, 0};
  FoldedClos::translateInterfaceIdToAddress(halfRadix, numLevels, rowRouters,
                                            id, &act);
  ASSERT_EQ(act, exp);

  id = 3;
  exp = {0, 1, 0, 0};
  FoldedClos::translateInterfaceIdToAddress(halfRadix, numLevels, rowRouters,
                                            id, &act);
  ASSERT_EQ(act, exp);

  id = 61;
  exp = {1, 2, 0, 2};
  FoldedClos::translateInterfaceIdToAddress(halfRadix, numLevels, rowRouters,
                                            id, &act);
  ASSERT_EQ(act, exp);
}

TEST(FoldedClos, translateInterfaceAddressToId) {
  std::vector<u32> addr;

  const u32 halfRadix = 3;
  const u32 numLevels = 4;
  const u32 rowRouters = 27;

  addr = {0, 0, 0, 0};
  ASSERT_EQ(0u, FoldedClos::translateInterfaceAddressToId(
      halfRadix, numLevels, rowRouters, &addr));

  addr = {1, 0, 0, 0};
  ASSERT_EQ(1u, FoldedClos::translateInterfaceAddressToId(
      halfRadix, numLevels, rowRouters, &addr));

  addr = {0, 1, 0, 0};
  ASSERT_EQ(3u, FoldedClos::translateInterfaceAddressToId(
      halfRadix, numLevels, rowRouters, &addr));

  addr = {1, 2, 0, 2};
  ASSERT_EQ(61u, FoldedClos::translateInterfaceAddressToId(
      halfRadix, numLevels, rowRouters, &addr));
}

TEST(FoldedClos, translateRouterIdToAddress) {
  std::vector<u32> act;
  std::vector<u32> exp;
  u32 id;

  const u32 halfRadix = 3;
  const u32 numLevels = 4;
  const u32 rowRouters = 27;

  id = 0;
  exp = {0, 0};
  FoldedClos::translateRouterIdToAddress(halfRadix, numLevels, rowRouters,
                                         id, &act);
  ASSERT_EQ(act, exp);

  id = 1;
  exp = {0, 1};
  FoldedClos::translateRouterIdToAddress(halfRadix, numLevels, rowRouters,
                                         id, &act);
  ASSERT_EQ(act, exp);

  id = 27;
  exp = {1, 0};
  FoldedClos::translateRouterIdToAddress(halfRadix, numLevels, rowRouters,
                                         id, &act);
  ASSERT_EQ(act, exp);

  id = 93;
  exp = {3, 12};
  FoldedClos::translateRouterIdToAddress(halfRadix, numLevels, rowRouters,
                                         id, &act);
  ASSERT_EQ(act, exp);
}

TEST(FoldedClos, translateRouterAddressToId) {
  std::vector<u32> addr;

  const u32 halfRadix = 3;
  const u32 numLevels = 4;
  const u32 rowRouters = 27;

  addr = {0, 0};
  ASSERT_EQ(0u, FoldedClos::translateRouterAddressToId(
      halfRadix, numLevels, rowRouters, &addr));

  addr = {0, 1};
  ASSERT_EQ(1u, FoldedClos::translateRouterAddressToId(
      halfRadix, numLevels, rowRouters, &addr));

  addr = {1, 0};
  ASSERT_EQ(27u, FoldedClos::translateRouterAddressToId(
      halfRadix, numLevels, rowRouters, &addr));

  addr = {3, 12};
  ASSERT_EQ(93u, FoldedClos::translateRouterAddressToId(
      halfRadix, numLevels, rowRouters, &addr));
}
