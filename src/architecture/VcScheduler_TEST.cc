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
#include <json/json.h>
#include <gtest/gtest.h>
#include <prim/prim.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "event/Component.h"
#include "architecture/VcScheduler.h"

#include "test/TestSetup_TEST.h"

class VcSchedulerTestClient : public VcScheduler::Client, public Component {
 public:
  static const s32 RequestVcsEvent = 123;
  static const s32 ReleaseVcEvent = 456;
  enum class Fsm : u8 { REQUESTING = 1, WAITING = 2, USING = 3, DONE = 4 };

  VcSchedulerTestClient(u32 _id, VcScheduler* _vcSch, u32 _totalVcs,
                        Simulator::Clock _clock, u32 _allocs, u32 _numRequests,
                        const std::unordered_set<u32>& _fixedRequests,
                        std::unordered_map<u32, u32>* _holdingCount)
      : Component("TestClient_" + std::to_string(_id), nullptr),
        id_(_id), vcSch_(_vcSch), totalVcs_(_totalVcs), clock_(_clock),
        fsm_(Fsm::REQUESTING), numRequests_(_numRequests),
        fixedRequests_(_fixedRequests), grantedVc_(U32_MAX),
        remaining_(_allocs), totalGrants_(0), holdingCount_(_holdingCount) {
    vcSch_->setClient(id_, this);
    addEvent(gSim->time(), 1, nullptr, RequestVcsEvent);
  }

  ~VcSchedulerTestClient() {
    assert(remaining_ == 0);
    assert(fsm_ == Fsm::DONE);
  }

  void checkDist(bool _failFast, bool _debug) {
    const f64 tolerance = 0.015;
    assert(fixedRequests_.size() > 0);
    if (_debug) {
      printf("total=%u\n", totalGrants_);
    }
    for (u32 v : fixedRequests_) {
      if (_debug) {
        printf("%u => %u\n", v, grantCount_[v]);
      }
      f64 percent = (f64)grantCount_[v] / totalGrants_;
      f64 expected = (f64)1.0 / fixedRequests_.size();
      ASSERT_NEAR(percent, expected, tolerance);
      if (_failFast) {
        assert(fabs(percent - expected) <= tolerance);
      }
    }
  }
  void processEvent(void* _event, s32 _type) {
    switch (_type) {
      case RequestVcsEvent:
        requestVcs();
        break;
      case ReleaseVcEvent:
        releaseVc();
        break;
      default:
        assert(false);
    }
  }

  void requestVcs() {
    assert(fsm_ == Fsm::REQUESTING);

    if (requests_.size() == 0) {
      if (fixedRequests_.size() > 0) {
        // fixed requests
        for (u32 v : fixedRequests_) {
          u64 m = 1000;
          bool res = requests_.insert({v, m}).second;
          (void)res;
          assert(res);
          vcSch_->request(id_, v, m);
        }
      } else {
        // new requests
        for (u32 idx = 0; idx < numRequests_; idx++) {
          // choose random VC
          u32 v = U32_MAX;
          do {
            v = gSim->rnd.nextU64(0, totalVcs_ - 1);
          } while (requests_.count(v) > 0);

          // submit request
          u64 m = gSim->rnd.nextU64(1000, 2000);
          bool res = requests_.insert({v, m}).second;
          (void)res;
          assert(res);
          vcSch_->request(id_, v, m);
        }
      }
    } else {
      // re-request of original requests
      for (auto it = requests_.cbegin(); it != requests_.cend(); ++it) {
        u32 vc = it->first;
        u64 metadata = it->second;
        vcSch_->request(id_, vc, metadata);
      }
    }

    fsm_ = Fsm::WAITING;
  }

  void vcSchedulerResponse(u32 _vc) override {
    assert(grantedVc_ == U32_MAX);
    assert(requests_.size() > 0);

    if (_vc != U32_MAX) {
      assert(requests_.count(_vc) == 1u);
      grantCount_[_vc]++;
      totalGrants_++;
      requests_.clear();
      grantedVc_ = _vc;
      assert((*holdingCount_)[_vc] == 0);
      (*holdingCount_)[_vc]++;

      // release later
      u64 releaseTime = gSim->futureCycle(clock_, gSim->rnd.nextU64(1, 5));
      addEvent(releaseTime, 1, nullptr, ReleaseVcEvent);
      fsm_ = Fsm::USING;
    } else {
      // re-request the same from before
      addEvent(gSim->time(), gSim->epsilon() + 1, nullptr, RequestVcsEvent);
      fsm_ = Fsm::REQUESTING;
    }
  }

