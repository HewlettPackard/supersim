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
#ifndef ARBITER_DUALSTAGECLASSARBITER_H_
#define ARBITER_DUALSTAGECLASSARBITER_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "arbiter/Arbiter.h"
#include "event/Component.h"

// groups requestors into classes for first stage of arbitration then
//  uses stage one output to drive stage two arbiter
class DualStageClassArbiter : public Arbiter {
 public:
  DualStageClassArbiter(const std::string& _name, const Component* _parent,
                        u32 _size, Json::Value _settings);
  ~DualStageClassArbiter();

  void setMetadata(u32 _port, const u64* _metadata) override;
  void setGrant(u32 _port, bool* _grant) override;

  void latch() override;
  u32 arbitrate() override;

 private:
  enum class MetadataFunc {NONE, MIN, MAX};

  MetadataFunc metadataFunc_;
  u32 numClasses_;
  u32 numGroups_;
  std::vector<u32> map_;

  bool* stage1Requests_;
  u64* stage1Metadatas_;
  bool* stage1Grants_;

  bool* stage2Requests_;

  Arbiter* stage1Arbiter_;
  Arbiter* stage2Arbiter_;
};

#endif  // ARBITER_DUALSTAGECLASSARBITER_H_
