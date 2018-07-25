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
#include "traffic/continuous/RandomExchangeNeighborCTP.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include <vector>

#include "network/cube/util.h"

RandomExchangeNeighborCTP::
RandomExchangeNeighborCTP(
    const std::string& _name, const Component* _parent,
    u32 _numTerminals, u32 _self, Json::Value _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  // parse the settings
  assert(_settings.isMember("dimensions") &&
         _settings["dimensions"].isArray());
  assert(_settings.isMember("concentration") &&
         _settings["concentration"].isUInt());
  const u32 dimensions = _settings["dimensions"].size();
  std::vector<u32> widths;
  widths.resize(dimensions);
  for (u32 i = 0; i < dimensions; i++) {
    widths.at(i) = _settings["dimensions"][i].asUInt();
  }
  u32 concentration = _settings["concentration"].asUInt();
  bool allTerminals = _settings["all_terminals"].asBool();

  std::vector<bool> dimMask(dimensions, false);
  if (_settings.isMember("enabled_dimensions") &&
      _settings["enabled_dimensions"].isArray()) {
    for (u32 dim = 0;  dim < dimensions; ++dim) {
      dimMask.at(dim) = _settings["enabled_dimensions"][dim].asBool();
    }
  } else {
    dimMask.at(0) = true;
  }

  assert(dimensions > 1);
  for (u32 i = 0; i < dimensions; i++) {
    assert(widths.at(i) % 2 == 0);
  }

  for (u32 dim = 0; dim < dimensions; ++dim) {
    if (dimMask.at(dim)) {
      std::vector<u32> addr;
      // get self as a vector address
      Cube::translateInterfaceIdToAddress(self_, widths, concentration, &addr);
      addr.at(dim + 1) = (addr.at(dim + 1) + widths.at(dim) - 1)
                         % widths.at(dim);
      if (allTerminals) {
        for (u32 conc = 0; conc < concentration; ++conc) {
          addr.at(0) = conc;
          u32 dstId = Cube::translateInterfaceAddressToId(&addr, widths,
                                                          concentration);
          dstVect_.emplace_back(dstId);
        }
      } else {
        u32 dstId = Cube::translateInterfaceAddressToId(&addr, widths,
                                                        concentration);
        dstVect_.emplace_back(dstId);
      }
      addr.at(dim + 1) = (addr.at(dim + 1) + 2) % widths.at(dim);
      if (allTerminals) {
        for (u32 conc = 0; conc < concentration; ++conc) {
          addr.at(0) = conc;
          u32 dstId = Cube::translateInterfaceAddressToId(&addr, widths,
                                                          concentration);
          dstVect_.emplace_back(dstId);
        }
      } else {
        u32 dstId = Cube::translateInterfaceAddressToId(&addr, widths,
                                                        concentration);
        dstVect_.emplace_back(dstId);
      }
    }
  }
}

RandomExchangeNeighborCTP::
~RandomExchangeNeighborCTP() {}

u32 RandomExchangeNeighborCTP::nextDestination() {
  return dstVect_.at(gSim->rnd.nextU64(0, dstVect_.size() - 1));
}

registerWithObjectFactory(
    "random_exchange_neighbor", ContinuousTrafficPattern,
    RandomExchangeNeighborCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
