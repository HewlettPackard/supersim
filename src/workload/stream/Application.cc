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
#include "workload/stream/Application.h"

#include <factory/Factory.h>

#include <cassert>

#include <vector>

#include "workload/NullTerminal.h"
#include "workload/stream/StreamTerminal.h"
#include "event/Simulator.h"
#include "network/Network.h"

namespace Stream {

Application::Application(
    const std::string& _name, const Component* _parent, u32 _id,
    Workload* _workload, MetadataHandler* _metadataHandler,
    Json::Value _settings)
    : ::Application(_name, _parent, _id, _workload, _metadataHandler,
                    _settings),
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
    gSim->getNetwork()->translateInterfaceIdToAddress(t, &address);
    Terminal* terminal;
    if (t == sourceTerminal_ || t == destinationTerminal_) {
      terminal = new StreamTerminal(tname, this, t, address, this,
                                    _settings["stream_terminal"]);
    } else {
      terminal = new NullTerminal(tname, this, t, address, this);
    }
    setTerminal(t, terminal);
  }
}

Application::~Application() {}

u32 Application::getSource() const {
  return sourceTerminal_;
}

u32 Application::getDestination() const {
  return destinationTerminal_;
}

f64 Application::percentComplete() const {
  StreamTerminal* t = reinterpret_cast<StreamTerminal*>(
      getTerminal(destinationTerminal_));
  return t->percentComplete();
}

void Application::start() {
  StreamTerminal* t = reinterpret_cast<StreamTerminal*>(
      getTerminal(destinationTerminal_));
  t->start();
}

void Application::stop() {
  StreamTerminal* t = reinterpret_cast<StreamTerminal*>(
      getTerminal(sourceTerminal_));
  t->stop();
  workload_->applicationDone(id_);
}

void Application::kill() {}

void Application::destinationReady(u32 _id) {
  dbgprintf("terminal = %u", _id);
  assert(_id == destinationTerminal_);

  // inform the workload that this application is ready
  workload_->applicationReady(id_);
}

void Application::destinationComplete(u32 _id) {
  dbgprintf("terminal = %u", _id);
  assert(_id == destinationTerminal_);

  // inform the workload that this application is complete
  workload_->applicationComplete(id_);
}

}  // namespace Stream

registerWithFactory("stream", ::Application, Stream::Application,
                    APPLICATION_ARGS);
