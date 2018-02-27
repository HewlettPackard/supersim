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
#include "network/dragonfly/util.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include <vector>


TEST(Dragonfly, computeOffset) {
  // u32 computeOffset(u32 _source, u32 _destination, u32 _width)
  u32 width;
  u32 src;
  u32 dst;

  src = 0;
  dst = 2;
  width = 3;
  ASSERT_EQ(2u, Dragonfly::computeOffset(src, dst, width));

  src = 2;
  dst = 1;
  width = 3;
  ASSERT_EQ(2u, Dragonfly::computeOffset(src, dst, width));

  src = 2;
  dst = 0;
  width = 4;
  ASSERT_EQ(2u, Dragonfly::computeOffset(src, dst, width));

  src = 0;
  dst = 0;
  width = 3;
  ASSERT_EQ(0u, Dragonfly::computeOffset(src, dst, width));
}

TEST(Dragonfly, computeLocalSrcPort) {
  /*
    u32 computeLocalSrcPort(u32 _portBase, u32 _offset, u32 _localWeight,
    u32 _weight)
  */
  u32 portBase = 4;
  u32 localWeight = 2;

  u32 offset;
  u32 weight;

  offset = 1;
  weight = 0;
  ASSERT_EQ(4u, Dragonfly::computeLocalSrcPort(portBase, offset, localWeight,
                                               weight));
  offset = 2;
  weight = 0;
  ASSERT_EQ(6u, Dragonfly::computeLocalSrcPort(portBase, offset, localWeight,
                                               weight));

  offset = 1;
  weight = 1;
  ASSERT_EQ(5u, Dragonfly::computeLocalSrcPort(portBase, offset, localWeight,
                                               weight));

  offset = 2;
  weight = 1;
  ASSERT_EQ(7u, Dragonfly::computeLocalSrcPort(portBase, offset, localWeight,
                                               weight));
}

TEST(Dragonfly, computeLocalDstPort) {
  /*
    u32 computeLocalDstPort(u32 _portBase, u32 _offset, u32 _localWidth,
    u32 _localWeight, u32 _weight)
  */
  u32 portBase = 4;
  u32 localWeight = 2;
  u32 localWidth = 3;

  u32 offset;
  u32 weight;

  offset = 1;
  weight = 0;
  ASSERT_EQ(6u, Dragonfly::computeLocalDstPort(portBase, offset, localWidth,
                                               localWeight, weight));
  offset = 1;
  weight = 1;
  ASSERT_EQ(7u, Dragonfly::computeLocalDstPort(portBase, offset, localWidth,
                                               localWeight, weight));
  offset = 2;
  weight = 0;
  ASSERT_EQ(4u, Dragonfly::computeLocalDstPort(portBase, offset, localWidth,
                                               localWeight, weight));
  offset = 2;
  weight = 1;
  ASSERT_EQ(5u, Dragonfly::computeLocalDstPort(portBase, offset, localWidth,
                                               localWeight, weight));
}
TEST(Dragonfly, computeGlobalToRouterMap) {
  /*
    computeGlobalToRouterMap(u32 _routerGlobalPortBase,
    u32 _globalPortsPerRouter,
    u32 _globalWidth, u32 _globalWeight,
    u32 _localWidth,
    u32 _thisGlobalWeight, u32 _thisGlobalOffset,
    u32* _globalPort, u32* _localRouter,
    u32* _localPort)
  */
  u32 routerGlobalPortBase = 8;
  u32 localWidth = 3;
  u32 globalWidth = 4;
  u32 globalWeight = 2;
  u32 globalPortsPerRouter = 2;

  u32 globalPort;
  u32 localRouter;
  u32 localPort;
  u32 w, o;

  w = 0;
  o = 1;
  Dragonfly::computeGlobalToRouterMap(routerGlobalPortBase,
                                      globalPortsPerRouter,
                                      globalWidth,
                                      globalWeight, localWidth, w, o,
                                      &globalPort, &localRouter, &localPort);
  ASSERT_EQ(globalPort, 0u);
  ASSERT_EQ(localRouter, 0u);
  ASSERT_EQ(localPort, 8u);

  w = 1;
  o = 1;
  Dragonfly::computeGlobalToRouterMap(routerGlobalPortBase,
                                      globalPortsPerRouter,
                                      globalWidth,
                                      globalWeight, localWidth, w, o,
                                      &globalPort, &localRouter, &localPort);
  ASSERT_EQ(globalPort, 3u);
  ASSERT_EQ(localRouter, 1u);
  ASSERT_EQ(localPort, 9u);


  w = 0;
  o = 2;
  Dragonfly::computeGlobalToRouterMap(routerGlobalPortBase,
                                      globalPortsPerRouter,
                                      globalWidth,
                                      globalWeight, localWidth, w, o,
                                      &globalPort, &localRouter, &localPort);
  ASSERT_EQ(globalPort, 1u);
  ASSERT_EQ(localRouter, 0u);
  ASSERT_EQ(localPort, 9u);

  w = 0;
  o = 3;
  Dragonfly::computeGlobalToRouterMap(routerGlobalPortBase,
                                      globalPortsPerRouter,
                                      globalWidth,
                                      globalWeight, localWidth, w, o,
                                      &globalPort, &localRouter, &localPort);
  ASSERT_EQ(globalPort, 2u);
  ASSERT_EQ(localRouter, 1u);
  ASSERT_EQ(localPort, 8u);


  w = 1;
  o = 2;
  Dragonfly::computeGlobalToRouterMap(routerGlobalPortBase,
                                      globalPortsPerRouter,
                                      globalWidth,
                                      globalWeight, localWidth, w, o,
                                      &globalPort, &localRouter, &localPort);
  ASSERT_EQ(globalPort, 4u);
  ASSERT_EQ(localRouter, 2u);
  ASSERT_EQ(localPort, 8u);

  w = 1;
  o = 3;
  Dragonfly::computeGlobalToRouterMap(routerGlobalPortBase,
                                      globalPortsPerRouter,
                                      globalWidth,
                                      globalWeight, localWidth, w, o,
                                      &globalPort, &localRouter, &localPort);
  ASSERT_EQ(globalPort, 5u);
  ASSERT_EQ(localRouter, 2u);
  ASSERT_EQ(localPort, 9u);
}

