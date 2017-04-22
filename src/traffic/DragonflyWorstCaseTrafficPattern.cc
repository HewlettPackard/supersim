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
#include "traffic/DragonflyWorstCaseTrafficPattern.h"

#include <factory/Factory.h>

#include <cassert>

DragonflyWorstCaseTrafficPattern::DragonflyWorstCaseTrafficPattern(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : TrafficPattern(_name, _parent, _numTerminals, _self, _settings) {
  // verify settings exist
  assert(_settings.isMember("group_count"));
  assert(_settings.isMember("group_size"));
  assert(_settings.isMember("concentration"));
  assert(_settings.isMember("random"));

  // compute fixed values
  groupCount_ = _settings["group_count"].asUInt();
  groupSize_ = _settings["group_size"].asUInt();
  concentration_ = _settings["concentration"].asUInt();
  random_ = _settings["random"].asBool();

  // verify matching system size
  assert((concentration_ * groupSize_ * groupCount_) == numTerminals_);

  // precompute values needed for destination calculation
  u32 termsPerGroup = groupSize_ * concentration_;
  selfGroup_ = self_ / termsPerGroup;
  u32 localIndex = self_ % termsPerGroup;
  selfLocal_ = localIndex / concentration_;
  selfConc_ = localIndex % concentration_;
  destGroup_ = (selfGroup_ + (groupCount_ / 2)) % groupCount_;
}

DragonflyWorstCaseTrafficPattern::~DragonflyWorstCaseTrafficPattern() {}

u32 DragonflyWorstCaseTrafficPattern::nextDestination() {
  // determine dest address values
  u32 destLocal;
  u32 destConc;
  if (random_) {
    u32 localIndex = gSim->rnd.nextU64(0, groupSize_ * concentration_ - 1);
    destLocal = localIndex / concentration_;
    destConc = localIndex % concentration_;
  } else {
    destLocal = selfLocal_;
    destConc = selfConc_;
  }

  // reconstruct dest id
  return ((destGroup_ * groupSize_ * concentration_) +
          (destLocal * concentration_) +
          (destConc));
}

registerWithFactory("dragonfly_worstcase", TrafficPattern,
                    DragonflyWorstCaseTrafficPattern, TRAFFICPATTERN_ARGS);
