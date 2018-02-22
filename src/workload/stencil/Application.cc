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
#include "workload/stencil/Application.h"

#include <factory/ObjectFactory.h>
#include <fio/InFile.h>
#include <strop/strop.h>

#include <cassert>

#include <tuple>
#include <vector>

#include "workload/stencil/StencilTerminal.h"
#include "event/Simulator.h"
#include "network/Network.h"

namespace Stencil {

Application::Application(
    const std::string& _name, const Component* _parent, u32 _id,
    Workload* _workload, MetadataHandler* _metadataHandler,
    Json::Value _settings)
    : ::Application(_name, _parent, _id, _workload, _metadataHandler,
                    _settings) {
  // read in the exchange matrix
  //  for each terminal, this determines:
  //   1. the destination and sizes of messages to send
  //   2. the number of messages to receive
  assert(_settings.isMember("exchange_messages") &&
         _settings["exchange_messages"].isString());

  // this is the send messages
  std::vector<std::vector<std::tuple<u32, u32> > > exchangeSendMessages(
      numTerminals());
  // this is the recv message counts
  std::vector<u32> exchangeRecvMessages(numTerminals(), 0);

  // read the file
  fio::InFile inf(_settings["exchange_messages"].asString(), fio::kDefaultDelim,
                  1048576);
  fio::InFile::Status sts = fio::InFile::Status::OK;
  u32 lineNum = 0;
  dbgprintf("reading exchange matrix");
  while (sts == fio::InFile::Status::OK) {
    std::string line;
    sts = inf.getLine(&line);
    assert(sts != fio::InFile::Status::ERROR);
    if (sts == fio::InFile::Status::OK) {
      if (line.size() > 0) {
        std::vector<std::string> strs = strop::split(line, ',');
        if (strs.size() != numTerminals()) {
          fprintf(stderr, "line has %lu lines but numTerminals is %u\n",
                  strs.size(), numTerminals());
          assert(false);
        }
        for (u32 dst = 0; dst < strs.size(); dst++) {
          u32 size = std::stoul(strs.at(dst));

          // this sending a message
          if (size > 0) {
            // register send message
            exchangeSendMessages.at(lineNum).push_back(
                std::make_tuple(dst, size));

            // register recv message
            exchangeRecvMessages.at(dst)++;
          }
        }
        lineNum++;
      }
    }
  }
  assert(lineNum == numTerminals());

  // organize the send messages
  assert(_settings.isMember("send_order") &&
         _settings["send_order"].isString());
  std::string sendOrder = _settings["send_order"].asString();
  for (u32 t = 0; t < numTerminals(); t++) {
    if (sendOrder == "ascending") {
      // do nothing, it is already in order
    } else if (sendOrder == "descending") {
      // reverse the order of the vector
      std::reverse(exchangeSendMessages.at(t).begin(),
                   exchangeSendMessages.at(t).end());
    } else if (sendOrder == "random") {
      // randomize the order of the vector
      gSim->rnd.shuffle(&exchangeSendMessages.at(t));
    } else {
      fprintf(stderr, "invalid send order: %s\n", sendOrder.c_str());
      assert(false);
    }
  }

  // all terminals are the same
  for (u32 t = 0; t < numTerminals(); t++) {
    std::string tname = "StencilTerminal_" + std::to_string(t);
    std::vector<u32> address;
    gSim->getNetwork()->translateInterfaceIdToAddress(t, &address);
    StencilTerminal* terminal = new StencilTerminal(
        tname, this, t, address,
        exchangeSendMessages.at(t),
        exchangeRecvMessages.at(t),
        this, _settings["stencil_terminal"]);
    setTerminal(t, terminal);
  }

  // initialize counters
  completedTerminals_ = 0;
  doneTerminals_ = 0;

  // this application is immediately ready
  addEvent(0, 0, nullptr, 0);
}

Application::~Application() {}

f64 Application::percentComplete() const {
  f64 percentSum = 0.0;
  for (u32 idx = 0; idx < numTerminals(); idx++) {
    StencilTerminal* t = reinterpret_cast<StencilTerminal*>(getTerminal(idx));
    percentSum += t->percentComplete();
  }
  return percentSum / numTerminals();
}

void Application::start() {
  for (u32 idx = 0; idx < numTerminals(); idx++) {
    StencilTerminal* t = reinterpret_cast<StencilTerminal*>(getTerminal(idx));
    t->start();
  }
}

void Application::stop() {
  // this application is done
  workload_->applicationDone(id_);
}

void Application::kill() {}

void Application::terminalComplete(u32 _id) {
  completedTerminals_++;
  assert(completedTerminals_ <= numTerminals());
  if (completedTerminals_ == numTerminals()) {
    dbgprintf("all terminals are done");
    workload_->applicationComplete(id_);
  }
}

void Application::processEvent(void* _event, s32 _type) {
  dbgprintf("application ready");
  workload_->applicationReady(id_);
}

}  // namespace Stencil

registerWithObjectFactory("stencil", ::Application, Stencil::Application,
                          APPLICATION_ARGS);
