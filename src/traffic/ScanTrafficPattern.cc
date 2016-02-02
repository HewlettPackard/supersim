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
#include "traffic/ScanTrafficPattern.h"

#include <cassert>

ScanTrafficPattern::ScanTrafficPattern(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : AlternatingTrafficPattern(_name, _parent, _numTerminals, _self,
                                _settings) {
  // set the direction of scan
  assert(_settings.isMember("direction"));
  std::string dir = _settings["direction"].asString();
  if (dir == "ascend") {
    ascend_ = true;
  } else if (dir == "descend") {
    ascend_ = false;
  } else if (dir == "random") {
    ascend_ = gSim->rnd.nextBool();
  } else {
    fprintf(stderr, "invalid direction spec: %s\n", dir.c_str());
    assert(false);
  }

  // set the initial destination
  assert(_settings.isMember("initial"));
  if ((_settings["initial"].isString()) &&
      (_settings["initial"].asString() == "random")) {
    do {
      next_ = gSim->rnd.nextU64(0, numTerminals - 1);
    } while (!sendToSelf && next_ == self);
  } else if ((_settings["initial"].isUInt()) &&
             (_settings["initial"].asUInt() < numTerminals)) {
    next_ = _settings["initial"].asUInt();
    if (next_ == self && !sendToSelf) {
      advance();
      assert(next_ != self);  // is this even possible?
    }
  } else {
    fprintf(stderr, "invalid initial spec\n");
    assert(false);
  }
}

ScanTrafficPattern::~ScanTrafficPattern() {}

u32 ScanTrafficPattern::nextDestination() {
  u32 dest = next_;
  do {
    advance();
  } while (!sendToSelf && next_ == self);
  return dest;
}

void ScanTrafficPattern::advance() {
  if (ascend_) {
    next_ = (next_ + 1) % numTerminals;
  } else {
    next_ = (next_ == 0) ? numTerminals - 1 : next_ - 1;
  }
}
