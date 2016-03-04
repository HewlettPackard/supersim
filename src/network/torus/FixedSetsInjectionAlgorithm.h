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
#ifndef NETWORK_TORUS_FIXEDSETSINJECTIONALGORITHM_H_
#define NETWORK_TORUS_FIXEDSETSINJECTIONALGORITHM_H_

#include <prim/prim.h>

#include <set>
#include <string>

#include "event/Component.h"
#include "network/InjectionAlgorithm.h"
#include "interface/Interface.h"

namespace Torus {

class FixedSetsInjectionAlgorithm : public InjectionAlgorithm {
 public:
  FixedSetsInjectionAlgorithm(
      const std::string& _name, const Component* _parent, Interface* _interface,
      u64 _latency, u32 _numVcs, u32 _numSets, const std::set<u32>& _sets);
  ~FixedSetsInjectionAlgorithm();

 protected:
  void processRequest(
      Message* _message, InjectionAlgorithm::Response* _response) override;

 private:
  const u32 numVcs_;
  const u32 numSets_;
  const std::set<u32> sets_;
};

}  // namespace Torus

#endif  // NETWORK_TORUS_FIXEDSETSINJECTIONALGORITHM_H_
