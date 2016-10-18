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
#ifndef CONGESTION_PHANTOMBUFFEROCCUPANCY_H_
#define CONGESTION_PHANTOMBUFFEROCCUPANCY_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "congestion/CongestionStatus.h"

class PhantomBufferOccupancy : public CongestionStatus {
 public:
  PhantomBufferOccupancy(const std::string& _name, const Component* _parent,
                         PortedDevice* _device, Json::Value _settings);
  ~PhantomBufferOccupancy();

  void processEvent(void* _event, s32 _type);

 protected:
  void performInitCredits(u32 _port, u32 _vc, u32 _credits) override;
  void performIncrementCredit(u32 _port, u32 _vc) override;
  void performDecrementCredit(u32 _port, u32 _vc) override;
  f64 computeStatus(u32 _port, u32 _vc) const override;

 private:
  const f64 valueCoeff_;
  const f64 lengthCoeff_;
  std::vector<u32> maximums_;
  std::vector<u32> counts_;
  std::vector<u32> windows_;
};

#endif  // CONGESTION_PHANTOMBUFFEROCCUPANCY_H_
