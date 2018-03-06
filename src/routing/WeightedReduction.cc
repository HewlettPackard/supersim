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
#include "routing/WeightedReduction.h"

#include <factory/FunctionFactory.h>
#include <factory/ObjectFactory.h>

#include <cassert>

#include "congestion/util.h"

WeightedReduction::WeightedReduction(
    const std::string& _name, const Component* _parent,
    const PortedDevice* _device, RoutingMode _mode, bool _ignoreDuplicates,
    Json::Value _settings)
    : Reduction(_name, _parent, _device, _mode, _ignoreDuplicates, _settings),
      congestionBias_(_settings["congestion_bias"].asDouble()),
      independentBias_(_settings["independent_bias"].asDouble()),
      nonMinWeightFunc_(retrieveNonMinimalWeightFunc(
          _settings["non_minimal_weight_func"].asString())) {
  // check inputs
  assert(!_settings["congestion_bias"].isNull());
  assert(!_settings["independent_bias"].isNull());
  assert(nonMinWeightFunc_ != nullptr);
}

WeightedReduction::~WeightedReduction() {}

void WeightedReduction::process(
    u32 _minHops,
    const std::unordered_set<std::tuple<u32, u32, u32, f64> >& _minimal,
    const std::unordered_set<std::tuple<u32, u32, u32, f64> >& _nonMinimal,
    std::unordered_set<std::tuple<u32, u32> >* _outputs,
    bool* _allMinimal) {
  // find the minimally weighted options
  f64 minWeight = F64_MAX;

  // search minimal first
  f64 minCongestion = F64_MAX;
  for (const auto& t : _minimal) {
    f64 weight = std::get<3>(t) * _minHops;

    if (congestionLessThan(weight, minWeight)) {
      // new lowest weight
      minCongestion = std::get<3>(t);
      minWeight = weight;
      _outputs->clear();
      _outputs->insert(std::make_tuple(std::get<0>(t), std::get<1>(t)));
    } else if (congestionEqualTo(weight, minWeight)) {
      // equal lowest weight
      _outputs->insert(std::make_tuple(std::get<0>(t), std::get<1>(t)));
    }
  }

  // now search non-minimal
  f64 nonMin = false;
  for (const auto& t : _nonMinimal) {
    f64 weight = nonMinWeightFunc_(
        _minHops, std::get<2>(t), minCongestion, std::get<3>(t),
        congestionBias_, independentBias_);

    if (congestionLessThan(weight, minWeight)) {
      // new lowest weight
      nonMin = true;
      minWeight = weight;
      _outputs->clear();
      _outputs->insert(std::make_tuple(std::get<0>(t), std::get<1>(t)));
    } else if (congestionEqualTo(weight, minWeight) && nonMin) {
      // equal lowest weight
      _outputs->insert(std::make_tuple(std::get<0>(t), std::get<1>(t)));
    }
  }

  // set the allMinimal flag
  *_allMinimal = !nonMin;
}

registerWithObjectFactory("weighted", Reduction,
                          WeightedReduction, REDUCTION_ARGS);
