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
#include "arbiter/DualStageClassArbiter.h"

#include <factory/ObjectFactory.h>

#include <cassert>
#include <cstring>

DualStageClassArbiter::DualStageClassArbiter(
    const std::string& _name, const Component* _parent, u32 _size,
    Json::Value _settings)
    : Arbiter(_name, _parent, _size, _settings) {
  // parse the classes settings to get stage 1 size and class assignments
  assert(_settings.isMember("classes") &&
         _settings["classes"].isUInt());
  numClasses_ = _settings["classes"].asUInt();
  assert(numClasses_ > 0);
  assert(_settings.isMember("class_map") &&
         _settings["class_map"].isArray());
  numGroups_ = _settings["class_map"].size();
  assert(size_ % numGroups_ == 0);
  std::vector<u32> classes(numGroups_, U32_MAX);
  for (u32 group = 0; group < numGroups_; group++) {
    u32 groupClass = _settings["class_map"][group].asUInt();
    assert(groupClass < numClasses_);
    classes.at(group) = groupClass;
  }

  // determine the method for computing the metadata for stage 1
  assert(_settings.isMember("metadata_func") &&
         _settings["metadata_func"].isString());
  std::string metadataFunc = _settings["metadata_func"].asString();
  if (metadataFunc == "none") {
    metadataFunc_ = DualStageClassArbiter::MetadataFunc::NONE;
  } else if (metadataFunc == "min") {
    metadataFunc_ = DualStageClassArbiter::MetadataFunc::MIN;
  } else if (metadataFunc == "max") {
    metadataFunc_ = DualStageClassArbiter::MetadataFunc::MAX;
  } else {
    fprintf(stderr, "invalid metadata function: %s\n",
            metadataFunc.c_str());
    assert(false);
  }

  // create client->class mapping
  map_.resize(size_, U32_MAX);
  for (u32 client = 0; client < size_; client++) {
    u32 clientGroup = client % numGroups_;
    map_.at(client) = classes.at(clientGroup);
  }

  // create the stage1 arrays
  stage1Requests_ = new bool[numClasses_];
  stage1Metadatas_ = new u64[numClasses_];
  stage1Grants_ = new bool[numClasses_];

  // create the stage2 arrays
  stage2Requests_ = new bool[size_];

  // create the stage 1 arbiter
  stage1Arbiter_ = Arbiter::create(
      "Stage1Arbiter", this, numClasses_, _settings["stage1_arbiter"]);

  // link the stage 1 arbiter to the internal inputs and outputs
  for (u32 classs = 0; classs < numClasses_; classs++) {
    stage1Arbiter_->setRequest(classs, &stage1Requests_[classs]);
    stage1Arbiter_->setMetadata(classs, &stage1Metadatas_[classs]);
    stage1Arbiter_->setGrant(classs, &stage1Grants_[classs]);
  }

  // create the stage 2 arbiter
  stage2Arbiter_ = Arbiter::create(
      "Stage2Arbiter", this, size_, _settings["stage2_arbiter"]);

  // line the stage 2 arbiter inputs (outputs happen later)
  for (u32 client = 0; client < size_; client++) {
    stage2Arbiter_->setRequest(client, &stage2Requests_[client]);
  }
}

DualStageClassArbiter::~DualStageClassArbiter() {
  delete[] stage1Requests_;
  delete[] stage1Metadatas_;
  delete[] stage1Grants_;

  delete[] stage2Requests_;

  delete stage1Arbiter_;
  delete stage2Arbiter_;
}

void DualStageClassArbiter::setMetadata(u32 _client, const u64* _metadata) {
  Arbiter::setMetadata(_client, _metadata);
  stage2Arbiter_->setMetadata(_client, metadatas_[_client]);
}

void DualStageClassArbiter::setGrant(u32 _client, bool* _grant) {
  Arbiter::setGrant(_client, _grant);
  stage2Arbiter_->setGrant(_client, grants_[_client]);
}

void DualStageClassArbiter::latch() {
  stage1Arbiter_->latch();
  stage2Arbiter_->latch();
}

u32 DualStageClassArbiter::arbitrate() {
  // create requests and metadatas for the stage 1 arbiter
  memset(stage1Requests_, false, sizeof(bool) * numClasses_);
  for (u32 client = 0; client < size_; client++) {
    // printf("client %u [class=%u]: request=%u metadata=%lu\n",
    //        client, map_.at(client % numGroups_), *requests_[client],
    //        *metadatas_[client]);
    if (*requests_[client]) {
      u32 clientGroup = client % numGroups_;
      u32 clientClass = map_.at(clientGroup);
      // handle the metadata comparison
      if (stage1Requests_[clientClass]) {
        switch (metadataFunc_) {
          case DualStageClassArbiter::MetadataFunc::NONE:
            break;
          case DualStageClassArbiter::MetadataFunc::MIN:
            stage1Metadatas_[clientClass] = std::min(
                *metadatas_[client], stage1Metadatas_[clientClass]);
            break;
          case DualStageClassArbiter::MetadataFunc::MAX:
            stage1Metadatas_[clientClass] = std::max(
                *metadatas_[client], stage1Metadatas_[clientClass]);
            break;
          default:
            assert(false);
        }
      } else {
        // just take the client's metadata the first time
        stage1Metadatas_[clientClass] = *metadatas_[client];
      }
      // if any client in group is requesting, then client group is too!
      stage1Requests_[clientClass] = true;
    }
  }

  // run stage 1 arbiter
  memset(stage1Grants_, false, sizeof(bool) * numClasses_);
  stage1Arbiter_->arbitrate();

  for (u32 classs = 0; classs < numClasses_; classs++) {
    // printf("class %u grant = %u\n", classs, stage1Grants_[classs]);
  }

  // set the requests for stage 2
  //  only clients that won stage 1 arbitration can request stage 2
  for (u32 client = 0; client < size_; client++) {
    u32 clientGroup = client % numGroups_;
    u32 clientClass = map_.at(clientGroup);
    stage2Requests_[client] = stage1Grants_[clientClass] && *requests_[client];
    // printf("stage2 request %u [class=%u]: = %u -> %u\n",
    //        client, map_.at(client % numGroups_), *requests_[client],
    //        stage2Requests_[client]);
  }

  // run stage 2 arbiter
  u32 winner = stage2Arbiter_->arbitrate();
  // printf("winner is client %u\n", winner);
  // for (u32 client = 0; client < size_; client++) {
  //   printf("stage2 grant %u = %u\n", client, *grants_[client]);
  // }
  // printf("\n");

  return winner;
}

registerWithObjectFactory("dual_stage_class", Arbiter,
                    DualStageClassArbiter, ARBITER_ARGS);
