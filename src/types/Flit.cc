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
#include "types/Flit.h"

#include <cassert>

#include "types/Packet.h"

Flit::Flit(u32 _id, bool _isHead, bool _isTail, Packet* _packet)
    : id_(_id), head_(_isHead), tail_(_isTail), packet_(_packet),
      vc_(U32_MAX), sendTime_(U64_MAX), receiveTime_(U64_MAX) {}

Flit::~Flit() {}

u32 Flit::id() const {
  return id_;
}

bool Flit::isHead() const {
  return head_;
}

bool Flit::isTail() const {
  return tail_;
}

Packet* Flit::packet() const {
  return packet_;
}

u32 Flit::getVc() const {
  return vc_;
}

void Flit::setVc(u32 _vc) {
  vc_ = _vc;
}

u32 Flit::getTrafficClass() const {
  return packet_->getTrafficClass();
}

void Flit::setTrafficClass(u32 _trafficClass) {
  packet_->setTrafficClass(_trafficClass);
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
