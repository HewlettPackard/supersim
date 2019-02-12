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
#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>
#include <strop/strop.h>

#include <unordered_set>
#include <vector>
#include <iostream>

#include "event/Component.h"
#include "network/cube/util.h"
#include "router/Router.h"
#include "test/TestSetup_TEST.h"

namespace {

TEST(HyperX, computeMinimalHops) {
  std::vector<u32> src;
  std::vector<u32> dst;
  u32 exp;
  u32 dimensions;

  src = {2, 0};
  dst = {0, 1};
  dimensions = 1;
  exp = 2;
  ASSERT_EQ(exp, HyperX::computeMinimalHops(&src, &dst, dimensions));

  src = {0, 0, 0};
  dst = {0, 2, 2};
  dimensions = 2;
  exp = 3;
  ASSERT_EQ(exp, HyperX::computeMinimalHops(&src, &dst, dimensions));

  src = {0, 1, 0, 0};
  dst = {0, 2, 2, 2};
  dimensions = 3;
  exp = 4;
  ASSERT_EQ(exp, HyperX::computeMinimalHops(&src, &dst, dimensions));
}

Json::Value makeJSON(u32 _numPorts, u32 _numVcs) {
  Json::Value settings;
  settings["num_ports"] = _numPorts;
  settings["num_vcs"] = _numVcs;
  return settings;
}
}  // namespace

class TestRouter : public Router {
 public:
  TestRouter(const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
             const std::unordered_map<u32, f64>& _congStatus)
      : Router("TestRouter " + strop::vecString<u32>(_address), nullptr,
               nullptr, 0, _address, _numPorts, _numVcs,
               std::vector<std::tuple<u32, u32> >(), nullptr,
               makeJSON(_numPorts, _numVcs)) {
    congStatus_ = _congStatus;
  }
  ~TestRouter() {}
  void setInputChannel(u32 _port, Channel* _channel) override {}
  Channel* getInputChannel(u32 _port) const override { return nullptr; }
  void setOutputChannel(u32 port, Channel* _channel) override {}
  Channel* getOutputChannel(u32 _port) const override { return nullptr; }
  void sendCredit(u32 _port, u32 _vc) override {}
  void receiveCredit(u32 _port, Credit* _credit) override {}
  void sendFlit(u32 _port, Flit* _flit) override {}
  void receiveFlit(u32 _port, Flit* _flit) override {}
  f64 congestionStatus(u32 _inputPort, u32 _inputVc, u32 _outputPort,
                       u32 _outputVc) const override {
    u32 vcIdx = vcIndex(_outputPort, _outputVc);
    std::unordered_map<u32, f64>::const_iterator got = congStatus_.find(vcIdx);
    if (got == congStatus_.end()) {
      return 1.0;
    }
    return congStatus_.at(vcIdx);
  }

 private:
  std::unordered_map<u32, f64> congStatus_;
};

void intNodeTestCongestion(
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _dimWidths,
    const std::vector<u32>& _dimWeights,
    u32 _conc,
    u32 _vcSet, u32 _numVcSets, u32 _numVcs,
    u32 _refNumBuckets,
    const std::unordered_set<u32>& _idSet,
    const std::unordered_map<u32, f64> _congStatus,
    HyperX::IntNodeAlgFunc _intNodeAlgFunc) {
  std::vector<u32> addr;
  const u32 conc = 1;
  u32 numBuckets;
  u32 nonZeroBuckets = 0;
  const u64 kRounds = 10000;
  std::vector<u64> buckets;

  TestRouter* router;
  u32 numPorts;
  numPorts = conc;
  for (u32 dim = 0; dim < _dimWidths.size(); dim++) {
    numPorts += _dimWidths.at(dim) * _dimWeights.at(dim);
  }

  numBuckets = 1;
  for (u32 idx = 0; idx < _dimWidths.size(); idx++) {
    numBuckets *= _dimWidths.at(idx);
  }
  buckets.resize(numBuckets);

  for (u64 idx = 0; idx < kRounds; idx++) {
    router = new TestRouter(_sourceRouter, numPorts, _numVcs, _congStatus);
    _intNodeAlgFunc(router, 0, 0, _sourceRouter, _destinationTerminal,
                    _dimWidths, _dimWeights, _conc, _vcSet, _numVcSets, _numVcs,
                    &addr);
    u32 id = Cube::translateInterfaceAddressToId(&addr, _dimWidths, conc);
    const std::unordered_set<u32>::const_iterator got = _idSet.find(id);
    ASSERT_FALSE(got == _idSet.end());
    buckets.at(id)++;
    delete router;
  }

  f64 sum = 0;
  for (u64 b = 0; b < numBuckets; b++) {
    sum += buckets.at(b);
    if (buckets.at(b) != 0) {
      nonZeroBuckets++;
    }
  }
  ASSERT_EQ(nonZeroBuckets, _refNumBuckets);
  f64 mean = sum / nonZeroBuckets;

  sum = 0;
  for (u64 b = 0; b < numBuckets; b++) {
    if (buckets.at(b) != 0) {
      f64 c = (static_cast<f64>(buckets.at(b)) - mean);
      c *= c;
      sum += c;
    }
  }

  f64 stdDev = std::sqrt(sum / nonZeroBuckets);
  stdDev /= kRounds;

  ASSERT_NEAR(stdDev, 0.0, 0.015);
}

void intNodeTest(const std::vector<u32>& _sourceRouter,
                 const std::vector<u32>* _destinationTerminal,
                 const std::vector<u32>& _dimWidths,
                 u32 _refNumBuckets,
                 const std::unordered_set<u32>& _idSet,
                 HyperX::IntNodeAlgFunc _intNodeAlgFunc) {
  std::vector<u32> weights(_dimWidths.size(), 1u);
  std::unordered_map<u32, f64> congStatus;
  intNodeTestCongestion(_sourceRouter, _destinationTerminal, _dimWidths,
                        weights, 1, 0, 1, 1,
                        _refNumBuckets, _idSet, congStatus, _intNodeAlgFunc);
}

