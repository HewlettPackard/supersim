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
#ifndef APPLICATION_SIMPLEMEM_APPLICATION_H_
#define APPLICATION_SIMPLEMEM_APPLICATION_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "application/Application.h"

class MetadataHandler;

namespace SimpleMem {

class Application : public ::Application {
 public:
  Application(const std::string& _name, const Component* _parent,
              MetadataHandler* _metadataHandler, Json::Value _settings);
  ~Application();
  f64 percentComplete() const override;
  u32 numVcs() const;
  u32 totalMemory() const;
  u32 memorySlice() const;
  u32 blockSize() const;
  u32 bytesPerFlit() const;
  u32 headerOverhead() const;
  u32 maxPacketSize() const;
  void processorComplete(u32 _id);
  void processEvent(void* _event, s32 _type) override;

 private:
  u32 numVcs_;
  u32 totalMemory_;
  u32 memorySlice_;
  u32 blockSize_;
  u32 bytesPerFlit_;
  u32 headerOverhead_;
  u32 maxPacketSize_;

  u32 remainingProcessors_;
};

}  // namespace SimpleMem

#endif  // APPLICATION_SIMPLEMEM_APPLICATION_H_
