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
#include "traffic/continuous/GroupAttackCTP.h"

#include <factory/ObjectFactory.h>

#include <cassert>

GroupAttackCTP::GroupAttackCTP(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  // verify settings exist
  assert(_settings.isMember("group_size"));  // num routers per group
  assert(_settings.isMember("concentration"));  // num terminals per router
  assert(_settings.isMember("random"));
  assert(_settings.isMember("mode"));  // format of destination

  // compute fixed values
  groupSize_ = _settings["group_size"].asUInt();
  concentration_ = _settings["concentration"].asUInt();
  groupCount_ = numTerminals_ / (groupSize_ * concentration_);
  random_ = _settings["random"].asBool();

  // verify matching system size
  assert((concentration_ * groupSize_ * groupCount_) == numTerminals_);

  // precompute values needed for destination calculation
  u32 termsPerGroup = groupSize_ * concentration_;
  selfGroup_ = self_ / termsPerGroup;
  u32 localIndex = self_ % termsPerGroup;
  selfLocal_ = localIndex / concentration_;
  selfConc_ = localIndex % concentration_;

  // compute destination from mode format
  if (_settings["mode"].isString()) {
    if (_settings["mode"].asString() == "half") {
      destGroup_ = (selfGroup_ + (groupCount_ / 2)) % groupCount_;
    } else if (_settings["mode"].asString() == "opposite") {
      destGroup_ = (groupCount_ - 1) - selfGroup_;
    }
  } else if (_settings["mode"].isInt()) {
    s32 offset = _settings["mode"].asInt();
    // don't rely on loop around
    assert((u32)abs(offset) < groupCount_);
    s32 destGroup = ((s32)selfGroup_ +
                     ((s32)groupCount_ + offset)) % (s32)groupCount_;
    if (destGroup < 0) {
      destGroup += groupCount_;
    }
    destGroup_ = (u32)destGroup;
  }
  assert(destGroup_ < groupCount_);
}

GroupAttackCTP::~GroupAttackCTP() {}

u32 GroupAttackCTP::nextDestination() {
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

registerWithObjectFactory("group_attack", ContinuousTrafficPattern,
                          GroupAttackCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