void distRoutingTest(
    const std::vector<u32>& _sourceRouter,
    const std::vector<u32>* _destinationTerminal,
    const std::vector<u32>& _widths, const std::vector<u32>& _weights,
    u32 _conc, u32 _vcSet, u32 _numVcSets, u32 _numVcs, bool _shortCut,
    u32 _maxOutputs,
    const std::unordered_set< std::tuple<u32, u32> > _refOutputPorts,
    u32 numOutputs, bool _vcOutput,
    const std::unordered_map<u32, f64>& _congStatus,
    HyperX::MinRoutingAlgFunc _routingAlgFunc,
    HyperX::FirstHopRoutingAlgFunc _firstHopAlgFunc) {
  std::unordered_set< std::tuple<u32, u32, f64> > vcPool;
  std::unordered_set< std::tuple<u32, u32, f64> > outputPorts;
  TestRouter* router;
  u32 numPorts;
  numPorts = _conc;
  for (u32 dim = 0; dim < _widths.size(); dim++) {
    numPorts += _widths.at(dim) * _weights.at(dim);
  }
  router = new TestRouter(_sourceRouter, numPorts, _numVcs, _congStatus);

  u32 numBuckets;
  u32 nonZeroBuckets = 0;
  const u64 kRounds = 10000;
  std::vector<u64> buckets;

  numBuckets = 1;
  for (u32 idx = 0; idx < _widths.size(); idx++) {
    numBuckets *= (_conc + _widths.at(idx) * _weights.at(idx)) * _numVcs;
  }
  buckets.resize(numBuckets);

  for (u64 idx = 0; idx < kRounds; idx++) {
    if (_routingAlgFunc) {
      _routingAlgFunc(router, 0, 0, _widths, _weights, _conc,
                      _destinationTerminal, {_vcSet}, _numVcSets, _numVcs,
                      &vcPool);
    } else {
      _firstHopAlgFunc(router, 0, 0, _widths, _weights, _conc,
                       _destinationTerminal, _vcSet, _numVcSets, _numVcs,
                       _shortCut, &vcPool);
    }

    if (_vcOutput) {
      HyperX::makeOutputVcSet(&vcPool, _maxOutputs, HyperX::OutputAlg::Rand,
                              &outputPorts);
    } else {
      HyperX::makeOutputPortSet(&vcPool, {_vcSet}, _numVcSets, _numVcs,
                                _maxOutputs, HyperX::OutputAlg::Rand,
                                &outputPorts);
    }

    ASSERT_EQ(outputPorts.size(), numOutputs);
    for (auto& it : outputPorts) {
      u32 port = std::get<0>(it);
      u32 vc = std::get<1>(it);
      std::tuple<u32, u32> t(port, vc);
      u32 vcIdx = router->vcIndex(port, vc);
      const std::unordered_set< std::tuple<u32, u32> >::const_iterator got
          = _refOutputPorts.find(t);
      ASSERT_FALSE(got == _refOutputPorts.end());
      buckets.at(vcIdx)++;
    }
  }
  delete router;

  f64 sum = 0;
  for (u64 b = 0; b < numBuckets; b++) {
    sum += buckets.at(b);
    if (buckets.at(b) != 0) {
      nonZeroBuckets++;
    }
    // printf("[%lu] = %lu\n", b, buckets.at(b));
  }
  ASSERT_EQ(nonZeroBuckets, _refOutputPorts.size());
  if (nonZeroBuckets == 0) {
    return;
  }
  f64 mean = sum / nonZeroBuckets;

  sum = 0;
  for (u64 b = 0; b < numBuckets; b++) {
    if (buckets.at(b) != 0) {
      f64 c = (static_cast<f64>(buckets.at(b)) - mean);
      c *= c;
      sum += c;
    }
  }

  f64 stdDev = std::sqrt(sum / nonZeroBuckets);
  // printf("stdDev = %f\n", stdDev);
  stdDev /= kRounds;
  // printf("relStdDev = %f\n", stdDev);

  ASSERT_NEAR(stdDev, 0.0, 0.015);
}

TEST(HyperXUtil, isDestinationRouter) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst;
  std::vector<u32> widths, weights;
  u32 conc, numVcs;
  std::unordered_map<u32, f64> congStatus;

  conc = 2; numVcs = 1; src = {0}; widths = {2}; weights = {1};
  TestRouter* router;
  u32 numPorts;
  numPorts = conc;
  for (u32 dim = 0; dim < widths.size(); dim++) {
    numPorts += widths.at(dim) * weights.at(dim);
  }
  router = new TestRouter(src, numPorts, numVcs, congStatus);

  dst = {0, 1};
  ASSERT_FALSE(HyperX::isDestinationRouter(router, &dst));

  dst = {1, 1};
  ASSERT_FALSE(HyperX::isDestinationRouter(router, &dst));

  dst = {0, 0};
  ASSERT_TRUE(HyperX::isDestinationRouter(router, &dst));

  dst = {1, 0};
  ASSERT_TRUE(HyperX::isDestinationRouter(router, &dst));

  delete router;
}

