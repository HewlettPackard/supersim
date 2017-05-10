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
#include "workload/blast/BlastTerminal.h"

#include <fio/InFile.h>
#include <mut/mut.h>
#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include <algorithm>

#include "network/Network.h"
#include "stats/MessageLog.h"
#include "types/Flit.h"
#include "types/Packet.h"
#include "workload/blast/Application.h"
#include "workload/util.h"

// these are event types
#define kRequestEvt (0xFA)
#define kResponseEvt (0x82)

// this app defines the following message OpCodes
static const u32 kRequestMsg = kRequestEvt;
static const u32 kResponseMsg = kResponseEvt;

namespace Blast {

BlastTerminal::BlastTerminal(const std::string& _name, const Component* _parent,
                             u32 _id, const std::vector<u32>& _address,
                             ::Application* _app, Json::Value _settings)
    : ::Terminal(_name, _parent, _id, _address, _app) {
  // get the injection rate
  assert(_settings.isMember("request_injection_rate") &&
         _settings["request_injection_rate"].isDouble());
  requestInjectionRate_ = _settings["request_injection_rate"].asDouble();
  assert(requestInjectionRate_ >= 0.0 && requestInjectionRate_ <= 1.0);

  // if relative injection is specified, modify the injection accordingly
  if (_settings.isMember("relative_injection")) {
    // if a file is given, it is a csv of injection rates
    fio::InFile inf(_settings["relative_injection"].asString());
    std::string line;
    u32 lineNum = 0;
    fio::InFile::Status sts = fio::InFile::Status::OK;
    bool foundMe = false;
    for (lineNum = 0; sts == fio::InFile::Status::OK;) {
      sts = inf.getLine(&line);
      assert(sts != fio::InFile::Status::ERROR);
      if (sts == fio::InFile::Status::OK) {
        if (line.size() > 0) {
          std::vector<std::string> strs = strop::split(line, ',');
          assert(strs.size() == 1);
          f64 ri = std::stod(strs.at(0));
          assert(ri >= 0.0);
          if (lineNum == id_) {
            requestInjectionRate_ *= ri;
            foundMe = true;
            break;
          }
          lineNum++;
        }
      }
    }
    assert(foundMe);
  }

  // transaction quantity limitation
  assert(_settings.isMember("num_transactions"));
  numTransactions_ = _settings["num_transactions"].asUInt();

  // max packet size
  maxPacketSize_  = _settings["max_packet_size"].asUInt();
  assert(maxPacketSize_ > 0);

  // create a traffic pattern
  trafficPattern_ = ContinuousTrafficPattern::create(
      "TrafficPattern", this, application()->numTerminals(), id_,
      _settings["traffic_pattern"]);

  // create a message size distribution
  messageSizeDistribution_ = MessageSizeDistribution::create(
      "MessageSizeDistribution", this, _settings["message_size_distribution"]);

  // traffic class of injection of requests
  assert(_settings.isMember("request_traffic_class"));
  requestTrafficClass_ = _settings["request_traffic_class"].asUInt();

  // limited tracker entries might delay new requests from being generated,
  //  this is the flag if the send operation has been stalled
  sendStalled_ = false;

  // enablement of request/response flows
  assert(_settings.isMember("enable_responses") &&
         _settings["enable_responses"].isBool());
  enableResponses_ = _settings["enable_responses"].asBool();

  // latency of request processing
  assert(!enableResponses_ ||
         _settings.isMember("request_processing_latency"));
  requestProcessingLatency_ = _settings["request_processing_latency"].asUInt();

  // limitation of outstanding transactions
  assert(!enableResponses_ ||
         _settings.isMember("max_outstanding_transactions"));
  maxOutstandingTransactions_ =
      _settings["max_outstanding_transactions"].asUInt();

  // traffic class of injection of responses
  assert(!enableResponses_ || _settings.isMember("response_traffic_class"));
  responseTrafficClass_ = _settings["response_traffic_class"].asUInt();

  // warmup/saturation detector
  fsm_ = BlastTerminal::Fsm::WARMING;
  warmupInterval_ = _settings["warmup_interval"].asUInt();
  assert(warmupInterval_ > 0);
  warmupFlitsReceived_ = 0;
  warmupWindow_ = _settings["warmup_window"].asUInt();
  assert(warmupWindow_ >= 5);
  maxWarmupAttempts_ = _settings["warmup_attempts"].asUInt();
  assert(maxWarmupAttempts_ > 0);
  warmupAttempts_ = 0;
  enrouteSamplePos_ = 0;
  fastFailSample_ = U32_MAX;

  // choose a random number of cycles in the future to start
  // make an event to start the BlastTerminal in the future
  if (requestInjectionRate_ > 0.0) {
    u32 maxMsg = messageSizeDistribution_->maxMessageSize();
    u64 cycles = cyclesToSend(requestInjectionRate_, maxMsg);
    cycles = gSim->rnd.nextU64(1, 1 + cycles * 3);
    u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, 1) +
        ((cycles - 1) * gSim->cycleTime(Simulator::Clock::CHANNEL));
    dbgprintf("start time is %lu", time);
    addEvent(time, 0, nullptr, kRequestEvt);
  } else {
    dbgprintf("not running");
  }

