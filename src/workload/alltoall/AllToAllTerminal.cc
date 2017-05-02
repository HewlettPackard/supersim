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
#include "workload/alltoall/AllToAllTerminal.h"

#include <mut/mut.h>

#include <cassert>
#include <cmath>

#include <algorithm>

#include "network/Network.h"
#include "stats/MessageLog.h"
#include "types/Flit.h"
#include "types/Packet.h"
#include "workload/alltoall/Application.h"
#include "workload/util.h"

// these are event types
#define kRequestEvt (0xFA)
#define kResponseEvt (0x82)

// this app defines the following message OpCodes
static const u32 kRequestMsg = kRequestEvt;
static const u32 kResponseMsg = kResponseEvt;

// this app sends the following data with each request
namespace {
struct RequestData {
  u32 iteration;
};
}  // namespace

namespace AllToAll {

AllToAllTerminal::AllToAllTerminal(
    const std::string& _name, const Component* _parent, u32 _id,
    const std::vector<u32>& _address, ::Application* _app,
    Json::Value _settings)
    : ::Terminal(_name, _parent, _id, _address, _app) {
  // get the injection rate
  assert(_settings.isMember("request_injection_rate") &&
         _settings["request_injection_rate"].isDouble());
  requestInjectionRate_ = _settings["request_injection_rate"].asDouble();
  assert(requestInjectionRate_ >= 0.0 && requestInjectionRate_ <= 1.0);

  // iteration quantity limitation
  assert(_settings.isMember("num_iterations"));
  numIterations_ = _settings["num_iterations"].asUInt();
  assert(_settings.isMember("perform_barriers") &&
         _settings["perform_barriers"].isBool());
  performBarriers_ = _settings["perform_barriers"].asBool();
  inBarrier_ = false;

  // max packet size
  maxPacketSize_  = _settings["max_packet_size"].asUInt();
  assert(maxPacketSize_ > 0);

  // create a traffic pattern
  trafficPattern_ = DistributionTrafficPattern::create(
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

  // start time delay
  assert(_settings.isMember("delay"));
  delay_ = _settings["delay"].asUInt();

  // initialize the counters
  requestsSent_ = 0;
  loggableCompleteCount_ = 0;

  // initialize the iteration state
  sendIteration_ = 0;
  recvIteration_ = 0;
  sendWaitingForRecv_ = false;

  // always debug
  debug_ = true;
}

AllToAllTerminal::~AllToAllTerminal() {
  assert(sendStalled_ == false);
  assert(sendWaitingForRecv_ == false);
  assert(outstandingTransactions_.size() == 0);

  assert((sendIteration_ == recvIteration_) &&
         (sendIteration_ == numIterations_) &&
         (outstandingTransactions_.size() == 0));

  delete trafficPattern_;
  delete messageSizeDistribution_;
}

void AllToAllTerminal::processEvent(void* _event, s32 _type) {
  switch (_type) {
    case kRequestEvt:
      assert(_event == nullptr);
      sendNextRequest();
      break;

    case kResponseEvt:
      sendNextResponse(reinterpret_cast<Message*>(_event));
      break;

    default:
      assert(false);
      break;
  }
}

f64 AllToAllTerminal::percentComplete() const {
  if (numIterations_ == 0) {
    return 1.0;
  } else {
    u32 totalTransactions = numIterations_ * trafficPattern_->size();
    u32 count = loggableCompleteCount_;
    return (f64)count / (f64)totalTransactions;
  }
}

f64 AllToAllTerminal::requestInjectionRate() const {
  return requestInjectionRate_;
}

void AllToAllTerminal::start() {
  Application* app = reinterpret_cast<Application*>(application());

  if (numIterations_ == 0) {
    dbgprintf("complete");
    app->terminalComplete(id_);
  } else {
    // choose a random number of cycles in the future to start
    // make an event to start the AllToAllTerminal in the future
    if (requestInjectionRate_ > 0.0) {
      u32 maxMsg = messageSizeDistribution_->maxMessageSize();
      u64 cycles = cyclesToSend(requestInjectionRate_, maxMsg);
      cycles = gSim->rnd.nextU64(delay_, delay_ + cycles * 3);
      u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, 1) +
          ((cycles - 1) * gSim->cycleTime(Simulator::Clock::CHANNEL));
      dbgprintf("start time is %lu", time);
      addEvent(time, 0, nullptr, kRequestEvt);
    } else {
      dbgprintf("not running");
    }
  }
}

void AllToAllTerminal::exitBarrier() {
  assert(performBarriers_);
  assert(!sendWaitingForRecv_);

  // make sure we are in a barrier
  assert(inBarrier_);
  assert(recvIteration_ == sendIteration_);

  // start sending again
  dbgprintf("unwaiting");
  inBarrier_ = false;
  if (sendIteration_ != numIterations_) {
    u64 reqTime = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);
    addEvent(reqTime, 0, nullptr, kRequestEvt);
  }
}

void AllToAllTerminal::handleDeliveredMessage(Message* _message) {
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

    // check if complete (send was last requests only)
    if (!enableResponses_) {
      checkCompletion();
    }
  }
}