TEST(HyperXUtil, hopsLeft) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst;
  std::vector<u32> widths, weights;
  u32 conc, numVcs;
  std::unordered_map<u32, f64> congStatus;

  conc = 2; numVcs = 1; src = {0}; widths = {2}; weights = {1};
  TestRouter* router;
  u32 numPorts;
  numPorts = conc;
  for (u32 dim = 0; dim < widths.size(); dim++) {
    numPorts += widths.at(dim) * weights.at(dim);
  }
  router = new TestRouter(src, numPorts, numVcs, congStatus);

  dst = {0, 1};
  ASSERT_EQ(HyperX::hopsLeft(router, &dst), 1u);

  dst = {1, 1};
  ASSERT_EQ(HyperX::hopsLeft(router, &dst), 1u);

  dst = {0, 0};
  ASSERT_EQ(HyperX::hopsLeft(router, &dst), 0u);

  dst = {1, 0};
  ASSERT_EQ(HyperX::hopsLeft(router, &dst), 0u);

  delete router;

  src = {0, 0}; dst = {0, 1, 0}; widths = {2, 2}; weights = {1, 1};
  numPorts = conc;
  for (u32 dim = 0; dim < widths.size(); dim++) {
    numPorts += widths.at(dim) * weights.at(dim);
  }
  router = new TestRouter(src, numPorts, numVcs, congStatus);

  dst = {0, 1, 0};
  ASSERT_EQ(HyperX::hopsLeft(router, &dst), 1u);

  dst = {0, 1, 1};
  ASSERT_EQ(HyperX::hopsLeft(router, &dst), 2u);

  dst = {0, 0, 1};
  ASSERT_EQ(HyperX::hopsLeft(router, &dst), 1u);

  delete router;
}

TEST(HyperXUtil, intNodeMoveUnaligned) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src;
  std::vector<u32> dst;
  std::vector<u32> widths;
  std::unordered_set<u32> idSet;

  src = {0}; dst = {0, 1}; widths = {2};
  idSet = {0, 1};
  intNodeTest(src, &dst, widths, idSet.size(), idSet,
              HyperX::intNodeMoveUnaligned);

  src = {0}; dst = {0, 2}; widths = {3};
  idSet = {0, 1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet,
              HyperX::intNodeMoveUnaligned);

  src = {1}; dst = {0, 2}; widths = {4};
  idSet = {0, 1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet,
              HyperX::intNodeMoveUnaligned);

  src = {0, 0}; dst = {0, 1, 0}; widths = {2, 2};
  idSet = {0, 1};
  intNodeTest(src, &dst, widths, idSet.size(), idSet,
              HyperX::intNodeMoveUnaligned);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2};
  idSet = {0, 1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet,
              HyperX::intNodeMoveUnaligned);

  src = {0, 0, 0}; dst = {0, 1, 0, 0}; widths = {2, 2, 2};
  idSet = {0, 1};
  intNodeTest(src, &dst, widths, idSet.size(), idSet,
              HyperX::intNodeMoveUnaligned);

  src = {0, 0, 0}; dst = {0, 1, 1, 1}; widths = {2, 2, 2};
  idSet = {0, 1, 2, 3, 4, 5, 6, 7};
  intNodeTest(src, &dst, widths, idSet.size(), idSet,
              HyperX::intNodeMoveUnaligned);

  src = {1, 1}; dst = {0, 3, 2}; widths = {5, 4};
  for (u32 ind = 0; ind < 20; ind++) {
    idSet.insert(ind);
  }
  intNodeTest(src, &dst, widths, idSet.size(), idSet,
              HyperX::intNodeMoveUnaligned);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};
  for (u32 ind = 0; ind < 20; ind++) {
    idSet.insert(ind);
  }
  intNodeTest(src, &dst, widths, idSet.size(), idSet,
              HyperX::intNodeMoveUnaligned);

  src = {3, 1}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {5, 6, 7, 8, 9};
  intNodeTest(src, &dst, widths, idSet.size(), idSet,
              HyperX::intNodeMoveUnaligned);
}

TEST(HyperXUtil, intNodeReg) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src;
  std::vector<u32> dst;
  std::vector<u32> widths;
  std::unordered_set<u32> idSet;

  src = {0}; dst = {0, 1}; widths = {2};
  idSet = {0, 1};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeReg);

  src = {0}; dst = {0, 2}; widths = {3};
  idSet = {0, 1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeReg);

  src = {1}; dst = {0, 2}; widths = {4};
  idSet = {0, 1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeReg);

  src = {0, 0}; dst = {0, 1, 0}; widths = {2, 2};
  idSet = {0, 1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeReg);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2};
  idSet = {0, 1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeReg);

  src = {0, 0, 0}; dst = {0, 1, 0, 0}; widths = {2, 2, 2};
  idSet = {0, 1, 2, 3, 4, 5, 6, 7};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeReg);

  src = {0, 0, 0}; dst = {0, 1, 1, 1}; widths = {2, 2, 2};
  idSet = {0, 1, 2, 3, 4, 5, 6, 7};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeReg);

  src = {1, 1}; dst = {0, 3, 2}; widths = {5, 4};
  for (u32 ind = 0; ind < 20; ind++) {
    idSet.insert(ind);
  }
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeReg);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};
  for (u32 ind = 0; ind < 20; ind++) {
    idSet.insert(ind);
  }
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeReg);

  src = {3, 1}; dst = {0, 1, 1}; widths = {5, 4};
  for (u32 ind = 0; ind < 20; ind++) {
    idSet.insert(ind);
  }
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeReg);
}

TEST(HyperXUtil, intNodeSrc) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src;
  std::vector<u32> dst;
  std::vector<u32> widths;
  std::unordered_set<u32> idSet;

  src = {0}; dst = {0, 1}; widths = {2};
  idSet = {0, 1};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrc);

  src = {0}; dst = {0, 2}; widths = {3};
  idSet = {0, 1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrc);

  src = {1}; dst = {0, 2}; widths = {4};
  idSet = {0, 1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrc);

  src = {0, 0}; dst = {0, 1, 0}; widths = {2, 2};
  idSet = {0, 1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrc);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2};
  idSet = {0, 1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrc);

  src = {0, 0, 0}; dst = {0, 1, 0, 0}; widths = {2, 2, 2};
  idSet = {0, 1, 2, 4};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrc);

  src = {0, 0, 0}; dst = {0, 1, 1, 1}; widths = {2, 2, 2};
  idSet = {0, 1, 2, 4};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrc);

  src = {1, 1}; dst = {0, 3, 2}; widths = {5, 4};
  idSet = {1, 5, 6, 7, 8, 9, 11, 16};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrc);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {3, 8, 10, 11, 12, 13, 14, 18};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrc);

  src = {3, 1}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {3, 5, 6, 7, 8, 9, 13, 18};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrc);
}

