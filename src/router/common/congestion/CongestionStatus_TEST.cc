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
#include "router/common/congestion/CongestionStatus_TEST.h"

#include <gtest/gtest.h>

#include <queue>
#include <tuple>

#include "event/Component.h"
#include "test/TestSetup_TEST.h"

/*********************** CongestionTestRouter class ***************************/

CongestionTestRouter::CongestionTestRouter(
    const std::string& _name, const Component* _parent,
    const std::vector<u32>& _address, MetadataHandler* _metadataHandler,
    Json::Value _settings)
    : Router(_name, _parent, _address, _metadataHandler, _settings),
      congestionStatus_(nullptr) {
  outputChannels_.resize(numPorts_);
}

CongestionTestRouter::~CongestionTestRouter() {}

void CongestionTestRouter::setCongestionStatus(
    CongestionStatus* _congestionStatus) {
  congestionStatus_ = _congestionStatus;
}

void CongestionTestRouter::setInputChannel(u32 _port, Channel* _channel) {
  assert(false);
}

Channel* CongestionTestRouter::getInputChannel(u32 _port) {
  assert(false);
}

void CongestionTestRouter::setOutputChannel(u32 _port, Channel* _channel) {
  outputChannels_.at(_port) = _channel;
}

Channel* CongestionTestRouter::getOutputChannel(u32 _port) {
  return outputChannels_.at(_port);
}

void CongestionTestRouter::receiveFlit(u32 _port, Flit* _flit) {
  assert(false);
}

void CongestionTestRouter::receiveCredit(u32 _port, Credit* _credit) {
  assert(false);
}

void CongestionTestRouter::sendCredit(u32 _port, u32 _vc) {
  assert(false);
}

void CongestionTestRouter::sendFlit(u32 _port, Flit* _flit) {
  assert(false);
}

f64 CongestionTestRouter::congestionStatus(u32 _port, u32 _vc) const {
  return congestionStatus_->status(_port, _vc);
}

/************************* CongestionStatusTest class ************************/

class CongestionStatusTest : public CongestionStatus {
 public:
  CongestionStatusTest(
      const std::string& _name, const Component* _parent, Router* _router,
      Json::Value _settings)
      : CongestionStatus(_name, _parent, _router, _settings), ok_(true) {
    maxCredits_.resize(numPorts_ * numVcs_, 0);
    currCredits_.resize(numPorts_ * numVcs_, 0);
  }

  ~CongestionStatusTest() {}

  void addExpectedEvent(u32 _port, u32 _vc, u64 _time, u8 _epsilon, s32 _type) {
    events_.push(std::make_tuple(_port, _vc, _time, _epsilon, _type));
  }

  void performInitCredits(u32 _port, u32 _vc, u32 _credits) override {
    assert(_credits > 0);
    u32 vcIdx = router_->vcIndex(_port, _vc);
    maxCredits_.at(vcIdx) += _credits;
    currCredits_.at(vcIdx) += _credits;
  }

  void performIncrementCredit(u32 _port, u32 _vc) override {
    dbgprintf("port=%u vc=%u", _port, _vc);

    u32 vcIdx = router_->vcIndex(_port, _vc);
    assert(currCredits_.at(vcIdx) < maxCredits_.at(vcIdx));
    currCredits_.at(vcIdx)++;

    if ((events_.size() < 1) ||
        (std::get<0>(events_.front()) != _port) ||
        (std::get<1>(events_.front()) != _vc) ||
        (std::get<2>(events_.front()) != gSim->time()) ||
        (std::get<3>(events_.front()) != gSim->epsilon()) ||
        (std::get<4>(events_.front()) != CongestionStatus::INCR)) {
      ok_ = false;
      dbgprintf("expected=(%u,%u,%lu,%u,%i) got=(%u,%u,%lu,%u,%i)",
                _port, _vc, gSim->time(), gSim->epsilon(),
                CongestionStatus::INCR, std::get<0>(events_.front()),
                std::get<1>(events_.front()), std::get<2>(events_.front()),
                std::get<3>(events_.front()), std::get<4>(events_.front()));
    }
    if (ok_) {
      events_.pop();
    }
  }

