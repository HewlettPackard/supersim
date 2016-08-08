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
#include "traffic/DimRotateTrafficPattern.h"

#include <cassert>

#include <vector>

#include "network/cube/util.h"

DimRotateTrafficPattern::DimRotateTrafficPattern(
    const std::string& _name, const Component* _parent,
    u32 _numTerminals, u32 _self, Json::Value _settings)
    : TrafficPattern(_name, _parent, _numTerminals, _self) {
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

  for (u32 i = 1; i < dimensions/2; i++) {
    assert(widths.at(i) == widths.at(dimensions - i - 1));
  }

  assert(_settings.isMember("direction"));
  assert(_settings["direction"].isString());
  std::string dir = _settings["direction"].asString();

  // get self as a vector address
  std::vector<u32> addr;
  Cube::computeTerminalAddress(_self, widths, concentration, &addr);

  if (dir == "left") {
    u32 tmp = addr.at(1);
    for (u32 dim = 1; dim < dimensions; dim++) {
      addr.at(dim) = addr.at(dim + 1);
    }
    addr.at(dimensions) = tmp;
  } else if (dir == "right") {
    u32 tmp = addr.at(dimensions);
    for (u32 dim = dimensions; dim > 1; dim--) {
      addr.at(dim) = addr.at(dim - 1);
    }
    addr.at(1) = tmp;
  } else {
    fprintf(stderr, "invalid direction spec: %s\n", dir.c_str());
    assert(false);
  }

  // compute the tornado destination id
  dest_ = Cube::computeTerminalId(&addr, widths, concentration);
}

DimRotateTrafficPattern::~DimRotateTrafficPattern() {}

u32 DimRotateTrafficPattern::nextDestination() {
  return dest_;
}
