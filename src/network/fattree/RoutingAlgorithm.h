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
#ifndef NETWORK_FATTREE_ROUTINGALGORITHM_H_
#define NETWORK_FATTREE_ROUTINGALGORITHM_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <tuple>
#include <vector>

#include "event/Component.h"
#include "routing/RoutingAlgorithm.h"
#include "router/Router.h"

#define FATTREE_ROUTINGALGORITHM_ARGS \
  const std::string&, const Component*, Router*, u32, u32, u32, u32,  \
    const std::vector<std::tuple<u32, u32, u32> >*, Json::Value

namespace FatTree {

class RoutingAlgorithm : public ::RoutingAlgorithm {
 public:
  RoutingAlgorithm(
      const std::string& _name, const Component* _parent,
      Router* _router, u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
      const std::vector<std::tuple<u32, u32, u32> >* _radices,
      Json::Value _settings);
  virtual ~RoutingAlgorithm();

  // this is a routing algorithm factory for the fat tree topology
  static RoutingAlgorithm* create(FATTREE_ROUTINGALGORITHM_ARGS);

 protected:
  const std::vector<std::tuple<u32, u32, u32> >* radices_;
};

}  // namespace FatTree

#endif  // NETWORK_FATTREE_ROUTINGALGORITHM_H_
