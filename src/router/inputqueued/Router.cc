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
#include "router/inputqueued/Router.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include "architecture/util.h"
#include "congestion/CongestionSensor.h"
#include "network/Network.h"
#include "router/inputqueued/InputQueue.h"
#include "router/inputqueued/OutputQueue.h"

namespace InputQueued {

Router::Router(
    const std::string& _name, const Component* _parent, Network* _network,
    u32 _id, const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
    const std::vector<std::tuple<u32, u32> >& _protocolClassVcs,
    MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Router(_name, _parent, _network, _id, _address, _numPorts, _numVcs,
               _protocolClassVcs, _metadataHandler, _settings),
      congestionMode_(parseCongestionMode(
          _settings["congestion_mode"].asString())) {
  // determine the size of credits
  creditSize_ = numVcs_ * (u32)std::ceil(
      (f64)gSim->cycleTime(Simulator::Clock::CHANNEL) /
      (f64)gSim->cycleTime(Simulator::Clock::ROUTER));

  // queue depths
  inputQueueDepth_ = 0;
  inputQueueTailored_ = false;
  inputQueueMult_ = 0;
  inputQueueMax_ = 0;
  inputQueueMin_ = 0;
  assert(_settings.isMember("input_queue_mode"));

  if (_settings["input_queue_mode"].asString() == "tailored") {
    inputQueueTailored_ = true;
    inputQueueMult_ = _settings["input_queue_depth"].asDouble();
    assert(inputQueueMult_ > 0.0);
    // max and min queue depth
    assert(_settings.isMember("input_queue_min"));
    inputQueueMin_ = _settings["input_queue_min"].asUInt();
    assert(_settings.isMember("input_queue_max"));
    inputQueueMax_ = _settings["input_queue_max"].asUInt();
    assert(inputQueueMin_ <= inputQueueMax_);
  } else if (_settings["input_queue_mode"].asString() == "fixed") {
    inputQueueTailored_ = false;
    inputQueueDepth_ = _settings["input_queue_depth"].asUInt();
    assert(inputQueueDepth_ > 0);
  } else {
    fprintf(stderr, "Wrong input queue mode, options: tailor or fixed\n");
    assert(false);
  }

  // pipeline control
  assert(_settings.isMember("vca_swa_wait") &&
         _settings["vca_swa_wait"].isBool());
  bool vcaSwaWait = _settings["vca_swa_wait"].asBool();
  u32 outputQueueDepth = _settings["output_queue_depth"].asUInt();
  assert(outputQueueDepth > 0);

  // create a congestion status device
  congestionSensor_ = CongestionSensor::create(
      "CongestionSensor", this, this, _settings["congestion_sensor"]);

  // when running in output mode, ensure the congestion status module is not
  //  operating in normalized mode on a per-VC basis because the output queues
  //  aren't divised per-VC
  if (congestionMode_ == Router::CongestionMode::kOutput) {
    assert(
        (congestionSensor_->style() == CongestionSensor::Style::kNull) ||
        (congestionSensor_->style() != CongestionSensor::Style::kNormalized) ||
        (congestionSensor_->mode() != CongestionSensor::Mode::kVc));
  }

  // create the crossbar and schedulers
  crossbar_ = new Crossbar("Crossbar", this, numPorts_ * numVcs_, numPorts_,
                           Simulator::Clock::ROUTER, _settings["crossbar"]);
  vcScheduler_ = new VcScheduler(
      "VcScheduler", this, numPorts_ * numVcs_, numPorts_ * numVcs_,
      Simulator::Clock::ROUTER, _settings["vc_scheduler"]);
  crossbarScheduler_ = new CrossbarScheduler(
      "CrossbarScheduler", this, numPorts_ * numVcs_, numPorts_ * numVcs_,
      numPorts_, 0, Simulator::Clock::ROUTER, _settings["crossbar_scheduler"]);

  // create routing algorithms, input queues, link to routing algorithm,
  //  crossbar, and schedulers
  routingAlgorithms_.resize(numPorts_ * numVcs_);
  inputQueues_.resize(numPorts_ * numVcs_, nullptr);
  for (u32 port = 0; port < numPorts_; port++) {
    for (u32 vc = 0; vc < numVcs_; vc++) {
      u32 vcIdx = vcIndex(port, vc);

      // create the name suffix
      std::string nameSuffix = "_" + std::to_string(port) + "_" +
                               std::to_string(vc);

      // routing algorithm
      std::string rfname = "RoutingAlgorithm" + nameSuffix;
      RoutingAlgorithm* rf = network_->createRoutingAlgorithm(
          port, vc, rfname, this, this);
      routingAlgorithms_.at(vcIdx) = rf;

      // compute the client index (same for VC alloc, SW alloc, and Xbar)
      u32 clientIndex = (port * numVcs_) + vc;

      // input queue
      std::string iqName = "InputQueue" + nameSuffix;
      InputQueue* iq = new InputQueue(
          iqName, this, this, inputQueueDepth_, port, numVcs_, vc, vcaSwaWait,
          rf, vcScheduler_, clientIndex, crossbarScheduler_, clientIndex,
          crossbar_, clientIndex, congestionSensor_);
      inputQueues_.at(vcIdx) = iq;

      // register the input queue with VC and crossbar schedulers
      vcScheduler_->setClient(clientIndex, iq);
      crossbarScheduler_->setClient(clientIndex, iq);
    }
  }

  // determine the credit updates the output queue will need to provide
  bool oqDecrWatcher = congestionMode_ == Router::CongestionMode::kOutput;

  // output queues, link to crossbar
  outputQueues_.resize(numPorts_, nullptr);
  for (u32 port = 0; port < numPorts_; port++) {
    // create the name suffix
    std::string oqName = "OutputQueue_" + std::to_string(port);

    // output queue
    OutputQueue* oq = new OutputQueue(
        oqName, this, this, outputQueueDepth, port, congestionSensor_,
        oqDecrWatcher);
    outputQueues_.at(port) = oq;

    // register the output queue as the main crossbar receiver
    crossbar_->setReceiver(port, oq, 0);
  }

  // allocate slots for I/O channels
  inputChannels_.resize(numPorts_, nullptr);
  outputChannels_.resize(numPorts_, nullptr);
}

Router::~Router() {
  delete congestionSensor_;
  delete crossbar_;
  delete vcScheduler_;
  delete crossbarScheduler_;
  for (u32 vc = 0; vc < (numPorts_ * numVcs_); vc++) {
    delete routingAlgorithms_.at(vc);
    delete inputQueues_.at(vc);
  }
  for (u32 port = 0; port < numPorts_; port++) {
    delete outputQueues_.at(port);
  }
}

void Router::setInputChannel(u32 _port, Channel* _channel) {
  assert(inputChannels_.at(_port) == nullptr);
  inputChannels_.at(_port) = _channel;
  _channel->setSink(this, _port);
}

Channel* Router::getInputChannel(u32 _port) const {
  return inputChannels_.at(_port);
}

void Router::setOutputChannel(u32 _port, Channel* _channel) {
  assert(outputChannels_.at(_port) == nullptr);
  outputChannels_.at(_port) = _channel;
  _channel->setSource(this, _port);
}

Channel* Router::getOutputChannel(u32 _port) const {
  return outputChannels_.at(_port);
}

void Router::initialize() {
  // set input queue depth
  for (u32 port = 0; port < numPorts_; port++) {
    u32 queueDepth = inputQueueDepth_;
    if (inputQueueTailored_) {
      if (inputChannels_.at(port)) {
        u32 channelLatency = inputChannels_.at(port)->latency();
        queueDepth = computeTailoredBufferLength(
            inputQueueMult_, inputQueueMin_, inputQueueMax_, channelLatency);
      } else {
        // if no channel, make no queuing and inf credits
        queueDepth = 0;
      }
    }
    for (u32 vc = 0; vc < numVcs_; vc++) {
      // set depth
      u32 vcIdx = vcIndex(port, vc);
      inputQueues_.at(vcIdx)->setDepth(queueDepth);
    }
  }

  // init credits
  for (u32 port = 0; port < numPorts_; port++) {
    // donwstream queue depth
    u32 credits = inputQueueDepth_;
    if (inputQueueTailored_) {
      if (outputChannels_.at(port)) {
        u32 channelLatency = outputChannels_.at(port)->latency();
        credits = computeTailoredBufferLength(
            inputQueueMult_, inputQueueMin_, inputQueueMax_, channelLatency);
      } else {
        // if no channel, make no queuing and inf credits
        credits = U32_MAX;
      }
    }

    for (u32 vc = 0; vc < numVcs_; vc++) {
      u32 vcIdx = vcIndex(port, vc);
      // initialize the credit count in the CrossbarScheduler
      crossbarScheduler_->initCredits(vcIdx, credits);

      // initialize the credit count in the CongestionStatus for downstream
      //  queues
      congestionSensor_->initCredits(vcIdx, credits);
    }
  }
}

void Router::receiveFlit(u32 _port, Flit* _flit) {
  u32 vc = _flit->getVc();
  InputQueue* iq = inputQueues_.at(vcIndex(_port, vc));
  iq->receiveFlit(0, _flit);

  // inform base class of arrival
  if (_flit->isHead()) {
    packetArrival(_port, _flit->packet());
  }
}

void Router::receiveCredit(u32 _port, Credit* _credit) {
  while (_credit->more()) {
    u32 vc = _credit->getNum();
    u32 vcIdx = vcIndex(_port, vc);
    crossbarScheduler_->incrementCredit(vcIdx);
    if (congestionMode_ == Router::CongestionMode::kDownstream) {
      congestionSensor_->incrementCredit(vcIdx);
    }
  }
  delete _credit;
}

void Router::sendCredit(u32 _port, u32 _vc) {
  // ensure there is an outgoing credit for the next time slot
  assert(_vc < numVcs_);
  Credit* credit = inputChannels_.at(_port)->getNextCredit();
  if (credit == nullptr) {
    credit = new Credit(creditSize_);
    inputChannels_.at(_port)->setNextCredit(credit);
  }

  // mark the credit with the specified VC
  credit->putNum(_vc);
}

void Router::sendFlit(u32 _port, Flit* _flit) {
  assert(outputChannels_.at(_port)->getNextFlit() == nullptr);
  outputChannels_.at(_port)->setNextFlit(_flit);

  // inform base class of departure
  if (_flit->isHead()) {
    packetDeparture(_port, _flit->packet());
  }
}

f64 Router::congestionStatus(u32 _inputPort, u32 _inputVc,
                             u32 _outputPort, u32 _outputVc) const {
  return congestionSensor_->status(_inputPort, _inputVc, _outputPort,
                                   _outputVc);
}

Router::CongestionMode Router::parseCongestionMode(const std::string& _mode) {
  if (_mode == "output") {
    return Router::CongestionMode::kOutput;
  } else if (_mode == "downstream") {
    return Router::CongestionMode::kDownstream;
  } else {
    fprintf(stderr, "invalid congestion mode: %s\n", _mode.c_str());
    assert(false);
  }
}

}  // namespace InputQueued

registerWithObjectFactory("input_queued", ::Router,
                          InputQueued::Router, ROUTER_ARGS);
