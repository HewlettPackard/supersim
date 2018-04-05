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
#include "traffic/continuous/LocalRemoteRandomCTP.h"

#include <factory/ObjectFactory.h>

#include <cassert>

LocalRemoteRandomCTP::LocalRemoteRandomCTP(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  // verify settings exist
  assert(_settings.isMember("block_size"));  // num terminals per block
  assert(_settings.isMember("local_probability"));

  // compute fixed values
  blockSize_ = _settings["block_size"].asUInt();
  localProbability_ = _settings["local_probability"].asDouble();

  // verify matching system size
  numBlocks_ = numTerminals_ / blockSize_;
  assert(numBlocks_ > 1);
  assert(numTerminals_ % blockSize_ == 0);
  localBlock_ = self_ / blockSize_;
  const f64 TOLERANCE = 1e-6;
  assert(localProbability_ > (0.0 - TOLERANCE));
  assert(localProbability_ < (1.0 + TOLERANCE));
  allLocal_ = std::abs(localProbability_ - 0.0) < TOLERANCE;
  allRemote_ = std::abs(localProbability_ - 1.0) < TOLERANCE;
}

LocalRemoteRandomCTP::~LocalRemoteRandomCTP() {}

u32 LocalRemoteRandomCTP::nextDestination() {
  // determine if local or remote
  bool local;
  if (allLocal_) {
    local = true;
  } else if (allRemote_) {
    local = false;
  } else {
    local = gSim->rnd.nextF64() < localProbability_;
  }

  // determine destination
  u32 dstBlock;
  if (local) {
    dstBlock = localBlock_;
  } else {
    dstBlock = gSim->rnd.nextU64(0, numBlocks_ - 2);
    if (dstBlock >= localBlock_) {
      dstBlock++;
    }
  }

  u32 dst = dstBlock * blockSize_ + gSim->rnd.nextU64(0, blockSize_ - 1);
  return dst;
}

registerWithObjectFactory("local_remote_random", ContinuousTrafficPattern,
                          LocalRemoteRandomCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
