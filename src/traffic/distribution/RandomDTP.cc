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
#include "traffic/distribution/RandomDTP.h"

#include <factory/ObjectFactory.h>

#include <cassert>

RandomDTP::RandomDTP(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : DistributionTrafficPattern(_name, _parent, _numTerminals, _self,
                                 _settings) {
  assert(_settings.isMember("send_to_self"));
  sendToSelf_ = _settings["send_to_self"].asBool();

  reset();
}

RandomDTP::~RandomDTP() {}

u32 RandomDTP::size() const {
  if (sendToSelf_) {
    return numTerminals_;
  } else {
    return numTerminals_ - 1;
  }
}

u32 RandomDTP::nextDestination() {
  assert(!destinations_.empty());
  u32 dest = destinations_.back();
  destinations_.pop_back();
  return dest;
}

bool RandomDTP::complete() const {
  return destinations_.empty();
}

void RandomDTP::reset() {
  destinations_.clear();
  for (u32 dest = 0; dest < numTerminals_; dest++) {
    if (sendToSelf_ || dest != self_) {
      destinations_.push_back(dest);
    }
  }
  gSim->rnd.shuffle(&destinations_);
}

registerWithObjectFactory("random", DistributionTrafficPattern,
                          RandomDTP, DISTRIBUTIONTRAFFICPATTERN_ARGS);