TEST(HyperXUtil, intNodeDst) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src;
  std::vector<u32> dst;
  std::vector<u32> widths;
  std::unordered_set<u32> idSet;

  src = {0}; dst = {0, 1}; widths = {2};
  idSet = {0, 1};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeDst);

  src = {0}; dst = {0, 2}; widths = {3};
  idSet = {0, 1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeDst);

  src = {1}; dst = {0, 2}; widths = {4};
  idSet = {0, 1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeDst);

  src = {0, 0}; dst = {0, 1, 0}; widths = {2, 2};
  idSet = {0, 1, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeDst);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2};
  idSet = {1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeDst);

  src = {0, 0, 0}; dst = {0, 1, 0, 0}; widths = {2, 2, 2};
  idSet = {0, 1, 3, 5};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeDst);

  src = {0, 0, 0}; dst = {0, 1, 1, 1}; widths = {2, 2, 2};
  idSet = {3, 5, 6, 7};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeDst);

  src = {1, 1}; dst = {0, 3, 2}; widths = {5, 4};
  idSet = {3, 8, 10, 11, 12, 13, 14, 18};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeDst);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {1, 5, 6, 7, 8, 9, 11, 16};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeDst);

  src = {3, 1}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {1, 5, 6, 7, 8, 9, 11, 16};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeDst);
}

TEST(HyperXUtil, intNodeSrcDst) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src;
  std::vector<u32> dst;
  std::vector<u32> widths;
  std::unordered_set<u32> idSet;

  src = {0}; dst = {0, 1}; widths = {2};
  idSet = {0, 1};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrcDst);

  src = {0}; dst = {0, 2}; widths = {3};
  idSet = {0, 1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrcDst);

  src = {1}; dst = {0, 2}; widths = {4};
  idSet = {0, 1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrcDst);

  src = {0, 0}; dst = {0, 1, 0}; widths = {2, 2};
  idSet = {0, 1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrcDst);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2};
  idSet = {0, 1, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrcDst);

  src = {0, 0, 0}; dst = {0, 1, 0, 0}; widths = {2, 2, 2};
  idSet = {0, 1, 2, 3, 4, 5};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrcDst);

  src = {0, 0, 0}; dst = {0, 1, 1, 1}; widths = {2, 2, 2};
  idSet = {0, 1, 2, 3, 4, 5, 6, 7};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrcDst);

  src = {1, 1}; dst = {0, 3, 2}; widths = {5, 4};
  idSet = {1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrcDst);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrcDst);

  src = {3, 1}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {1, 3, 5, 6, 7, 8, 9, 11, 13, 16, 18};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeSrcDst);
}

TEST(HyperXUtil, intNodeMinV) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  u32 conc, vcSet, numVcSets, numVcs;
  std::vector<u32> src;
  std::vector<u32> dst;
  std::vector<u32> widths, weights;
  std::unordered_set<u32> idSet;
  std::unordered_map<u32, f64> congStatus;

  src = {0}; dst = {0, 1}; widths = {2};
  idSet = {1};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinV);

  src = {0}; dst = {0, 2}; widths = {3};
  idSet = {1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinV);

  src = {1}; dst = {0, 2}; widths = {4};
  idSet = {0, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinV);

  src = {0, 0}; dst = {0, 1, 0}; widths = {2, 2};
  idSet = {1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinV);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2};
  idSet = {1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinV);

  src = {0, 0, 0}; dst = {0, 1, 0, 0}; widths = {2, 2, 2};
  idSet = {1, 2, 4};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinV);

  src = {0, 0, 0}; dst = {0, 1, 1, 1}; widths = {2, 2, 2};
  idSet = {1, 2, 4};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinV);

  src = {1, 1}; dst = {0, 3, 2}; widths = {5, 4};
  idSet = {1, 5, 7, 8, 9, 11, 16};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinV);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {3, 8, 10, 11, 12, 14, 18};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinV);

  src = {3, 1}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {3, 5, 6, 7, 9, 13, 18};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinV);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  congStatus = {{9, 0.1}, {11, 0.6}, {17, 0.2}, {19, 0.2}};
  idSet = {1};
  intNodeTestCongestion(src, &dst, widths, weights, conc, vcSet, numVcSets,
                        numVcs, idSet.size(), idSet, congStatus,
                        HyperX::intNodeMinV);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  congStatus = {{9, 0.2}, {11, 0.2}, {17, 0.2}, {19, 0.2}};
  idSet = {1, 2};
  intNodeTestCongestion(src, &dst, widths, weights, conc, vcSet, numVcSets,
                        numVcs, idSet.size(), idSet, congStatus,
                        HyperX::intNodeMinV);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  congStatus = {{16, 0.2}, {19, 0.3}, {22, 0.2}};
  idSet = {2, 4};
  intNodeTestCongestion(src, &dst, widths, weights, conc, vcSet, numVcSets,
                        numVcs, idSet.size(), idSet, congStatus,
                        HyperX::intNodeMinV);
}

