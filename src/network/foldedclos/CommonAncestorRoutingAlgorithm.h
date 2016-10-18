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
#ifndef NETWORK_FOLDEDCLOS_COMMONANCESTORROUTINGALGORITHM_H_
#define NETWORK_FOLDEDCLOS_COMMONANCESTORROUTINGALGORITHM_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "network/RoutingAlgorithm.h"
#include "router/Router.h"

namespace FoldedClos {

class CommonAncestorRoutingAlgorithm : public RoutingAlgorithm {
 public:
  CommonAncestorRoutingAlgorithm(
      const std::string& _name, const Component* _parent, Router* _router,
      u64 _latency, u32 _baseVc, u32 _numVcs, u32 _numPorts, u32 _numLevels,
      u32 _inputPort, Json::Value _settings);
  ~CommonAncestorRoutingAlgorithm();

 protected:
  void processRequest(
      Flit* _flit, RoutingAlgorithm::Response* _response) override;

 private:
  enum class Mode : uint8_t { ALL, PORT, VC };
  Mode parseMode(const std::string& _mode) const;

  const u32 numPorts_;
  const u32 numLevels_;
  const u32 inputPort_;

  const bool leastCommonAncestor_;
  const Mode mode_;
  const bool adaptive_;
};

}  // namespace FoldedClos

#endif  // NETWORK_FOLDEDCLOS_COMMONANCESTORROUTINGALGORITHM_H_
