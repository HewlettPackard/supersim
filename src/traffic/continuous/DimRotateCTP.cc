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
#include "traffic/continuous/DimRotateCTP.h"

#include <factory/Factory.h>

#include <cassert>

#include <vector>

#include "network/cube/util.h"

DimRotateCTP::DimRotateCTP(
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
  Cube::computeTerminalAddress(self_, widths, concentration, &addr);

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

DimRotateCTP::~DimRotateCTP() {}

u32 DimRotateCTP::nextDestination() {
  return dest_;
}

registerWithFactory("dim_rotate", ContinuousTrafficPattern,
                    DimRotateCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
