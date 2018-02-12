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
#include "metadata/LocalTimestampMetadataHandler.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include <string>

#include "workload/Application.h"
#include "event/Simulator.h"
#include "types/Packet.h"

LocalTimestampMetadataHandler::LocalTimestampMetadataHandler(
    Json::Value _settings)
    : MetadataHandler(_settings) {}

LocalTimestampMetadataHandler::~LocalTimestampMetadataHandler() {}

void LocalTimestampMetadataHandler::packetInterfaceArrival(
    const Interface* _iface, Packet* _packet) {
  _packet->setMetadata(gSim->time());
}

void LocalTimestampMetadataHandler::packetRouterArrival(
    const Router* _router, u32 _port, Packet* _packet) {
  _packet->setMetadata(gSim->time());
}

registerWithObjectFactory("local_timestamp", MetadataHandler,
                          LocalTimestampMetadataHandler, METADATAHANDLER_ARGS);
