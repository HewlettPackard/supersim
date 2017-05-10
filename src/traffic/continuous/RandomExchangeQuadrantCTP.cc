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
#include "traffic/continuous/RandomExchangeQuadrantCTP.h"

#include <factory/Factory.h>

#include <cassert>

#include <vector>

#include "network/cube/util.h"

RandomExchangeQuadrantCTP::
RandomExchangeQuadrantCTP(
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

  assert(dimensions > 1);
  for (u32 i = 0; i < dimensions; i++) {
    assert(widths.at(i) % 2 == 0);
  }

  std::vector<u32> addr;
  // get self as a vector address
  Cube::computeTerminalAddress(self_, widths, concentration, &addr);

  u32 selfQuadrant = 0;
  for (u32 i = 0; i < dimensions; ++i) {
    if (addr.at(i + 1) >= widths.at(i) / 2) {
      selfQuadrant += (1 << i);
    }
  }

  for (u32 dstIdx = 0; dstIdx < numTerminals_; ++dstIdx) {
    std::vector<u32> dstAddr;
    Cube::computeTerminalAddress(dstIdx, widths, concentration, &dstAddr);
    u32 dstQuadrant = 0;
    for (u32 i = 0; i < dimensions; ++i) {
      if (dstAddr.at(i + 1) >= widths.at(i) / 2) {
        dstQuadrant += (1 << i);
      }
    }
    u32 numQuadrants = (1 << dimensions) - 1;
    if ((dstQuadrant + selfQuadrant) == numQuadrants) {
      dstVect_.emplace_back(dstIdx);
    }
  }
}

RandomExchangeQuadrantCTP::
~RandomExchangeQuadrantCTP() {}

u32 RandomExchangeQuadrantCTP::nextDestination() {
  return dstVect_.at(gSim->rnd.nextU64(0, dstVect_.size() - 1));
}

registerWithFactory("random_exchange_quadrant", ContinuousTrafficPattern,
                    RandomExchangeQuadrantCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