TEST(Dragonfly, translateInterfaceIdToAddress) {
  /*
    void translateInterfaceIdToAddress(
    u32 _concentration, u32 _localWidth,
    u32 _id, std::vector<u32>* _address);
  */
  u32 localWidth = 3;
  u32 conc = 4;
  u32 id;
  std::vector<u32> address;
  std::vector<u32> exp;

  id = 16;
  exp = {0, 1, 1};
  Dragonfly::translateInterfaceIdToAddress(conc, localWidth, id, &address);
  ASSERT_EQ(address, exp);

  id = 0;
  exp = {0, 0, 0};
  Dragonfly::translateInterfaceIdToAddress(conc, localWidth, id, &address);
  ASSERT_EQ(address, exp);

  id = 47;
  exp = {3, 2, 3};
  Dragonfly::translateInterfaceIdToAddress(conc, localWidth, id, &address);
  ASSERT_EQ(address, exp);

  id = 30;
  exp = {2, 1, 2};
  Dragonfly::translateInterfaceIdToAddress(conc, localWidth, id, &address);
  ASSERT_EQ(address, exp);
}

TEST(Dragonfly, translateInterfaceAddressToId) {
  /*
    u32 translateInterfaceAddressToId(
    u32 _concentration, u32 _localWidth,
    const std::vector<u32>* _address);
  */
  u32 localWidth = 3;
  u32 conc = 4;
  std::vector<u32> address;

  address = {0, 1, 3};
  ASSERT_EQ(40u, Dragonfly::translateInterfaceAddressToId(conc, localWidth,
                                                          &address));

  address = {1, 0, 2};
  ASSERT_EQ(25u, Dragonfly::translateInterfaceAddressToId(conc, localWidth,
                                                          &address));

  address = {2, 2, 1};
  ASSERT_EQ(22u, Dragonfly::translateInterfaceAddressToId(conc, localWidth,
                                                          &address));

  address = {0, 0, 0};
  ASSERT_EQ(0u, Dragonfly::translateInterfaceAddressToId(conc, localWidth,
                                                         &address));

  address = {3, 2, 3};
  ASSERT_EQ(47u, Dragonfly::translateInterfaceAddressToId(conc, localWidth,
                                                          &address));
}

TEST(Dragonfly, translateRouterIdToAddress) {
  /*
    void translateRouterIdToAddress(
    u32 _localWidth,
    u32 _id, std::vector<u32>* _address);
  */
  u32 localWidth = 3;
  u32 id;
  std::vector<u32> address;
  std::vector<u32> exp;

  id = 10;
  exp = {1, 3};
  Dragonfly::translateRouterIdToAddress(localWidth, id, &address);
  ASSERT_EQ(address, exp);

  id = 0;
  exp = {0, 0};
  Dragonfly::translateRouterIdToAddress(localWidth, id, &address);
  ASSERT_EQ(address, exp);

  id = 3;
  exp = {0, 1};
  Dragonfly::translateRouterIdToAddress(localWidth, id, &address);
  ASSERT_EQ(address, exp);

  id = 8;
  exp = {2, 2};
  Dragonfly::translateRouterIdToAddress(localWidth, id, &address);
  ASSERT_EQ(address, exp);

  id = 6;
  exp = {0, 2};
  Dragonfly::translateRouterIdToAddress(localWidth, id, &address);
  ASSERT_EQ(address, exp);
}

