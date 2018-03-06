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
#include "routing/Reduction.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include "event/Simulator.h"

Reduction::Reduction(const std::string& _name, const Component* _parent,
                     const PortedDevice* _device, RoutingMode _mode,
                     bool _ignoreDuplicates, Json::Value _settings)
    : Component(_name, _parent), device_(_device), mode_(_mode),
      maxOutputs_(_settings["max_outputs"].asUInt()),
      ignoreDuplicates_(_ignoreDuplicates),
      start_(true) {
  // check inputs
  assert(!_settings["max_outputs"].isNull());
}

Reduction::~Reduction() {}

Reduction* Reduction::create(
    const std::string& _name, const Component* _parent,
    const PortedDevice* _device, RoutingMode _mode, bool _ignoreDuplicates,
    Json::Value _settings) {
  // retrieve algorithm
  const std::string& algorithm = _settings["algorithm"].asString();

  // attempt to build the reduction algorithm
  Reduction* reduction = factory::ObjectFactory<
    Reduction, REDUCTION_ARGS>::create(
        algorithm, _name, _parent, _device, _mode, _ignoreDuplicates,
        _settings);

  // check that the factory had the algorithm
  if (reduction == nullptr) {
    fprintf(stderr, "unknown reduction algorithm: %s\n", algorithm.c_str());
    assert(false);
  }
  return reduction;
}

void Reduction::add(u32 _port, u32 _vc, u32 _hops, f64 _congestion) {
  // detect restart of state machine
  if (start_) {
    check_.clear();
    minimal_.clear();
    nonMinimal_.clear();
    minHops_ = U32_MAX;
    intermediate_.clear();
    outputs_.clear();
    start_ = false;
  }

  // check that this input hasn't already been specified
  u32 input = U32_MAX;
  assert(_port < device_->numPorts());
  if (routingModeIsPort(mode_)) {
    assert(_vc == U32_MAX);
    input = _port;
  } else {
    assert(_vc < device_->numVcs());
    input = device_->vcIndex(_port, _vc);
  }
  if (!ignoreDuplicates_) {
    assert(check_.count(input) == 0);
  }
  check_.insert(input);

  // keep track of the minimum hop entry
  if (_hops < minHops_) {
    // move old minimal to non-minimal
    for (const auto& t : minimal_) {
      nonMinimal_.insert(t);
    }
    minimal_.clear();

    // update new minimal value
    minHops_ = _hops;

    // insert the new minimal
    minimal_.insert(std::make_tuple(_port, _vc, _hops, _congestion));
  } else if (_hops == minHops_) {
    // minimal route
    minimal_.insert(std::make_tuple(_port, _vc, _hops, _congestion));
  } else {
    // non-minimal route
    nonMinimal_.insert(std::make_tuple(_port, _vc, _hops, _congestion));
  }
}

const std::unordered_set<std::tuple<u32, u32> >* Reduction::reduce(
    bool* _allMinimal) {
  // handle state machine
  assert(!start_);
  start_ = true;
  assert(minimal_.size() > 0);

  // send the input information to the subclass for processing
  process(minHops_, minimal_, nonMinimal_, &intermediate_, &allMinimal_);
  assert(intermediate_.size() > 0);

  // reduce the subclass' set to a maximum of 'maxOutputs_'
  while ((intermediate_.size() > 0) &&
         (maxOutputs_ == 0 || outputs_.size() < maxOutputs_)) {
    // randomly pull element out
    auto it = intermediate_.begin();
    std::advance(it, gSim->rnd.nextU64(0, intermediate_.size() - 1));
    outputs_.insert(*it);
    intermediate_.erase(it);
  }

  // set the minimal flag
  if (_allMinimal) {
    *_allMinimal = allMinimal_;
  }

  return &outputs_;
}
