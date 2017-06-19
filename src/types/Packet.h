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
#ifndef TYPES_PACKET_H_
#define TYPES_PACKET_H_

#include <prim/prim.h>

#include <vector>

class Flit;
class Message;

class Packet {
 public:
  Packet(u32 _id, u32 _numFlits, Message* _message);

  // this deletes all flit data as well (if they aren't nullptr)
  virtual ~Packet();

  u32 id() const;

  u32 numFlits() const;
  Flit* getFlit(u32 _index) const;
  void setFlit(u32 _index, Flit* _flit);

  u32 getTrafficClass() const;
  void setTrafficClass(u32 _trafficClass);

  Message* message() const;

  void incrementHopCount();
  u32 getHopCount() const;

  u64 headLatency() const;
  u64 serializationLatency() const;
  u64 totalLatency() const;

  u64 getMetadata() const;
  void setMetadata(u64 _metadata);

  void* getRoutingExtension() const;
  void setRoutingExtension(void* _ext);

 private:
  u32 id_;
  std::vector<Flit*> flits_;
  Message* message_;

  u32 hopCount_;
  u64 metadata_;

  void* routingExtension_;
};

#endif  // TYPES_PACKET_H_
