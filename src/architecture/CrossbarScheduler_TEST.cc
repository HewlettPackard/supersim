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

#include <algorithm>
#include <string>
#include <tuple>

#include "architecture/CrossbarScheduler.h"
#include "event/Component.h"
#include "types/Flit.h"
#include "types/Packet.h"

#include "test/TestSetup_TEST.h"

class CrossbarSchedulerTestClient : public CrossbarScheduler::Client,
                                    public Component {
 public:
  static const s32 RequestPortEvent = 123;
  static const s32 GiveBackCreditEvent = 456;
  enum class Fsm : u8 { REQUESTING = 1, WAITING = 2, DONE = 3 };

  CrossbarSchedulerTestClient(
      u32 _id, CrossbarScheduler* _xbarSch, u32 _totalVcs, u32 _crossbarPorts,
      Simulator::Clock _clock, u32 _allocs)
      : Component("TestClient_" + std::to_string(_id), nullptr),
        id_(_id), xbarSch_(_xbarSch), totalVcs_(_totalVcs),
        crossbarPorts_(_crossbarPorts), clock_(_clock), fsm_(Fsm::REQUESTING),
        request_(std::make_tuple(U32_MAX, U32_MAX, nullptr)),
        remaining_(_allocs) {
    debug_ = !true;
    xbarSch_->setClient(id_, this);
    addEvent(gSim->time(), 1, nullptr, RequestPortEvent);
  }

  ~CrossbarSchedulerTestClient() {
    assert(remaining_ == 0);
    assert(fsm_ == Fsm::DONE);
  }

  void processEvent(void* _event, s32 _type) {
    u32* vcPtr;
    switch (_type) {
      case RequestPortEvent:
        requestPort();
        break;
      case GiveBackCreditEvent:
        vcPtr = reinterpret_cast<u32*>(_event);
        assert(*vcPtr < totalVcs_);
        xbarSch_->incrementCredit(*vcPtr);
        delete vcPtr;
        break;
      default:
        assert(false);
    }
  }

  void requestPort() {
    assert(fsm_ == Fsm::REQUESTING);

    if (std::get<0>(request_) == U32_MAX) {
      // new request
      u32 vcIdx = gSim->rnd.nextU64(0, totalVcs_ - 1);
      u32 port = vcIdx / (totalVcs_ / crossbarPorts_);
      u64 metadata = gSim->rnd.nextU64(1000, 2000 - 1);

      // we have to make a packet and flit to have some metadata
      Packet* packet = new Packet(0, 1, nullptr);
      Flit* flit = new Flit(0, true, true, packet);
      packet->setFlit(0, flit);
      packet->setMetadata(metadata);

      // make the request
      dbgprintf("requesting %u %u %lu", port, vcIdx, metadata);
      request_ = std::make_tuple(port, vcIdx, flit);
      xbarSch_->request(id_, port, vcIdx, flit);
    } else {
      // re-request
      u32 port = std::get<0>(request_);
      u32 vcIdx = std::get<1>(request_);
      Flit* flit = std::get<2>(request_);
      xbarSch_->request(id_, port, vcIdx, flit);
    }

    fsm_ = Fsm::WAITING;
  }

  void crossbarSchedulerResponse(u32 _port, u32 _vcIdx) override {
    assert(remaining_ > 0);
    assert(std::get<0>(request_) != U32_MAX);

    if (_port != U32_MAX) {
      u32 port = std::get<0>(request_);
      u32 vcIdx = std::get<1>(request_);
      Flit* flit = std::get<2>(request_);
      Packet* packet = flit->packet();
      delete packet;
      std::get<0>(request_) = U32_MAX;

      assert(port == _port);
      assert(vcIdx == _vcIdx);

      // use credit
      dbgprintf("using %u (via port %u)", vcIdx, port);
      xbarSch_->decrementCredit(vcIdx);
      remaining_--;

      // release later
      u64 releaseTime = gSim->futureCycle(clock_, gSim->rnd.nextU64(2, 6));
      u32* vcPtr = new u32;
      *vcPtr = vcIdx;
      addEvent(releaseTime, 1, vcPtr, GiveBackCreditEvent);
    }

    if (remaining_ > 0) {
      addEvent(gSim->time(), gSim->epsilon() + 1, nullptr, RequestPortEvent);
      fsm_ = Fsm::REQUESTING;
    } else {
      fsm_ = Fsm::DONE;
    }
  }

 private:
  u32 id_;
  CrossbarScheduler* xbarSch_;
  u32 totalVcs_;
  u32 crossbarPorts_;
  Simulator::Clock clock_;
  Fsm fsm_;
  std::tuple<u32, u32, Flit*> request_;
  u32 remaining_;
};

TEST(CrossbarScheduler, basic) {
  const u32 ALLOCS_PER_CLIENT = 100;

  std::vector<std::tuple<bool, bool, bool>> styles = {
    // packet-buffer flow control
    std::make_tuple(true, true, false),
    // flit-buffer flow control
    std::make_tuple(false, false, false),
    // flit-buffer flow control with winner take all
    std::make_tuple(false, true, true),
    // packet-buffer flow control with packet interleaving
    std::make_tuple(true, false, false)
  };

  for (auto style : styles) {
    for (u32 C = 1; C < 16; C++) {
      for (u32 P = 1; P < 16; P++) {
        for (u32 Vd = 1; Vd < 4; Vd++) {
          u32 V = P * Vd;

          // setup
          TestSetup testSetup(12, 12, 0x1234567890abcdf);
          Json::Value arbSettings;
          arbSettings["type"] = "random";
          Json::Value allocSettings;
          allocSettings["type"] = "r_separable";
          allocSettings["resource_arbiter"] = arbSettings;
          allocSettings["slip_latch"] = true;
          Json::Value schSettings;
          schSettings["allocator"] = allocSettings;
          schSettings["full_packet"] = std::get<0>(style);
          schSettings["packet_lock"] = std::get<1>(style);
          schSettings["idle_unlock"] = std::get<2>(style);
          CrossbarScheduler* xbarSch =
              new CrossbarScheduler(
                  "XbarSch", nullptr, C, V, P, 0, Simulator::Clock::CORE,
                  schSettings);
          assert(xbarSch->numClients() == C);
          assert(xbarSch->totalVcs() == V);
          assert(xbarSch->crossbarPorts() == P);
          for (u32 v = 0; v < V; v++) {
            xbarSch->initCredits(v, 3);
          }

          std::vector<CrossbarSchedulerTestClient*> clients(C);
          for (u32 c = 0; c < C; c++) {
            clients[c] = new CrossbarSchedulerTestClient(
                c, xbarSch, V, P, Simulator::Clock::CORE, ALLOCS_PER_CLIENT);
          }

          // run the simulator
          gSim->simulate();

          // tear down
          delete xbarSch;
          for (u32 c = 0; c < C; c++) {
            delete clients[c];
          }
        }
      }
    }
  }
}
