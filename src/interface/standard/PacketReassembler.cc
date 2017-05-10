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
#include "interface/standard/PacketReassembler.h"

#include <cassert>
#include <cstdio>

#include "types/Message.h"

namespace Standard {

PacketReassembler::PacketReassembler(const std::string& _name,
                                     const Component* _parent)
    : Component(_name, _parent) {
  expSourceId_  = U32_MAX;
  expPacketId_  = U32_MAX;
  expFlitId_    = 0;
}

PacketReassembler::~PacketReassembler() {}

Packet* PacketReassembler::receiveFlit(Flit* _flit) {
  Packet* packet = nullptr;
  u32 sourceId = _flit->packet()->message()->getSourceId();
  u32 packetId = _flit->packet()->id();
  u32 flitId   = _flit->id();

  dbgprintf("src=%u pkt=%u flit=%u/%u",
            sourceId, packetId, flitId,
            _flit->packet()->numFlits());

  // if expected packet id isn't yet set, set it
  if (expPacketId_ == U32_MAX) {
    assert(flitId == 0);
    expSourceId_ = sourceId;
    expPacketId_ = packetId;
  }

  // check sourceId, packetId, and flitId
  if (sourceId != expSourceId_) {
    fprintf(stderr, "error name: %s\n", fullName().c_str());
    assert(false);
  }
  if (packetId != expPacketId_) {
    assert(false);
  }
  if (flitId != expFlitId_) {
    assert(false);
  }

  assert(flitId < _flit->packet()->numFlits());
  assert(packetId < _flit->packet()->message()->numPackets());

  // if this is the last flit of the packet
  if (flitId == (_flit->packet()->numFlits() - 1)) {
    expSourceId_ = U32_MAX;  // clear expected source id
    expPacketId_ = U32_MAX;  // clear expected packet id
    expFlitId_ = 0;  // expect flit 0 on the next packet
    packet = _flit->packet();
  } else {
    expFlitId_ = flitId + 1;
  }
  return packet;
}

}  // namespace Standard