  // initialize the counters
  loggableCompleteCount_ = 0;
}

BlastTerminal::~BlastTerminal() {
  assert(sendStalled_ == false);
  assert(outstandingTransactions_.size() == 0);

  delete trafficPattern_;
  delete messageSizeDistribution_;
}

void BlastTerminal::processEvent(void* _event, s32 _type) {
  switch (_type) {
    case kRequestEvt:
      assert(_event == nullptr);
      if (fsm_ != BlastTerminal::Fsm::DRAINING) {
        sendNextRequest();
      }
      break;

    case kResponseEvt:
      sendNextResponse(reinterpret_cast<Message*>(_event));
      break;

    default:
      assert(false);
      break;
  }
}

f64 BlastTerminal::percentComplete() const {
  if (fsm_ >= BlastTerminal::Fsm::LOGGING) {
    if (numTransactions_ == 0) {
      return 1.0;
    } else {
      u32 count = std::min(loggableCompleteCount_, numTransactions_);
      return (f64)count / (f64)numTransactions_;
    }
  } else {
    return 0.0;
  }
}

f64 BlastTerminal::requestInjectionRate() const {
  return requestInjectionRate_;
}

void BlastTerminal::stopWarming() {
  fsm_ = BlastTerminal::Fsm::WARM_BLABBING;
}

void BlastTerminal::startLogging() {
  // clear the samples in case it hasn't already happened
  enrouteSampleTimes_.clear();
  enrouteSampleValues_.clear();

  fsm_ = BlastTerminal::Fsm::LOGGING;
  if (requestInjectionRate_ > 0.0 && numTransactions_ == 0) {
    complete();
  }
}

void BlastTerminal::stopLogging() {
  fsm_ = BlastTerminal::Fsm::LOG_BLABBING;
  if ((requestInjectionRate_ > 0.0) &&
      (numTransactions_ == 0 || transactionsToLog_.size() == 0)) {
    done();
  }
}

void BlastTerminal::stopSending() {
  fsm_ = BlastTerminal::Fsm::DRAINING;
}

void BlastTerminal::handleDeliveredMessage(Message* _message) {
  // process for each warmup window
  if (fsm_ == BlastTerminal::Fsm::WARMING) {
    warmDetector(_message);
  }

  // handle request only transaction tracking
  u32 msgType = _message->getOpCode();
  u64 transId = _message->getTransaction();
  if (msgType == kRequestMsg) {
    if (!enableResponses_) {
      // dbgprintf("R erase trans = %lu", transId);
      completeTracking(_message);
    }

    // log message if tagged
    if (transactionsToLog_.count(transId) == 1) {
      Application* app = reinterpret_cast<Application*>(application());
      app->workload()->messageLog()->logMessage(_message);

      // end this transaction in the log if appropriate
      if (!enableResponses_) {
        completeLoggable(_message);
      }
    }
  }
}

void BlastTerminal::handleReceivedMessage(Message* _message) {
  Application* app = reinterpret_cast<Application*>(application());
  u32 msgType = _message->getOpCode();
  u64 transId = _message->getTransaction();

  // handle request/response transaction tracking
  if (msgType == kResponseMsg) {
    assert(enableResponses_);

    // complete the tracking of this transaction
    // dbgprintf("R/R erase trans = %lu", transId);
    completeTracking(_message);

    // log message if tagged
    if (transactionsToLog_.count(transId) == 1) {
      // log the message
      app->workload()->messageLog()->logMessage(_message);

      // end this transaction in the log
      completeLoggable(_message);
    }
  }

  if (enableResponses_ && msgType == kRequestMsg) {
    // signal for requests to generate responses when responses are enabled
    // register an event to process the request
    if (requestProcessingLatency_ == 0) {
      sendNextResponse(_message);
    } else {
      u64 respTime = gSim->futureCycle(Simulator::Clock::CHANNEL,
                                       requestProcessingLatency_);
      addEvent(respTime, 0, _message, kResponseEvt);
    }
  }

  if (msgType == kResponseMsg && sendStalled_) {
    // if responses are enabled and requests have been stalled due to limited
    //  tracker entries, signal a send operation to resume
    sendStalled_ = false;
    if (fsm_ != BlastTerminal::Fsm::DRAINING) {
      u64 reqTime = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);
      addEvent(reqTime, 0, nullptr, kRequestEvt);
    }
  }

  // delete the message if no longer needed
  if ((!enableResponses_ && msgType == kRequestMsg) ||
      (msgType == kResponseMsg)) {
    delete _message;
  }
}

