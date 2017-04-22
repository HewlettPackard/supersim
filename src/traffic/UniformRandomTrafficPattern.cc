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
#include "traffic/UniformRandomTrafficPattern.h"

#include <factory/Factory.h>

#include <cassert>

UniformRandomTrafficPattern::UniformRandomTrafficPattern(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : TrafficPattern(_name, _parent, _numTerminals, _self, _settings) {
  assert(_settings.isMember("send_to_self"));
  sendToSelf_ = _settings["send_to_self"].asBool();
}

UniformRandomTrafficPattern::~UniformRandomTrafficPattern() {}

u32 UniformRandomTrafficPattern::nextDestination() {
  u32 dest;
  do {
    dest = gSim->rnd.nextU64(0, numTerminals_ - 1);
  } while (!sendToSelf_ && dest == self_);
  return dest;
}

registerWithFactory("uniform_random", TrafficPattern,
                    UniformRandomTrafficPattern, TRAFFICPATTERN_ARGS);
