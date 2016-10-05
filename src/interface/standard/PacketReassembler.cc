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
  u32 sourceId = _flit->getPacket()->getMessage()->getSourceId();
  u32 packetId = _flit->getPacket()->getId();
  u32 flitId   = _flit->getId();

  dbgprintf("src=%u pkt=%u flit=%u/%u",
            sourceId, packetId, flitId,
            _flit->getPacket()->numFlits());

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

  assert(flitId < _flit->getPacket()->numFlits());
  assert(packetId < _flit->getPacket()->getMessage()->numPackets());

  // if this is the last flit of the packet
  if (flitId == (_flit->getPacket()->numFlits() - 1)) {
    expSourceId_ = U32_MAX;  // clear expected source id
    expPacketId_ = U32_MAX;  // clear expected packet id
    expFlitId_ = 0;  // expect flit 0 on the next packet
    packet = _flit->getPacket();
  } else {
    expFlitId_ = flitId + 1;
  }
  return packet;
}

}  // namespace Standard
