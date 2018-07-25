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
#include "traffic/continuous/DimReverseCTP.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include <vector>

#include "network/cube/util.h"

DimReverseCTP::DimReverseCTP(
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

  // get self as a vector address
  std::vector<u32> addr;
  Cube::translateInterfaceIdToAddress(self_, widths, concentration, &addr);

  for (u32 dim = 0; dim < dimensions/2; dim++) {
    u32 tmp = addr.at(dim + 1);
    addr.at(dim + 1) = addr.at(dimensions - dim);
    addr.at(dimensions - dim) = tmp;
  }

  // compute the tornado destination id
  dest_ = Cube::translateInterfaceAddressToId(&addr, widths, concentration);
}

DimReverseCTP::~DimReverseCTP() {}

u32 DimReverseCTP::nextDestination() {
  return dest_;
}

registerWithObjectFactory("dim_reverse", ContinuousTrafficPattern,
                          DimReverseCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