TEST(HyperXUtil, intNodeMinP) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  u32 conc, vcSet, numVcSets, numVcs;
  std::vector<u32> src;
  std::vector<u32> dst;
  std::vector<u32> widths, weights;
  std::unordered_set<u32> idSet;
  std::unordered_map<u32, f64> congStatus;

  src = {0}; dst = {0, 1}; widths = {2};
  idSet = {1};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinP);

  src = {0}; dst = {0, 2}; widths = {3};
  idSet = {1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinP);

  src = {1}; dst = {0, 2}; widths = {4};
  idSet = {0, 2, 3};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinP);

  src = {0, 0}; dst = {0, 1, 0}; widths = {2, 2};
  idSet = {1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinP);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2};
  idSet = {1, 2};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinP);

  src = {0, 0, 0}; dst = {0, 1, 0, 0}; widths = {2, 2, 2};
  idSet = {1, 2, 4};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinP);

  src = {0, 0, 0}; dst = {0, 1, 1, 1}; widths = {2, 2, 2};
  idSet = {1, 2, 4};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinP);

  src = {1, 1}; dst = {0, 3, 2}; widths = {5, 4};
  idSet = {1, 5, 7, 8, 9, 11, 16};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinP);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {3, 8, 10, 11, 12, 14, 18};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinP);

  src = {3, 1}; dst = {0, 1, 1}; widths = {5, 4};
  idSet = {3, 5, 6, 7, 9, 13, 18};
  intNodeTest(src, &dst, widths, idSet.size(), idSet, HyperX::intNodeMinP);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  congStatus = {{9, 0.1}, {11, 0.6}, {17, 0.2}, {19, 0.2}};
  idSet = {2};
  intNodeTestCongestion(src, &dst, widths, weights, conc, vcSet, numVcSets,
                        numVcs, idSet.size(), idSet, congStatus,
                        HyperX::intNodeMinP);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  congStatus = {{9, 0.1}, {11, 0.25}, {17, 0.2}, {19, 0.2}};
  idSet = {1};
  intNodeTestCongestion(src, &dst, widths, weights, conc, vcSet, numVcSets,
                        numVcs, idSet.size(), idSet, congStatus,
                        HyperX::intNodeMinP);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  congStatus = {{9, 0.1}, {11, 0.3}, {17, 0.2}, {19, 0.2}};
  idSet = {1, 2};
  intNodeTestCongestion(src, &dst, widths, weights, conc, vcSet, numVcSets,
                        numVcs, idSet.size(), idSet, congStatus,
                        HyperX::intNodeMinP);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  congStatus = {{9, 0.2}, {11, 0.2}, {17, 0.2}, {19, 0.2}};
  idSet = {1, 2};
  intNodeTestCongestion(src, &dst, widths, weights, conc, vcSet, numVcSets,
                        numVcs, idSet.size(), idSet, congStatus,
                        HyperX::intNodeMinP);
}

