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
#ifndef NETWORK_HYPERX_UTIL_H_
#define NETWORK_HYPERX_UTIL_H_

#include <prim/prim.h>

#include <unordered_set>
#include <vector>
#include <tuple>

#include "router/Router.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace HyperX {

u32 computeMinimalHops(const std::vector<u32>* _source,
                       const std::vector<u32>* _destination,
                       u32 _dimensions);

u32 computeOutputPort(u32 _base, u32 _offset, u32 _dimWeight, u32 _weight);
u32 computeSrcDstOffset(u32 _src, u32 _dst, u32 _dimWidth);

enum class MinRoutingAlg : u8 {RMINV, RMINP, AMINV, AMINP};
enum class IntNodeAlg : u8 {REG, SRC, DST, SRCDST, UNALIGNED, MINV, MINP, NONE};
enum class BaseRoutingAlg : u8 {DORV, DORP, RMINV, RMINP, AMINV, AMINP};
enum class NonMinRoutingAlg : u8 {VAL, LCQV, LCQP};
enum class AdaptiveRoutingAlg : u8 {DDALV, DDALP, DOALV, DOALP, VDALV, VDALP};
enum class SkippingRoutingAlg : u8 {DORV, DORP, DOALV, DOALP};
enum class DecisionScheme : u8 {MW, ST, TW};
enum class BiasScheme : u8 {REGULAR, BIMODAL, PROPORTIONAL, DIFFERENTIAL,
  PROPORTIONALDIF};
enum class HopCountMode : u8 {ABS, NORM};
enum class OutputAlg : u8 {Rand, Min};

typedef void (*IntNodeAlgFunc)(
    Router*, u32, u32, const std::vector<u32>&, const std::vector<u32>*,
    const std::vector<u32>&, const std::vector<u32>&,
    u32, u32, u32, u32, std::vector<u32>*);

typedef void (*MinRoutingAlgFunc)(
    Router*, u32, u32,
    const std::vector<u32>&,
    const std::vector<u32>&, u32,
    const std::vector<u32>*,
    const std::vector<u32>&, u32, u32,
    std::unordered_set< std::tuple<u32, u32, f64> >*);

typedef void (*FirstHopRoutingAlgFunc)(
    Router*, u32, u32, const std::vector<u32>&, const std::vector<u32>&,
    u32, const std::vector<u32>*, u32, u32, u32, bool,
    std::unordered_set< std::tuple<u32, u32, f64> >*);

bool isDestinationRouter(
    Router* _router, const std::vector<u32>* _destinationAddress);

u32 hopsLeft(
    Router* _router, const std::vector<u32>* _destinationAddress);

u32 computeInputPortDim(const std::vector<u32>& _dimensionWidths,
                        const std::vector<u32>& _dimensionWeights,
                        u32 _concentration, u32 _inputPort);
void intNodeReg(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address);

void intNodeMoveUnaligned(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address);

void intNodeSrc(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address);

void intNodeDst(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address);

void intNodeSrcDst(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address);

void intNodeMinV(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address);

void intNodeMinP(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    std::vector<u32>* _address);

void makeOutputVcSet(
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool,
    u32 _maxOutputs, OutputAlg _outputAlg,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputPorts);

void makeOutputPortSet(
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    u32 _maxOutputs, OutputAlg _outputAlg,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputPorts);

f64 getAveragePortCongestion(
    Router* _router, u32 _inputPort, u32 _inputVc, u32 _outputPort,
    const std::vector<u32>& _vcSets,
    u32 _numVcSets, u32 _numVcs);

void dimOrderVcRoutingOutput(
    Router* router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

void dimOrderPortRoutingOutput(
    Router* router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

void randMinVcRoutingOutput(
    Router* router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

void randMinPortRoutingOutput(
    Router* router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

void adaptiveMinVcRoutingOutput(
    Router* router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSet, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

void adaptiveMinPortRoutingOutput(
    Router* router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    const std::vector<u32>& _vcSets, u32 _numVcSets, u32 _numVcs,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

void valiantsRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, bool _shortCut,
    IntNodeAlg _intNodeAlg, BaseRoutingAlg _routingAlg, Flit* _flit,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

void ugalRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, bool _shortCut, bool _minAllVcSets,
    IntNodeAlg _intNodeAlg, BaseRoutingAlg _routingAlg,
    NonMinRoutingAlg _nonMinimalAlg,
    Flit* _flit, f64* _weightReg, f64* _weightVal,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputPortsReg,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputPortsVal);

void lcqVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, bool _shortCut,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

void lcqPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, bool _shortCut,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

void doalPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _baseVc, u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin);

void doalVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _baseVc, u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin);

void ddalPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin);

void ddalVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin);

void vdalPortRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress, u32 _baseVc,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit, bool _multiDeroute,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin);

void vdalVcRoutingOutput(
    Router* _router, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    const std::vector<u32>* _destinationAddress, u32 _baseVc,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs, Flit* _flit, bool _multiDeroute,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _outputVcsNonMin);

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
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

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
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool);

void monolithicWeighted(
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsMin,
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsNonMin,
    f64 _hopsLeft, f64 _hopsIncr, f64 _iBias, f64 _cBias, BiasScheme _biasMode,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool,
    bool* _nonMin);

void stagedThreshold(
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsMin,
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsNonMin,
    f64 _thresholdMin, f64 _thresholdNonMin,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool,
    bool* _nonMin);

void thresholdWeighted(
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsMin,
    const std::unordered_set< std::tuple<u32, u32, f64> >& _outputVcsNonMin,
    f64 _hopsLeft, f64 _hopsIncr, f64 _threshold,
    std::unordered_set< std::tuple<u32, u32, f64> >* _vcPool,
    bool* _nonMin);

}  // namespace HyperX

template <typename T>
const T* uSetRandElement(const std::unordered_set<T>& uSet);

template <typename T>
const T* uSetMinCong(const std::unordered_set<T>& uSet);

#include "network/hyperx/util.tcc"

#endif  // NETWORK_HYPERX_UTIL_H_
