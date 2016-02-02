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
#ifndef NETWORK_FOLDEDCLOS_MCAROUTINGFUNCTION_H_
#define NETWORK_FOLDEDCLOS_MCAROUTINGFUNCTION_H_

#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "network/RoutingFunction.h"
#include "router/Router.h"

namespace FoldedClos {

class McaRoutingFunction : public RoutingFunction {
 public:
  McaRoutingFunction(const std::string& _name, const Component* _parent,
                     u64 _latency, Router* _router, u32 _numVcs, u32 _numPorts,
                     u32 _numLevels, u32 _level, u32 _inputPort, bool _allVcs);
  ~McaRoutingFunction();

 protected:
  void processRequest(
      Flit* _flit, RoutingFunction::Response* _response) override;

 private:
  Router* router_;
  u32 numVcs_;
  u32 numPorts_;
  u32 numLevels_;
  u32 level_;
  u32 inputPort_;
  bool allVcs_;
};

}  // namespace FoldedClos

#endif  // NETWORK_FOLDEDCLOS_MCAROUTINGFUNCTION_H_