TEST(HyperXUtil, dimOrderVcRoutingAlgorithm) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst, widths, weights;
  u32 conc, vcSet, numVcSets, numVcs, numOutputs;
  std::unordered_set< std::tuple<u32, u32> > refOutputPorts;

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 0; numVcSets = 1; numVcs = 4;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 1u),
                    std::tuple<u32, u32>(1u, 2u), std::tuple<u32, u32>(1u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 0; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 2u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(1u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {0}; dst = {0, 1}; widths = {2}; weights = {2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(3u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {0}; dst = {0, 1}; widths = {2}; weights = {2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(3u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {0}; dst = {1, 0}; widths = {2}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  numOutputs = 0;
  refOutputPorts = {};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {1}; dst = {1, 2}; widths = {4}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(2u, 0u), std::tuple<u32, u32>(2u, 2u),
                    std::tuple<u32, u32>(3u, 0u), std::tuple<u32, u32>(3u, 2u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {2}; dst = {1, 1}; widths = {4}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(6u, 0u), std::tuple<u32, u32>(6u, 2u),
                    std::tuple<u32, u32>(7u, 0u), std::tuple<u32, u32>(7u, 2u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {0, 0}; dst = {0, 1, 0}; widths = {2, 2}; weights = {2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(3u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {0, 0}; dst = {0, 0, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(5u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(3u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(5u, 1u), std::tuple<u32, u32>(6u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};  weights = {1, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(4u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::dimOrderVcRoutingOutput, nullptr);
}

/* TEST(HyperXUtil, fullMinRoutingAlgorithm) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst, widths, weights;
  u32 conc, vcSet, numVcSets, numVcs;
  std::unordered_set< std::tuple<u32, u32> > outputPorts;
  std::unordered_set< std::tuple<u32, u32> > refOutputPorts;

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 0; numVcSets = 1; numVcs = 4;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 1u),
                    std::tuple<u32, u32>(1u, 2u), std::tuple<u32, u32>(1u, 3u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 0; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 2u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 1; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(1u, 1u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {0}; dst = {0, 1}; widths = {2}; weights = {2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(3u, 1u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {0}; dst = {0, 1}; widths = {2}; weights = {2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(3u, 1u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {0}; dst = {1, 0}; widths = {2}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  refOutputPorts = {};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {1}; dst = {1, 2}; widths = {4}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(2u, 0u), std::tuple<u32, u32>(2u, 2u),
                    std::tuple<u32, u32>(3u, 0u), std::tuple<u32, u32>(3u, 2u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {2}; dst = {1, 1}; widths = {4}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(6u, 0u), std::tuple<u32, u32>(6u, 2u),
                    std::tuple<u32, u32>(7u, 0u), std::tuple<u32, u32>(7u, 2u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {0, 0}; dst = {0, 1, 0}; widths = {2, 2}; weights = {2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(3u, 1u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {0, 0}; dst = {0, 0, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(5u, 1u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(3u, 1u),
                    std::tuple<u32, u32>(4u, 1u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(5u, 1u), std::tuple<u32, u32>(6u, 1u),
                    std::tuple<u32, u32>(7u, 1u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};  weights = {1, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  refOutputPorts = {std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(10u, 1u),
                    std::tuple<u32, u32>(11u, 1u)};
  singleRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                    refOutputPorts, std::vector<std::tuple<u32, u32> >(),
                    HyperX::fullRandMinRoutingOutput);
} */

TEST(HyperXUtil, randMinVcRoutingAlgorithm) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst, widths, weights;
  u32 conc, vcSet, numVcSets, numVcs, numOutputs;
  std::unordered_set< std::tuple<u32, u32> > refOutputPorts;

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 0; numVcSets = 1; numVcs = 4;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 1u),
                    std::tuple<u32, u32>(1u, 2u), std::tuple<u32, u32>(1u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::randMinVcRoutingOutput, nullptr);

  src = {1}; dst = {1, 2}; widths = {4}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(2u, 0u), std::tuple<u32, u32>(2u, 2u),
                    std::tuple<u32, u32>(3u, 0u), std::tuple<u32, u32>(3u, 2u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::randMinVcRoutingOutput, nullptr);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(5u, 1u), std::tuple<u32, u32>(6u, 1u),
                    std::tuple<u32, u32>(7u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::randMinVcRoutingOutput, nullptr);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};  weights = {1, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(10u, 1u),
                    std::tuple<u32, u32>(11u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::randMinVcRoutingOutput, nullptr);
}

TEST(HyperXUtil, randMinPortRoutingAlgorithm) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst, widths, weights;
  u32 conc, vcSet, numVcSets, numVcs, numOutputs;
  std::unordered_set< std::tuple<u32, u32> > refOutputPorts;

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 0; numVcSets = 1; numVcs = 4;
  numOutputs = 4;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 1u),
                    std::tuple<u32, u32>(1u, 2u), std::tuple<u32, u32>(1u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false,
                  std::unordered_map<u32, f64>(),
                  HyperX::randMinPortRoutingOutput, nullptr);

  src = {1}; dst = {1, 2}; widths = {4}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  numOutputs = 2;
  refOutputPorts = {std::tuple<u32, u32>(2u, 0u), std::tuple<u32, u32>(2u, 2u),
                    std::tuple<u32, u32>(3u, 0u), std::tuple<u32, u32>(3u, 2u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false,
                  std::unordered_map<u32, f64>(),
                  HyperX::randMinPortRoutingOutput, nullptr);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(5u, 1u), std::tuple<u32, u32>(6u, 1u),
                    std::tuple<u32, u32>(7u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false,
                  std::unordered_map<u32, f64>(),
                  HyperX::randMinPortRoutingOutput, nullptr);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};  weights = {1, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(10u, 1u),
                    std::tuple<u32, u32>(11u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false,
                  std::unordered_map<u32, f64>(),
                  HyperX::randMinPortRoutingOutput, nullptr);
}

TEST(HyperXUtil, adaptiveMinVcRoutingAlgorithm) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst, widths, weights;
  u32 conc, vcSet;
  u32 numVcSets, numVcs, numOutputs;
  std::unordered_set< std::tuple<u32, u32> > refOutputPorts;
  std::unordered_map<u32, f64> congStatus;

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 0; numVcSets = 1; numVcs = 4;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 1u),
                    std::tuple<u32, u32>(1u, 2u), std::tuple<u32, u32>(1u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::adaptiveMinVcRoutingOutput, nullptr);

  src = {1}; dst = {1, 2}; widths = {4}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(2u, 0u), std::tuple<u32, u32>(2u, 2u),
                    std::tuple<u32, u32>(3u, 0u), std::tuple<u32, u32>(3u, 2u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::adaptiveMinVcRoutingOutput, nullptr);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(5u, 1u), std::tuple<u32, u32>(6u, 1u),
                    std::tuple<u32, u32>(7u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::adaptiveMinVcRoutingOutput, nullptr);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};  weights = {1, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(10u, 1u),
                    std::tuple<u32, u32>(11u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(),
                  HyperX::adaptiveMinVcRoutingOutput, nullptr);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 1;
  congStatus = {{9, 0.1}, {11, 0.6}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true, congStatus,
                  HyperX::adaptiveMinVcRoutingOutput, nullptr);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 1;
  congStatus = {{9, 0.2}, {11, 0.2}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(2u, 3u),
                    std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(4u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true, congStatus,
                  HyperX::adaptiveMinVcRoutingOutput, nullptr);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  congStatus = {{16, 0.2}, {19, 0.3}, {22, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(5u, 1u), std::tuple<u32, u32>(7u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true, congStatus,
                  HyperX::adaptiveMinVcRoutingOutput, nullptr);
}

TEST(HyperXUtil, adaptiveMinPortRoutingAlgorithm) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst, widths, weights;
  u32 conc, vcSet, numVcSets, numVcs, numOutputs;
  std::unordered_set< std::tuple<u32, u32> > refOutputPorts;
  std::unordered_map<u32, f64> congStatus;

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 0; numVcSets = 1; numVcs = 4;
  numOutputs = 4;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 1u),
                    std::tuple<u32, u32>(1u, 2u), std::tuple<u32, u32>(1u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false,
                  std::unordered_map<u32, f64>(),
                  HyperX::adaptiveMinPortRoutingOutput, nullptr);

  src = {1}; dst = {1, 2}; widths = {4}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  numOutputs = 2;
  refOutputPorts = {std::tuple<u32, u32>(2u, 0u), std::tuple<u32, u32>(2u, 2u),
                    std::tuple<u32, u32>(3u, 0u), std::tuple<u32, u32>(3u, 2u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false,
                  std::unordered_map<u32, f64>(),
                  HyperX::adaptiveMinPortRoutingOutput, nullptr);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(5u, 1u), std::tuple<u32, u32>(6u, 1u),
                    std::tuple<u32, u32>(7u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false,
                  std::unordered_map<u32, f64>(),
                  HyperX::adaptiveMinPortRoutingOutput, nullptr);

  src = {3, 2}; dst = {0, 1, 1}; widths = {5, 4};  weights = {1, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(10u, 1u),
                    std::tuple<u32, u32>(11u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false,
                  std::unordered_map<u32, f64>(),
                  HyperX::adaptiveMinPortRoutingOutput, nullptr);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 2;
  congStatus = {{9, 0.1}, {11, 0.6}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(4u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false, congStatus,
                  HyperX::adaptiveMinPortRoutingOutput, nullptr);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 2;
  congStatus = {{9, 0.1}, {11, 0.25}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(2u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false, congStatus,
                  HyperX::adaptiveMinPortRoutingOutput, nullptr);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 2;
  congStatus = {{9, 0.1}, {11, 0.3}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(2u, 3u),
                    std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(4u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false, congStatus,
                  HyperX::adaptiveMinPortRoutingOutput, nullptr);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 2;
  congStatus = {{9, 0.2}, {11, 0.2}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(2u, 3u),
                    std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(4u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false, congStatus,
                  HyperX::adaptiveMinPortRoutingOutput, nullptr);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  congStatus = {{16, 0.2}, {19, 0.3}, {22, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(5u, 1u), std::tuple<u32, u32>(7u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false, congStatus,
                  HyperX::adaptiveMinPortRoutingOutput, nullptr);
}

TEST(HyperXUtil, lcqVcRoutingAlgorithm) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst, widths, weights;
  u32 conc, vcSet, numVcSets, numVcs, numOutputs;
  std::unordered_set< std::tuple<u32, u32> > refOutputPorts;
  std::unordered_map<u32, f64> congStatus;

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 0; numVcSets = 1; numVcs = 4;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 1u),
                    std::tuple<u32, u32>(1u, 2u), std::tuple<u32, u32>(1u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs,
                  true, std::unordered_map<u32, f64>(), nullptr,
                  HyperX::lcqVcRoutingOutput);

  src = {1}; dst = {1, 2}; widths = {4}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  refOutputPorts = {std::tuple<u32, u32>(2u, 0u), std::tuple<u32, u32>(2u, 2u),
                    std::tuple<u32, u32>(3u, 0u), std::tuple<u32, u32>(3u, 2u),
                    std::tuple<u32, u32>(4u, 0u), std::tuple<u32, u32>(4u, 2u),
                    std::tuple<u32, u32>(5u, 0u), std::tuple<u32, u32>(5u, 2u),
                    std::tuple<u32, u32>(6u, 0u), std::tuple<u32, u32>(6u, 2u),
                    std::tuple<u32, u32>(7u, 0u), std::tuple<u32, u32>(7u, 2u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true,
                  std::unordered_map<u32, f64>(), nullptr,
                  HyperX::lcqVcRoutingOutput);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 1;
  congStatus = {{9, 0.1}, {11, 0.6}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true, congStatus,
                  nullptr, HyperX::lcqVcRoutingOutput);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 1;
  congStatus = {{9, 0.2}, {11, 0.2}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(2u, 3u),
                    std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(4u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true, congStatus,
                  nullptr, HyperX::lcqVcRoutingOutput);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  congStatus = {{16, 0.2}, {19, 0.3}, {22, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(5u, 1u), std::tuple<u32, u32>(7u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, true, congStatus,
                  nullptr, HyperX::lcqVcRoutingOutput);
}

TEST(HyperXUtil, lcqPortRoutingAlgorithm) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst, widths, weights;
  u32 conc, vcSet, numVcSets, numVcs, numOutputs;
  std::unordered_set< std::tuple<u32, u32> > refOutputPorts;
  std::unordered_map<u32, f64> congStatus;

  src = {0}; dst = {0, 1}; widths = {2}; weights = {1};
  conc = 1; vcSet = 0; numVcSets = 1; numVcs = 4;
  numOutputs = 4;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 1u),
                    std::tuple<u32, u32>(1u, 2u), std::tuple<u32, u32>(1u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false,
                  std::unordered_map<u32, f64>(), nullptr,
                  HyperX::lcqPortRoutingOutput);

  src = {1}; dst = {1, 2}; widths = {4}; weights = {2};
  conc = 2; vcSet = 0; numVcSets = 2; numVcs = 3;
  numOutputs = 2;
  refOutputPorts = {std::tuple<u32, u32>(2u, 0u), std::tuple<u32, u32>(2u, 2u),
                    std::tuple<u32, u32>(3u, 0u), std::tuple<u32, u32>(3u, 2u),
                    std::tuple<u32, u32>(4u, 0u), std::tuple<u32, u32>(4u, 2u),
                    std::tuple<u32, u32>(5u, 0u), std::tuple<u32, u32>(5u, 2u),
                    std::tuple<u32, u32>(6u, 0u), std::tuple<u32, u32>(6u, 2u),
                    std::tuple<u32, u32>(7u, 0u), std::tuple<u32, u32>(7u, 2u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false,
                  std::unordered_map<u32, f64>(), nullptr,
                  HyperX::lcqPortRoutingOutput);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 2;
  congStatus = {{9, 0.1}, {11, 0.6}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(4u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false, congStatus,
                  nullptr, HyperX::lcqPortRoutingOutput);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 2;
  congStatus = {{9, 0.1}, {11, 0.25}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(2u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false, congStatus,
                  nullptr, HyperX::lcqPortRoutingOutput);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 2;
  congStatus = {{9, 0.1}, {11, 0.3}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(2u, 3u),
                    std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(4u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false, congStatus,
                  nullptr, HyperX::lcqPortRoutingOutput);

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {2, 2};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 4;
  numOutputs = 2;
  congStatus = {{9, 0.2}, {11, 0.2}, {17, 0.2}, {19, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(2u, 1u), std::tuple<u32, u32>(2u, 3u),
                    std::tuple<u32, u32>(4u, 1u), std::tuple<u32, u32>(4u, 3u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false, congStatus,
                  nullptr, HyperX::lcqPortRoutingOutput);

  src = {0, 0, 0}; dst = {0, 0, 1, 1}; widths = {2, 2, 2}; weights = {3, 2, 1};
  conc = 2; vcSet = 1; numVcSets = 2; numVcs = 3;
  numOutputs = 1;
  congStatus = {{16, 0.2}, {19, 0.3}, {22, 0.2}};
  refOutputPorts = {std::tuple<u32, u32>(5u, 1u), std::tuple<u32, u32>(7u, 1u)};
  distRoutingTest(src, &dst, widths, weights, conc, vcSet, numVcSets, numVcs,
                  false, 1, refOutputPorts, numOutputs, false, congStatus,
                  nullptr, HyperX::lcqPortRoutingOutput);
}

TEST(HyperXUtil, valiantsRoutingAlgorithm) {
  TestSetup ts(1, 1, 1, 0xBAADF00D);
  std::vector<u32> src, dst, widths, weights;
  u32 conc, vcSet, numVcSets, numVcs;
  std::unordered_set< std::tuple<u32, u32, f64> > outputPorts;
  std::unordered_set< std::tuple<u32, u32> > refOutputPorts;
  std::unordered_map<u32, f64> congStatus;

  Message* m = new Message(1, nullptr);
  Packet* p = new Packet(0, 1, m);
  m->setPacket(0, p);
  Flit* f = new Flit(0, true, false, p);
  p->setFlit(0, f);
  HyperX::IntNodeAlg intNodeAlg = HyperX::IntNodeAlg::DST;
  HyperX::BaseRoutingAlg routingAlg = HyperX::BaseRoutingAlg::DORP;

  src = {0, 0}; dst = {0, 1, 1}; widths = {2, 2}; weights = {1, 1};
  conc = 1; vcSet = 1; numVcSets = 2; numVcs = 4;
  refOutputPorts = {std::tuple<u32, u32>(1u, 0u), std::tuple<u32, u32>(1u, 1u),
                    std::tuple<u32, u32>(1u, 2u), std::tuple<u32, u32>(1u, 3u)};

  TestRouter* router;
  u32 numPorts;
  numPorts = conc;
  std::vector<u32>* false_src;
  for (u32 dim = 0; dim < widths.size(); dim++) {
    numPorts += widths.at(dim) * weights.at(dim);
  }
  router = new TestRouter(src, numPorts, numVcs, congStatus);

  HyperX::valiantsRoutingOutput(router, 0, 0, widths, weights, conc, &dst,
                                vcSet, numVcSets, numVcs, false,
                                intNodeAlg, routingAlg, f, &outputPorts);
  ASSERT_NE(p->getRoutingExtension(), nullptr);

  delete reinterpret_cast<const std::vector<u32>*> (p->getRoutingExtension());
  p->setRoutingExtension(nullptr);
  HyperX::valiantsRoutingOutput(router, 0, 0, widths, weights, conc, &dst,
                                vcSet, numVcSets, numVcs, true,
                                intNodeAlg, routingAlg, f, &outputPorts);
  ASSERT_NE(p->getRoutingExtension(), nullptr);

  dst = {0, 0, 0};
  HyperX::valiantsRoutingOutput(router, 0, 0, widths, weights, conc, &dst,
                                vcSet, numVcSets, numVcs, true,
                                intNodeAlg, routingAlg, f, &outputPorts);
  ASSERT_EQ(p->getRoutingExtension(), nullptr);
  for (auto& it : outputPorts) {
    ASSERT_EQ(std::get<0>(it), 0u);
  }

  false_src = new std::vector<u32>({U32_MAX, 0, 0});
  dst = {0, 1, 1};
  p->incrementHopCount();
  p->setRoutingExtension(false_src);
  HyperX::valiantsRoutingOutput(router, 0, 0, widths, weights, conc, &dst,
                                vcSet, numVcSets, numVcs, false,
                                intNodeAlg, routingAlg, f, &outputPorts);
  ASSERT_EQ(p->getRoutingExtension(), nullptr);

  false_src = new std::vector<u32>({U32_MAX, 0, 0});
  p->setRoutingExtension(false_src);
  HyperX::valiantsRoutingOutput(router, 0, 0, widths, weights, conc, &dst,
                                vcSet, numVcSets, numVcs, true,
                                intNodeAlg, routingAlg, f, &outputPorts);
  ASSERT_EQ(p->getRoutingExtension(), nullptr);

  false_src = new std::vector<u32>({U32_MAX, 0, 0});
  p->setRoutingExtension(false_src);
  dst = {0, 0, 0};
  HyperX::valiantsRoutingOutput(router, 0, 0, widths, weights, conc, &dst,
                                vcSet, numVcSets, numVcs, true,
                                intNodeAlg, routingAlg, f, &outputPorts);
  ASSERT_EQ(p->getRoutingExtension(), nullptr);
  for (auto& it : outputPorts) {
    ASSERT_EQ(std::get<0>(it), 0u);
  }

  // Test with p->getRoutingExtension() == nullptr after stage 0 to 1 transition
  dst = {0, 1, 1};
  HyperX::valiantsRoutingOutput(router, 0, 0, widths, weights, conc, &dst,
                                vcSet, numVcSets, numVcs, false,
                                intNodeAlg, routingAlg, f, &outputPorts);
  ASSERT_EQ(p->getRoutingExtension(), nullptr);

  HyperX::valiantsRoutingOutput(router, 0, 0, widths, weights, conc, &dst,
                                vcSet, numVcSets, numVcs, true,
                                intNodeAlg, routingAlg, f, &outputPorts);
  ASSERT_EQ(p->getRoutingExtension(), nullptr);

  dst = {0, 0, 0};
  HyperX::valiantsRoutingOutput(router, 0, 0, widths, weights, conc, &dst,
                                vcSet, numVcSets, numVcs, true,
                                intNodeAlg, routingAlg, f, &outputPorts);
  ASSERT_EQ(p->getRoutingExtension(), nullptr);
  for (auto& it : outputPorts) {
    ASSERT_EQ(std::get<0>(it), 0u);
  }

  delete router;
  delete m;
  // delete f;
  // delete p;
}
