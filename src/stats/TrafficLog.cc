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
#include "stats/TrafficLog.h"

#include <cassert>

#include <string>
#include <sstream>

#include "event/Simulator.h"
#include "types/Packet.h"
#include "types/Flit.h"

TrafficLog::TrafficLog(Json::Value _settings)
    : outFile_(nullptr) {
  if (!_settings["file"].isNull()) {
    // create file
    outFile_ = new fio::OutFile(_settings["file"].asString());

    // write header
    outFile_->write("time,device,inputPort,inputVc,outputPort,outputVc,"
                    "flits\n");
  }
}

TrafficLog::~TrafficLog() {
  if (outFile_) {
    delete outFile_;
  }
}

void TrafficLog::logTraffic(
    const Component* _device, u32 _inputPort, u32 _inputVc, u32 _outputPort,
    u32 _outputVc, u32 _flits) {
  if (outFile_) {
    std::stringstream ss;
    ss << gSim->time() << ',';
    ss << _device->name() << ',';
    ss << _inputPort << ',';
    ss << _inputVc << ',';
    ss << _outputPort << ',';
    ss << _outputVc << ',';
    ss << _flits << '\n';
    outFile_->write(ss.str());
  }
}
