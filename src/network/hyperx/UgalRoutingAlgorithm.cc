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
#include "network/hyperx/UgalRoutingAlgorithm.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include "types/Message.h"
#include "types/Packet.h"

namespace HyperX {

UgalRoutingAlgorithm::UgalRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights,
    u32 _concentration, Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs,
                       _inputPort, _inputVc, _dimensionWidths,
                       _dimensionWeights, _concentration, _settings) {
  // VC set mapping:
  //  0 = injection from terminal port, to intermediate destination
  //  1 = switching dimension increments VC count
  //  ...
  //  N = last hop to dimension N to intermediate destination
  //  N + 1 = first hop from intermediate to final destination
  //  ...
  //  2N = last hop to dimension N
  //  we can eject flit to destination terminal using any VC

  assert(_settings.isMember("minimal") && _settings["minimal"].isString());
  assert(_settings.isMember("non_minimal") &&
         _settings["non_minimal"].isString());
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
  std::string nonMinimalType = _settings["non_minimal"].asString();
  assert(_settings.isMember("short_cut"));
  shortCut_ = _settings["short_cut"].asBool();

  if (nonMinimalType == "valiants") {
    nonMinimalAlg_ = NonMinRoutingAlg::VAL;
    assert(_settings.isMember("intermediate_node") &&
           _settings["intermediate_node"].isString());
    std::string intermediateNode = _settings["intermediate_node"].asString();
    if (intermediateNode == "regular") {
      intNodeAlg_ = IntNodeAlg::REG;
    } else if (intermediateNode == "source") {
      intNodeAlg_ = IntNodeAlg::SRC;
    } else if (intermediateNode == "dest") {
      intNodeAlg_ = IntNodeAlg::DST;
    } else if (intermediateNode == "source_dest") {
      intNodeAlg_ = IntNodeAlg::SRCDST;
    } else if (intermediateNode == "unaligned") {
      intNodeAlg_ = IntNodeAlg::UNALIGNED;
    } else if (intermediateNode == "minimal_vc") {
      intNodeAlg_ = IntNodeAlg::MINV;
    } else if (intermediateNode == "minimal_port") {
      intNodeAlg_ = IntNodeAlg::MINP;
    } else {
      fprintf(stderr, "Unknown inter node algorithm:");
      fprintf(stderr, " '%s'\n", intermediateNode.c_str());
      assert(false);
    }
  } else if (nonMinimalType == "least_congested_queue") {
    if (outputType == "vc") {
      nonMinimalAlg_ = NonMinRoutingAlg::LCQV;
      intNodeAlg_ = IntNodeAlg::NONE;
    } else if (outputType == "port") {
      nonMinimalAlg_ = NonMinRoutingAlg::LCQP;
      intNodeAlg_ = IntNodeAlg::NONE;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else {
    fprintf(stderr, "Unknown non-minimal algorithm:");
    fprintf(stderr, " '%s'\n", nonMinimalType.c_str());
    assert(false);
  }

  if (outputType == "port") {
    outputTypePort_ = true;
  } else if (outputType == "vc") {
    outputTypePort_ = false;
  } else {
    fprintf(stderr, "Unknown output type:");
    fprintf(stderr, " '%s'\n", outputType.c_str());
    assert(false);
  }

  if (minimalType == "dimension_order") {
    if (outputType == "vc") {
      routingAlg_ = BaseRoutingAlg::DORV;
    } else if (outputType == "port") {
      routingAlg_ = BaseRoutingAlg::DORP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else if (minimalType == "random") {
    if (outputType == "vc") {
      routingAlg_ = BaseRoutingAlg::RMINV;
    } else if (outputType == "port") {
      routingAlg_ = BaseRoutingAlg::RMINP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else if (minimalType == "adaptive") {
    if (outputType == "vc") {
      routingAlg_ = BaseRoutingAlg::AMINV;
    } else if (outputType == "port") {
      routingAlg_ = BaseRoutingAlg::AMINP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else {
    fprintf(stderr, "Unknown minimal algorithm:");
    fprintf(stderr, " '%s'\n", minimalType.c_str());
    assert(false);
  }

  if (_settings["decision_scheme"].asString() == "monolithic_weighted") {
    assert(_settings.isMember("independent_bias"));
    iBias_ = _settings["independent_bias"].asFloat();
    decisionScheme_ = DecisionScheme::MW;
    assert(_settings.isMember("congestion_bias"));
    cBias_ = _settings["congestion_bias"].asFloat();

    assert(_settings.isMember("bias_mode"));
    if (_settings["bias_mode"].asString() == "regular") {
      biasMode_ = BiasScheme::REGULAR;
    } else if (_settings["bias_mode"].asString() == "bimodal") {
      biasMode_ = BiasScheme::BIMODAL;
    } else if (_settings["bias_mode"].asString() == "proportional") {
      biasMode_ = BiasScheme::PROPORTIONAL;
    } else if (_settings["bias_mode"].asString() == "differential") {
      biasMode_ = BiasScheme::DIFFERENTIAL;
    } else if (_settings["bias_mode"].asString() ==
        "proportional_differential") {
      biasMode_ = BiasScheme::PROPORTIONALDIF;
    } else {
      fprintf(stderr, "Unknown weighting scheme:");
      fprintf(stderr, " '%s'\n", _settings["bias_mode"].asString().c_str());
      assert(false);
    }
  } else if (_settings["decision_scheme"].asString() == "staged_threshold") {
    decisionScheme_ = DecisionScheme::ST;
    assert(_settings.isMember("threshold_min"));
    assert(_settings.isMember("threshold_nonmin"));
    thresholdMin_ = _settings["threshold_min"].asDouble();
    thresholdNonMin_ = _settings["threshold_nonmin"].asDouble();
  } else if (_settings["decision_scheme"].asString() == "threshold_weighted") {
    decisionScheme_ = DecisionScheme::TW;
    assert(_settings.isMember("threshold"));
    threshold_ = _settings["threshold"].asDouble();
  } else {
    fprintf(stderr, "Unknown decision scheme:");
    fprintf(stderr, " '%s'\n", _settings["decision_scheme"].asString().c_str());
    assert(false);
  }

  if (decisionScheme_ == DecisionScheme::MW ||
      decisionScheme_ == DecisionScheme::TW) {
    // ensure hop_count_mode if using weights
    assert(_settings.isMember("hop_count_mode"));
    if (_settings["hop_count_mode"].asString() == "absolute") {
      hopCountMode_ = HopCountMode::ABS;
    } else if (_settings["hop_count_mode"].asString() == "normalized") {
      hopCountMode_ = HopCountMode::NORM;
    } else {
      fprintf(stderr, "Unknown hop_count scheme:");
      fprintf(stderr, " '%s'\n",
              _settings["hop_count_mode"].asString().c_str());
      assert(false);
    }
  }

  if ((routingAlg_ == BaseRoutingAlg::DORV) ||
      (routingAlg_ == BaseRoutingAlg::DORP)) {
    assert(_numVcs >= 2);
  } else if ((intNodeAlg_ == IntNodeAlg::REG) ||
             (intNodeAlg_ == IntNodeAlg::UNALIGNED)) {
    assert(_numVcs >= _dimensionWidths.size() * 2);
  } else {
    assert(_numVcs >= _dimensionWidths.size() + 1);
  }
  assert(_settings.isMember("min_all_vc_sets"));
  minAllVcSets_ = _settings["min_all_vc_sets"].asBool();
}

UgalRoutingAlgorithm::~UgalRoutingAlgorithm() {}

void UgalRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  const std::vector<u32>* destAddress =
      _flit->packet()->message()->getDestinationAddress();

  Packet* packet = _flit->packet();
  f64 weightReg = 0.0, weightVal = 0.0;

  u32 vcSet = U32_MAX;
  u32 numVcSets = (routingAlg_ == BaseRoutingAlg::DORP) ||
                          (routingAlg_ == BaseRoutingAlg::DORV)
                      ? 1
                      : dimensionWidths_.size();
  if ((intNodeAlg_ == IntNodeAlg::REG) ||
      (intNodeAlg_ == IntNodeAlg::UNALIGNED)) {
    numVcSets *= 2;
  } else {
    numVcSets += 1;
  }

  if (packet->getHopCount() == 0) {
    vcSet = baseVc_;
  } else {
    if ((routingAlg_ == BaseRoutingAlg::DORP) ||
        (routingAlg_ == BaseRoutingAlg::DORV)) {
      vcSet = baseVc_ + (_flit->getVc() - baseVc_) % numVcSets;
    } else {
      vcSet = baseVc_ + (_flit->getVc() - baseVc_ + 1) % numVcSets;
    }
  }

  ugalRoutingOutput(
      router_, inputPort_, inputVc_,
      dimensionWidths_,
      dimensionWeights_, concentration_,
      vcSet, numVcSets, baseVc_ + numVcs_,
      shortCut_, minAllVcSets_, intNodeAlg_,
      routingAlg_, nonMinimalAlg_,
      _flit, &weightReg, &weightVal,
      &vcPoolReg_, &vcPoolVal_);

  const std::vector<u32>* intermediateAddress =
      reinterpret_cast<const std::vector<u32>*>(packet->getRoutingExtension());

  if (packet->getHopCount() == 0) {
    if (!intermediateAddress) {
      // int == src, valiant gave results we don't want
      vcPoolVal_.clear();
    } else {
      // verify int != src - REMOVE THIS WHEN CONFIDENT
      assert(destAddress);
      bool match = true;
      const std::vector<u32>& routerAddress = router_->address();
      for (u32 idx = 0; idx < intermediateAddress->size() - 1; idx++) {
        if (routerAddress.at(idx) != intermediateAddress->at(idx + 1)) {
          match = false;
          break;
        }
      }
      assert(!match);
      // if int == dst, valiant gave results we don't want
      match = true;
      for (u32 idx = 0; idx < intermediateAddress->size() - 1; idx++) {
        if (destAddress->at(idx + 1) != intermediateAddress->at(idx + 1)) {
          match = false;
          break;
        }
      }
      if (match) {
        delete intermediateAddress;
        intermediateAddress = nullptr;
        packet->setRoutingExtension(nullptr);
        vcPoolVal_.clear();
      }
    }
  }

  // Eject packet
  if (((vcPoolReg_.empty()) && packet->getHopCount() == 0) ||
      ((vcPoolVal_.empty()) && packet->getHopCount() > 0)) {
    if (intermediateAddress) {
      packet->setRoutingExtension(nullptr);
      delete intermediateAddress;
    }
    // assert is destination router
    const std::vector<u32>& routerAddress = router_->address();
    assert(destAddress);
    for (u32 dim = 0; dim < routerAddress.size(); dim++) {
      assert(routerAddress.at(dim) == destAddress->at(dim + 1));
    }
    // we can use any VC to eject packet
    for (u64 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
      _response->add(destAddress->at(0), vc);
    }
    return;
  }

  if ((packet->getHopCount() > 0) && (routingAlg_ != BaseRoutingAlg::DORP) &&
      (routingAlg_ != BaseRoutingAlg::DORV)) {
    assert(vcSet > 0);
  }

  u32 hopsReg = 0;
  u32 hopsVal = 0;

  if (packet->getHopCount() == 0) {
    // ex: [x,y,z] for router, [c,x,y,z] for destination
    const std::vector<u32>& routerAddress = router_->address();

    for (u32 dim = 0; dim < routerAddress.size(); dim++) {
      if (routerAddress.at(dim) != destAddress->at(dim + 1)) {
        hopsReg++;
      }
    }

    if (intermediateAddress == nullptr) {
      // Intermediate address in VAL was equal to SRC
      // Routing converges to base_algorithm
      // vcPoolVal_.empty();
      hopsVal = U32_MAX;
      assert(vcPoolVal_.empty());
    } else {
      assert(routerAddress.size() == (destAddress->size() - 1));
      for (u32 dim = 0; dim < routerAddress.size(); dim++) {
        if (intermediateAddress->at(dim + 1) != destAddress->at(dim + 1)) {
          hopsVal++;
        }
        if (routerAddress.at(dim) != intermediateAddress->at(dim + 1)) {
          hopsVal++;
        }
      }
    }

    if ((nonMinimalAlg_ == NonMinRoutingAlg::LCQV) ||
        (nonMinimalAlg_ == NonMinRoutingAlg::LCQP)) {
      // For LCQ we do not have intermediateAddress (it is always null)
      // but we take extra one hop
      hopsVal = hopsReg + 1;
    }

    u32 hopIncr = U32_MAX;
    switch (hopCountMode_) {
      case HopCountMode::ABS:
        hopIncr = hopsVal - hopsReg;
        break;
      case HopCountMode::NORM:
        hopIncr = hopsReg;
        break;
      default:
        assert(false);
    }

    assert(hopsReg > 0);
    bool nonMin = false;
    if (decisionScheme_ == DecisionScheme::MW) {
      monolithicWeighted(vcPoolReg_, vcPoolVal_,
                         hopsReg, hopIncr, iBias_, cBias_, biasMode_,
                         &vcPool_, &nonMin);
    } else if (decisionScheme_ == DecisionScheme::ST) {
      stagedThreshold(vcPoolReg_, vcPoolVal_,
                      thresholdMin_, thresholdNonMin_,
                      &vcPool_, &nonMin);
    } else if (decisionScheme_ == DecisionScheme::TW) {
      thresholdWeighted(vcPoolReg_, vcPoolVal_,
                        hopsReg, hopIncr, threshold_,
                        &vcPool_, &nonMin);
    } else {
      fprintf(stderr, "Unknown decision scheme\n");
      assert(false);
    }

    if (!nonMin) {  // minimal
      delete reinterpret_cast<const std::vector<u32>*>(
          packet->getRoutingExtension());
      packet->setRoutingExtension(nullptr);
    } else {
      if ((routingAlg_ == BaseRoutingAlg::DORP) ||
          (routingAlg_ == BaseRoutingAlg::DORV)) {
        // If we're taking deroute in UGAL-DOR, we have intermediate node, and
        // VC set = 1
        vcSet = baseVc_ + 1;
      }
    }

    if (outputTypePort_) {
      std::vector<u32> vcSets;
      if (!minAllVcSets_ || nonMin) {  // minAllVCsets off or going nonmin
        vcSets.push_back(vcSet);
      } else {  // enable all vc sets for initial hop & minimal
        if ((routingAlg_ == BaseRoutingAlg::DORV) ||
            (routingAlg_ == BaseRoutingAlg::DORP)) {
          // DOR 2 vcSets (0 and 1)
          vcSets.push_back(baseVc_ + 0);
          vcSets.push_back(baseVc_ + 1);
        } else if ((intNodeAlg_ == IntNodeAlg::REG) ||
                   (intNodeAlg_ == IntNodeAlg::UNALIGNED)) {
          // distance class vcSets 0 and Dim
          vcSets.push_back(baseVc_ + 0);
          vcSets.push_back(baseVc_ + dimensionWidths_.size());
        } else {
          // 2 VcSets (0 and 1)
          vcSets.push_back(baseVc_ + 0);
          vcSets.push_back(baseVc_ + 1);
        }
      }
      // vcSets is a vector
      makeOutputPortSet(&vcPool_, vcSets, numVcSets, baseVc_ + numVcs_,
                        maxOutputs_, outputAlg_, &outputPorts_);
    } else {
      makeOutputVcSet(&vcPool_, maxOutputs_, outputAlg_, &outputPorts_);
    }
  } else {  // hopcount > 0
    if (intermediateAddress != nullptr) {
      if ((routingAlg_ == BaseRoutingAlg::DORP) ||
          (routingAlg_ == BaseRoutingAlg::DORV)) {
        // If we're taking deroute in UGAL-DOR, we have intermediate node, and
        // VC set = 1
        vcSet = baseVc_ + 1;
      }
    }
    if (outputTypePort_) {
      makeOutputPortSet(&vcPoolVal_, {vcSet}, numVcSets, baseVc_ + numVcs_,
                        maxOutputs_, outputAlg_, &outputPorts_);
    } else {
      makeOutputVcSet(&vcPoolVal_, maxOutputs_, outputAlg_, &outputPorts_);
    }
  }

  for (auto& it : outputPorts_) {
    // never give a response value that could eject
    assert(std::get<0>(it) >= concentration_);
    _response->add(std::get<0>(it), std::get<1>(it));
  }
}
}  // namespace HyperX

registerWithObjectFactory("ugal", HyperX::RoutingAlgorithm,
                    HyperX::UgalRoutingAlgorithm,
                    HYPERX_ROUTINGALGORITHM_ARGS);
