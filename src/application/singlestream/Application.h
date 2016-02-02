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
#ifndef APPLICATION_SINGLESTREAM_APPLICATION_H_
#define APPLICATION_SINGLESTREAM_APPLICATION_H_

#include <jsoncpp/json/json.h>
#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "application/Application.h"

class MetadataHandler;

namespace SingleStream {

class Application : public ::Application {
 public:
  Application(const std::string& _name, const Component* _parent,
              MetadataHandler* _metadataHandler, Json::Value _settings);
  ~Application();
  f64 percentComplete() const override;
  u32 getSource() const;
  u32 getDestination() const;
  void receivedFirst(u32 _id);
  void sentLast(u32 _id);

 private:
  u32 sourceTerminal_;
  u32 destinationTerminal_;
  bool doMonitoring_;
};

}  // namespace SingleStream

#endif  // APPLICATION_SINGLESTREAM_APPLICATION_H_