  void performDecrementCredit(u32 _port, u32 _vc) override {
    dbgprintf("port=%u vc=%u", _port, _vc);

    u32 vcIdx = router_->vcIndex(_port, _vc);
    assert(currCredits_.at(vcIdx) > 0);
    currCredits_.at(vcIdx)--;

    if ((events_.size() < 1) ||
        (std::get<0>(events_.front()) != _port) ||
        (std::get<1>(events_.front()) != _vc) ||
        (std::get<2>(events_.front()) != gSim->time()) ||
        (std::get<3>(events_.front()) != gSim->epsilon()) ||
        (std::get<4>(events_.front()) != CongestionStatus::DECR)) {
      ok_ = false;
      dbgprintf("expected=(%u,%u,%lu,%u,%i) got=(%u,%u,%lu,%u,%i)",
                _port, _vc, gSim->time(), gSim->epsilon(),
                CongestionStatus::DECR, std::get<0>(events_.front()),
                std::get<1>(events_.front()), std::get<2>(events_.front()),
                std::get<3>(events_.front()), std::get<4>(events_.front()));
    }
    if (ok_) {
      events_.pop();
    }
  }

  f64 computeStatus(u32 _port, u32 _vc) const override {
    u32 vcIdx = router_->vcIndex(_port, _vc);
    f64 value = ((f64)maxCredits_.at(vcIdx) - (f64)currCredits_.at(vcIdx)) /
        (f64)maxCredits_.at(vcIdx);
    dbgprintf("%f/%f=%f", (f64)currCredits_.at(vcIdx),
              (f64)maxCredits_.at(vcIdx), value);
    return value;
  }

  bool ok() const {
    return ok_;
  }

  bool done() const {
    return events_.size() == 0;
  }

 private:
  std::queue<std::tuple<u32, u32, u64, u8, s32> > events_;
  std::vector<u32> maxCredits_;
  std::vector<u32> currCredits_;
  bool ok_;
};

/************************* CreditHandler utility class ************************/

CreditHandler::CreditHandler(
    const std::string& _name, const Component* _parent,
    CongestionStatus* _congestionStatus, Router* _router)
    : Component(_name, _parent), congestionStatus_(_congestionStatus),
      router_(_router) {}

CreditHandler::~CreditHandler() {}

void CreditHandler::setEvent(u32 _port, u32 _vc, u64 _time, u8 _epsilon,
                             s32 _type) {
  u32 vcIdx = router_->vcIndex(_port, _vc);
  addEvent(_time, _epsilon, reinterpret_cast<void*>(vcIdx), _type);
}

void CreditHandler::processEvent(void* _event, s32 _type) {
  u32 vcIdx = static_cast<u32>(reinterpret_cast<u64>(_event));
  u32 port, vc;
  router_->vcIndexInv(vcIdx, &port, &vc);
  switch (_type) {
    case CongestionStatus::INCR:
      dbgprintf("incrementing port=%u vc=%u", port, vc);
      congestionStatus_->incrementCredit(vcIdx);
      break;

    case CongestionStatus::DECR:
      dbgprintf("decrementing port=%u vc=%u", port, vc);
      congestionStatus_->decrementCredit(vcIdx);
      break;

    default:
      assert(false);
  }
}

/************************* StatusCheck utility class **************************/

StatusCheck::StatusCheck(const std::string& _name, const Component* _parent,
                         CongestionStatus* _congestionStatus)
    : Component(_name, _parent), congestionStatus_(_congestionStatus) {}

StatusCheck::~StatusCheck() {}

void StatusCheck::setEvent(u64 _time, u8 _epsilon, u32 _port, u32 _vc,
                           f64 _expected) {
  StatusCheck::Event* evt = new StatusCheck::Event({_port, _vc, _expected});
  addEvent(_time, _epsilon, evt, 0);
}

void StatusCheck::processEvent(void* _event, s32 _type) {
  Event* evt = reinterpret_cast<Event*>(_event);
  f64 sts = congestionStatus_->status(evt->port, evt->vc);
  ASSERT_DOUBLE_EQ(sts, evt->exp);
  delete evt;
}

