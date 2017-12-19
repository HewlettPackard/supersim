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
#include "router/inputoutputqueued/Router.h"

#include <factory/Factory.h>

#include <cassert>
#include <cmath>

#include "congestion/CongestionSensor.h"
#include "network/Network.h"
#include "router/inputoutputqueued/Ejector.h"
#include "router/inputoutputqueued/InputQueue.h"
#include "router/inputoutputqueued/OutputQueue.h"

namespace InputOutputQueued {

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

  // queue depths and pipeline control
  inputQueueDepth_ = _settings["input_queue_depth"].asUInt();
  assert(inputQueueDepth_ > 0);
  assert(_settings.isMember("vca_swa_wait") &&
         _settings["vca_swa_wait"].isBool());
  bool vcaSwaWait = _settings["vca_swa_wait"].asBool();
  outputQueueDepth_ = _settings["output_queue_depth"].asUInt();
  assert(outputQueueDepth_ > 0);

  // create a congestion status device
  congestionSensor_ = CongestionSensor::create(
      "CongestionSensor", this, this, _settings["congestion_sensor"]);

  // create crossbar and schedulers
  crossbar_ = new Crossbar(
      "Crossbar", this, numPorts_ * numVcs_, numPorts_ * numVcs_,
      Simulator::Clock::ROUTER, _settings["crossbar"]);
  vcScheduler_ = new VcScheduler(
      "VcScheduler", this, numPorts_ * numVcs_, numPorts_ * numVcs_,
      Simulator::Clock::ROUTER, _settings["vc_scheduler"]);
  crossbarScheduler_ = new CrossbarScheduler(
      "CrossbarScheduler", this, numPorts_ * numVcs_, numPorts_ * numVcs_,
      numPorts_ * numVcs_, 0, Simulator::Clock::ROUTER,
      _settings["crossbar_scheduler"]);

  // determine the credit updates the input queue will need to provide
  bool iqDecrWatcher =
      ((congestionMode_ == Router::CongestionMode::kOutput) ||
       (congestionMode_ == Router::CongestionMode::kOutputAndDownstream));

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
      u32 clientIndex = vcIdx;

      // input queue
      std::string iqName = "InputQueue" + nameSuffix;
      InputQueue* iq = new InputQueue(
          iqName, this, this, inputQueueDepth_, port, numVcs_, vc, vcaSwaWait,
          rf, vcScheduler_, clientIndex, crossbarScheduler_, clientIndex,
          crossbar_, clientIndex, congestionSensor_, iqDecrWatcher);
      inputQueues_.at(vcIdx) = iq;

      // register the input queue with VC and crossbar schedulers
      vcScheduler_->setClient(clientIndex, iq);
      crossbarScheduler_->setClient(clientIndex, iq);
    }
  }

  // determine the credit updates the output queue will need to provide
  bool oqIncrWatcher = congestionMode_ == Router::CongestionMode::kOutput;
  bool oqDecrWatcher = congestionMode_ == Router::CongestionMode::kDownstream;

  // output queues, schedulers, and crossbar
  outputQueues_.resize(numPorts_ * numVcs_, nullptr);
  outputCrossbarSchedulers_.resize(numPorts_, nullptr);
  outputCrossbars_.resize(numPorts_, nullptr);
  ejectors_.resize(numPorts_, nullptr);
  for (u32 port = 0; port < numPorts_; port++) {
    // output port switch allocator
    std::string outputCrossbarSchedulerName =
        "OutputCrossbarScheduler_" + std::to_string(port);
    outputCrossbarSchedulers_.at(port) = new CrossbarScheduler(
        outputCrossbarSchedulerName, this, numVcs_, numVcs_, 1, port * numVcs_,
        Simulator::Clock::CHANNEL, _settings["output_crossbar_scheduler"]);

    // output crossbar
    std::string outputCrossbarName = "OutputCrossbar_" + std::to_string(port);
    outputCrossbars_.at(port) = new Crossbar(
        outputCrossbarName, this, numVcs_, 1, Simulator::Clock::CHANNEL,
        _settings["output_crossbar"]);

    // ejector
    std::string ejName = "Ejector_" + std::to_string(port);
    ejectors_.at(port) = new Ejector(ejName, this, port);
    outputCrossbars_.at(port)->setReceiver(0, ejectors_.at(port), 0);

    // queues
    for (u32 vc = 0; vc < numVcs_; vc++) {
      u32 vcIdx = vcIndex(port, vc);

      // create the name suffix
      std::string nameSuffix = "_" + std::to_string(port) + "_" +
          std::to_string(vc);

      // compute the client indexes
      u32 clientIndexOut = vc;  // sw alloc and output crossbar
      u32 clientIndexMain = vcIdx;  // main crossbar

      // output queue
      std::string oqName = "OutputQueue" + nameSuffix;
      OutputQueue* oq = new OutputQueue(
          oqName, this, outputQueueDepth_, port, vc,
          outputCrossbarSchedulers_.at(port), clientIndexOut,
          outputCrossbars_.at(port), clientIndexOut, crossbarScheduler_,
          clientIndexMain, congestionSensor_, clientIndexMain, oqIncrWatcher,
          oqDecrWatcher);
      outputQueues_.at(clientIndexMain) = oq;

      // register the output queue with switch allocator
      outputCrossbarSchedulers_.at(port)->setClient(clientIndexOut, oq);

      // register the output queue as the main crossbar receiver
      crossbar_->setReceiver(clientIndexMain, oq, 0);
    }
  }

  // allocate slots for I/O channels
  inputChannels_.resize(numPorts_, nullptr);
  outputChannels_.resize(numPorts_, nullptr);
}

