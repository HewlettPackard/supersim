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
#include "application/simplemem/Application.h"

#include <cassert>

#include <vector>

#include "application/simplemem/MemoryTerminal.h"
#include "application/simplemem/ProcessorTerminal.h"
#include "event/Simulator.h"
#include "network/Network.h"

#define ISPOW2INT(X) (((X) != 0) && !((X) & ((X) - 1)))  /*glibc trick*/
#define ISPOW2(X) (ISPOW2INT(X) == 0 ? false : true)

namespace SimpleMem {

Application::Application(const std::string& _name, const Component* _parent,
                         MetadataHandler* _metadataHandler,
                         Json::Value _settings)
    : ::Application(_name, _parent, _metadataHandler, _settings) {
  numVcs_ = gSim->getNetwork()->numVcs();
  assert(numVcs_ > 0);

  // check the memory system setup
  memorySlice_ = _settings["memory_slice"].asUInt();
  totalMemory_ = memorySlice_ * (numTerminals() / 2);
  blockSize_ = _settings["block_size"].asUInt();
  assert(ISPOW2(blockSize_));
  assert((memorySlice_ % blockSize_) == 0);

  bytesPerFlit_ = _settings["bytes_per_flit"].asUInt();
  assert(bytesPerFlit_ > 0);
  headerOverhead_ = _settings["header_overhead"].asUInt();
  maxPacketSize_ = _settings["max_packet_size"].asUInt();

  // create terminals
  remainingProcessors_ = 0;
  for (u32 t = 0; t < numTerminals(); t++) {
    std::vector<u32> address;
    gSim->getNetwork()->translateIdToAddress(t, &address);
    std::string idStr = std::to_string(t / 2);
    if ((t & 0x1) == 0) {
      // even terminals are memory terminals
      std::string tname = "MemoryTerminal_" + idStr;
      MemoryTerminal* terminal = new MemoryTerminal(
          tname, this, t, address, memorySlice_, this,
          _settings["memory_terminal"]);
      setTerminal(t, terminal);
    } else {
      // odd terminals are processor terminals
      std::string tname = "ProcessorTerminal_" + idStr;
      ProcessorTerminal* terminal = new ProcessorTerminal(
          tname, this, t, address, this, _settings["processor_terminal"]);
      setTerminal(t, terminal);
      remainingProcessors_++;
    }
  }

  // this application always wants monitor
  addEvent(0, 0, nullptr, 0);
}

Application::~Application() {}

f64 Application::percentComplete() const {
  f64 percentSum = 0.0;
  u32 processorCount = 0;
  // odd terminals are processor terminals
  for (u32 idx = 1; idx < numTerminals(); idx += 2) {
    ProcessorTerminal* t =
        reinterpret_cast<ProcessorTerminal*>(getTerminal(idx));
    percentSum += t->percentComplete();
    processorCount++;
  }
  return percentSum / processorCount;
}

u32 Application::numVcs() const {
  return numVcs_;
}

u32 Application::totalMemory() const {
  return totalMemory_;
}

u32 Application::memorySlice() const {
  return memorySlice_;
}

u32 Application::blockSize() const {
  return blockSize_;
}

u32 Application::bytesPerFlit() const {
  return bytesPerFlit_;
}

u32 Application::headerOverhead() const {
  return headerOverhead_;
}

u32 Application::maxPacketSize() const {
  return maxPacketSize_;
}

void Application::processorComplete(u32 _id) {
  remainingProcessors_--;
  if (remainingProcessors_ == 0) {
    dbgprintf("Processing complete");
    gSim->endMonitoring();
  }
}

void Application::processEvent(void* _event, s32 _type) {
  dbgprintf("SimpleMem application starting\n");
  gSim->startMonitoring();
}

}  // namespace SimpleMem
