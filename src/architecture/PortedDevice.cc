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
#include "architecture/PortedDevice.h"

#include <cassert>

PortedDevice::PortedDevice(u32 _id, const std::vector<u32>& _address,
                           u32 _numPorts, u32 _numVcs)
    : id_(_id), address_(_address), numPorts_(_numPorts), numVcs_(_numVcs) {}

PortedDevice::~PortedDevice() {}

u32 PortedDevice::id() const {
  return id_;
}

const std::vector<u32>& PortedDevice::address() const {
  return address_;
}

u32 PortedDevice::numPorts() const {
  return numPorts_;
}

u32 PortedDevice::numVcs() const {
  return numVcs_;
}

u32 PortedDevice::vcIndex(u32 _port, u32 _vc) const {
  return (_port * numVcs_) + _vc;
}

void PortedDevice::vcIndexInv(u32 _vcIdx, u32* _port, u32* _vc) const {
  assert(_vcIdx < (numPorts_ * numVcs_));
  *_port = _vcIdx / numVcs_;
  *_vc = _vcIdx % numVcs_;
}
