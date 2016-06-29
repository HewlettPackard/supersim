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
#include "application/singlestream/Application.h"

#include <cassert>

#include <vector>

#include "application/singlestream/StreamTerminal.h"
#include "event/Simulator.h"
#include "network/Network.h"

namespace SingleStream {

Application::Application(const std::string& _name, const Component* _parent,
                         MetadataHandler* _metadataHandler,
                         Json::Value _settings)
    : ::Application(_name, _parent, _metadataHandler, _settings),
      doMonitoring_(true) {
  // the index of the pair of communicating terminals
  s32 src = _settings["source_terminal"].asInt();
  if (src < 0) {
    sourceTerminal_ = gSim->rnd.nextU64(0, numTerminals() - 1);
  } else {
    sourceTerminal_ = src;
  }
  dbgprintf("source terminal is %u", sourceTerminal_);

  s32 dst = _settings["destination_terminal"].asInt();
  if (dst < 0) {
    do {
      destinationTerminal_ = gSim->rnd.nextU64(0, numTerminals() - 1);
    } while ((numTerminals() != 1) &&
             (destinationTerminal_ == sourceTerminal_));
  } else {
    destinationTerminal_ = dst;
  }
  dbgprintf("destination terminal is %u", destinationTerminal_);
  assert(sourceTerminal_ < numTerminals());
  assert(destinationTerminal_ < numTerminals());

  // all terminals are the same
  for (u32 t = 0; t < numTerminals(); t++) {
    std::string tname = "Terminal_" + std::to_string(t);
    std::vector<u32> address;
    gSim->getNetwork()->translateTerminalIdToAddress(t, &address);
    StreamTerminal* terminal = new StreamTerminal(
        tname, this, t, address, this, _settings["terminal"]);
    setTerminal(t, terminal);
  }
}

Application::~Application() {}

f64 Application::percentComplete() const {
  StreamTerminal* t = reinterpret_cast<StreamTerminal*>(
      getTerminal(destinationTerminal_));
  return t->percentComplete();
}

u32 Application::getSource() const {
  return sourceTerminal_;
}

u32 Application::getDestination() const {
  return destinationTerminal_;
}

void Application::receivedFirst(u32 _id) {
  dbgprintf("receivedFirst(%u)", _id);
  assert(_id == destinationTerminal_);

  // upon first received, start monitoring
  printf("starting monitoring\n");
  if (doMonitoring_) {
    gSim->startMonitoring();
  }
}

void Application::sentLast(u32 _id) {
  dbgprintf("sentLast(%u)", _id);
  assert(_id == sourceTerminal_);

  // after last sent message, stop monitoring
  printf("ending monitoring\n");
  if (gSim->getMonitoring() == false) {
    doMonitoring_ = false;
    printf("*** monitoring attempted to end before starting.\n"
           "*** you need to have a larger num_messages value.\n"
           "*** simulation will continue without monitoring.\n");
  } else {
    gSim->endMonitoring();
  }
}

}  // namespace SingleStream
