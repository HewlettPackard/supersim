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
#include "types/Flit.h"

#include <cassert>

#include "types/Packet.h"

Flit::Flit(u32 _id, bool _isHead, bool _isTail, Packet* _packet)
    : id_(_id), head_(_isHead), tail_(_isTail), packet_(_packet),
      sendTime_(U64_MAX), receiveTime_(U64_MAX) {}

Flit::~Flit() {}

u32 Flit::getId() const {
  return id_;
}

bool Flit::isHead() const {
  return head_;
}

bool Flit::isTail() const {
  return tail_;
}

Packet* Flit::getPacket() const {
  return packet_;
}

u32 Flit::getVc() const {
  return vc_;
}

void Flit::setVc(u32 _vc) {
  vc_ = _vc;
}

void Flit::setSendTime(u64 _time) {
  assert(_time != U64_MAX);
  sendTime_ = _time;
}

u64 Flit::getSendTime() const {
  assert(sendTime_ != U64_MAX);
  return sendTime_;
}

void Flit::setReceiveTime(u64 _time) {
  assert(_time != U64_MAX);
  receiveTime_ = _time;
}

u64 Flit::getReceiveTime() const {
  assert(receiveTime_ != U64_MAX);
  return receiveTime_;
}
