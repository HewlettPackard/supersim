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
#ifndef INTERFACE_STANDARD_PACKETREASSEMBLER_H_
#define INTERFACE_STANDARD_PACKETREASSEMBLER_H_

#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "types/Flit.h"
#include "types/Packet.h"

namespace Standard {

class PacketReassembler : public Component {
 public:
  PacketReassembler(const std::string& _name, const Component* _parent);
  ~PacketReassembler();
  Packet* receiveFlit(Flit* _flit);

 private:
  u32 expSourceId_;
  u32 expPacketId_;
  u32 expFlitId_;
};

}  // namespace Standard

#endif  // INTERFACE_STANDARD_PACKETREASSEMBLER_H_