TEST(Dragonfly, translateRouterAddressToId) {
  /*
    u32 translateRouterAddressToId(
    u32 _localWidth,
    const std::vector<u32>* _address);
  */
  u32 localWidth = 3;
  std::vector<u32> address;

  address = {0, 0};
  ASSERT_EQ(0u, Dragonfly::translateRouterAddressToId(localWidth,
                                                      &address));
  address = {1, 1};
  ASSERT_EQ(4u, Dragonfly::translateRouterAddressToId(localWidth,
                                                      &address));
  address = {0, 2};
  ASSERT_EQ(6u, Dragonfly::translateRouterAddressToId(localWidth,
                                                      &address));
  address = {2, 3};
  ASSERT_EQ(11u, Dragonfly::translateRouterAddressToId(localWidth,
                                                       &address));
}

TEST(Dragonfly, computeMinimalHops) {
  /*
    u32 computeMinimalHops(const std::vector<u32>* _source,
    const std::vector<u32>* _destination,
    u32 _globalWidth, u32 _globalWeight,
    u32 _routerGlobalPortBase,
    u32 _localWidth);
  */
  std::vector<u32> src;
  std::vector<u32> dst;
  u32 globalWidth = 33;
  u32 globalWeight = 1;
  u32 localWidth = 8;
  u32 routerPortBase = 11;
  u32 concentration = 4;
  u32 globalPortsPerRouter = 4;

  Dragonfly::translateInterfaceIdToAddress(concentration, localWidth, 1038,
                                           &src);
  std::vector<u32> expSrc({2, 3, 32});
  ASSERT_EQ(src, expSrc);
  Dragonfly::translateInterfaceIdToAddress(concentration, localWidth, 15, &dst);
  std::vector<u32> expDst({3, 3, 0});
  ASSERT_EQ(dst, expDst);
  ASSERT_EQ(4u, Dragonfly::computeMinimalHops(&src, &dst, globalWidth,
                                              globalWeight,
                                              routerPortBase,
                                              globalPortsPerRouter,
                                              localWidth));
  globalWidth = 4;
  globalWeight = 2;
  localWidth = 3;
  routerPortBase = 8;
  globalPortsPerRouter = 2;

  // same group
  // same router
  src = {0, 2, 0};
  dst = {1, 2, 0};
  ASSERT_EQ(1u, Dragonfly::computeMinimalHops(&src, &dst, globalWidth,
                                              globalWeight,
                                              routerPortBase,
                                              globalPortsPerRouter,
                                              localWidth));
  // diff router
  src = {0, 2, 0};
  dst = {1, 0, 0};
  ASSERT_EQ(2u, Dragonfly::computeMinimalHops(&src, &dst, globalWidth,
                                              globalWeight,
                                              routerPortBase,
                                              globalPortsPerRouter,
                                              localWidth));
  // diff group
  // same exiting router | same entering router
  src = {0, 2, 0};
  dst = {0, 1, 3};
  ASSERT_EQ(2u, Dragonfly::computeMinimalHops(&src, &dst, globalWidth,
                                              globalWeight,
                                              routerPortBase,
                                              globalPortsPerRouter,
                                              localWidth));
  // same exiting router | diff entering router
  src = {0, 0, 2};
  dst = {0, 0, 3};
  ASSERT_EQ(3u, Dragonfly::computeMinimalHops(&src, &dst, globalWidth,
                                              globalWeight,
                                              routerPortBase,
                                              globalPortsPerRouter,
                                              localWidth));
  // diff exiting router | same entering router
  src = {0, 2, 2};
  dst = {0, 1, 3};
  ASSERT_EQ(3u, Dragonfly::computeMinimalHops(&src, &dst, globalWidth,
                                              globalWeight,
                                              routerPortBase,
                                              globalPortsPerRouter,
                                              localWidth));
  // diff exiting router | diff entering router
  src = {0, 1, 0};
  dst = {0, 1, 2};
  ASSERT_EQ(4u, Dragonfly::computeMinimalHops(&src, &dst, globalWidth,
                                              globalWeight,
                                              routerPortBase,
                                              globalPortsPerRouter,
                                              localWidth));
}
