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
#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include <cmath>

#include <string>
#include <unordered_set>

#include "event/Component.h"
#include "network/Channel.h"
#include "types/Credit.h"
#include "types/CreditReceiver.h"
#include "types/Flit.h"
#include "types/FlitReceiver.h"

#include "test/TestSetup_TEST.h"


const bool DEBUG = false;

class Source;
class Sink;

class Source : public Component, public CreditReceiver {
 public:
  explicit Source(Channel* _channel);
  ~Source();
  void setSink(Sink* _sink);
  void load(u32 _count, u64 _totalCycles);
  void expect(u64 _time);
  void processEvent(void* _event, s32 _type);
  void receiveCredit(u32 _port, Credit* _credit);

 private:
  Channel* channel_;
  u32 port_;
  Sink* sink_;
  std::unordered_set<u64> expected_;
};

class Sink : public Component, public FlitReceiver {
 public:
  explicit Sink(Channel* _channel);
  ~Sink();
  void setSource(Source* _source);
  void load(u32 _count, u64 _totalCycles);
  void expect(u64 _time);
  void processEvent(void* _event, s32 _type);
  void receiveFlit(u32 _port, Flit* _flit);

 private:
  Channel* channel_;
  u32 port_;
  Source* source_;
  std::unordered_set<u64> expected_;
};

/* Source impl */

Source::Source(Channel* _channel)
    : Component("Source", nullptr), channel_(_channel) {
  debug_ = DEBUG;
  channel_->setSource(this, port_ = gSim->rnd.nextU64(0, U32_MAX));
}

Source::~Source() {
  assert(expected_.size() == 0);
}

void Source::setSink(Sink* _sink) {
  sink_ = _sink;
}

void Source::load(u32 _count, u64 _totalCycles) {
  assert(_count <= _totalCycles);

  // list all available times
  std::unordered_set<u64> injectCycles;
  for (u64 cycle = 1; cycle <= _totalCycles; cycle++) {
    injectCycles.insert(cycle);
  }

  // create _count injection events
  while (_count > 0) {
    _count--;
    auto it = injectCycles.begin();
    std::advance(it, gSim->rnd.nextU64(0, injectCycles.size() - 1));
    u64 injectCycle = *it;
    injectCycles.erase(it);
    u64 injectTime = gSim->futureCycle(Simulator::Clock::CHANNEL, injectCycle);
    dbgprintf("source inject at %lu(%lu)", injectCycle, injectTime);
    addEvent(injectTime, 0, nullptr, 0);
    sink_->expect(gSim->futureCycle(Simulator::Clock::CHANNEL,
                                    injectCycle + channel_->latency()));
  }
}

void Source::expect(u64 _time) {
  assert(expected_.count(_time) == 0);
  expected_.insert(_time);
  dbgprintf("source expect at %lu", _time);
}

void Source::processEvent(void* _event, s32 _type) {
  Flit* flit = new Flit(0, false, false, nullptr);
  assert(channel_->getNextFlit() == nullptr);
  channel_->setNextFlit(flit);
  dbgprintf("source injecting at %lu", gSim->time());
}

void Source::receiveCredit(u32 _port, Credit* _credit) {
  dbgprintf("source receiving credit at %lu", gSim->time());
  assert(_port == port_);
  assert(expected_.count(gSim->time()) == 1);
  expected_.erase(gSim->time());
  delete _credit;
}

/* Sink impl */

Sink::Sink(Channel* _channel)
    : Component("Sink", nullptr), channel_(_channel) {
  debug_ = DEBUG;
  channel_->setSink(this, port_ = gSim->rnd.nextU64(0, U32_MAX));
}

Sink::~Sink() {
  assert(expected_.size() == 0);
}

void Sink::setSource(Source* _source) {
  source_ = _source;
}

void Sink::load(u32 _count, u64 _totalCycles) {
  assert(_count <= _totalCycles);

  // list all available times
  std::unordered_set<u64> injectCycles;
  for (u64 cycle = 1; cycle <= _totalCycles; cycle++) {
    injectCycles.insert(cycle);
  }

  // create _count injection events
  while (_count > 0) {
    _count--;
    auto it = injectCycles.begin();
    std::advance(it, gSim->rnd.nextU64(0, injectCycles.size() - 1));
    u64 injectCycle = *it;
    injectCycles.erase(it);
    u64 injectTime = gSim->futureCycle(Simulator::Clock::CHANNEL, injectCycle);
    dbgprintf("sink inject at %lu(%lu)", injectCycle, injectTime);
    addEvent(injectTime, 0, nullptr, 0);
    source_->expect(gSim->futureCycle(Simulator::Clock::CHANNEL,
                                      injectCycle + channel_->latency()));
  }
}

void Sink::expect(u64 _time) {
  assert(expected_.count(_time) == 0);
  expected_.insert(_time);
  dbgprintf("sink expect at %lu", _time);
}

void Sink::processEvent(void* _event, s32 _type) {
  Credit* credit = new Credit(1);
  assert(channel_->getNextCredit() == nullptr);
  channel_->setNextCredit(credit);
  dbgprintf("sink injecting at %lu", gSim->time());
}

void Sink::receiveFlit(u32 _port, Flit* _flit) {
  dbgprintf("sink receiving flit at %lu", gSim->time());
  assert(_port == port_);
  assert(expected_.count(gSim->time()) == 1);
  expected_.erase(gSim->time());
  delete _flit;
}

/* Monitoring */

class EndMonitoring : public Component {
 public:
  EndMonitoring(Channel* _channel, u32 _cycles)
      : Component("Timer", nullptr), channel_(_channel) {
    channel_->startMonitoring();
    addEvent(gSim->futureCycle(Simulator::Clock::CHANNEL, _cycles),
             0, nullptr, 0);
  }

  ~EndMonitoring() {}

  void processEvent(void* _event, s32 _type) {
    channel_->endMonitoring();
  }

 private:
  Channel* channel_;
};


/* Test driver */

TEST(Channel, full) {
  u64 seed = 12345678;
  for (u32 cycleTime = 1; cycleTime <= 100; cycleTime += 26) {
    TestSetup setup(cycleTime, cycleTime, seed++);

    const u32 latency = gSim->rnd.nextU64(1, 5);

    Json::Value settings;
    settings["latency"] = latency;
    Channel c("TestChannel", nullptr, settings);

    Source source(&c);
    Sink sink(&c);
    source.setSink(&sink);
    sink.setSource(&source);

    const u32 clocks = 10000;
    const u32 flits = gSim->rnd.nextU64(1, clocks);
    const u32 credits = gSim->rnd.nextU64(1, clocks);
    source.load(flits, clocks);
    sink.load(credits, clocks);

    EndMonitoring ender(&c, clocks);

    gSim->simulate();

    f64 actUtil = c.utilization();
    f64 expUtil = static_cast<f64>(flits) / clocks;
    f64 absDelta = std::abs(actUtil - expUtil);
    ASSERT_LE(absDelta, 0.0001);
  }
}
