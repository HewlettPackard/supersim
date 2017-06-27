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
#include "metadata/MetadataHandler.h"

#include <factory/Factory.h>

#include <cassert>

#include <string>

MetadataHandler::MetadataHandler(Json::Value _settings) {}

MetadataHandler::~MetadataHandler() {}

MetadataHandler* MetadataHandler::create(Json::Value _settings) {
  // retrieve the type
  const std::string& type = _settings["type"].asString();

  // attempt to create the metadata handler
  MetadataHandler* mh = factory::Factory<MetadataHandler, METADATAHANDLER_ARGS>
      ::create(type, _settings);

  // check that the factory had this type
  if (mh == nullptr) {
    fprintf(stderr, "unknown metadata handler type: %s\n", type.c_str());
    assert(false);
  }
  return mh;
}

void MetadataHandler::packetInjection(
    const Application* _app, Packet* _packet) {}

void MetadataHandler::packetInterfaceArrival(
    const Interface* _iface, Packet* _packet) {}

void MetadataHandler::packetInterfaceDeparture(
    const Interface* _iface, Packet* _packet) {}

void MetadataHandler::packetRouterArrival(
    const Router* _router, u32 _port, Packet* _packet) {}

void MetadataHandler::packetRouterDeparture(
    const Router* _router, u32 _port, Packet* _packet) {}
