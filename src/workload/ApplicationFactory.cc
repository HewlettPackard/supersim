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
#include "workload/ApplicationFactory.h"

#include <cassert>

#include "workload/Application.h"
#include "workload/blast/Application.h"
#include "workload/pulse/Application.h"
#include "workload/simplemem/Application.h"
#include "workload/stream/Application.h"

Application* ApplicationFactory::createApplication(
    const std::string& _name, const Component* _parent, u32 _id,
    Workload* _workload, MetadataHandler* _metadataHandler,
    Json::Value _settings) {
  std::string type = _settings["type"].asString();
  if (type == "blast") {
    return new Blast::Application(_name, _parent, _id, _workload,
                                  _metadataHandler, _settings);
  } else if (type == "pulse") {
    return new Pulse::Application(_name, _parent, _id, _workload,
                                  _metadataHandler, _settings);
  } else if (type == "simple_mem") {
    return new SimpleMem::Application(_name, _parent, _id, _workload,
                                      _metadataHandler, _settings);
  } else if (type == "stream") {
    return new Stream::Application(_name, _parent, _id, _workload,
                                   _metadataHandler, _settings);
  } else {
    fprintf(stderr, "unknown application type: %s\n", type.c_str());
    assert(false);
  }
}
