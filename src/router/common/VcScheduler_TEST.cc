/*
 * Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
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
#include <utility>

#include "event/Component.h"
#include "router/common/VcScheduler.h"

#include "test/TestSetup_TEST.h"

class VcSchedulerTestClient : public VcScheduler::Client, public Component {
 public:
  static const s32 RequestVcsEvent = 123;
  static const s32 ReleaseVcEvent = 456;
  enum class Fsm : u8 { REQUESTING = 1, WAITING = 2, USING = 3, DONE = 4 };

  VcSchedulerTestClient(u32 _id, VcScheduler* _vcSch, u32 _numVcs, u32 _allocs)
      : Component("TestClient_" + std::to_string(_id), nullptr),
        id_(_id), vcSch_(_vcSch), numVcs_(_numVcs), fsm_(Fsm::REQUESTING),
        grantedVc_(U32_MAX), remaining_(_allocs) {
    vcSch_->setClient(id_, this);
    addEvent(gSim->time(), 1, nullptr, RequestVcsEvent);
  }

  ~VcSchedulerTestClient() {
    assert(remaining_ == 0);
    assert(fsm_ == Fsm::DONE);
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
      // new request
      u32 v = gSim->rnd.nextU64(0, numVcs_ - 1);
      u64 m = gSim->rnd.nextU64(1000, 2000);
      assert(requests_.count(v) == 0);
      requests_.insert({v, m});
      vcSch_->request(id_, v, m);
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
      requests_.clear();
      grantedVc_ = _vc;

      // release later
      u64 releaseTime = gSim->futureCycle(gSim->rnd.nextU64(1, 1));
      addEvent(releaseTime, 1, nullptr, ReleaseVcEvent);
      fsm_ = Fsm::USING;
    } else {
      // re-request the same from before
      addEvent(gSim->time(), gSim->epsilon() + 1, nullptr, RequestVcsEvent);
      fsm_ = Fsm::REQUESTING;
    }
  }

  void releaseVc() {
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
  u32 numVcs_;
  Fsm fsm_;
  std::unordered_map<u32, u64> requests_;
  u32 grantedVc_;
  u32 remaining_;
};

TEST(VcScheduler, basic) {
  const u32 ALLOCS_PER_CLIENT = 100;

  for (u32 C = 1; C < 16; C++) {
    for (u32 V = 1; V < 16; V++) {
      // setup
      TestSetup testSetup(12, 0x1234567890abcdf);
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
      VcScheduler* vcSch = new VcScheduler("VcSch", nullptr, C, V, schSettings);
      assert(vcSch->numClients() == C);
      assert(vcSch->numVcs() == V);

      std::vector<VcSchedulerTestClient*> clients(C);
      for (u32 c = 0; c < C; c++) {
        clients[c] = new VcSchedulerTestClient(c, vcSch, V, ALLOCS_PER_CLIENT);
      }

      // run the simulator
      gSim->simulate();

      // tear down
      delete vcSch;
      for (u32 c = 0; c < C; c++) {
        delete clients[c];
      }
    }
  }
}
