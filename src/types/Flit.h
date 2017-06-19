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
#ifndef TYPES_FLIT_H_
#define TYPES_FLIT_H_

#include <prim/prim.h>

class Packet;

class Flit {
 public:
  Flit(u32 _id, bool _isHead, bool _isTail, Packet* _packet);
  virtual ~Flit();

  u32 id() const;
  bool isHead() const;
  bool isTail() const;
  Packet* packet() const;

  u32 getVc() const;
  void setVc(u32 vc);

  void setSendTime(u64 time);
  u64 getSendTime() const;
  void setReceiveTime(u64 time);
  u64 getReceiveTime() const;
  void setTrafficClass(u32 _trafficClass);
  u32 getTrafficClass() const;

 private:
  u32 id_;
  bool head_;
  bool tail_;
  Packet* packet_;
  u32 vc_;

  u64 sendTime_;
  u64 receiveTime_;
};

#endif  // TYPES_FLIT_H_
