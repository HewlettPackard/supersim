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
#ifndef NETWORK_ROUTINGALGORITHM_H_
#define NETWORK_ROUTINGALGORITHM_H_

#include <prim/prim.h>

#include <string>
#include <utility>
#include <vector>

#include "event/Component.h"
#include "types/Flit.h"

class Router;

class RoutingAlgorithm : public Component {
 public:
  /*
   * This class defines a routing algorithm response. This will be given to the
   *  routing algorithm upon routing request by the client. It is also returned
   *  to the client by the routing algorithm.
   */
  class Response {
   public:
    Response();
    ~Response();
    void clear();
    void add(u32 _port, u32 _vc);
    u32 size() const;
    void get(u32 _index, u32* _port, u32* _vc) const;
    void link(const RoutingAlgorithm* _algorithm);

   private:
    std::vector<std::pair<u32, u32> > response_;
    const RoutingAlgorithm* algorithm_;
  };

  /*
   * This class defines the interface required to interact with a
   *  RoutingAlgorithm. Clients will receive a call from the RoutingAlgorithm
   *  using the routingAlgorithmResponse() function.
   */
  class Client {
   public:
    Client();
    virtual ~Client();
    virtual void routingAlgorithmResponse(Response* _response) = 0;
  };

  /*
   * This defines the RoutingAlgorithm interface. Specific implementations
   *  must override the processRequest() function.
   */
  RoutingAlgorithm(const std::string& _name, const Component* _parent,
                   Router* _router, u32 _latency, u32 _baseVc, u32 _numVcs);
  virtual ~RoutingAlgorithm();
  u32 latency() const;
  u32 baseVc() const;
  u32 numVcs() const;
  void request(Client* _client, Flit* _flit, Response* _response);
  virtual void vcScheduled(Flit* _flit, u32 _port, u32 _vc);
  void processEvent(void* _event, s32 _type) override;

 protected:
  virtual void processRequest(Flit* _flit, Response* _response) = 0;

  Router* router_;
  const u32 baseVc_;
  const u32 numVcs_;

 private:
  class EventPackage {
   public:
    Client* client;
    Flit* flit;
    Response* response;
  };

  const u32 latency_;
};

#endif  // NETWORK_ROUTINGALGORITHM_H_
