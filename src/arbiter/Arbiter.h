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
#ifndef ARBITER_ARBITER_H_
#define ARBITER_ARBITER_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"

#define ARBITER_ARGS const std::string&, const Component*, u32, Json::Value

class Arbiter : public Component {
 public:
  // constructor
  Arbiter(const std::string& _name, const Component* _parent, u32 _size,
          Json::Value _settings);
  virtual ~Arbiter();

  // this defines the arbiter factory
  static Arbiter* create(ARBITER_ARGS);

  // returns number of inputs & outputs
  u32 size() const;
  // maps the request to the specified port
  void setRequest(u32 _port, const bool* _request);
  // maps the metadata to the specified port
  void setMetadata(u32 _port, const u64* _metadata);
  // maps the output to the specified port
  void setGrant(u32 _port, bool* _grant);

  // latches any state from the last cycle
  //  override when necessary
  virtual void latch();

  // computes the arbitration logic, must be overridden
  //  should only set grants true (logically OR)
  //  returns the winner, or U32_MAX when nothing granted
  virtual u32 arbitrate() = 0;

 protected:
  std::vector<const bool*> requests_;
  std::vector<const u64*> metadatas_;
  std::vector<bool*> grants_;
  const u32 size_;
};

#endif  // ARBITER_ARBITER_H_
