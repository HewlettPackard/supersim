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
#ifndef INTERFACE_STANDARD_EJECTOR_H_
#define INTERFACE_STANDARD_EJECTOR_H_

#include <prim/prim.h>

#include <string>

#include "interface/standard/Interface.h"
#include "event/Component.h"
#include "types/Flit.h"
#include "types/FlitReceiver.h"

namespace Standard {

class Interface;

class Ejector : public Component, public FlitReceiver {
 public:
  Ejector(const std::string& _name, Interface* _interface);
  ~Ejector();

  // called by crossbar (FlitReceiver)
  void receiveFlit(u32 _port, Flit* _flit) override;

 private:
  Interface* interface_;
  u32 portId_;
  u64 lastSetTime_;
};

}  // namespace Standard

#endif  // INTERFACE_STANDARD_EJECTOR_H_
