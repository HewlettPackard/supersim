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
#ifndef ALLOCATOR_WAVEFRONTALLOCATOR_H_
#define ALLOCATOR_WAVEFRONTALLOCATOR_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "allocator/Allocator.h"
#include "event/Component.h"

class WavefrontAllocator : public Allocator {
 public:
  WavefrontAllocator(const std::string& _name, const Component* _parent,
                     u32 _numClients, u32 _numResources,
                     Json::Value _settings);
  ~WavefrontAllocator();

  void setRequest(u32 _client, u32 _resource, bool* _request) override;
  void setMetadata(u32 _client, u32 _resource, u64* _metadata) override;
  void setGrant(u32 _client, u32 _resource, bool* _grant) override;
  void allocate() override;

 private:
  enum class PriorityScheme {kSequential, kRandom};

  void toRowCol(u32 _client, u32 _resource, u32* _row, u32* _col) const;
  u32 toIndex(u32 _row, u32 _col) const;
  u32 toRow(u32 _line, u32 _col) const;

  std::vector<bool*> requests_;
  std::vector<bool*> grants_;

  u32 rows_;
  u32 cols_;
  PriorityScheme scheme_;

  std::vector<bool> colGrants_;
  std::vector<bool> rowGrants_;
  u32 startingLine_;
};

#endif  // ALLOCATOR_WAVEFRONTALLOCATOR_H_
