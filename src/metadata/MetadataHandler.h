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
#ifndef METADATA_METADATAHANDLER_H_
#define METADATA_METADATAHANDLER_H_

#include <json/json.h>

class Application;
class Packet;

#define METADATAHANDLER_ARGS Json::Value

class MetadataHandler {
 public:
  explicit MetadataHandler(Json::Value _settings);
  virtual ~MetadataHandler();

  // this is the metadata handler factory
  static MetadataHandler* create(METADATAHANDLER_ARGS);

  virtual void packetInjection(Application* _app, Packet* _packet) = 0;
  virtual void packetArrival(Packet* _packet) = 0;
};

#endif  // METADATA_METADATAHANDLER_H_
