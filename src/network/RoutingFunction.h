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
#ifndef NETWORK_ROUTINGFUNCTION_H_
#define NETWORK_ROUTINGFUNCTION_H_

#include <prim/prim.h>

#include <string>
#include <utility>
#include <vector>

#include "event/Component.h"
#include "types/Flit.h"

class RoutingFunction : public Component {
 public:
  /*
   * This class defines a routing function response. This will be given to the
   *  routing function upon routing request by the client. It is also returned
   *  to the client by the routing function.
   */
  class Response {
   public:
    Response();
    ~Response();
    void clear();
    void add(u32 _port, u32 _vc);
    u32 size() const;
    void get(u32 _index, u32* _port, u32* _vc) const;

   private:
    std::vector<std::pair<u32, u32> > response_;
  };

  /*
   * This class defines the interface required to interact with a
   *  RoutingFunction. Clients will receive a call from the RoutingFunction
   *  using the routingFunctionResponse() function.
   */
  class Client {
   public:
    Client();
    virtual ~Client();
    virtual void routingFunctionResponse(Response* _response) = 0;
  };

  /*
   * This defines the RoutingFunction interface. Specific implementations
   *  must override the processRequest() function.
   */
  RoutingFunction(const std::string& _name, const Component* _parent,
                  u32 _latency);
  virtual ~RoutingFunction();
  u32 latency() const;
  void request(Client* _client, Flit* _flit, Response* _response);
  void processEvent(void* _event, s32 _type) override;

 protected:
  virtual void processRequest(Flit* _flit, Response* _response) = 0;

 private:
  class EventPackage {
   public:
    Client* client;
    Flit* flit;
    Response* response;
  };
  u32 latency_;
};

#endif  // NETWORK_ROUTINGFUNCTION_H_
