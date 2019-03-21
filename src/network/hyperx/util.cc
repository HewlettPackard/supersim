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
#include "network/hyperx/util.h"

#include <colhash/tuplehash.h>

#include <cassert>

#include <algorithm>
#include <iostream>
#include <set>

#include "network/cube/util.h"

const f64 TOLERANCE = 1e-6;

namespace HyperX {

u32 computeMinimalHops(const std::vector<u32>* _source,
                       const std::vector<u32>* _destination,
                       u32 _dimensions) {
  u32 minHops = 1;
  for (u32 dim = 0; dim < _dimensions; dim++) {
    if (_source->at(dim+1) != _destination->at(dim+1)) {
      minHops += 1;
    }
  }
  return minHops;
}


u32 computeOutputPort(u32 _base, u32 _offset, u32 _dimWeight, u32 _weight) {
  assert(_weight < _dimWeight);
  return _base + ((_offset - 1) * _dimWeight) + _weight;
}

u32 computeSrcDstOffset(u32 _src, u32 _dst, u32 _dimWidth) {
  if (_dst > _src) {
    return _dst - _src;
  } else {
    return ((_dst + _dimWidth) - _src);
  }
}

/**************************UTILITY FUNCTIONS**********************************/

bool isDestinationRouter(
    Router* _router, const std::vector<u32>* _destinationAddress) {

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  // determine the next dimension to work on
  u32 dim;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      break;
    }
  }
  if (dim == routerAddress.size()) {
    return true;
  }
  return false;
}

u32 hopsLeft(Router* _router, const std::vector<u32>* _destinationAddress) {
  u32 hops = 0;
  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  // determine the next dimension to work on
  for (u32 dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      hops++;
    }
  }
  return hops;
}

u32 computeInputPortDim(const std::vector<u32>& _dimensionWidths,
                        const std::vector<u32>& _dimensionWeights,
                        u32 _concentration, u32 _inputPort) {
  // determine which network dimension this port is attached to
  if (_inputPort < _concentration) {
    return U32_MAX;  // terminal dimension
  }

  u32 port = _inputPort - _concentration;
  for (u32 dim = 0; dim < _dimensionWeights.size(); dim++) {
    if (port < (_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim)) {
      return dim;
    } else {
      port -= (_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim);
    }
  }

  assert(false);
}

/*******************INTERMEDIATE DESTINATION FOR VALIANTS**********************/

void intNodeReg(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address) {

  u32 dimensions = _dimensionWidths.size();
  _address->resize(1 + dimensions);

  u32 numTerminals = Cube::computeNumTerminals(_dimensionWidths,
                                               _concentration);
  u64 intId = gSim->rnd.nextU64(0, numTerminals - 1);
  Cube::translateInterfaceIdToAddress(intId, _dimensionWidths, _concentration,
                                      _address);
  _address->at(0) = 0;
}

void intNodeMoveUnaligned(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address) {
  u32 dimensions = _dimensionWidths.size();
  _address->resize(1 + dimensions);

  std::unordered_set<u32> nodesUnAligned;

  u32 numRouters = Cube::computeNumRouters(_dimensionWidths);

  for (u32 routerId = 0; routerId < numRouters; ++routerId) {
    std::vector<u32> routerAddr;
    Cube::translateRouterIdToAddress(routerId, _dimensionWidths, &routerAddr);
    bool aligned = true;
    for (u32 dim = 0; dim < _dimensionWidths.size(); dim++) {
      if ((_sourceRouter.at(dim) == _destinationTerminal->at(dim + 1)) &&
          (_sourceRouter.at(dim) != routerAddr.at(dim))) {
        aligned = false;
      }
    }
    if (aligned) {  // not done twice
      bool res = nodesUnAligned.emplace(routerId).second;
      assert(res);
    }
  }

  const u32* it = uSetRandElement(nodesUnAligned);
  std::vector<u32> ancestor;
  Cube::translateRouterIdToAddress(*it, _dimensionWidths, &ancestor);

  for (u32 ind = 0; ind < _sourceRouter.size(); ind++) {
    _address->at(ind + 1) = ancestor.at(ind);
  }
  _address->at(0) = 0;
}

void intNodeSrc(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address) {
  std::unordered_set<u32> ancestors;
  u32 dimensions = _dimensionWidths.size();
  _address->resize(1 + dimensions);

  for (u32 dim = 0; dim < dimensions; dim++) {
    for (u32 idx = 0; idx < _dimensionWidths.at(dim); idx++) {
      std::vector<u32> tmpVec(_sourceRouter);
      tmpVec.at(dim) = idx;
      u32 ancestorId = Cube::translateRouterAddressToId(
          &tmpVec, _dimensionWidths);
      ancestors.emplace(ancestorId);
    }
  }

  const u32* it = uSetRandElement(ancestors);
  std::vector<u32> ancestor;
  Cube::translateRouterIdToAddress(*it, _dimensionWidths, &ancestor);

  for (u32 ind = 0; ind < _sourceRouter.size(); ind++) {
    _address->at(ind + 1) = ancestor.at(ind);
  }
  _address->at(0) = 0;
}

void intNodeDst(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address) {
  std::unordered_set<u32> ancestors;
  u32 dimensions = _dimensionWidths.size();
  _address->resize(1 + dimensions);

  for (u32 dim = 0; dim < dimensions; dim++) {
    for (u32 idx = 0; idx < _dimensionWidths.at(dim); idx++) {
      std::vector<u32> tmpVec(dimensions);
      for (u32 ind = 0; ind < dimensions; ind++) {
        tmpVec.at(ind) = _destinationTerminal->at(ind + 1);
      }
      tmpVec.at(dim) = idx;
      u32 ancestorId = Cube::translateRouterAddressToId(
          &tmpVec, _dimensionWidths);
      ancestors.emplace(ancestorId);
    }
  }

  const u32* it = uSetRandElement(ancestors);
  std::vector<u32> ancestor;
  Cube::translateRouterIdToAddress(*it, _dimensionWidths, &ancestor);

  for (u32 ind = 0; ind < _sourceRouter.size(); ind++) {
    _address->at(ind + 1) = ancestor.at(ind);
  }
  _address->at(0) = 0;
}

void intNodeSrcDst(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address) {
  std::unordered_set<u32> ancestors;
  u32 dimensions = _dimensionWidths.size();
  _address->resize(1 + dimensions);

  for (u32 dim = 0; dim < dimensions; dim++) {
    for (u32 idx = 0; idx < _dimensionWidths.at(dim); idx++) {
      std::vector<u32> tmpVec(_sourceRouter);
      tmpVec.at(dim) = idx;
      u32 ancestorId = Cube::translateRouterAddressToId(
          &tmpVec, _dimensionWidths);
      ancestors.emplace(ancestorId);
    }
  }

  for (u32 dim = 0; dim < dimensions; dim++) {
    for (u32 idx = 0; idx < _dimensionWidths.at(dim); idx++) {
      std::vector<u32> tmpVec(dimensions);
      for (u32 ind = 0; ind < dimensions; ind++) {
        tmpVec.at(ind) = _destinationTerminal->at(ind + 1);
      }
      tmpVec.at(dim) = idx;
      u32 ancestorId = Cube::translateRouterAddressToId(
          &tmpVec, _dimensionWidths);
      ancestors.emplace(ancestorId);
    }
  }

  const u32* it = uSetRandElement(ancestors);
  std::vector<u32> ancestor;
  Cube::translateRouterIdToAddress(*it, _dimensionWidths, &ancestor);

  for (u32 ind = 0; ind < _sourceRouter.size(); ind++) {
    _address->at(ind + 1) = ancestor.at(ind);
  }
  _address->at(0) = 0;
}

