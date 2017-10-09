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
#ifndef CONGESTION_NULLSENSOR_H_
#define CONGESTION_NULLSENSOR_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "congestion/CongestionSensor.h"

class NullSensor : public CongestionSensor {
 public:
  NullSensor(const std::string& _name, const Component* _parent,
             PortedDevice* _device, Json::Value _settings);
  ~NullSensor();

  // CreditWatcher interface
  void initCredits(u32 _vcIdx, u32 _credits);
  void incrementCredit(u32 _vcIdx);
  void decrementCredit(u32 _vcIdx);

  // style and mode reporting
  CongestionSensor::Style style() const override;
  CongestionSensor::Mode mode() const override;

 protected:
  // see CongestionSensor::computeStatus
  f64 computeStatus(u32 _inputPort, u32 _inputVc, u32 _outputPort,
                    u32 _outputVc) const override;
};

#endif  // CONGESTION_NULLSENSOR_H_
