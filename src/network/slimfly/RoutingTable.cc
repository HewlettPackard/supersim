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
  pathTable_[strop::vecString<u32>(dstAddr)] =
      hopTable_[strop::vecString<u32>(thruAddr)];
}

u32 RoutingTable::getNumHops(const std::vector<u32>& dstAddr) const {
  auto hopIter = hopTable_.find(strop::vecString<u32>(dstAddr));
  if (hopIter != hopTable_.end()) {
    return 1;
  } else {
    auto pathIter = pathTable_.find(strop::vecString<u32>(dstAddr));
    if (pathIter != pathTable_.end()) {
      return 2;
    } else {
      return 0;
    }
  }
}

u32 RoutingTable::getOutPort(const std::vector<u32>& dstAddr) const {
  auto hopIter = hopTable_.find(strop::vecString<u32>(dstAddr));
  if (hopIter != hopTable_.end()) {
    return hopIter->second;
  } else {
    auto pathIter = pathTable_.find(strop::vecString<u32>(dstAddr));
    if (pathIter != pathTable_.end()) {
      return pathIter->second;
    } else {
      assert(false);
      return UINT_MAX;
    }
  }
}

}  // namespace SlimFly
