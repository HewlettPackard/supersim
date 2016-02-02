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
#ifndef ROUTER_COMMON_VCSCHEDULER_H_
#define ROUTER_COMMON_VCSCHEDULER_H_

#include <jsoncpp/json/json.h>
#include <prim/prim.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "allocator/Allocator.h"
#include "event/Component.h"

class VcScheduler : public Component {
 public:
  /*
   * This class defines the interface required to interact with the VcScheduler
   *  component. Clients will receive a call from the VcScheduler using the
   *  vcSchedulerResponse() function. If a VC was allocated, it will be given.
   *  If no VC was allocated, U32_MAX is returned.
   */
  class Client {
   public:
    Client();
    virtual ~Client();
    virtual void vcSchedulerResponse(u32 _vc) = 0;
  };

  // constructor and destructor
  VcScheduler(const std::string& _name, const Component* _parent,
              u32 _numClients, u32 _numVcs, Json::Value _settings);
  ~VcScheduler();

  // constant attributes
  u32 numClients() const;
  u32 numVcs() const;

  // links a client to the scheduler
  void setClient(u32 _id, Client* _client);

  // requesting and releasing VCs
  void request(u32 _client, u32 _vc, u64 _metadata);
  void releaseVc(u32 _vc);

  // event processing
  void processEvent(void* _event, s32 _type);

 private:
  const u32 numClients_;
  const u32 numVcs_;

  std::vector<Client*> clients_;
  std::vector<bool> clientRequested_;

  std::vector<bool> vcTaken_;

  bool* requests_;
  u64* metadatas_;
  bool* grants_;
  Allocator* allocator_;
  bool allocEventSet_;

  // this creates an index for requests_, metadatas_, and grants_
  u64 index(u64 _client, u64 _vc) const;
};

#endif  // ROUTER_COMMON_VCSCHEDULER_H_
