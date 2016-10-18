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
#include "congestion/CongestionStatusFactory.h"

#include <cassert>

#include "congestion/BufferOccupancy.h"
#include "congestion/PhantomBufferOccupancy.h"

CongestionStatus* CongestionStatusFactory::createCongestionStatus(
    const std::string& _name, const Component* _parent, PortedDevice* _device,
    Json::Value _settings) {
  std::string algorithm = _settings["algorithm"].asString();

  if (algorithm == "buffer_occupancy") {
    return new BufferOccupancy(_name, _parent, _device, _settings);
  } else if (algorithm == "phantom_buffer_occupancy") {
    return new PhantomBufferOccupancy(_name, _parent, _device, _settings);
  } else {
    fprintf(stderr, "unknown congestion status algorithm: %s\n",
            algorithm.c_str());
    assert(false);
  }
}
