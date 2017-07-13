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
#include "metadata/ZeroMetadataHandler.h"

#include <factory/Factory.h>

#include "event/Simulator.h"
#include "types/Packet.h"

ZeroMetadataHandler::ZeroMetadataHandler(Json::Value _settings)
    : MetadataHandler(_settings) {}

ZeroMetadataHandler::~ZeroMetadataHandler() {}

void ZeroMetadataHandler::packetInjection(
    const Application* _app, Packet* _packet) {
  _packet->setMetadata(0);
}

registerWithFactory("zero", MetadataHandler,
                    ZeroMetadataHandler, METADATAHANDLER_ARGS);
