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
#include "network/fattree/RoutingAlgorithm.h"

#include <factory/ObjectFactory.h>

#include <cassert>

namespace FatTree {

RoutingAlgorithm::RoutingAlgorithm(
     const std::string& _name, const Component* _parent,
     Router* _router, u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
     const std::vector<std::tuple<u32, u32, u32> >* _radices,
     Json::Value _settings)
    : ::RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                         _inputVc, _settings),
      radices_(_radices) {}

RoutingAlgorithm::~RoutingAlgorithm() {}

RoutingAlgorithm* RoutingAlgorithm::create(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<std::tuple<u32, u32, u32> >* _radices,
    Json::Value _settings) {
  // retrieve the algorithm
  const std::string& algorithm = _settings["algorithm"].asString();

  // attempt to create the routing algorithm
  RoutingAlgorithm* ra = factory::ObjectFactory<
    RoutingAlgorithm, FATTREE_ROUTINGALGORITHM_ARGS>::create(
        algorithm, _name, _parent, _router, _baseVc, _numVcs, _inputPort,
        _inputVc, _radices, _settings);

  // check that the factory had this type
  if (ra == nullptr) {
    fprintf(stderr, "invalid FatTree routing algorithm: %s\n",
            algorithm.c_str());
    assert(false);
  }
  return ra;
}

}  // namespace FatTree
