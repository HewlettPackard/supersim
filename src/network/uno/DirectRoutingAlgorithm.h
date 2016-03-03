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
#ifndef NETWORK_UNO_DIRECTROUTINGALGORITHM_H_
#define NETWORK_UNO_DIRECTROUTINGALGORITHM_H_

#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "network/RoutingAlgorithm.h"
#include "router/Router.h"

namespace Uno {

class DirectRoutingAlgorithm : public RoutingAlgorithm {
 public:
  DirectRoutingAlgorithm(const std::string& _name, const Component* _parent,
                         Router* _router, u64 _latency, u32 _numVcs,
                         u32 _concentration);
  ~DirectRoutingAlgorithm();

 protected:
  void processRequest(
      Flit* _flit, RoutingAlgorithm::Response* _response) override;

 private:
  const u32 numVcs_;
  const u32 concentration_;
};

}  // namespace Uno

#endif  // NETWORK_UNO_DIRECTROUTINGALGORITHM_H_
