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
#ifndef ALLOCATOR_RCSEPARABLEALLOCATOR_H_
#define ALLOCATOR_RCSEPARABLEALLOCATOR_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "allocator/Allocator.h"
#include "arbiter/Arbiter.h"
#include "event/Component.h"

class RcSeparableAllocator : public Allocator {
 public:
  RcSeparableAllocator(const std::string& _name, const Component* _parent,
                       u32 _numClients, u32 _numResources,
                       Json::Value _settings);
  ~RcSeparableAllocator();

  void setRequest(u32 _client, u32 _resource, bool* _request) override;
  void setMetadata(u32 _client, u32 _resource, u64* _metadata) override;
  void setGrant(u32 _client, u32 _resource, bool* _grant) override;
  void allocate() override;

 private:
  std::vector<Arbiter*> resourceArbiters_;
  std::vector<Arbiter*> clientArbiters_;

  std::vector<bool*> requests_;
  std::vector<u64*> metadatas_;
  bool* intermediates_;
  std::vector<bool*> grants_;

  u32 iterations_;
  bool slipLatch_;  // iSLIP selective priority latching

  u64 index(u64 _client, u64 _resource) const;
};

#endif  // ALLOCATOR_RCSEPARABLEALLOCATOR_H_
