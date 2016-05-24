/*
 * Copyright 2016 Ashish Chaudhari, Franky Romero, Nehal Bhandari, Wasam Altoyan
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

#include "network/slimfly/RoutingTable.h"
#include <strop/strop.h>
#include <cassert>

namespace SlimFly {

RoutingTable::RoutingTable(const std::vector<u32>& _srcAddr)
  : srcAddr_(_srcAddr) {
}

void RoutingTable::addHop(
    u32 srcPort, const std::vector<u32>& dstAddr) {
  hopTable_[strop::vecString<u32>(dstAddr)] = srcPort;
}

void RoutingTable::addPath(
  const std::vector<u32>& dstAddr, const std::vector<u32>& thruAddr) {
  assert(hopTable_.count(strop::vecString<u32>(thruAddr)));

  std::string dstAddrStr = strop::vecString<u32>(dstAddr);
  if (pathTable_.count(dstAddrStr) == 0) {
    pathTable_[dstAddrStr] = std::set<std::string>();
  }
  addrMap_[strop::vecString<u32>(thruAddr)] = thruAddr;
  pathTable_.at(dstAddrStr).insert(strop::vecString<u32>(thruAddr));
}

u32 RoutingTable::getNumHops(const std::vector<u32>& dstAddr) const {
  if (hopTable_.count(strop::vecString<u32>(dstAddr))) {
    return 1;   // hop table by definition contains nodes 1 hop away
  } else {
    if (pathTable_.count(strop::vecString<u32>(dstAddr))) {
      return 2;   // max diameter == 2 so if not in hop table then here
    } else {
      return 0;
    }
  }
}

u32 RoutingTable::getPortNum(const std::vector<u32>& hopAddr) const {
  auto hopIter = hopTable_.find(strop::vecString<u32>(hopAddr));
  assert(hopIter != hopTable_.end());
  return hopIter->second;
}

const std::vector<RoutingTable::PathInfo> RoutingTable::getPaths(
    const std::vector<u32>& dstAddr) const {
  std::string dstAddrStr = strop::vecString<u32>(dstAddr);
  if (hopTable_.count(dstAddrStr)) {
    return std::vector<PathInfo>(1,
      PathInfo(dstAddr, hopTable_.at(dstAddrStr)));
  } else if (pathTable_.count(dstAddrStr)) {
    std::vector<PathInfo> retVal;
    for (auto thruAddr : pathTable_.at(dstAddrStr)) {
      retVal.push_back(PathInfo(
        addrMap_.find(thruAddr)->second, hopTable_.find(thruAddr)->second));
    }
    return retVal;
  } else {
    return std::vector<PathInfo>();
  }
}


}  // namespace SlimFly
