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
#include "types/Packet.h"

#include <cassert>

#include "types/Flit.h"
#include "types/Message.h"

Packet::Packet(u32 _id, u32 _numFlits, Message* _message)
    : id_(_id), message_(_message), hopCount_(0),
      metadata_(U64_MAX), routingExtension_(nullptr) {
  flits_.resize(_numFlits);
}

Packet::~Packet() {
  for (u32 f = 0; f < flits_.size(); f++) {
    if (flits_.at(f)) {
      delete flits_.at(f);
    }
  }
  assert(routingExtension_ == nullptr);
}

u32 Packet::id() const {
  return id_;
}

u32 Packet::numFlits() const {
  return flits_.size();
}

Flit* Packet::getFlit(u32 _index) const {
  return flits_.at(_index);
}

void Packet::setFlit(u32 _index, Flit* _flit) {
  flits_.at(_index) = _flit;
}

u32 Packet::getProtocolClass() const {
  return message_->getProtocolClass();
}

Message* Packet::message() const {
  return message_;
}

void Packet::incrementHopCount() {
  hopCount_++;
}

u32 Packet::getHopCount() const {
  return hopCount_;
}

u64 Packet::headLatency() const {
  Flit* head = flits_.at(0);
  return head->getReceiveTime() - head->getSendTime();
}

u64 Packet::serializationLatency() const {
  Flit* head = flits_.at(0);
  Flit* tail = flits_.at(flits_.size() - 1);
  return tail->getReceiveTime() - head->getReceiveTime();
}

u64 Packet::totalLatency() const {
  Flit* head = flits_.at(0);
  Flit* tail = flits_.at(flits_.size() - 1);
  return tail->getReceiveTime() - head->getSendTime();
}

u64 Packet::getMetadata() const {
  assert(metadata_ != U64_MAX);
  return metadata_;
}

void Packet::setMetadata(u64 _metadata) {
  assert(_metadata != U64_MAX);
  metadata_ = _metadata;
}

void* Packet::getRoutingExtension() const {
  return routingExtension_;
}

void Packet::setRoutingExtension(void* _ext) {
  routingExtension_ = _ext;
}