  void releaseVc() {
    assert((*holdingCount_)[grantedVc_] == 1);
    (*holdingCount_)[grantedVc_]--;
    vcSch_->releaseVc(grantedVc_);
    grantedVc_ = U32_MAX;
    remaining_--;
    if (remaining_ > 0) {
      fsm_ = Fsm::REQUESTING;
      requestVcs();
    } else {
      fsm_ = Fsm::DONE;
    }
  }

 private:
  u32 id_;
  VcScheduler* vcSch_;
  u32 totalVcs_;
  Simulator::Clock clock_;
  Fsm fsm_;
  u32 numRequests_;
  const std::unordered_set<u32>& fixedRequests_;
  std::unordered_map<u32, u64> requests_;
  u32 grantedVc_;
  u32 remaining_;
  std::unordered_map<u32, u32> grantCount_;
  u32 totalGrants_;
  std::unordered_map<u32, u32>* holdingCount_;
};

TEST(VcScheduler, basic) {
  const u32 ALLOCS_PER_CLIENT = 100;

  for (u32 C = 1; C < 16; C += 2) {
    for (u32 V = 1; V < 16; V += 2) {
      for (u32 R = 1; R < V; R += 2) {
        // setup
        TestSetup testSetup(12, 12, 12, 0x1234567890abcdf);

        std::unordered_set<u32> requests;

        Json::Value arbSettings;
        arbSettings["type"] = "random";
        Json::Value allocSettings;
        allocSettings["type"] = "rc_separable";
        allocSettings["resource_arbiter"] = arbSettings;
        allocSettings["client_arbiter"] = arbSettings;
        allocSettings["iterations"] = 1;
        allocSettings["slip_latch"] = true;
        Json::Value schSettings;
        schSettings["allocator"] = allocSettings;
        VcScheduler* vcSch = new VcScheduler(
            "VcSch", nullptr, C, V, Simulator::Clock::ROUTER, schSettings);
        assert(vcSch->numClients() == C);
        assert(vcSch->totalVcs() == V);

        std::vector<VcSchedulerTestClient*> clients(C);
        std::unordered_map<u32, u32> holdingCount;
        for (u32 c = 0; c < C; c++) {
          clients[c] = new VcSchedulerTestClient(
              c, vcSch, V, Simulator::Clock::ROUTER, ALLOCS_PER_CLIENT, R,
              requests, &holdingCount);
        }

        // run the simulator
        gSim->initialize();
        gSim->simulate();

        // tear down
        delete vcSch;
        for (u32 c = 0; c < C; c++) {
          delete clients[c];
        }
      }
    }
  }
}

TEST(VcScheduler, dist) {
  /*
   * This is a long but very useful test.
   */
  const u32 C = 8;
  const u32 V = 16;
  const u32 R = 0;
  const u32 ALLOCS_PER_CLIENT = 20000;
  const bool DEBUG = false;

  for (const std::string& arb : {"random", "comparing", "lslp"}) {
    for (u32 N : {1, 4, 12}) {
      TestSetup testSetup(12, 12, 12, 0x1234567890abcdf * N);

      std::unordered_set<u32> requests;

      for (u32 n = 0; n < N; n++) {
        u32 v = U32_MAX;
        do {
          v = gSim->rnd.nextU64(0, V - 1);
        } while (requests.count(v) > 0);
        bool res = requests.insert(v).second;
        (void)res;
        assert(res);
      }
      assert(requests.size() == N);

      // setup
      Json::Value arbSettings;
      arbSettings["type"] = arb;
      arbSettings["greater"] = false;
      Json::Value allocSettings;
      allocSettings["type"] = "rc_separable";
      allocSettings["resource_arbiter"] = arbSettings;
      allocSettings["client_arbiter"] = arbSettings;
      allocSettings["iterations"] = 1;
      allocSettings["slip_latch"] = false;
      Json::Value schSettings;
      schSettings["allocator"] = allocSettings;
      VcScheduler* vcSch = new VcScheduler(
          "VcSch", nullptr, C, V, Simulator::Clock::ROUTER, schSettings);
      assert(vcSch->numClients() == C);
      assert(vcSch->totalVcs() == V);

      std::vector<VcSchedulerTestClient*> clients(C);
      std::unordered_map<u32, u32> holdingCount;
      for (u32 c = 0; c < C; c++) {
        clients[c] = new VcSchedulerTestClient(
            c, vcSch, V, Simulator::Clock::ROUTER, ALLOCS_PER_CLIENT, R,
            requests, &holdingCount);
      }

      // run the simulator
      gSim->initialize();
      gSim->simulate();

      // check
      for (u32 c = 0; c < C; c++) {
        clients[c]->checkDist(true, DEBUG);
      }

      // tear down
      delete vcSch;
      for (u32 c = 0; c < C; c++) {
        delete clients[c];
      }
    }
  }
}
