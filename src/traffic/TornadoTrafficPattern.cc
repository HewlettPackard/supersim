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
#include "traffic/TornadoTrafficPattern.h"

#include <cassert>

#include <vector>

#include "network/torus/util.h"

TornadoTrafficPattern::TornadoTrafficPattern(
    const std::string& _name, const Component* _parent,
    u32 _numTerminals, u32 _self, Json::Value _settings)
    : PermutationTrafficPattern(_name, _parent, _numTerminals, _self,
                                _settings) {
  // parse the settings
  assert(_settings.isMember("dimensions") &&
         _settings["dimensions"].isArray());
  assert(_settings.isMember("concentration") &&
         _settings["concentration"].isUInt());
  const u32 dimensions = _settings["dimensions"].size();
  std::vector<u32> widths(dimensions);
  for (u32 i = 0; i < dimensions; i++) {
    widths.at(i) = _settings["dimensions"][i].asUInt();
  }
  const u32 concentration = _settings["concentration"].asUInt();

  // get self as a vector address
  std::vector<u32> addr;
  Torus::computeAddress(_self, widths, concentration, &addr);

  // compute the tornado destination vector address
  for (u32 dim = 0; dim < dimensions; dim++) {
    u32 dimOffset = (widths.at(dim) - 1) / 2;
    u32 idx = dim + 1;
    addr.at(idx) = (addr.at(idx) + dimOffset) % widths.at(dim);
  }

  // compute the tornado destination id
  dest_ = Torus::computeId(&addr, widths, concentration);
}

TornadoTrafficPattern::~TornadoTrafficPattern() {}

u32 TornadoTrafficPattern::nextDestination() {
  return dest_;
}
