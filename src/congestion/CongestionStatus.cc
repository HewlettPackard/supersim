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
#include "congestion/CongestionStatus.h"

#include <factory/Factory.h>

#include <cassert>
#include <cmath>

#include "router/Router.h"

CongestionStatus::CongestionStatus(
    const std::string& _name, const Component* _parent, PortedDevice* _device,
    Json::Value _settings)
    : Component(_name, _parent), device_(_device),
      numPorts_(device_->numPorts()), numVcs_(device_->numVcs()),
      granularity_(_settings["granularity"].asUInt()) {
  assert(!_settings["granularity"].isNull());
}

CongestionStatus::~CongestionStatus() {}

CongestionStatus* CongestionStatus::create(
    const std::string& _name, const Component* _parent, PortedDevice* _device,
    Json::Value _settings) {
  // retrieve the algorithm
  const std::string& algorithm = _settings["algorithm"].asString();

  // attempt to build the congestion status
  CongestionStatus* cs = factory::Factory<
    CongestionStatus, CONGESTIONSTATUS_ARGS>::create(
        algorithm, _name, _parent, _device, _settings);

  // check that the algorithm exists within the factory
  if (cs == nullptr) {
    fprintf(stderr, "unknown congestion status algorithm: %s\n",
            algorithm.c_str());
    assert(false);
  }
  return cs;
}

f64 CongestionStatus::status(
    u32 _inputPort, u32 _inputVc, u32 _outputPort, u32 _outputVc) const {
  assert(gSim->epsilon() == 0);
  f64 value = computeStatus(_inputPort, _inputVc, _outputPort, _outputVc);
  assert(value >= 0.0);
  assert(value <= 1.0);
  if (granularity_ > 0) {
    value = round(value * granularity_) / granularity_;
  }
  return value;
}