void AllToAllTerminal::handleReceivedMessage(Message* _message) {
  Application* app = reinterpret_cast<Application*>(application());
  u32 msgType = _message->getOpCode();
  u64 transId = _message->getTransaction();

  assert(!(sendWaitingForRecv_ && sendStalled_));

  // handle requests (sends)
  if (msgType == kRequestMsg) {
    // pull out the request's iteration and delete the RequestData
    RequestData* reqData = reinterpret_cast<RequestData*>(_message->getData());
    u32 reqIteration = reqData->iteration;
    delete reqData;
    _message->setData(nullptr);

    // verify proper iteration received
    assert(reqIteration >= recvIteration_);
    assert(reqIteration <= recvIteration_ + 1);

    // add the received message to the appropriate iteration
    std::unordered_set<u32>& iterationReceivedSet =
        iterationReceived_[reqIteration];  // creates if not present
    u32 sourceId = _message->getSourceId();
    bool res = iterationReceivedSet.insert(sourceId).second;
    (void)res;  // unused
    assert(res);

    // try to clean up iterations in order
    bool advRecvIter = false;
    while (true) {
      // retrieve the set we care about (may not exist)
      std::unordered_set<u32>& iterationReceivedSet2 =
          iterationReceived_[recvIteration_];

      // check if set is full (iteration complete)
      if (iterationReceivedSet2.size() == trafficPattern_->size()) {
        // remove the current iteration state
        u32 cnt = iterationReceived_.erase(recvIteration_);
        (void)cnt;  // unused
        assert(cnt == 1);

        // advance to the next iteration
        recvIteration_++;
        advRecvIter = true;
      } else {
        // to clean up in order, break here
        break;
      }
    }

    // perform barrier entrance
    if (advRecvIter && performBarriers_) {
      app->terminalAtBarrier(id());
    }

    // handle the case where the send operation is stalled by the recv operation
    if (sendWaitingForRecv_ && (recvIteration_ >= sendIteration_)) {
      dbgprintf("unwaiting");
      assert(!performBarriers_);

      // disable the waiting flag
      sendWaitingForRecv_ = false;

      // schedule the next request
      u64 reqTime = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);
      addEvent(reqTime, 0, nullptr, kRequestEvt);
    }
  }

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

  // check if complete (recv came last) or
  // check if complete (send was last requests and responses)
  checkCompletion();

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
    u64 reqTime = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);
    addEvent(reqTime, 0, nullptr, kRequestEvt);
  }

  // delete the message if no longer needed
  if ((!enableResponses_ && msgType == kRequestMsg) ||
      (msgType == kResponseMsg)) {
    delete _message;
  }
}

void AllToAllTerminal::completeTracking(Message* _message) {
  // remove this transaction from the tracker
  u32 res = outstandingTransactions_.erase(_message->getTransaction());
  (void)res;  // unused
  assert(res == 1);

  // end the transaction
  endTransaction(_message->getTransaction());
}

void AllToAllTerminal::completeLoggable(Message* _message) {
  // clear the logging entry
  u64 res = transactionsToLog_.erase(_message->getTransaction());
  (void)res;  // unused
  assert(res == 1);

  // log the message/transaction
  Application* app = reinterpret_cast<Application*>(application());
  app->workload()->messageLog()->endTransaction(_message->getTransaction());
  loggableCompleteCount_++;
}

void AllToAllTerminal::checkCompletion() {
  // detect when done
  if ((sendIteration_ == recvIteration_) &&
      (sendIteration_ == numIterations_) &&
      (outstandingTransactions_.size() == 0)) {
    dbgprintf("complete");
    Application* app = reinterpret_cast<Application*>(application());
    app->terminalComplete(id_);
  }
}

void AllToAllTerminal::sendNextRequest() {
  Application* app = reinterpret_cast<Application*>(application());

  // determine if another request can be generated
  assert(!inBarrier_);
  if (sendIteration_ > recvIteration_) {
    // the send operation has completed before the recv operation
    dbgprintf("waiting");
    assert(!performBarriers_);
    assert(!sendStalled_);
    sendWaitingForRecv_ = true;
  } else if ((enableResponses_) &&
             (maxOutstandingTransactions_ != 0) &&
             (outstandingTransactions_.size() == maxOutstandingTransactions_)) {
    // can't generate a new request because the tracker is full
    // dbgprintf("stalled");
    assert(!sendWaitingForRecv_);
    sendStalled_ = true;
  } else {
    assert((!enableResponses_) ||
           (maxOutstandingTransactions_ == 0) ||
           (outstandingTransactions_.size() < maxOutstandingTransactions_));
    assert(!sendStalled_);
    assert(!sendWaitingForRecv_);

    // generate a new request
    u32 destination = trafficPattern_->nextDestination();
    u32 messageSize = messageSizeDistribution_->nextMessageSize();
    u32 trafficClass = requestTrafficClass_;
    u64 transaction = createTransaction();
    u32 msgType = kRequestMsg;
    u32 sendIteration = sendIteration_;

    // handle this iteration's distribution
    if (trafficPattern_->complete()) {
      // if barriers used, wait for barrier to start next send iteration
      if (performBarriers_) {
        inBarrier_ = true;
      }
      sendIteration_++;
      trafficPattern_->reset();
    }

    // start tracking the transaction
    // dbgprintf("insert trans = %lu", transaction);
    bool res = outstandingTransactions_.insert(transaction).second;
    (void)res;  // unused
    assert(res);

    // register the transaction for logging
    bool res2 = transactionsToLog_.insert(transaction).second;
    (void)res2;  // unused
    assert(res2);
    app->workload()->messageLog()->startTransaction(transaction);

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

    // set the data
    RequestData* reqData = new RequestData();
    reqData->iteration = sendIteration;
    message->setData(reqData);

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
    if (!inBarrier_ && sendIteration_ < numIterations_) {
      u64 cycles = cyclesToSend(requestInjectionRate_, messageSize);
      u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, cycles);
      if (time == gSim->time()) {
        sendNextRequest();
      } else {
        addEvent(time, 0, nullptr, kRequestEvt);
      }
    } else {
      dbgprintf("waiting");
    }
  }
}

void AllToAllTerminal::sendNextResponse(Message* _request) {
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

}  // namespace AllToAll