void intNodeMinV(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address) {
  u32 dim;
  u32 portBase = _concentration;
  f64 minCongestion = F64_POS_INF;

  std::unordered_set<u32> ancestors;
  u32 dimensions = _dimensionWidths.size();
  _address->resize(1 + dimensions);
  const f64 tolerance = 1e-6;
  // determine available dimensions
  for (dim = 0; dim < dimensions; dim++) {
    for (u32 idx = 0; idx < _dimensionWidths.at(dim) - 1; idx++) {
      for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
        u32 port = portBase + idx + weight;
        for (u32 vc = _vcSet; vc < _numVcs; vc += _numVcSets) {
          f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                     port, vc);
          if (congestion > (minCongestion + tolerance)) {
            continue;
          } else if (congestion < (minCongestion - tolerance)) {
            minCongestion = congestion;
            ancestors.clear();
          }
          std::vector<u32> tmpVec(_sourceRouter);
          if (idx < _sourceRouter.at(dim)) {
            tmpVec.at(dim) = idx;
          } else {
            tmpVec.at(dim) = idx + 1;
          }
          u32 ancestorId = Cube::translateRouterAddressToId(
              &tmpVec, _dimensionWidths);
          // we don't need to perform check here, because congestion is
          // for port,vc pair, and we're interested only in Router ID (port),
          // so we can have many port,vc pair with the same congestion
          // for the same port
          ancestors.emplace(ancestorId);
        }
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }

  const u32* it = uSetRandElement(ancestors);
  std::vector<u32> ancestor;
  Cube::translateRouterIdToAddress(*it, _dimensionWidths, &ancestor);

  for (u32 ind = 0; ind < _sourceRouter.size(); ind++) {
    _address->at(ind + 1) = ancestor.at(ind);
  }
  _address->at(0) = 0;
}

void intNodeMinP(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address) {
  u32 dim;
  u32 portBase = _concentration;
  f64 minCongestion = F64_POS_INF;

  std::unordered_set<u32> ancestors;
  u32 dimensions = _dimensionWidths.size();
  _address->resize(1 + dimensions);
  const f64 tolerance = 1e-6;

  // determine available dimensions
  for (dim = 0; dim < dimensions; dim++) {
    for (u32 idx = 0; idx < _dimensionWidths.at(dim) - 1; idx++) {
      for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
        u32 port = portBase + idx + weight;
        f64 congestion = getAveragePortCongestion(
            _router, _inputPort, _inputVc, port, {_vcSet}, _numVcSets, _numVcs);
        if (congestion > (minCongestion + tolerance)) {
          continue;
        } else if (congestion < (minCongestion-tolerance)) {
          minCongestion = congestion;
          ancestors.clear();
        }
        for (u32 vc = _vcSet; vc < _numVcs; vc += _numVcSets) {
          std::vector<u32> tmpVec(_sourceRouter);
          if (idx < _sourceRouter.at(dim)) {
            tmpVec.at(dim) = idx;
          } else {
            tmpVec.at(dim) = idx + 1;
          }
          u32 ancestorId = Cube::translateRouterAddressToId(
              &tmpVec, _dimensionWidths);
          // we don't need to perform check here, because congestion is
          // for port,vc pair, and we're interested only in Router ID (port),
          // so we can have many port,vc pair with the same congestion
          // for the same port
          ancestors.emplace(ancestorId);
        }
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }

  const u32* it = uSetRandElement(ancestors);
  std::vector<u32> ancestor;
  Cube::translateRouterIdToAddress(*it, _dimensionWidths, &ancestor);

  for (u32 ind = 0; ind < _sourceRouter.size(); ind++) {
    _address->at(ind + 1) = ancestor.at(ind);
  }
  _address->at(0) = 0;
}

/*******************MAX_OUTPUTS HANDLING FOR ROUTING ALGORITHMS***************/
void makeOutputVcSet(
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool,
    u32 _maxOutputs, OutputAlg _outputAlg,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputPorts) {
  _outputPorts->clear();

  if (_vcPool->size() != 0) {
    if (_maxOutputs == 0/*infinite*/) {
      for (auto& it : *_vcPool) {
        bool res = _outputPorts->emplace(it).second;
        assert(res);
      }
    } else {
      for (u32 i = 0; i < _maxOutputs; ++i) {
        if (_vcPool->size() == 0) {
          break;
        } else {
          const std::tuple<u32, u32, f64>* it;
          if (_outputAlg == OutputAlg::Rand) {
            it = uSetRandElement(*_vcPool);
          } else if (_outputAlg == OutputAlg::Min) {
            it = uSetMinCong(*_vcPool);
          } else {
            fprintf(stderr, "Unknown output algorithm\n");
            assert(false);
          }
          bool res = _outputPorts->emplace(*it).second;
          assert(res);
          _vcPool->erase(*it);
        }
      }
    }
  }
}

void makeOutputPortSet(
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool,
    const std::vector<u32>& _vcSets,
    u32 _numVcSets,
    u32 _numVcs,
    u32 _maxOutputs, OutputAlg _outputAlg,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputPorts) {
  _outputPorts->clear();

  if (_vcPool->size() != 0) {
    if (_maxOutputs == 0/*infinite*/) {
      for (auto& it : *_vcPool) {
        u32 port = std::get<0>(it);
        f64 congestion = std::get<2>(it);
        for (u32 vcSet : _vcSets) {  // loop through all vcSets
          for (u32 vc = vcSet; vc < _numVcs; vc += _numVcSets) {
            std::tuple<u32, u32, f64> t(port, vc, congestion);
            bool res = _outputPorts->emplace(t).second;
            assert(res);
          }
        }
      }
    } else {  // maxOutputs
      for (u32 i = 0; i < _maxOutputs; ++i) {
        if (_vcPool->size() == 0) {
          break;
        } else {
          const std::tuple<u32, u32, f64>* it;
          if (_outputAlg == OutputAlg::Rand) {
            it = uSetRandElement(*_vcPool);
          } else if (_outputAlg == OutputAlg::Min) {
            it = uSetMinCong(*_vcPool);
          } else {
            fprintf(stderr, "Unknown output algorithm\n");
            assert(false);
          }
          u32 port = std::get<0>(*it);
          f64 congestion = std::get<2>(*it);
          for (u32 vcSet : _vcSets) {  // loop through all vcSets
            for (u32 vc = vcSet; vc < _numVcs; vc += _numVcSets) {
              std::tuple<u32, u32, f64> t(port, vc, congestion);
              bool res = _outputPorts->emplace(t).second;
              assert(res);
            }
          }
          _vcPool->erase(*it);
        }
      }
    }
  }
}

f64 getAveragePortCongestion(
    Router* _router, u32 _inputPort, u32 _inputVc,
    u32 _outputPort,
    const std::vector<u32>& _vcSets,
    u32 _numVcSets, u32 _numVcs) {
  u32 _numVcsInSet = 0;
  f64 congestion = 0;
  for (u32 vcSet : _vcSets) {  // loop through vcSets (vector at 1st hop)
    for (u32 outputVc = vcSet; outputVc < _numVcs; outputVc += _numVcSets) {
      congestion += _router->congestionStatus(_inputPort, _inputVc, _outputPort,
                                              outputVc);
      _numVcsInSet += 1;
    }
  }
  return congestion;
}

/*******************DIMENSION ORDERED ROUTING ALGORITHM***********************/

void dimOrderVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  assert(_vcSets.size() > 0);
  _vcPool->clear();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      break;
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }

  // test if already at destination router
  if (dim != routerAddress.size()) {
    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = _destinationAddress->at(dim+1);
    u32 offset = computeSrcDstOffset(src, dst,
                                     _dimensionWidths.at(dim));

    // add all ports where the two routers are connecting
    for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
      u32 port = computeOutputPort(portBase, offset,
                                   _dimensionWeights.at(dim),
                                   weight);
      for (u32 vcSet : _vcSets) {  // loop through vcSets (vector at 1st hop)
        for (u32 vc = vcSet; vc < _numVcs; vc += _numVcSets) {
          // The code below returns congestion status
          // This information is not needed for DOR routing, which is completely
          // oblivious, it might be helpful for another routing algorithms that
          // use DOR as base routing in adaptive phases
          f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                     port, vc);
          std::tuple<u32, u32, f64> t(port, vc, congestion);
          bool res = _vcPool->emplace(t).second;
          assert(res);
        }
      }
    }
    // must have at least one route if not at destination
    assert(_vcPool->size() > 0);
  }
}

void dimOrderPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  _vcPool->clear();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      break;
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
  // test if already at destination router
  if (dim != routerAddress.size()) {
    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = _destinationAddress->at(dim+1);
    u32 offset = computeSrcDstOffset(src, dst,
                                     _dimensionWidths.at(dim));

    // add all ports where the two routers are connecting
    for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
      u32 port = computeOutputPort(portBase, offset,
                                   _dimensionWeights.at(dim),
                                   weight);
      // The code below returns congestion status
      // This information is not needed for DOR routing, which is completely
      // oblivious, it might be helpful for another routing algorithms that
      // use DOR as base routing in adaptive phases
      f64 congestion = getAveragePortCongestion(
          _router, _inputPort, _inputVc, port, _vcSets, _numVcSets, _numVcs);

      std::tuple<u32, u32, f64> t(port, 0, congestion);
      bool res = _vcPool->emplace(t).second;
      assert(res);
    }
    // must have at least one route if not at destination
    assert(_vcPool->size() > 0);
  }
}

/*******************RANDOM MINIMAL ROUTING ALGORITHMS ************************/

void randMinVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  _vcPool->clear();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      u32 src = routerAddress.at(dim);
      u32 dst = _destinationAddress->at(dim+1);
      u32 offset = computeSrcDstOffset(src, dst,
                                       _dimensionWidths.at(dim));

      // add all ports where the two routers are connecting
      for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
        u32 port = computeOutputPort(portBase, offset,
                                     _dimensionWeights.at(dim),
                                     weight);
        for (u32 vcSet : _vcSets) {
          for (u32 vc = vcSet; vc < _numVcs; vc += _numVcSets) {
            f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                       port, vc);
            std::tuple<u32, u32, f64> t(port, vc, congestion);
            bool res = _vcPool->emplace(t).second;
            assert(res);
          }
        }
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
}

void randMinPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  _vcPool->clear();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;

  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      u32 src = routerAddress.at(dim);
      u32 dst = _destinationAddress->at(dim+1);
      u32 offset = computeSrcDstOffset(src, dst,
                                       _dimensionWidths.at(dim));

      // add all ports where the two routers are connecting
      for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
        u32 port = computeOutputPort(portBase, offset,
                                     _dimensionWeights.at(dim),
                                     weight);
        f64 congestion = getAveragePortCongestion(
            _router, _inputPort, _inputVc, port, _vcSets, _numVcSets, _numVcs);
        std::tuple<u32, u32, f64> t(port, 0, congestion);
        bool res = _vcPool->emplace(t).second;
        assert(res);
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
}

/*******************ADAPTIVE MINIMAL ROUTING ALGORITHMS **********************/

void adaptiveMinVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  _vcPool->clear();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  f64 minCongestion = F64_POS_INF;

  // determine available dimensions
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      u32 src = routerAddress.at(dim);
      u32 dst = _destinationAddress->at(dim+1);
      u32 offset = computeSrcDstOffset(src, dst,
                                       _dimensionWidths.at(dim));

      // add all ports where the two routers are connecting
      for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
        u32 port = computeOutputPort(portBase, offset,
                                     _dimensionWeights.at(dim),
                                     weight);
        for (u32 vcSet : _vcSets) {
          for (u32 vc = vcSet; vc < _numVcs; vc += _numVcSets) {
            f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                       port, vc);
            if (congestion > minCongestion) {
              continue;
            } else if (congestion < minCongestion) {
              minCongestion = congestion;
              _vcPool->clear();
            }
            std::tuple<u32, u32, f64> t(port, vc, congestion);
            bool res = _vcPool->emplace(t).second;
            assert(res);
          }
        }
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
}

void adaptiveMinPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  _vcPool->clear();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  f64 minCongestion = F64_POS_INF;

  // determine available dimensions
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      u32 src = routerAddress.at(dim);
      u32 dst = _destinationAddress->at(dim+1);
      u32 offset = computeSrcDstOffset(src, dst,
                                       _dimensionWidths.at(dim));

      // add all ports where the two routers are connecting
      for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
        u32 port  = computeOutputPort(portBase, offset,
                                      _dimensionWeights.at(dim),
                                      weight);
        f64 congestion = getAveragePortCongestion(
            _router, _inputPort, _inputVc, port, _vcSets, _numVcSets, _numVcs);
        if (congestion > minCongestion) {
          continue;
        } else if (congestion < minCongestion) {
          minCongestion = congestion;
          _vcPool->clear();
        }
        std::tuple<u32, u32, f64> t(port, 0, congestion);
        bool res = _vcPool->emplace(t).second;
        assert(res);
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
}

/*******************VALIANTS ROUTING ALGORITHMS *******************************/

void valiantsRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, bool _shortCut,
    IntNodeAlg _intNodeAlg, BaseRoutingAlg _routingAlg, Flit* _flit,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  _vcPool->clear();

  Packet* packet = _flit->packet();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  if (_shortCut) {
    // If source == destination, don't pick intermediate address
    if (isDestinationRouter(_router, _destinationAddress)) {
      delete reinterpret_cast<const std::vector<u32>*>
          (packet->getRoutingExtension());
      packet->setRoutingExtension(nullptr);
      return;
    }
  }

  // create the routing extension if needed
  if (packet->getHopCount() == 0) {
    // should be first router encountered
    assert(packet->getRoutingExtension() == nullptr);

    // create routing extension header
    //  the extension is a vector with one dummy element then the address of the
    //  intermediate router
    std::vector<u32>* intAddr = new std::vector<u32>(1 + routerAddress.size());

    IntNodeAlgFunc intNodeAlgFunc;
    switch (_intNodeAlg) {
      case IntNodeAlg::REG:
        intNodeAlgFunc = &intNodeReg;
        break;
      case IntNodeAlg::SRC:
        intNodeAlgFunc = &intNodeSrc;
        break;
      case IntNodeAlg::DST:
        intNodeAlgFunc = &intNodeDst;
        break;
      case IntNodeAlg::SRCDST:
        intNodeAlgFunc = &intNodeSrcDst;
        break;
      case IntNodeAlg::UNALIGNED:
        intNodeAlgFunc = &intNodeMoveUnaligned;
        break;
      case IntNodeAlg::MINV:
        intNodeAlgFunc = &intNodeMinV;
        break;
      case IntNodeAlg::MINP:
        intNodeAlgFunc = &intNodeMinP;
        break;
      default:
        fprintf(stderr, "Unknown intermediate node algorithm\n");
        assert(false);
    }
    intNodeAlgFunc(_router, _inputPort, _inputVc, routerAddress,
                   _destinationAddress, _dimensionWidths, _dimensionWeights,
                   _concentration, _vcSet, _numVcSets, _numVcs, intAddr);

    intAddr->at(0) = U32_MAX;  // dummy

    packet->setRoutingExtension(intAddr);
  }

  // determine which stage we are in based on routing extension
  //  if routing extension is empty, it's stage 1
  u32 stage = (packet->getRoutingExtension() != nullptr) ? 0 : 1;

  // get a const pointer to the address (with leading dummy)
  const std::vector<u32>* intermediateAddress =
      reinterpret_cast<const std::vector<u32>*>(packet->getRoutingExtension());

  MinRoutingAlgFunc routingAlgFunc;
  switch (_routingAlg) {
    case BaseRoutingAlg::DORV: {
      routingAlgFunc = &dimOrderVcRoutingOutput;
      break;
    }
    case BaseRoutingAlg::DORP: {
      routingAlgFunc = &dimOrderPortRoutingOutput;
      break;
    }
    case BaseRoutingAlg::RMINV: {
      routingAlgFunc = &randMinVcRoutingOutput;
      break;
    }
    case BaseRoutingAlg::RMINP: {
      routingAlgFunc = &randMinPortRoutingOutput;
      break;
    }
    case BaseRoutingAlg::AMINV: {
      routingAlgFunc = &adaptiveMinVcRoutingOutput;
      break;
    }
    case BaseRoutingAlg::AMINP: {
      routingAlgFunc = &adaptiveMinPortRoutingOutput;
      break;
    }
    default: {
      fprintf(stderr, "Unknown routing algorithm\n");
      assert(false);
    }
  }

  if (stage == 0) {
    assert(packet->getRoutingExtension() != nullptr);

    u32 vcSet = _vcSet;
    routingAlgFunc(_router, _inputPort, _inputVc, _dimensionWidths,
                   _dimensionWeights, _concentration, intermediateAddress,
                   {_vcSet}, _numVcSets, _numVcs, _vcPool);

    // at destination (Int)
    if (_vcPool->empty()) {
      delete reinterpret_cast<const std::vector<u32>*>
          (packet->getRoutingExtension());
      packet->setRoutingExtension(nullptr);
      stage = 1;
      if ((_routingAlg == BaseRoutingAlg::DORP) ||
          (_routingAlg == BaseRoutingAlg::DORV)) {
        // Suppose for VAL-DOR we have VC set = 1 for stage 0 and
        // VC set = 0 for stage 1
        assert(vcSet > 0);
        vcSet--;
      }
      routingAlgFunc(_router, _inputPort, _inputVc, _dimensionWidths,
                     _dimensionWeights, _concentration, _destinationAddress,
                     {vcSet}, _numVcSets, _numVcs, _vcPool);
      return;
    }
    assert(!_vcPool->empty());
  }
  // going to Dest from Int
  if (stage == 1) {
    if (intermediateAddress) {
      assert(false);
    }
    routingAlgFunc(_router, _inputPort, _inputVc, _dimensionWidths,
                   _dimensionWeights, _concentration, _destinationAddress,
                   {_vcSet}, _numVcSets, _numVcs, _vcPool);
  }
}

void ugalRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    bool _shortCut, bool _minAllVcSets, IntNodeAlg _intNodeAlg,
    BaseRoutingAlg _routingAlg, NonMinRoutingAlg _nonMinimalAlg,
    Flit* _flit, f64* _weightReg, f64* _weightVal,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPoolReg,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPoolVal) {
  _vcPoolReg->clear();
  _vcPoolVal->clear();

  const std::vector<u32>* destAddress =
      _flit->packet()->message()->getDestinationAddress();

  Packet* packet = _flit->packet();

  MinRoutingAlgFunc routingAlgFunc;
  switch (_routingAlg) {
    case BaseRoutingAlg::DORV   : {
      routingAlgFunc = &dimOrderVcRoutingOutput;
      break;
    }
    case BaseRoutingAlg::DORP   : {
      routingAlgFunc = &dimOrderPortRoutingOutput;
      break;
    }
    case BaseRoutingAlg::RMINV : {
      routingAlgFunc = &randMinVcRoutingOutput;
      break;
    }
    case BaseRoutingAlg::RMINP : {
      routingAlgFunc = &randMinPortRoutingOutput;
      break;
    }
    case BaseRoutingAlg::AMINV : {
      routingAlgFunc = &adaptiveMinVcRoutingOutput;
      break;
    }
    case BaseRoutingAlg::AMINP : {
      routingAlgFunc = &adaptiveMinPortRoutingOutput;
      break;
    }
    default                    : {
      fprintf(stderr, "Unknown routing algorithm\n");
      assert(false);
    }
  }

  if ((packet->getHopCount() == 0) &&
      (_nonMinimalAlg == NonMinRoutingAlg::LCQP)) {
    lcqPortRoutingOutput(
        _router, _inputPort, _inputVc, _dimensionWidths, _dimensionWeights,
        _concentration, destAddress, _vcSet, _numVcSets, _numVcs, _shortCut,
        _vcPoolVal);
  } else if ((packet->getHopCount() == 0) &&
             (_nonMinimalAlg == NonMinRoutingAlg::LCQV)) {
    lcqVcRoutingOutput(
        _router, _inputPort, _inputVc, _dimensionWidths, _dimensionWeights,
        _concentration, destAddress, _vcSet, _numVcSets, _numVcs, _shortCut,
        _vcPoolVal);
  } else {
    u32 vcSet = _vcSet;
    if ((packet->getHopCount() == 0) &&
        ((_routingAlg == BaseRoutingAlg::DORP) ||
         (_routingAlg == BaseRoutingAlg::DORV))) {
      // If we're taking non-minimal in UGAL-DOR, we have intermediate node, and
      // VC set = 1
      vcSet++;
    }
    valiantsRoutingOutput(
        _router, _inputPort, _inputVc, _dimensionWidths, _dimensionWeights,
        _concentration, destAddress, vcSet, _numVcSets, _numVcs, _shortCut,
        _intNodeAlg, _routingAlg, _flit, _vcPoolVal);
  }

  if (packet->getHopCount() == 0) {
    // minimal routes only
    // note: vcSet is baseVc on hop count 1
    std::vector<u32> vcSets;
    if (!_minAllVcSets) {  // dont use all VCsets
      vcSets.push_back(_vcSet);
    } else {  // enable all vc sets for initial hop
      if ((_routingAlg == BaseRoutingAlg::DORV) ||
          (_routingAlg == BaseRoutingAlg::DORP)) {
        // DOR 2 vcSets (0 and 1)
        vcSets.push_back(_vcSet + 0);
        vcSets.push_back(_vcSet + 1);
      } else if ((_intNodeAlg == IntNodeAlg::REG) ||
                 (_intNodeAlg == IntNodeAlg::UNALIGNED)) {
        // distance class vcSets 0 and Dim
        vcSets.push_back(_vcSet + 0);
        vcSets.push_back(_vcSet + _dimensionWidths.size());
      } else {
        // 2 VcSets (0 and 1)
        vcSets.push_back(_vcSet + 0);
        vcSets.push_back(_vcSet + 1);
      }
    }
    routingAlgFunc(_router, _inputPort, _inputVc, _dimensionWidths,
                   _dimensionWeights, _concentration, destAddress,
                   vcSets, _numVcSets, _numVcs, _vcPoolReg);
  }
}

/*******************LEAST CONGESTED QUEUE ROUTING ALGORITHMS *****************/
void lcqVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, bool _shortCut,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  _vcPool->clear();

  u32 dim;
  u32 portBase = _concentration;
  f64 minCongestion = F64_POS_INF;

  if (_shortCut) {
    // If source == destination, don't pick intermediate address
    if (isDestinationRouter(_router, _destinationAddress)) {
      return;
    }
  }

  // determine available dimensions
  for (dim = 0; dim < _dimensionWidths.size(); dim++) {
    for (u32 idx = 0; idx < _dimensionWidths.at(dim) - 1; idx++) {
      for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
        u32 port = portBase + (idx * _dimensionWeights.at(dim)) + weight;
        for (u32 vc = _vcSet; vc < _numVcs; vc += _numVcSets) {
          f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                     port, vc);
          if (congestion > minCongestion) {
            continue;
          } else if (congestion < minCongestion) {
            minCongestion = congestion;
            _vcPool->clear();
          }
          std::tuple<u32, u32, f64> t(port, vc, congestion);
          bool res = _vcPool->emplace(t).second;
          assert(res);
        }
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
}

void lcqPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, bool _shortCut,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  _vcPool->clear();

  u32 dim;
  u32 portBase = _concentration;
  f64 minCongestion = F64_POS_INF;

  if (_shortCut) {
    // If source == destination, don't pick intermediate address
    if (isDestinationRouter(_router, _destinationAddress)) {
      return;
    }
  }

  // determine available dimensions
  for (dim = 0; dim < _dimensionWidths.size(); dim++) {
    for (u32 idx = 0; idx < _dimensionWidths.at(dim) - 1; idx++) {
      for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
        u32 port = portBase + (idx * _dimensionWeights.at(dim)) + weight;
        f64 congestion = getAveragePortCongestion(
            _router, _inputPort, _inputVc, port, {_vcSet}, _numVcSets, _numVcs);
        if (congestion > minCongestion) {
          continue;
        } else if (congestion < minCongestion) {
          minCongestion = congestion;
          _vcPool->clear();
        }
        std::tuple<u32, u32, f64> t(port, 0, congestion);
        bool res = _vcPool->emplace(t).second;
        assert(res);
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
}

/********************DAL ROUTING ALGORITHMS **********************************/
void doalPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _baseVc, u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin) {
  _outputVcsMin->clear();
  _outputVcsNonMin->clear();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  u32 derouted = (_vcSet - _baseVc) % 2;
  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      break;
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }

  // test if already at destination router
  if (dim != routerAddress.size()) {
    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = _destinationAddress->at(dim+1);
    u32 srcDstOffset = computeSrcDstOffset(src, dst,
                                           _dimensionWidths.at(dim));
    // add all ports where the two routers are connecting to outputPortsMin
    // add all other ports to outputPortsDer
    for (u32 offset = 1; offset < _dimensionWidths.at(dim); offset++) {
      for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
        u32 outPort = computeOutputPort(portBase, offset,
                                        _dimensionWeights.at(dim),
                                        weight);
        if (offset != srcDstOffset) {
          // deroute offset
          // derouting from minimal with VCset == 1
          if (derouted == 0) {
            // we can do that if we didn't deroute previously
            f64 congestion = getAveragePortCongestion(
                _router, _inputPort, _inputVc, outPort, {_vcSet}, _numVcSets,
                _numVcs);
            std::tuple<u32, u32, f64> t(outPort, 0, congestion);
            bool res = _outputVcsNonMin->emplace(t).second;
            assert(res);
          }
        } else {
          // minimal routes with VCset == 0
          f64 congestion = getAveragePortCongestion(
              _router, _inputPort, _inputVc, outPort,
              {_vcSet}, _numVcSets, _numVcs);
          std::tuple<u32, u32, f64> t(outPort, 0, congestion);
          bool res = _outputVcsMin->emplace(t).second;
          assert(res);
        }
      }
    }
  }
}

void doalVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _baseVc, u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin) {
  _outputVcsMin->clear();
  _outputVcsNonMin->clear();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  u32 derouted = (_vcSet - _baseVc) % 2;
  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      break;
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }

  // test if already at destination router
  if (dim != routerAddress.size()) {
    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = _destinationAddress->at(dim+1);
    u32 srcDstOffset = computeSrcDstOffset(src, dst,
                                           _dimensionWidths.at(dim));

    // add all ports where the two routers are connecting to outputPortsMin
    // add all other ports to outputPortsDer
    for (u32 offset = 1; offset < _dimensionWidths.at(dim); offset++) {
      for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
        u32 outPort = computeOutputPort(portBase, offset,
                                        _dimensionWeights.at(dim),
                                        weight);
        for (u32 vc = _vcSet; vc < _numVcs; vc += _numVcSets) {
          if (offset != srcDstOffset) {
            // deroute offset
            // derouting from minimal with VCset == 1
            if (derouted == 0) {
              f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                         outPort, vc);
              std::tuple<u32, u32, f64> t(outPort, vc, congestion);
              bool res = _outputVcsNonMin->emplace(t).second;
              assert(res);
            }
          } else {
            // minimal routes with VCset == 0
            f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                       outPort, vc);
            std::tuple<u32, u32, f64> t(outPort, vc, congestion);
            bool res = _outputVcsMin->emplace(t).second;
            assert(res);
          }
        }
      }
    }
  }
}

void ddalPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin) {
  _outputVcsMin->clear();
  _outputVcsNonMin->clear();
  Packet* packet = _flit->packet();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  if (packet->getHopCount() == 0) {
    assert(packet->getRoutingExtension() == nullptr);
    std::vector<u32>* re = new std::vector<u32>(_dimensionWidths.size(), 0);
    packet->setRoutingExtension(re);
  }

  std::set<u32> dimensions;
  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      dimensions.emplace(dim);
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }

  // test if already at destination router
  if (dimensions.empty()) {
    return;
  }

  portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (dimensions.find(dim) != dimensions.end()) {
      // more router-to-router hops needed
      u32 src = routerAddress.at(dim);
      u32 dst = _destinationAddress->at(dim+1);
      u32 srcDstOffset = computeSrcDstOffset(src, dst,
                                             _dimensionWidths.at(dim));

      // get a const pointer to the address (with leading dummy)
      std::vector<u32>* deroutedDims =
          reinterpret_cast<std::vector<u32>*>(packet->getRoutingExtension());
      u32 derouted = deroutedDims->at(dim);

      // add ports
      for (u32 offset = 1; offset < _dimensionWidths.at(dim); offset++) {
        for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
          u32 outPort = computeOutputPort(portBase, offset,
                                         _dimensionWeights.at(dim), weight);
          if ((offset != srcDstOffset) && (derouted == 0)) {
            f64 congestion = getAveragePortCongestion(
                _router, _inputPort, _inputVc, outPort, {_vcSet}, _numVcSets,
                _numVcs);
            std::tuple<u32, u32, f64> t(outPort, 0, congestion);
            bool res = _outputVcsNonMin->emplace(t).second;
            assert(res);
          }
          if ((offset == srcDstOffset) && (derouted > 0)) {
            f64 congestion = getAveragePortCongestion(
                _router, _inputPort, _inputVc, outPort, {_vcSet}, _numVcSets,
                _numVcs);
            assert(derouted == 1);
            std::tuple<u32, u32, f64> t(outPort, 0, congestion);
            bool res = _outputVcsMin->emplace(t).second;
            assert(res);
          }
          if ((offset == srcDstOffset) && (derouted == 0)) {
            f64 congestion = getAveragePortCongestion(
                _router, _inputPort, _inputVc, outPort, {_vcSet}, _numVcSets,
                _numVcs);
            std::tuple<u32, u32, f64> t(outPort, 0, congestion);
            bool res = _outputVcsMin->emplace(t).second;
            assert(res);
          }
        }
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
}

void ddalVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin) {
  _outputVcsMin->clear();
  _outputVcsNonMin->clear();
  Packet* packet = _flit->packet();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  if (packet->getHopCount() == 0) {
    assert(packet->getRoutingExtension() == nullptr);
    std::vector<u32>* re = new std::vector<u32>(_dimensionWidths.size(), 0);
    packet->setRoutingExtension(re);
  }

  std::set<u32> dimensions;
  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      dimensions.emplace(dim);
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }

  // test if already at destination router
  if (dimensions.empty()) {
    return;
  }

  portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (dimensions.find(dim) != dimensions.end()) {
      // more router-to-router hops needed
      u32 src = routerAddress.at(dim);
      u32 dst = _destinationAddress->at(dim+1);
      u32 srcDstOffset = computeSrcDstOffset(src, dst,
                                             _dimensionWidths.at(dim));
      // get a const pointer to the address (with leading dummy)
      std::vector<u32>* deroutedDims =
          reinterpret_cast<std::vector<u32>*>(packet->getRoutingExtension());
      u32 derouted = deroutedDims->at(dim);

      // add ports
      for (u32 offset = 1; offset < _dimensionWidths.at(dim); offset++) {
        for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
          u32 outPort = computeOutputPort(portBase, offset,
                                          _dimensionWeights.at(dim),
                                          weight);
          for (u32 vc = _vcSet; vc < _numVcs; vc += _numVcSets) {
            if ((offset != srcDstOffset) && (derouted == 0)) {
              // We can mark dim as derouted here, and that's truly sad
              // deroutedDims->at(dim)++;
              f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                         outPort, vc);
              std::tuple<u32, u32, f64> t(outPort, vc, congestion);
              bool res = _outputVcsNonMin->emplace(t).second;
              assert(res);
            }
            if ((offset == srcDstOffset) && (derouted > 0)) {
              assert(derouted == 1);
              f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                         outPort, vc);
              std::tuple<u32, u32, f64> t(outPort, vc, congestion);
              bool res = _outputVcsMin->emplace(t).second;
              assert(res);
            }
          }
          for (u32 vc = _vcSet + 1; vc < _numVcs; vc += _numVcSets) {
            if ((offset == srcDstOffset) && (derouted == 0)) {
              f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                         outPort, vc);
              std::tuple<u32, u32, f64> t(outPort, vc, congestion);
              bool res = _outputVcsMin->emplace(t).second;
              assert(res);
            }
          }
        }
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
}

void vdalPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress, u32 _baseVc,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit, bool _multiDeroute,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin) {
  _outputVcsMin->clear();
  _outputVcsNonMin->clear();
  Packet* packet = _flit->packet();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  u32 hops = packet->getHopCount();
  u32 hopsleft = hopsLeft(_router, _destinationAddress);

  assert((hops == _vcSet - _baseVc) || (hopsleft == 0));
  u32 vcSetsLeft = _numVcSets - _vcSet + _baseVc;
  assert(hopsleft == 0 || hopsleft <= vcSetsLeft);

  u32 prevDim = computeInputPortDim(_dimensionWidths, _dimensionWeights,
                                    _concentration, _inputPort);

  bool allowDeroutes = hopsleft < vcSetsLeft;

  std::set<u32> dimensions;
  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      dimensions.emplace(dim);
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }

  // test if already at destination router
  if (dimensions.empty()) {
    return;
  }

  portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (dimensions.find(dim) != dimensions.end()) {
      // more router-to-router hops needed
      u32 src = routerAddress.at(dim);
      u32 dst = _destinationAddress->at(dim+1);
      u32 srcDstOffset = computeSrcDstOffset(src, dst,
                                             _dimensionWidths.at(dim));
      // allow deroutes
      if ((dim == prevDim) && !_multiDeroute) {
        allowDeroutes = false;
      } else {
        allowDeroutes = hopsleft < vcSetsLeft;
      }

      // add all ports where the two routers are connecting to outputPortsMin
      // add all other ports to outputPortsDer
      for (u32 offset = 1; offset < _dimensionWidths.at(dim); offset++) {
        for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
          u32 outPort = computeOutputPort(portBase, offset,
                                          _dimensionWeights.at(dim),
                                          weight);
          if (offset != srcDstOffset) {
            // deroute offset
            if (allowDeroutes) {
              f64 congestion = getAveragePortCongestion(
                  _router, _inputPort, _inputVc, outPort, {_vcSet}, _numVcSets,
                  _numVcs);
              std::tuple<u32, u32, f64> t(outPort, 0, congestion);
              bool res = _outputVcsNonMin->emplace(t).second;
              assert(res);
            }
          } else {
            // minimal offset
            f64 congestion = getAveragePortCongestion(
                _router, _inputPort, _inputVc, outPort, {_vcSet}, _numVcSets,
                _numVcs);
            std::tuple<u32, u32, f64> t(outPort, 0, congestion);
            bool res = _outputVcsMin->emplace(t).second;
            assert(res);
          }
        }
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
}

void vdalVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress, u32 _baseVc,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit, bool _multiDeroute,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin) {
  _outputVcsMin->clear();
  _outputVcsNonMin->clear();
  Packet* packet = _flit->packet();

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  u32 hops = packet->getHopCount();
  u32 hopsleft = hopsLeft(_router, _destinationAddress);

  assert((hops == _vcSet - _baseVc) || (hopsleft == 0));
  u32 vcSetsLeft = _numVcSets - _vcSet + _baseVc;
  assert(hopsleft == 0 || hopsleft <= vcSetsLeft);

  u32 prevDim = computeInputPortDim(_dimensionWidths, _dimensionWeights,
                                    _concentration, _inputPort);

  bool allowDeroutes = hopsleft < vcSetsLeft;

  std::set<u32> dimensions;
  // determine the next dimension to work on
  u32 dim;
  u32 portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != _destinationAddress->at(dim+1)) {
      dimensions.emplace(dim);
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }

  // test if already at destination router
  if (dimensions.empty()) {
    return;
  }

  portBase = _concentration;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (dimensions.find(dim) != dimensions.end()) {
      // more router-to-router hops needed
      u32 src = routerAddress.at(dim);
      u32 dst = _destinationAddress->at(dim+1);
      u32 srcDstOffset = computeSrcDstOffset(src, dst,
                                             _dimensionWidths.at(dim));
      // allow deroutes
      if ((dim == prevDim) && !_multiDeroute) {
        allowDeroutes = false;
      } else {
        allowDeroutes = hopsleft < vcSetsLeft;
      }

      // add ports
      for (u32 offset = 1; offset < _dimensionWidths.at(dim); offset++) {
        for (u32 weight = 0; weight < _dimensionWeights.at(dim); weight++) {
          u32 outPort = computeOutputPort(portBase, offset,
                                          _dimensionWeights.at(dim),
                                          weight);
          for (u32 vc = _vcSet; vc < _numVcs; vc += _numVcSets) {
            if (offset != srcDstOffset) {
              // deroute offset
              if (allowDeroutes) {
                f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                           outPort, vc);
                std::tuple<u32, u32, f64> t(outPort, vc, congestion);
                bool res = _outputVcsNonMin->emplace(t).second;
                assert(res);
              }
            } else {
              // minimal offset
              f64 congestion = _router->congestionStatus(_inputPort, _inputVc,
                                                         outPort, vc);
              std::tuple<u32, u32, f64> t(outPort, vc, congestion);
              bool res = _outputVcsMin->emplace(t).second;
              assert(res);
            }
          }
        }
      }
    }
    portBase += ((_dimensionWidths.at(dim) - 1) * _dimensionWeights.at(dim));
  }
}

/********************SKIPPING DIMENSIONALLY ORDERED ROUTING ALGORITHMS********/

void skippingDimOrderRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _startingDim, u32 _baseVc, u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    Flit* _flit, f64 _iBias, f64 _cBias, f64 _step,
    f64 _threshold, f64 _thresholdMin, f64 _thresholdNonMin,
    SkippingRoutingAlg _routingAlg, DecisionScheme _decisionScheme,
    HopCountMode _hopCountMode,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcs1,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcs2,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcs3,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  _outputVcs1->clear();
  _outputVcs2->clear();
  _outputVcs3->clear();
  _vcPool->clear();
  bool nonMin = false;

  // ex: [x,y,z] for router, [c,x,y,z] for destination
  const std::vector<u32>& routerAddress = _router->address();
  assert(routerAddress.size() == (_destinationAddress->size() - 1));

  std::vector<u32> fakeDestinationAddress(*_destinationAddress);
  if (_startingDim > 0) {
    for (u32 dim = 1; dim < _startingDim; dim++) {
      fakeDestinationAddress.at(dim) = routerAddress.at(dim - 1);
    }
  }

  u32 hops = hopsLeft(_router, _destinationAddress);
  u32 hopIncr = U32_MAX;
  if (_hopCountMode == HopCountMode::ABS) {
    hopIncr = 1;
  } else if (_hopCountMode == HopCountMode::NORM) {
    hopIncr = hops;
  }

  // Put into outputVcs1 routing options without skipping dimension
  if (_routingAlg == SkippingRoutingAlg::DORV) {
    dimOrderVcRoutingOutput(_router, _inputPort, _inputVc, _dimensionWidths,
                            _dimensionWeights, _concentration,
                            &fakeDestinationAddress, {_vcSet}, _numVcSets,
                            _numVcs, _outputVcs1);
  } else if (_routingAlg == SkippingRoutingAlg::DORP) {
    dimOrderPortRoutingOutput(_router, _inputPort, _inputVc, _dimensionWidths,
                              _dimensionWeights, _concentration,
                              &fakeDestinationAddress, {_vcSet}, _numVcSets,
                              _numVcs, _outputVcs1);
  } else {
    if (_routingAlg == SkippingRoutingAlg::DOALV) {
      doalVcRoutingOutput(_router, _inputPort, _inputVc, _dimensionWidths,
                          _dimensionWeights, _concentration,
                          &fakeDestinationAddress, _baseVc, _vcSet, _numVcSets,
                          _numVcs, _flit, _outputVcs1, _outputVcs2);
    } else if (_routingAlg == SkippingRoutingAlg::DOALP) {
      doalPortRoutingOutput(_router, _inputPort, _inputVc, _dimensionWidths,
                            _dimensionWeights, _concentration,
                            &fakeDestinationAddress, _baseVc, _vcSet,
                            _numVcSets, _numVcs, _flit, _outputVcs1,
                            _outputVcs2);
    } else {
      fprintf(stderr, "Invalid skipping routing algorithm\n");
      assert(false);
    }

    if (_decisionScheme == DecisionScheme::MW) {
      monolithicWeighted(*_outputVcs1, *_outputVcs2,
                         hops, hopIncr, _iBias, _cBias, BiasScheme::REGULAR,
                         _vcPool, &nonMin);
      _vcPool->swap(*_outputVcs1);
      _vcPool->clear();
    } else if (_decisionScheme == DecisionScheme::ST) {
      stagedThreshold(*_outputVcs1, *_outputVcs2,
                      _thresholdMin, _thresholdNonMin,
                      _vcPool, &nonMin);
      _vcPool->swap(*_outputVcs1);
      _vcPool->clear();
    } else if (_decisionScheme == DecisionScheme::TW) {
      thresholdWeighted(*_outputVcs1, *_outputVcs2,
                        hops, hopIncr, _threshold,
                        _vcPool, &nonMin);
      _vcPool->swap(*_outputVcs1);
      _vcPool->clear();
    } else {
      fprintf(stderr, "Invalid decision scheme\n");
      assert(false);
    }
  }

  for (u32 dim = _startingDim; dim < _dimensionWidths.size(); dim++) {
    fakeDestinationAddress.at(dim+1) = routerAddress.at(dim);
    u32 skippedDims = dim - _startingDim + 1;

    // Put into outputVcs2 routing options with the next dimension skipped
    if (_routingAlg == SkippingRoutingAlg::DORV) {
      dimOrderVcRoutingOutput(_router, _inputPort, _inputVc, _dimensionWidths,
                              _dimensionWeights, _concentration,
                              &fakeDestinationAddress, {_vcSet}, _numVcSets,
                              _numVcs, _outputVcs2);
    } else if (_routingAlg == SkippingRoutingAlg::DORP) {
      dimOrderPortRoutingOutput(_router, _inputPort, _inputVc, _dimensionWidths,
                                _dimensionWeights, _concentration,
                                &fakeDestinationAddress, {_vcSet}, _numVcSets,
                                _numVcs, _outputVcs2);
    } else {
      if (_routingAlg == SkippingRoutingAlg::DOALV) {
        doalVcRoutingOutput(_router, _inputPort, _inputVc, _dimensionWidths,
                            _dimensionWeights, _concentration,
                            &fakeDestinationAddress, _baseVc, _vcSet,
                            _numVcSets, _numVcs, _flit, _outputVcs2,
                            _outputVcs3);
      } else if (_routingAlg == SkippingRoutingAlg::DOALP) {
        doalPortRoutingOutput(_router, _inputPort, _inputVc, _dimensionWidths,
                              _dimensionWeights, _concentration,
                              &fakeDestinationAddress, _baseVc, _vcSet,
                              _numVcSets, _numVcs, _flit, _outputVcs2,
                              _outputVcs3);
      } else {
        fprintf(stderr, "Invalid skipping routing algorithm\n");
        assert(false);
      }

      if (_decisionScheme == DecisionScheme::MW) {
        monolithicWeighted(*_outputVcs2, *_outputVcs3,
                           hops, hopIncr, _iBias, _cBias, BiasScheme::REGULAR,
                           _vcPool, &nonMin);
        _outputVcs2->clear();
        _vcPool->swap(*_outputVcs2);
      } else if (_decisionScheme == DecisionScheme::ST) {
        stagedThreshold(*_outputVcs2, *_outputVcs3,
                        _thresholdMin, _thresholdNonMin,
                        _vcPool, &nonMin);
        _outputVcs2->clear();
        _vcPool->swap(*_outputVcs2);
      } else if (_decisionScheme == DecisionScheme::TW) {
        thresholdWeighted(*_outputVcs1, *_outputVcs2,
                          hops, hopIncr, _threshold,
                          _vcPool, &nonMin);
        _vcPool->swap(*_outputVcs1);
        _vcPool->clear();
      } else {
        fprintf(stderr, "Invalid decision scheme\n");
        assert(false);
      }
    }

    // Make a choice between previous iteration (outputVcs1) and current
    // dimension skipped (outputVcs2), put result in outputVc1
    if (_decisionScheme == DecisionScheme::MW) {
      monolithicWeighted(*_outputVcs1, *_outputVcs2,
                         1, 0, _iBias, _cBias + _step * skippedDims,
                         BiasScheme::REGULAR,
                         _vcPool, &nonMin);
    } else if (_decisionScheme == DecisionScheme::ST) {
      stagedThreshold(*_outputVcs1, *_outputVcs2,
                      _thresholdMin, _step * skippedDims,
                      _vcPool, &nonMin);
      if (!_vcPool->empty()) {
        break;
      }
    } else if (_decisionScheme == DecisionScheme::TW) {
      thresholdWeighted(*_outputVcs1, *_outputVcs2,
                        1, 0, _threshold,
                        _vcPool, &nonMin);
    } else {
      fprintf(stderr, "Invalid decision scheme\n");
      assert(false);
    }
    _outputVcs1->swap(*_vcPool);
  }
  _vcPool->swap(*_outputVcs1);
}

void finishingDimOrderRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _baseVc, u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    Flit* _flit, f64 _iBias, f64 _cBias,
    f64 _threshold, f64 _thresholdMin, f64 _thresholdNonMin,
    SkippingRoutingAlg _routingAlg, DecisionScheme _decisionScheme,
    HopCountMode _hopCountMode,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcs1,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcs2,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool) {
  _outputVcs1->clear();
  _outputVcs2->clear();
  _vcPool->clear();
  bool nonMin = false;

  u32 hops = hopsLeft(_router, _destinationAddress);
  u32 hopIncr = U32_MAX;
  if (_hopCountMode == HopCountMode::ABS) {
    hopIncr = 1;
  } else if (_hopCountMode == HopCountMode::NORM) {
    hopIncr = hops;
  }

  if (_routingAlg == SkippingRoutingAlg::DORV) {
    dimOrderVcRoutingOutput(
        _router, _inputPort, _inputVc, _dimensionWidths, _dimensionWeights,
        _concentration, _destinationAddress, {_vcSet}, _numVcSets, _numVcs,
        _vcPool);
  } else if (_routingAlg == SkippingRoutingAlg::DORP) {
    dimOrderPortRoutingOutput(
        _router, _inputPort, _inputVc, _dimensionWidths, _dimensionWeights,
        _concentration, _destinationAddress, {_vcSet}, _numVcSets, _numVcs,
        _vcPool);
  } else {
    if (_routingAlg == SkippingRoutingAlg::DOALV) {
      doalVcRoutingOutput(
          _router, _inputPort, _inputVc, _dimensionWidths, _dimensionWeights,
          _concentration, _destinationAddress, _baseVc, _vcSet, _numVcSets,
          _numVcs, _flit, _outputVcs1, _outputVcs2);
    } else if (_routingAlg == SkippingRoutingAlg::DOALP) {
      doalPortRoutingOutput(
          _router, _inputPort, _inputVc, _dimensionWidths, _dimensionWeights,
          _concentration, _destinationAddress, _baseVc, _vcSet, _numVcSets,
          _numVcs, _flit, _outputVcs1, _outputVcs2);
    } else {
      fprintf(stderr, "Invalid skipping routing algorithm\n");
      assert(false);
    }

    if (_decisionScheme == DecisionScheme::MW) {
      monolithicWeighted(*_outputVcs1, *_outputVcs2,
                         hops, hopIncr, _iBias, _cBias, BiasScheme::REGULAR,
                         _vcPool, &nonMin);
    } else if (_decisionScheme == DecisionScheme::ST) {
      stagedThreshold(*_outputVcs1, *_outputVcs2,
                      _thresholdMin, _thresholdNonMin,
                      _vcPool, &nonMin);
    } else if (_decisionScheme == DecisionScheme::TW) {
      thresholdWeighted(*_outputVcs1, *_outputVcs2,
                        hops, hopIncr, _threshold,
                        _vcPool, &nonMin);
    } else {
      fprintf(stderr, "Invalid decision scheme\n");
      assert(false);
    }
  }
}

/********************DECISION SCHEMES ****************************************/

void monolithicWeighted(
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsMin,
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsNonMin,
    f64 _hopsLeft, f64 _hopsIncr,
    f64 _iBias, f64 _cBias, BiasScheme _biasMode,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool,
    bool* _nonMin) {
  _vcPool->clear();

  f64 weightMin = F64_MAX;
  *_nonMin = false;

  for (auto& it : _outputVcsMin) {
    f64 congestion = std::get<2>(it);
    f64 weight = _hopsLeft * congestion;
    f64 delta = weightMin - weight;
    f64 absDelta = std::abs(delta);

    if (delta > TOLERANCE) {  // replace
      weightMin = weight;
      _vcPool->clear();
      _vcPool->emplace(it);

    } else if (absDelta < TOLERANCE) {  // same (add)
      _vcPool->emplace(it);
    }
  }

  f64 qMin = weightMin / _hopsLeft;
  f64 weight = F64_MAX;

  for (auto& it : _outputVcsNonMin) {
    f64 congestion = std::get<2>(it);
    if (_biasMode == BiasScheme::REGULAR) {
      weight = (_hopsLeft + _hopsIncr) * (congestion + _cBias) + _iBias;
    } else if (_biasMode == BiasScheme::BIMODAL) {
      weight = (congestion + _cBias * (1 - qMin)) *
          (_hopsLeft + _hopsIncr) + _iBias;
    } else if (_biasMode == BiasScheme::DIFFERENTIAL) {
      weight = (congestion + _cBias  - qMin) *
          (_hopsLeft + _hopsIncr) + _iBias;
    } else if (_biasMode == BiasScheme::PROPORTIONAL) {
      weight = (congestion + _cBias * std::max(0.01, congestion) /
                std::max(0.01, qMin)) * (_hopsLeft + _hopsIncr) + _iBias;
    } else if (_biasMode == BiasScheme::PROPORTIONALDIF) {
      weight = (congestion + std::max(congestion - qMin, _cBias * (-qMin)) /
                std::max(0.01, qMin)) * (_hopsLeft + _hopsIncr) + _iBias;
    } else {
      fprintf(stderr, "Unknown weighting scheme\n");
      assert(false);
    }

    f64 delta = weightMin - weight;
    f64 absDelta = std::abs(delta);

    if (delta > TOLERANCE) {  // replace
      *_nonMin = true;
      weightMin = weight;
      _vcPool->clear();
      _vcPool->emplace(it);
    } else if ((absDelta < TOLERANCE) && (*_nonMin)) {  // same (add)
      _vcPool->emplace(it);
      *_nonMin = true;
    }
  }
}

void stagedThreshold(
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsMin,
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsNonMin,
    f64 _thresholdMin, f64 _thresholdNonMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool,
    bool* _nonMin) {
  _vcPool->clear();
  *_nonMin = false;

  // Under threshold
  for (auto& it : _outputVcsMin) {
    f64 congestion = std::get<2>(it);
    if (congestion < (_thresholdMin + 1e-6)) {
      // min < TH
      _vcPool->emplace(it);
    }
  }
  if (_vcPool->empty() && !_outputVcsNonMin.empty()) {
    for (auto& it : _outputVcsNonMin) {
      f64 congestion = std::get<2>(it);
      if (congestion < (_thresholdNonMin + 1e-6)) {
        // non-minimal < TH
        _vcPool->emplace(it);
        *_nonMin = true;
      }
    }
  }

  // Above threshold
  if (_vcPool->empty()) {
    if (!_outputVcsNonMin.empty()) {
      // no minimal > TH
      *_vcPool = _outputVcsNonMin;
      *_nonMin = true;
    } else {
      // min > TH
      *_vcPool = _outputVcsMin;
    }
  }
}

void thresholdWeighted(
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsMin,
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsNonMin,
    f64 _hopsLeft, f64 _hopsIncr,
    f64 _threshold,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool,
    bool* _nonMin) {
  _vcPool->clear();
  f64 leastCong = F64_MAX;
  *_nonMin = false;

  // under TH use (least congested) minimal route
  for (auto& it : _outputVcsMin) {
    f64 congMin = std::get<2>(it);
    f64 delta = leastCong - congMin;
    f64 absDelta = std::abs(delta);

    if (delta > TOLERANCE) {  // replace
      leastCong = congMin;
      _vcPool->clear();
      _vcPool->emplace(it);
    } else if (absDelta < TOLERANCE) {  // same (add)
      _vcPool->emplace(it);
    }
  }

  // if least congestion greater than TH use (leastCong) NONminimal routes
  if (leastCong >= (_threshold - TOLERANCE) && !_outputVcsNonMin.empty()) {
    _vcPool->clear();
    *_nonMin = true;
    // use non min with minimal congestion
    leastCong = F64_MAX;
    for (auto& it : _outputVcsNonMin) {
      f64 congNM = std::get<2>(it) * (_hopsLeft + _hopsIncr);
      f64 delta = leastCong - congNM;
      f64 absDelta = std::abs(delta);

      if (delta > TOLERANCE) {  // replace
        leastCong = congNM;
        _vcPool->clear();
        _vcPool->emplace(it);
      } else if (absDelta < TOLERANCE) {  // same (add)
        _vcPool->emplace(it);
      }
    }
  }
}

}  // namespace HyperX