void BlastTerminal::warmDetector(Message* _message) {
  // count flits received
  warmupFlitsReceived_ += _message->numFlits();
  if (warmupFlitsReceived_ >= warmupInterval_) {
    warmupFlitsReceived_ %= warmupInterval_;

    u32 msgs;
    u32 pkts;
    u32 flits;
    enrouteCount(&msgs, &pkts, &flits);
    dbgprintf("enroute: msgs=%u pkts=%u flits=%u", msgs, pkts, flits);

    // push this sample into the cyclic buffers
    if (enrouteSampleTimes_.size() < warmupWindow_) {
      enrouteSampleTimes_.push_back(gSim->cycle(Simulator::Clock::CHANNEL));
      enrouteSampleValues_.push_back(flits);
    } else {
      enrouteSampleTimes_.at(enrouteSamplePos_) = gSim->time();
      enrouteSampleValues_.at(enrouteSamplePos_) = flits;
      enrouteSamplePos_ = (enrouteSamplePos_ + 1) % warmupWindow_;
    }

    bool warmed = false;
    bool saturated = false;

    // run the fast fail logic for early saturation detection
    if (enrouteSampleTimes_.size() == warmupWindow_) {
      if (fastFailSample_ == U32_MAX) {
        fastFailSample_ = *std::max_element(enrouteSampleValues_.begin(),
                                            enrouteSampleValues_.end());
        dbgprintf("fast fail sample = %u", fastFailSample_);
      } else if (flits > (fastFailSample_ * 3)) {
        dbgprintf("fast fail detected");
        saturated = true;
      }
    }

    // after enough samples were taken, try to figure out network status using
    //  a sliding window linear regression
    if (enrouteSampleTimes_.size() == warmupWindow_) {
      warmupAttempts_++;
      dbgprintf("warmup attempt %u of %u",
                warmupAttempts_, maxWarmupAttempts_);
      f64 growthRate = mut::slope<u64>(enrouteSampleTimes_,
                                       enrouteSampleValues_);
      dbgprintf("growthRate: %e", growthRate);
      if (growthRate <= 0.0) {
        warmed = true;
      } else if (warmupAttempts_ == maxWarmupAttempts_) {
        saturated = true;
      }
    }

    // react to warmed or saturated
    if (warmed || saturated) {
      warm(saturated);
    }
  }
}

void BlastTerminal::warm(bool _saturated) {
  fsm_ = BlastTerminal::Fsm::WARM_BLABBING;
  Application* app = reinterpret_cast<Application*>(application());
  if (_saturated) {
    dbgprintf("saturated");
    app->terminalSaturated(id_);
  } else {
    dbgprintf("warmed");
    app->terminalWarmed(id_);
  }
  enrouteSampleTimes_.clear();
  enrouteSampleValues_.clear();
}

void BlastTerminal::complete() {
  Application* app = reinterpret_cast<Application*>(application());
  app->terminalComplete(id_);
}

void BlastTerminal::done() {
  Application* app = reinterpret_cast<Application*>(application());
  app->terminalDone(id_);
}

void BlastTerminal::completeTracking(Message* _message) {
  // remove this transaction from the tracker
  u32 res = outstandingTransactions_.erase(_message->getTransaction());
  (void)res;  // unused
  assert(res == 1);

  // end the transaction
  endTransaction(_message->getTransaction());
}

void BlastTerminal::completeLoggable(Message* _message) {
  // clear the logging entry
  u64 res = transactionsToLog_.erase(_message->getTransaction());
  (void)res;  // unused
  assert(res == 1);

  // log the message/transaction
  Application* app = reinterpret_cast<Application*>(application());
  app->workload()->messageLog()->endTransaction(_message->getTransaction());
  loggableCompleteCount_++;

  // detect when logging complete
  if (loggableCompleteCount_ == numTransactions_) {
    complete();
  }

  // detect when logging is empty
  if (fsm_ == BlastTerminal::Fsm::LOG_BLABBING) {
    if (transactionsToLog_.size() == 0) {
      done();
    }
  }
}