/************************* Basic Base Class Tests *****************************/

f64 expSts(u32 _curr, u32 _max, u32 _gran) {
  f64 value = ((f64)_max - (f64)_curr) / (f64)_max;
  if (_gran > 0) {
    value = round(value * _gran) / _gran;
  }
  return value;
}

TEST(CongestionStatus, latencyAndGranularity) {
  const bool debug = false;

  const u32 numPorts = 5;
  const u32 numVcs = 4;

  for (u32 latency = 1; latency <= 64; latency *= 2) {
    for (u32 granularity = 0; granularity <= 16; granularity += 2) {
      // printf("latency=%u granularity=%u\n", latency, granularity);

      TestSetup setup(1, 1, 1234);

      Json::Value routerSettings;
      routerSettings["num_ports"] = numPorts;
      routerSettings["num_vcs"] = numVcs;
      CongestionTestRouter router("Router", nullptr, std::vector<u32>({}),
                                  nullptr, routerSettings);
      router.setDebug(debug);

      Json::Value statusSettings;
      statusSettings["latency"] = latency;
      statusSettings["granularity"] = granularity;
      CongestionStatusTest status("CongestionStatus", &router, &router,
                                  statusSettings);
      status.setDebug(debug);

      router.setCongestionStatus(&status);

      std::vector<u32> maxCredits(numPorts * numVcs);
      std::vector<u32> currCredits(numPorts * numVcs);
      for (u32 port = 0; port < numPorts; port++) {
        for (u32 vc = 0; vc < numVcs; vc++) {
          u32 max = port * 100 + vc + 1;
          maxCredits.at(router.vcIndex(port, vc)) = max;
          currCredits.at(router.vcIndex(port, vc)) = max;
          status.initCredits(router.vcIndex(port, vc), max);
        }
      }

      CreditHandler crediter("CreditHandler", nullptr, &status, &router);
      crediter.setDebug(debug);

      StatusCheck check("StatusCheck", nullptr, &status);
      check.setDebug(debug);

      // set status check events for all VCs at time 0
      //  they should all be empty (status = 0.0)
      for (u32 port = 0; port < numPorts; port++) {
        for (u32 vc = 0; vc < numVcs; vc++) {
          check.setEvent(0, 0, port, vc, 0.0);
        }
      }

      for (u64 time = 1000; time < 2000; time++) {
        // choose a random port-VC pair to target and INCR or DECR
        u32 port = gSim->rnd.nextU64(0, numPorts - 1);
        u32 vc = gSim->rnd.nextU64(0, numVcs - 1);
        bool incr = gSim->rnd.nextBool();
        u32 vcIdx = router.vcIndex(port, vc);
        if (incr && currCredits.at(vcIdx) == maxCredits.at(vcIdx)) {
          incr = false;
        } else if (!incr && currCredits.at(vcIdx) == 0) {
          incr = true;
        }

        // set event for INCR or DECR
        s32 type = incr ? CongestionStatus::INCR : CongestionStatus::DECR;
        crediter.setEvent(port, vc, time, 1, type);
        status.addExpectedEvent(port, vc, time + latency - 1, 2, type);

        // update expected
        if (incr) {
          currCredits.at(vcIdx)++;
        } else {
          currCredits.at(vcIdx)--;
        }

        // set event to check status value
        f64 exp = expSts(currCredits.at(vcIdx), maxCredits.at(vcIdx),
                         granularity);
        check.setEvent(time + latency, 0, port, vc, exp);
      }

      // set status check events for all VCs at time 'FINISH'
      for (u32 port = 0; port < numPorts; port++) {
        for (u32 vc = 0; vc < numVcs; vc++) {
          u32 vcIdx = router.vcIndex(port, vc);
          f64 exp = expSts(currCredits.at(vcIdx), maxCredits.at(vcIdx),
                           granularity);
          check.setEvent(1000000, 0, port, vc, exp);
        }
      }

      gSim->simulate();

      ASSERT_TRUE(status.ok());
      ASSERT_TRUE(status.done());
    }
  }
}
