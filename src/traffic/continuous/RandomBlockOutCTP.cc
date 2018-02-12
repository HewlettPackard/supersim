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
#include "traffic/continuous/RandomBlockOutCTP.h"

#include <factory/ObjectFactory.h>

#include <cassert>

RandomBlockOutCTP::RandomBlockOutCTP(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  // verify settings exist
  assert(_settings.isMember("block_size"));

  // compute fixed values
  blockSize_ = _settings["block_size"].asUInt();
  assert(numTerminals_ % blockSize_ == 0);
  blockBase_ = (self_ / blockSize_) * blockSize_;
}

RandomBlockOutCTP::~RandomBlockOutCTP() {}

u32 RandomBlockOutCTP::nextDestination() {
  u32 valid = numTerminals_ - blockSize_;
  u32 rnd = gSim->rnd.nextU64(0, valid - 1);
  if (rnd >= blockBase_) {
    return rnd + blockSize_;
  } else {
    return rnd;
  }
}

registerWithObjectFactory("random_block_out", ContinuousTrafficPattern,
                          RandomBlockOutCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
