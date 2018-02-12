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
#include "routing/LeastCongestedMinimalReduction.h"

#include <factory/ObjectFactory.h>

#include "congestion/util.h"

LeastCongestedMinimalReduction::LeastCongestedMinimalReduction(
    const std::string& _name, const Component* _parent,
    const PortedDevice* _device, RoutingMode _mode, Json::Value _settings)
    : Reduction(_name, _parent, _device, _mode, _settings) {}

LeastCongestedMinimalReduction::~LeastCongestedMinimalReduction() {}

void LeastCongestedMinimalReduction::process(
    u32 _minHops,
    const std::unordered_set<std::tuple<u32, u32, u32, f64> >& _minimal,
    const std::unordered_set<std::tuple<u32, u32, u32, f64> >& _nonMinimal,
    std::unordered_set<std::tuple<u32, u32> >* _outputs,
    bool* _allMinimal) {
  f64 minCong = F64_POS_INF;
  for (const auto& t : _minimal) {
    f64 cong = std::get<3>(t);
    if (congestionLessThan(cong, minCong)) {
      _outputs->clear();
      _outputs->insert(std::make_tuple(std::get<0>(t), std::get<1>(t)));
      minCong = cong;
    } else if (congestionEqualTo(cong, minCong)) {
      _outputs->insert(std::make_tuple(std::get<0>(t), std::get<1>(t)));
    }
  }
  *_allMinimal = true;
}

registerWithObjectFactory("least_congested_minimal", Reduction,
                          LeastCongestedMinimalReduction, REDUCTION_ARGS);
