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
#include "congestion/CongestionSensor.h"

#include <factory/Factory.h>

#include <cassert>
#include <cmath>

#include <algorithm>

#include "router/Router.h"

CongestionSensor::CongestionSensor(
    const std::string& _name, const Component* _parent, PortedDevice* _device,
    Json::Value _settings)
    : Component(_name, _parent), device_(_device),
      numPorts_(device_->numPorts()), numVcs_(device_->numVcs()),
      granularity_(_settings["granularity"].asUInt()),
      minimum_(_settings["minimum"].asDouble()),
      offset_(_settings["offset"].asDouble()) {
  assert(!_settings["granularity"].isNull());
  assert(!_settings["minimum"].isNull());
  assert(!_settings["offset"].isNull());
  assert(minimum_ >= 0.0);
  assert(offset_ >= 0.0);
}

CongestionSensor::~CongestionSensor() {}

CongestionSensor* CongestionSensor::create(
    const std::string& _name, const Component* _parent, PortedDevice* _device,
    Json::Value _settings) {
  // retrieve the algorithm
  const std::string& algorithm = _settings["algorithm"].asString();

  // attempt to build the congestion status
  CongestionSensor* cs = factory::Factory<
    CongestionSensor, CONGESTIONSENSOR_ARGS>::create(
        algorithm, _name, _parent, _device, _settings);

  // check that the algorithm exists within the factory
  if (cs == nullptr) {
    fprintf(stderr, "unknown congestion status algorithm: %s\n",
            algorithm.c_str());
    assert(false);
  }
  return cs;
}

f64 CongestionSensor::status(
    u32 _inputPort, u32 _inputVc, u32 _outputPort, u32 _outputVc) const {
  assert(gSim->epsilon() == 0);

  // gather value from subclass
  f64 value = computeStatus(_inputPort, _inputVc, _outputPort, _outputVc);

  // check bounds
  assert(value >= 0.0);
  assert((style() != CongestionSensor::Style::kNormalized) ||
         (value <= 1.0));

  // apply granularization
  if (granularity_ > 0) {
    value = round(value * granularity_) / granularity_;
  }

  // apply minimum constraint
  value = offset_ + std::max(minimum_, value);
  if (style() == CongestionSensor::Style::kNormalized) {
    value = std::min(1.0, value);
  }

  return value;
}
