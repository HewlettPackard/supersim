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
#include "network/hyperx/MinRoutingAlgorithm.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include <utility>  // std::pair

#include "types/Message.h"
#include "types/Packet.h"

namespace HyperX {

MinRoutingAlgorithm::MinRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights,
    u32 _concentration, Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs,
                       _inputPort, _inputVc, _dimensionWidths,
                       _dimensionWeights, _concentration, _settings) {
  // VC set mapping:
  //  0 = injection from terminal port
  //  1 = switching dimension increments VC count
  //  ...
  //  N = last hop to dimension N
  //  we can eject flit to destination terminal using any VC
  assert(_numVcs >= _dimensionWidths.size());
  assert(_settings.isMember("minimal") && _settings["minimal"].isString());
  assert(_settings.isMember("output_type") &&
         _settings["output_type"].isString());
  assert(_settings.isMember("max_outputs") &&
         _settings["max_outputs"].isUInt());

  assert(_settings.isMember("output_algorithm") &&
         _settings["output_algorithm"].isString());
  if (_settings["output_algorithm"].asString() == "random") {
    outputAlg_ = OutputAlg::Rand;
  } else if (_settings["output_algorithm"].asString() == "minimal") {
    outputAlg_ = OutputAlg::Min;
  } else {
    fprintf(stderr, "Unknown output algorithm:");
    fprintf(stderr, " '%s'\n",
            _settings["output_algorithm"].asString().c_str());
    assert(false);
  }

  maxOutputs_ = _settings["max_outputs"].asUInt();

  std::string minimalType = _settings["minimal"].asString();
  std::string outputType = _settings["output_type"].asString();

  if (minimalType == "random") {
    if (outputType == "vc") {
      routingAlg_ = MinRoutingAlg::RMINV;
    } else if (outputType == "port") {
      routingAlg_ = MinRoutingAlg::RMINP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else if (minimalType == "adaptive") {
    if (outputType == "vc") {
      routingAlg_ = MinRoutingAlg::AMINV;
    } else if (outputType == "port") {
      routingAlg_ = MinRoutingAlg::AMINP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else if (minimalType == "dimension_order") {
    fprintf(stderr, "Available only for Non-Minimal Algorithms\n");
    assert(false);
  } else {
    fprintf(stderr, "Unknown minimal algorithm:");
    fprintf(stderr, " '%s'\n", minimalType.c_str());
    assert(false);
  }
}

MinRoutingAlgorithm::~MinRoutingAlgorithm() {}

void MinRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();

  Packet* packet = _flit->packet();
  u32 vcSet = U32_MAX;
  u32 numVcSets = dimensionWidths_.size();
  if (packet->getHopCount() == 0) {
    vcSet = baseVc_;
  } else {
    vcSet = baseVc_ + (_flit->getVc() - baseVc_ + 1) % numVcSets;
  }

  switch (routingAlg_) {
    case MinRoutingAlg::RMINV: {
      randMinVcRoutingOutput(
          router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
          concentration_, destinationAddress, {vcSet}, numVcSets,
          baseVc_ + numVcs_, &vcPool_);
      makeOutputVcSet(&vcPool_, maxOutputs_, outputAlg_, &outputPorts_);
      break;
    }
    case MinRoutingAlg::RMINP: {
      randMinPortRoutingOutput(
          router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
          concentration_, destinationAddress, {vcSet}, numVcSets,
          baseVc_ + numVcs_, &vcPool_);
      makeOutputPortSet(&vcPool_, {vcSet}, numVcSets, baseVc_ + numVcs_,
                        maxOutputs_, outputAlg_, &outputPorts_);
      break;
    }
    case MinRoutingAlg::AMINV: {
      adaptiveMinVcRoutingOutput(
          router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
          concentration_, destinationAddress, {vcSet}, numVcSets,
          baseVc_ + numVcs_, &vcPool_);
      makeOutputVcSet(&vcPool_, maxOutputs_, outputAlg_, &outputPorts_);
      break;
    }
    case MinRoutingAlg::AMINP: {
      adaptiveMinPortRoutingOutput(
          router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
          concentration_, destinationAddress, {vcSet}, numVcSets,
          baseVc_ + numVcs_, &vcPool_);
      makeOutputPortSet(&vcPool_, {vcSet}, numVcSets, baseVc_ + numVcs_,
                        maxOutputs_, outputAlg_, &outputPorts_);
      break;
    }
    default: {
      fprintf(stderr, "Unknown routing algorithm\n");
      assert(false);
    }
  }

  if (outputPorts_.empty()) {
    // we can use any VC to eject packet
    for (u64 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
      _response->add(destinationAddress->at(0), vc);
    }
    return;
  }

  for (auto& it : outputPorts_) {
    if (packet->getHopCount() > 0) {
      assert(vcSet > 0);
    }
    _response->add(std::get<0>(it), std::get<1>(it));
  }
}

}  // namespace HyperX

registerWithObjectFactory("minimal", HyperX::RoutingAlgorithm,
                    HyperX::MinRoutingAlgorithm,
                    HYPERX_ROUTINGALGORITHM_ARGS);
