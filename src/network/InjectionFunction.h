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
#ifndef NETWORK_INJECTIONFUNCTION_H_
#define NETWORK_INJECTIONFUNCTION_H_

#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "types/Message.h"

class Interface;

class InjectionFunction : public Component {
 public:
  /*
   * This class defines an injection function response. This will be given to
   *  the injection function upon injection request by the client. It is also
   *  returned to the client by the injection function.
   */
  class Response {
   public:
    Response();
    ~Response();
    void clear();
    void add(u32 _vc);
    u32 size() const;
    void get(u32 _index, u32* _vc) const;

   private:
    std::vector<u32> response_;
  };

  /*
   * This class defines the interface required to interact with an
   *  InjectionFunction. Clients will receive a call from the InjectionFunction
   *  using the injectionFunctionResponse() function.
   */
  class Client {
   public:
    Client();
    virtual ~Client();
    virtual void injectionFunctionResponse(Message* _message,
                                           Response* _response) = 0;
  };

  /*
   * This defines the InjectionFunction interface. Specific implementations
   *  must override the processRequest() function.
   */
  InjectionFunction(const std::string& _name, const Component* _parent,
                    Interface* _interface, u32 _latency);
  virtual ~InjectionFunction();
  u32 latency() const;
  void request(Client* _client, Message* _message, Response* _response);
  void processEvent(void* _event, s32 _type) override;

 protected:
  virtual void processRequest(Message* _message, Response* _response) = 0;

  Interface* interface_;

 private:
  class EventPackage {
   public:
    Client* client;
    Message* message;
    Response* response;
  };
  u32 latency_;
};

#endif  // NETWORK_INJECTIONFUNCTION_H_
