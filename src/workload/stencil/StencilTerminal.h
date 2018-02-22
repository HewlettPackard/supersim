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
#ifndef WORKLOAD_STENCIL_STENCILTERMINAL_H_
#define WORKLOAD_STENCIL_STENCILTERMINAL_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <tuple>
#include <vector>

#include "event/Component.h"
#include "workload/Terminal.h"

class Application;

namespace Stencil {

class Application;

class StencilTerminal : public Terminal {
 public:
  StencilTerminal(
      const std::string& _name, const Component* _parent, u32 _id,
      const std::vector<u32>& _address,
      const std::vector<std::tuple<u32, u32> >& _exchangeSendMessages,
      u32 _exchangeRecvMessages, ::Application* _app, Json::Value _settings);
  ~StencilTerminal();
  void processEvent(void* _event, s32 _type) override;
  f64 percentComplete() const;
  void start();

 protected:
  void handleDeliveredMessage(Message* _message) override;
  void handleReceivedMessage(Message* _message) override;

 private:
  // kWaiting = waiting to start
  // kCompute = computing
  // kExchange = exchanging information in halo
  // kCollective = performance collective operation
  // kDone = done with all iterations
  enum class Fsm : s32 {kWaiting = 0, kCompute = 1, kExchange = 2,
      kCollective = 3, kDone = 4};

  void advanceFsm();  // enqueue event to perform transition
  void transitionFsm();  // called by processEvent
  void processReceivedMessage(Message* _message);  // called by processEvent

  void startCompute();
  void finishCompute();

  void startExchange();
  void handleExchangeMessage(Message* _message);
  void finishExchange();

  void startCollective();
  void handleCollectiveMessage(Message* _message);
  void finishCollective();

  void generateMessage(u32 _destination, u32 _size, u32 _protocolClass,
                       u32 _msgType);

  static u32 encodeOp(Fsm _state, u32 _iteration);
  static Fsm decodeState(u32 _opCode);
  static u32 decodeIteration(u32 _opCode);
  u32 collectiveDestination(u32 _offset);
  u32 collectiveSource(u32 _offset);

  // traffic generation
  const u32 numIterations_;
  const u32 maxPacketSize_;  // flits
  const std::vector<std::tuple<u32, u32> > exchangeSendMessages_;  // {dst,size}
  const u32 exchangeRecvMessages_;
  const u32 collectiveSize_;

  // protocol classes
  const u32 exchangeProtocolClass_;
  const u32 collectiveProtocolClass_;

  // compute time modeling
  const u64 computeDelay_;

  // state tracking
  u32 iteration_;
  Fsm fsm_;
  std::unordered_map<u32, u32> exchangeRecvCount_;  // [iter]=count
  std::unordered_map<u32, std::unordered_set<u32> > collectiveRecv_;  // [iter]
  u32 collectiveOffset_;
};

}  // namespace Stencil

#endif  // WORKLOAD_STENCIL_STENCILTERMINAL_H_
