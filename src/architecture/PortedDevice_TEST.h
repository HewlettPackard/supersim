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
#ifndef ARCHITECTURE_PORTEDDEVICE_TEST_H_
#define ARCHITECTURE_PORTEDDEVICE_TEST_H_

#include "architecture/PortedDevice.h"

#include <json/json.h>

// this is a test ported device
class TestPortedDevice : public PortedDevice {
 public:
  TestPortedDevice(u32 _numPorts, u32 _numVcs);
  ~TestPortedDevice();

  void setInputChannel(u32 _port, Channel* _channel) override;
  Channel* getInputChannel(u32 _port) const override;
  void setOutputChannel(u32 port, Channel* _channel) override;
  Channel* getOutputChannel(u32 _port) const override;
};

#endif  // ARCHITECTURE_PORTEDDEVICE_TEST_H_
