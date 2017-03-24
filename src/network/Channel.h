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
#ifndef NETWORK_CHANNEL_H_
#define NETWORK_CHANNEL_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"

class Flit;
class FlitReceiver;
class Credit;
class CreditReceiver;

class Channel : public Component {
 public:
  Channel(const std::string& _name, const Component* _parent,
          u32 _numVcs, Json::Value _settings);
  ~Channel();
  u32 latency() const;
  void setSource(CreditReceiver* _source, u32 _port);
  void setSink(FlitReceiver* _sink, u32 _port);
  void startMonitoring();
  void endMonitoring();
  f64 utilization(u32 _vc) const;  // U32_MAX for total
  void processEvent(void* _event, s32 _type) override;

  /*
   * This retrieves the flit that exists in the event queue for the next
   * flit time in the future. nullptr is returned if it has not been set.
   */
  Flit* getNextFlit() const;

  /*
   * Sets 'flit' to be the next flit to traverse the channel. This inserts
   * an event into the event queue. If an existing flit is already set for
   * this time, an assertion will fail!
   * This returns the time the flit will be injected into the channel,
   * which is guaranteed to be in the future.
   */
  u64 setNextFlit(Flit* _flit);

  /*
   * This retrieves the credit that exists in the event queue for the next
   * credit time in the future. nullptr is returned if it has not been set.
   */
  Credit* getNextCredit() const;

  /*
   * Sets 'credit' to be the next credit to traverse the channel. This inserts
   * an event into the event queue. If an existing credit is already set for
   * this time, an assertion will fail!
   * This returns the time the credit will be injected into the channel,
   * which is guaranteed to be in the future.
   */
  u64 setNextCredit(Credit* _credit);

 private:
  const u32 latency_;
  const u32 numVcs_;

  u64 nextFlitTime_;
  Flit* nextFlit_;
  u64 nextCreditTime_;
  Credit* nextCredit_;
  bool monitoring_;
  u64 monitorTime_;
  std::vector<u64> monitorCounts_;

  CreditReceiver* source_;  // sends flits, receives credits
  u32 sourcePort_;
  FlitReceiver* sink_;   // receives flits, sends credits
  u32 sinkPort_;
};

#endif  // NETWORK_CHANNEL_H_
