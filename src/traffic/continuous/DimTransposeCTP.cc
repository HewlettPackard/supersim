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
#include "traffic/continuous/DimTransposeCTP.h"

#include <factory/Factory.h>

#include <cassert>

#include <vector>

#include "network/cube/util.h"

DimTransposeCTP::DimTransposeCTP(
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
  assert(dimensions > 1);
  std::vector<u32> widths(dimensions);
  for (u32 i = 0; i < dimensions; i++) {
    widths.at(i) = _settings["dimensions"][i].asUInt();
  }
  const u32 concentration = _settings["concentration"].asUInt();

  std::vector<u32> workingDims(2);
  std::vector<bool> dimMask(dimensions, false);
  if (_settings.isMember("enabled_dimensions") &&
      _settings["enabled_dimensions"].isArray()) {
    u32 trueCntr = 0;
    for (u32 dim = 0;  dim < dimensions; ++dim) {
      dimMask.at(dim) = _settings["enabled_dimensions"][dim].asBool();
      if (dimMask.at(dim) == true) {
        trueCntr++;
      }
    }
    assert(trueCntr == 2);
  } else {
    dimMask.at(0) = true;
    dimMask.at(1) = true;
  }

  u32 idx = 0;
  for (u32 dim = 0;  dim < dimensions; ++dim) {
    if (dimMask.at(dim) == true) {
      workingDims.at(idx) = dim;
      idx++;
    }
  }
  assert(widths.at(workingDims.at(0)) == widths.at(workingDims.at(1)));

  // get self as a vector address
  std::vector<u32> addr;
  Cube::computeTerminalAddress(self_, widths, concentration, &addr);

  u32 idx0 = workingDims.at(0) + 1;
  u32 idx1 = workingDims.at(1) + 1;
  u32 tmp = addr.at(idx0);
  addr.at(idx0) = addr.at(idx1);
  addr.at(idx1) = tmp;

  // compute the tornado destination id
  dest_ = Cube::computeTerminalId(&addr, widths, concentration);
}

DimTransposeCTP::~DimTransposeCTP() {}

u32 DimTransposeCTP::nextDestination() {
  return dest_;
}

registerWithFactory("dim_transpose", ContinuousTrafficPattern,
                    DimTransposeCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
