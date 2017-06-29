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
#include "network/hyperx/DimOrderRoutingAlgorithm.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include <unordered_set>

#include "network/hyperx/util.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace HyperX {

DimOrderRoutingAlgorithm::DimOrderRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _dimensionWidths, _dimensionWeights,
                       _concentration, _settings) {
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

  std::string outputType = _settings["output_type"].asString();
  if (outputType == "port") {
    outputTypePort_ = true;
  } else if (outputType == "vc") {
    outputTypePort_ = false;
  } else {
    fprintf(stderr, "Unknown output type:");
    fprintf(stderr, " '%s'\n", outputType.c_str());
    assert(false);
  }

  maxOutputs_ = _settings["max_outputs"].asUInt();
}

DimOrderRoutingAlgorithm::~DimOrderRoutingAlgorithm() {}

void DimOrderRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  if (outputTypePort_) {
    dimOrderPortRoutingOutput(
        router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
        concentration_, destinationAddress, {baseVc_}, 1, baseVc_ + numVcs_,
        &vcPool_);
    makeOutputPortSet(&vcPool_, {baseVc_}, 1, baseVc_ + numVcs_, maxOutputs_,
                      outputAlg_, &outputPorts_);
  } else {
    dimOrderVcRoutingOutput(
        router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
        concentration_, destinationAddress, {baseVc_}, 1, baseVc_ + numVcs_,
        &vcPool_);
    makeOutputVcSet(&vcPool_, maxOutputs_, outputAlg_, &outputPorts_);
  }

  if (outputPorts_.empty()) {
    // we can use any VC to eject packet
    for (u64 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
      _response->add(destinationAddress->at(0), vc);
    }
    return;
  } else {
    for (auto& it : outputPorts_) {
      _response->add(std::get<0>(it), std::get<1>(it));
    }
  }
}

}  // namespace HyperX

registerWithObjectFactory("dimension_order", HyperX::RoutingAlgorithm,
                          HyperX::DimOrderRoutingAlgorithm,
                          HYPERX_ROUTINGALGORITHM_ARGS);
