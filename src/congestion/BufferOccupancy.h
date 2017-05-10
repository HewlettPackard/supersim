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
#ifndef CONGESTION_BUFFEROCCUPANCY_H_
#define CONGESTION_BUFFEROCCUPANCY_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "congestion/CongestionStatus.h"

class BufferOccupancy : public CongestionStatus {
 public:
  BufferOccupancy(const std::string& _name, const Component* _parent,
                  PortedDevice* _device, Json::Value _settings);
  ~BufferOccupancy();

 protected:
  void performInitCredits(u32 _port, u32 _vc, u32 _credits) override;
  void performIncrementCredit(u32 _port, u32 _vc) override;
  void performDecrementCredit(u32 _port, u32 _vc) override;
  f64 computeStatus(u32 _port, u32 _vc) const override;

 private:
  enum class Mode {kVc, kPort};
  static Mode parseMode(const std::string& _mode);

  const Mode mode_;
  std::vector<u32> maximums_;
  std::vector<u32> counts_;
};

#endif  // CONGESTION_BUFFEROCCUPANCY_H_
