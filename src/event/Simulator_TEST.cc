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
#include "event/Simulator.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "test/TestSetup_TEST.h"

namespace {
class StatusCheck : public Component {
 public:
  StatusCheck(const std::string& _name, const Component* _parent)
      : Component(_name, _parent) {}
  ~StatusCheck() {}

  void setEvent(u64 _time, u8 _epsilon, Simulator::Clock _clock, u32 _cycles,
                u64 _expected) {
    Event* evt = new Event({_clock, _cycles, _expected});
    addEvent(_time, _epsilon, evt, 0);
  }

  void processEvent(void* _event, s32 _type) {
    Event* evt = reinterpret_cast<Event*>(_event);
    u64 act = gSim->futureCycle(evt->clock, evt->cycles);
    ASSERT_EQ(act, evt->exp);
  }

 private:
  struct Event {
    Simulator::Clock clock;
    u32 cycles;
    u64 exp;
  };
};
}  // namespace

TEST(Simulator, futureCycle) {
  for (u8 eps = 0; eps < 3; eps++) {
    TestSetup ts(1000, 333, 500, 6493389);

    StatusCheck checker("checker", nullptr);
    checker.setEvent(0, eps, Simulator::Clock::CHANNEL, 1, 1000);
    checker.setEvent(0, eps, Simulator::Clock::ROUTER, 1, 333);
    checker.setEvent(0, eps, Simulator::Clock::INTERFACE, 1, 500);

    checker.setEvent(0, eps, Simulator::Clock::CHANNEL, 3, 3000);
    checker.setEvent(0, eps, Simulator::Clock::ROUTER, 3, 999);
    checker.setEvent(0, eps, Simulator::Clock::INTERFACE, 3, 1500);

    checker.setEvent(2567, eps, Simulator::Clock::CHANNEL, 1, 3000);
    checker.setEvent(823, eps, Simulator::Clock::ROUTER, 1, 999);
    checker.setEvent(1456, eps, Simulator::Clock::INTERFACE, 1, 1500);

    checker.setEvent(4294976391, eps, Simulator::Clock::ROUTER, 1, 4294976724);

    gSim->initialize();
    gSim->simulate();
  }
}
