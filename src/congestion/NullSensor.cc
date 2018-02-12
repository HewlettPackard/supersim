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
#include "congestion/NullSensor.h"

#include <factory/ObjectFactory.h>

#include <cassert>
#include <cmath>

#include <algorithm>

NullSensor::NullSensor(
    const std::string& _name, const Component* _parent, PortedDevice* _device,
    Json::Value _settings)
    : CongestionSensor(_name, _parent, _device, _settings) {}

NullSensor::~NullSensor() {}

void NullSensor::initCredits(u32 _vcIdx, u32 _credits) {}

void NullSensor::incrementCredit(u32 _vcIdx) {}

void NullSensor::decrementCredit(u32 _vcIdx) {}

CongestionSensor::Style NullSensor::style() const {
  return CongestionSensor::Style::kNull;
}

CongestionSensor::Mode NullSensor::mode() const {
  return CongestionSensor::Mode::kNull;
}

f64 NullSensor::computeStatus(
    u32 _inputPort, u32 _inputVc, u32 _outputPort, u32 _outputVc) const {
  assert(false);  // you can't ask me this!
}

registerWithObjectFactory("null_sensor", CongestionSensor,
                          NullSensor, CONGESTIONSENSOR_ARGS);