void BlastTerminal::sendNextRequest() {
  Application* app = reinterpret_cast<Application*>(application());

  // determine if another request can be generated
  if ((!enableResponses_) ||
      (maxOutstandingTransactions_ == 0) ||
      (outstandingTransactions_.size() < maxOutstandingTransactions_)) {
    assert(!sendStalled_);
    assert(fsm_ != BlastTerminal::Fsm::DRAINING);

    // generate a new request
    u32 destination = trafficPattern_->nextDestination();
    u32 messageSize = messageSizeDistribution_->nextMessageSize();
    u32 trafficClass = requestTrafficClass_;
    u64 transaction = createTransaction();
    u32 msgType = kRequestMsg;

    // start tracking the transaction
    // dbgprintf("insert trans = %lu", transaction);
    bool res = outstandingTransactions_.insert(transaction).second;
    (void)res;  // unused
    assert(res);

    // if in logging phase, register the transaction for logging
    if (fsm_ == BlastTerminal::Fsm::LOGGING) {
      bool res2 = transactionsToLog_.insert(transaction).second;
      (void)res2;  // unused
      assert(res2);
      app->workload()->messageLog()->startTransaction(transaction);
    }

    // determine the number of packets
    u32 numPackets = messageSize / maxPacketSize_;
    if ((messageSize % maxPacketSize_) > 0) {
      numPackets++;
    }

    // create the message object
    Message* message = new Message(numPackets, nullptr);
    message->setTrafficClass(trafficClass);
    message->setTransaction(transaction);
    message->setOpCode(msgType);

    // create the packets
    u32 flitsLeft = messageSize;
    for (u32 p = 0; p < numPackets; p++) {
      u32 packetLength = flitsLeft > maxPacketSize_ ?
          maxPacketSize_ : flitsLeft;

      Packet* packet = new Packet(p, packetLength, message);
      message->setPacket(p, packet);

      // create flits
      for (u32 f = 0; f < packetLength; f++) {
        bool headFlit = f == 0;
        bool tailFlit = f == (packetLength - 1);
        Flit* flit = new Flit(f, headFlit, tailFlit, packet);
        packet->setFlit(f, flit);
      }
      flitsLeft -= packetLength;
    }

    // send the message
    u32 msgId = sendMessage(message, destination);
    (void)msgId;  // unused

    // determine when to send the next request
    u64 cycles = cyclesToSend(requestInjectionRate_, messageSize);
    u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, cycles);
    if (time == gSim->time()) {
      sendNextRequest();
    } else {
      addEvent(time, 0, nullptr, kRequestEvt);
    }
  } else {
    assert(fsm_ != BlastTerminal::Fsm::DRAINING);

    // can't generate a new request because the tracker is full
    // dbgprintf("tracker is full, new requests are stalled");
    sendStalled_ = true;
  }
}

void BlastTerminal::sendNextResponse(Message* _request) {
  assert(enableResponses_);

  // process the request received to make a response
  u32 destination = _request->getSourceId();
  u32 messageSize = messageSizeDistribution_->nextMessageSize(_request);
  u32 trafficClass = responseTrafficClass_;
  u64 transaction = _request->getTransaction();
  // dbgprintf("turning around trans = %lu", transaction);
  u32 msgType = kResponseMsg;

  // delete the request
  delete _request;

  // determine the number of packets
  u32 numPackets = messageSize / maxPacketSize_;
  if ((messageSize % maxPacketSize_) > 0) {
    numPackets++;
  }

  // create the message object
  Message* message = new Message(numPackets, nullptr);
  message->setTrafficClass(trafficClass);
  message->setTransaction(transaction);
  message->setOpCode(msgType);

  // create the packets
  u32 flitsLeft = messageSize;
  for (u32 p = 0; p < numPackets; p++) {
    u32 packetLength = flitsLeft > maxPacketSize_ ?
        maxPacketSize_ : flitsLeft;

    Packet* packet = new Packet(p, packetLength, message);
    message->setPacket(p, packet);

    // create flits
    for (u32 f = 0; f < packetLength; f++) {
      bool headFlit = f == 0;
      bool tailFlit = f == (packetLength - 1);
      Flit* flit = new Flit(f, headFlit, tailFlit, packet);
      packet->setFlit(f, flit);
    }
    flitsLeft -= packetLength;
  }

  // send the message
  u32 msgId = sendMessage(message, destination);
  (void)msgId;  // unused
}

}  // namespace Blast
