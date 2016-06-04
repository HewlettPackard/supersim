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
#include <cassert>
#include "traffic/NearestNeighborTrafficPattern.h"

NearestNeighborTrafficPattern::NearestNeighborTrafficPattern(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : AlternatingTrafficPattern(_name, _parent, _numTerminals, _self,
                                _settings) {
  assert(_settings.isMember("range"));
  u32 range = _settings["range"].asUInt();

  lower = (static_cast<int>(self - range) > 0) ?
            static_cast<int>(self - range) : 0;
  upper = ((self + range) < numTerminals) ?
                self + range : numTerminals - 1;
}

NearestNeighborTrafficPattern::~NearestNeighborTrafficPattern() {}

u32 NearestNeighborTrafficPattern::nextDestination() {
  u32 dest;
  do {
    dest = gSim->rnd.nextU64(lower, upper);
  } while (!sendToSelf && dest == self);
  return dest;
}
