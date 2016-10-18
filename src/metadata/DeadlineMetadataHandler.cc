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
#include "metadata/DeadlineMetadataHandler.h"

#include <cassert>

#include <string>

#include "workload/Application.h"
#include "event/Simulator.h"
#include "types/Message.h"
#include "types/Packet.h"

DeadlineMetadataHandler::DeadlineMetadataHandler(Json::Value _settings) {
  assert(_settings.isMember("delay"));
  delay_ = _settings["delay"].asUInt64();
  std::string alg = _settings["algorithm"].asString();
  if (alg == "message") {
    alg_ = Algorithm::kMessage;
  } else if (alg == "transaction") {
    alg_ = Algorithm::kTransaction;
  } else {
    fprintf(stderr, "invalid deadline algorithm: %s\n", alg.c_str());
    assert(false);
  }
}

DeadlineMetadataHandler::~DeadlineMetadataHandler() {}

void DeadlineMetadataHandler::packetInjection(Application* _app,
                                              Packet* _packet) {
  u64 metadata = U64_MAX;
  switch (alg_) {
    case Algorithm::kMessage:
      metadata = gSim->time() + delay_;
      break;
    case Algorithm::kTransaction:
      metadata = _app->transactionCreationTime(
          _packet->message()->getTransaction());
      break;
    default:
      assert(false);
      break;
  }
  _packet->setMetadata(metadata);
}

void DeadlineMetadataHandler::packetArrival(Packet* _packet) {
  // this isn't used in this handler
}
