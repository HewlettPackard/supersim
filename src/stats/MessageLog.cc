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
#include "stats/MessageLog.h"

#include <cassert>

#include <string>
#include <sstream>

#include "types/Packet.h"
#include "types/Flit.h"

MessageLog::MessageLog(Json::Value _settings)
  : outFile_(_settings["file"].asString()) {
  assert(!_settings["file"].isNull());
}

MessageLog::~MessageLog() {}

void MessageLog::logMessage(const Message* _message) {
  std::stringstream ss;
  ss << "+M" << ',';
  ss << _message->getId() << ',';
  ss << _message->getSourceId() << ',';
  ss << _message->getDestinationId() << ',';
  ss << _message->getTransaction() << '\n';
  for (u32 p = 0; p < _message->numPackets(); p++) {
    Packet* packet = _message->getPacket(p);
    ss << " +P" << ',';
    ss << packet->getId() << ',';
    ss << packet->getHopCount() << '\n';
    for (u32 f = 0; f < packet->numFlits(); f++) {
      Flit* flit = packet->getFlit(f);
      ss << "   F" << ',';
      ss << flit->getId() << ',';
      ss << flit->getSendTime() << ',';
      ss << flit->getReceiveTime() << '\n';
    }
    ss << " -P\n";
  }
  ss << "-M\n";

  outFile_.write(ss.str());
}

void MessageLog::startTransaction(u64 _trans) {
  std::stringstream ss;
  ss << "+T" << ',' << _trans << '\n';
  outFile_.write(ss.str());
}

void MessageLog::endTransaction(u64 _trans) {
  std::stringstream ss;
  ss << "-T" << ',' << _trans << '\n';
  outFile_.write(ss.str());
}
