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
#include "traffic/continuous/LocalRandomRemoteAttackCTP.h"

#include <factory/ObjectFactory.h>

#include <cassert>

LocalRandomRemoteAttackCTP::LocalRandomRemoteAttackCTP(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  // verify settings exist
  assert(_settings.isMember("block_size"));  // num terminals per block
  assert(_settings.isMember("local_probability"));
  assert(_settings.isMember("remote_mode"));

  // compute fixed values
  blockSize_ = _settings["block_size"].asUInt();
  localProbability_ = _settings["local_probability"].asDouble();

  // verify matching system size
  numBlocks_ = numTerminals_ / blockSize_;
  assert(numBlocks_ > 1);
  assert(numTerminals_ % blockSize_ == 0);
  localBlock_ = self_ / blockSize_;
  assert(localProbability_ > 0.0);
  assert(localProbability_ < 1.0);

  // compute destination from mode format
  if (_settings["remote_mode"].isString()) {
    if (_settings["remote_mode"].asString() == "half") {
      remoteBlock_ = (localBlock_ + (numBlocks_ / 2)) % numBlocks_;
    } else if (_settings["remote_mode"].asString() == "opposite") {
      remoteBlock_ = (numBlocks_ - 1) - localBlock_;
    }
  } else if (_settings["remote_mode"].isInt()) {
    s32 offset = _settings["remote_mode"].asInt();
    // don't rely on loop around
    assert((u32)abs(offset) < numBlocks_);
    s32 remoteBlock = ((s32)localBlock_ +
                     ((s32)numBlocks_ + offset)) % (s32)numBlocks_;
    if (remoteBlock < 0) {
      remoteBlock += numBlocks_;
    }
    remoteBlock_ = (u32)remoteBlock;
  }
  assert(remoteBlock_ != localBlock_);
  assert(remoteBlock_ < numBlocks_);
}

LocalRandomRemoteAttackCTP::~LocalRandomRemoteAttackCTP() {}

u32 LocalRandomRemoteAttackCTP::nextDestination() {
  // determine if local or remote
  bool local = gSim->rnd.nextF64() < localProbability_;

  // determine destination
  u32 dstBlock;
  if (local) {
    dstBlock = localBlock_;
  } else {
    dstBlock = remoteBlock_;
  }

  u32 dst = dstBlock * blockSize_ + gSim->rnd.nextU64(0, blockSize_ - 1);
  return dst;
}

registerWithObjectFactory(
    "local_random_remote_attack", ContinuousTrafficPattern,
    LocalRandomRemoteAttackCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
