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
#ifndef NETWORK_SLIMFLY_ROUTINGTABLE_H_
#define NETWORK_SLIMFLY_ROUTINGTABLE_H_

#include <prim/prim.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>

namespace SlimFly {

/*
 * This class defines a routing table for a slimfly router.
 * In general the API can work for any netowrk, but the slimfly-specific
 * assumption here is that paths through router will always have a length
 * of 0, 1 or 2.
 */
class RoutingTable {
 public:
  struct PathInfo {
    PathInfo(const std::vector<u32>& _thruAddr, u32 _portNum)
    : thruAddr(_thruAddr), outPortNum(_portNum) {
    }
    const std::vector<u32> thruAddr;
    const u32 outPortNum;
  };

  explicit RoutingTable(const std::vector<u32>& _srcAddr);

  // Register a hop in the table. Called when adding an edge
  // to a network.
  void addHop(u32 srcPort, const std::vector<u32>& dstAddr);

  // Register a path in the table. A path is necessarily 2 hops
  // with the source as this router, destination dstAddr and the
  // intermediate hop thruAddr
  void addPath(
    const std::vector<u32>& dstAddr, const std::vector<u32>& thruAddr);

  // Return the router that this table represents
  inline const std::vector<u32>& getAddr() const {
    return srcAddr_;
  }

  // Return number of hops between this router and dstAddr
  u32 getNumHops(const std::vector<u32>& dstAddr) const;

  // For a valid hop, return the port number given the hop addr
  u32 getPortNum(const std::vector<u32>& hopAddr) const;

  // Return all possible "optimal" paths between the current router
  // and dstAddr. If the destination is a single hop away, only that
  // address will be returned even if there are more paths with >1 hop.
  const std::vector<PathInfo> getPaths(const std::vector<u32>& dstAddr) const;

 private:
  typedef std::unordered_map<std::string, u32> HopMap;
  typedef std::unordered_map<std::string, std::set<std::string> > PathMap;
  typedef std::unordered_map<std::string, std::vector<u32> > AddrMap;

  const std::vector<u32> srcAddr_;
  HopMap hopTable_;
  PathMap pathTable_;
  AddrMap addrMap_;
};

}  // namespace SlimFly
#endif  // NETWORK_SLIMFLY_ROUTINGTABLE_H_
