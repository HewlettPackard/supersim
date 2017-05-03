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
#include "traffic/continuous/ContinuousTrafficPattern.h"

#include <factory/Factory.h>

#include <cassert>

ContinuousTrafficPattern::ContinuousTrafficPattern(
    const std::string& _name, const Component* _parent,
    u32 _numTerminals, u32 _self, Json::Value _settings)
    : Component(_name, _parent), numTerminals_(_numTerminals), self_(_self) {
  assert(numTerminals_ > 0);
  assert(self_ < numTerminals_);
}

ContinuousTrafficPattern::~ContinuousTrafficPattern() {}

ContinuousTrafficPattern* ContinuousTrafficPattern::create(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings) {
  // retrieve type
  const std::string& type = _settings["type"].asString();

  // try to construct a traffic pattern
  ContinuousTrafficPattern* tp = factory::Factory<
    ContinuousTrafficPattern, CONTINUOUSTRAFFICPATTERN_ARGS>
      ::create(type, _name, _parent, _numTerminals, _self, _settings);

  // check that the factory had an entry for that type
  if (tp == nullptr) {
    fprintf(stderr, "unknown continuous traffic pattern: %s\n", type.c_str());
    assert(false);
  }
  return tp;
}
