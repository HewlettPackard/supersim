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
#ifndef ARCHITECTURE_PORTEDDEVICE_H_
#define ARCHITECTURE_PORTEDDEVICE_H_

#include <prim/prim.h>

#include <vector>

#include "network/Channel.h"

class PortedDevice {
 public:
  PortedDevice(u32 _id, const std::vector<u32>& _address, u32 _numPorts,
               u32 _numVcs);
  virtual ~PortedDevice();

  u32 id() const;
  const std::vector<u32>& address() const;

  u32 numPorts() const;
  u32 numVcs() const;

  u32 vcIndex(u32 _port, u32 _vc) const;
  void vcIndexInv(u32 _index, u32* _port, u32* _vc) const;

  virtual void setInputChannel(u32 _port, Channel* _channel) = 0;
  virtual Channel* getInputChannel(u32 _port) const = 0;
  virtual void setOutputChannel(u32 port, Channel* _channel) = 0;
  virtual Channel* getOutputChannel(u32 _port) const = 0;

 protected:
  const u32 id_;
  const std::vector<u32> address_;
  const u32 numPorts_;
  const u32 numVcs_;
};

#endif  // ARCHITECTURE_PORTEDDEVICE_H_