Router::~Router() {
  delete congestionSensor_;
  delete vcScheduler_;
  delete crossbarScheduler_;
  delete crossbar_;
  for (u32 vc = 0; vc < (numPorts_ * numVcs_); vc++) {
    delete routingAlgorithms_.at(vc);
    delete inputQueues_.at(vc);
    delete outputQueues_.at(vc);
  }
  for (u32 port = 0; port < numPorts_; port++) {
    delete outputCrossbarSchedulers_.at(port);
    delete outputCrossbars_.at(port);
    delete ejectors_.at(port);
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
  // init credits
  for (u32 port = 0; port < numPorts_; port++) {
    for (u32 vc = 0; vc < numVcs_; vc++) {
      u32 vcIdx = vcIndex(port, vc);

      // initialize the credit count in the CrossbarScheduler
      crossbarScheduler_->initCredits(vcIdx, outputQueueDepth_);

      // initialize the credit count in the CongestionSensor for downstream
      //  queues
      if ((congestionMode_ == Router::CongestionMode::kDownstream) ||
          (congestionMode_ == Router::CongestionMode::kOutputAndDownstream)) {
        congestionSensor_->initCredits(vcIdx, inputQueueDepth_);
      }

      // initialize the credit count in the OutputCrossbarScheduler
      outputCrossbarSchedulers_.at(port)->initCredits(vc, inputQueueDepth_);

      // initialize the credit count in the CongestionSensor for output queues
      if ((congestionMode_ == Router::CongestionMode::kOutput) ||
          (congestionMode_ == Router::CongestionMode::kOutputAndDownstream)) {
        congestionSensor_->initCredits(vcIdx, outputQueueDepth_);
      }

      // if congestion mode sees output and downstream queues, account for the
      //  one extra buffer slot in the output queue switch allocation pipeline
      if (congestionMode_ == Router::CongestionMode::kOutputAndDownstream) {
        congestionSensor_->initCredits(vcIdx, 1);
      }
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

    // give the output crossbar a credit
    outputCrossbarSchedulers_.at(_port)->incrementCredit(vc);

    // if the downstream credits are part of congestion, give the congestion
    //  status module a credit
    if ((congestionMode_ == Router::CongestionMode::kDownstream) ||
        (congestionMode_ == Router::CongestionMode::kOutputAndDownstream)) {
      u32 vcIdx = vcIndex(_port, vc);
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
  } else if (_mode == "output_and_downstream") {
    return Router::CongestionMode::kOutputAndDownstream;
  } else {
    fprintf(stderr, "invalid congestion mode: %s\n", _mode.c_str());
    assert(false);
  }
}

}  // namespace InputOutputQueued

registerWithFactory("input_output_queued", ::Router,
                    InputOutputQueued::Router, ROUTER_ARGS);
