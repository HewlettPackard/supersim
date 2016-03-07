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
#ifndef ROUTER_COMMON_CROSSBARSCHEDULER_H_
#define ROUTER_COMMON_CROSSBARSCHEDULER_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "allocator/Allocator.h"
#include "event/Component.h"
#include "types/Flit.h"

class CrossbarScheduler : public Component {
 public:
  /*
   * This class defines the interface required to interact with the
   *  CrossbarScheduler component. Clients will receive a call from the
   *  CrossbarScheduler using the crossbarSchedulerResponse() function.
   *  If a crossbar output port was allocated, it will be given. If no
   *  port was allocated, U32_MAX is returned. If the client will use the
   *  port allocated, then decrementCreditCount() should be called during
   *  the same cycle after/during crossbarSchedulerResponse().
   */
  class Client {
   public:
    Client();
    virtual ~Client();
    virtual void crossbarSchedulerResponse(u32 _port, u32 _vcIdx) = 0;
  };

  // constructor and destructor
  CrossbarScheduler(const std::string& _name, const Component* _parent,
                    u32 _numClients, u32 _totalVcs, u32 _crossbarPorts,
                    Json::Value _settings);
  ~CrossbarScheduler();

  // constant attributes
  u32 numClients() const;
  u32 totalVcs() const;
  u32 crossbarPorts() const;

  // links a client to the scheduler
  void setClient(u32 _id, Client* _client);

  // requests to send a flit to a VC
  void request(u32 _client, u32 _port, u32 _vcIdx, Flit* _flit);

  // credit counts
  void initCreditCount(u32 _vcIdx, u32 _credits);
  void incrementCreditCount(u32 _vcIdx);
  void decrementCreditCount(u32 _vcIdx);
  u32 getCreditCount(u32 _vcIdx) const;

  // event processing
  void processEvent(void* _event, s32 _type);

 private:
  const u32 numClients_;
  const u32 totalVcs_;
  const u32 crossbarPorts_;

  std::vector<Client*> clients_;
  std::vector<u32> clientRequestPorts_;
  std::vector<u32> clientRequestVcs_;
  std::vector<bool> clientRequestTails_;  // request is a tail

  std::vector<u32> credits_;
  std::vector<u32> maxCredits_;
  std::unordered_map<u32, u32> incrCredits_;

  bool* requests_;
  u64* metadatas_;
  bool* grants_;

  std::vector<bool> anyRequests_;  // someone has requested port
  std::vector<u32> portLocks_;  // output port locks

  Allocator* allocator_;

  const bool packetLock_;  // packets lock the channel
  const bool idleUnlock_;  // locks are deactivated when idle (others want)

  enum class EventAction : u8 { NONE = 0, CREDITS = 1, RUNALLOC = 2 };
  EventAction eventAction_;

  // this creates an index for requests_, metadatas_, vcs_, and grants_
  u64 index(u64 _client, u64 _port) const;
};

#endif  // ROUTER_COMMON_CROSSBARSCHEDULER_H_
