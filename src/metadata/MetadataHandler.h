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
#ifndef METADATA_METADATAHANDLER_H_
#define METADATA_METADATAHANDLER_H_

#include <json/json.h>
#include <prim/prim.h>

class Application;
class Interface;
class Packet;
class Router;

#define METADATAHANDLER_ARGS Json::Value

class MetadataHandler {
 public:
  explicit MetadataHandler(Json::Value _settings);
  virtual ~MetadataHandler();

  // this is the metadata handler factory
  static MetadataHandler* create(METADATAHANDLER_ARGS);

  // this is called by the terminal when the packet is being injected
  virtual void packetInjection(
      const Application* _app, Packet* _packet);

  // this is called by the interface when it is received
  virtual void packetInterfaceArrival(
      const Interface* _iface, Packet* _packet);

  // this is called by the interface when it is sent
  virtual void packetInterfaceDeparture(
      const Interface* _iface, Packet* _packet);

  // this is called by the router when it is received
  virtual void packetRouterArrival(
      const Router* _router, u32 _port, Packet* _packet);

  // this is called by the router when it is sent
  virtual void packetRouterDeparture(
      const Router* _router, u32 _port, Packet* _packet);
};

#endif  // METADATA_METADATAHANDLER_H_
