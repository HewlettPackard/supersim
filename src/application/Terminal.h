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
#ifndef APPLICATION_TERMINAL_H_
#define APPLICATION_TERMINAL_H_

#include <prim/prim.h>

#include <unordered_set>
#include <string>
#include <vector>

#include "application/RateMonitor.h"
#include "event/Component.h"
#include "stats/RateLog.h"
#include "types/Message.h"
#include "types/MessageReceiver.h"

class Application;

class Terminal : public Component, public MessageReceiver {
 public:
  Terminal(const std::string& _name, const Component* _parent, u32 _id,
           const std::vector<u32>& _address, Application* _app);
  virtual ~Terminal();
  u32 getId() const;
  const std::vector<u32>& getAddress() const;
  void startRateMonitors();
  void endRateMonitors();
  void logRates(RateLog* _rateLog);
  u32 messagesSent() const;
  u32 messagesReceived() const;
  u32 transactionsCreated() const;
  void setMessageReceiver(MessageReceiver* _receiver);
  Application* getApplication() const;

  /*
   * This function is called by the network when a message is being given
   * to this terminal.
   * NOTE: THIS MUST BE EXPLICITLY CALLED BY SUBCLASSES
   */
  void receiveMessage(Message* _message) override;

  /*
   * This informs this terminal that the message has entered
   * the network interface. This allows the owner to generate messages
   * back-to-back without being concerned about message queuing.
   * NOTE: THIS MUST BE EXPLICITLY CALLED BY SUBCLASSES
   */
  virtual void messageEnteredInterface(Message* _message);

  /*
   * This informs this terminal that the message has exited the network and
   * that this terminal doesn't need to worry about memory deallocation.
   * Overriding implementation must call this!
   * NOTE: THIS MUST BE EXPLICITLY CALLED BY SUBCLASSES
   */
  virtual void messageExitedNetwork(Message* _message);

  /*
   * This returns the number of messages, packet, and flits that have been send
   * by this terminal but have not yet been received at the destination.
   */
  void enrouteCount(u32* _messages, u32* _packets, u32* _flits) const;

 protected:
  /*
   * This function is used by subclasses when they send a message.
   * This function performs the following (in order):
   *  1. Sets the message's owner (to this).
   *  2. Sets the message's Id.
   *  3. Sets the message's source Id.
   *  4. Sets the message's source address.
   *  5. Sets the message's destination Id.
   *  6. Sets the message's destination address.
   *  7. Sends the message.
   *  8. Adds the message to the internal outstanding messages set.
   * This returns the message Id.
   */
  u32 sendMessage(Message* _message, u32 _destinationId);

  /*
   * Subclass implementations can use this to generate new transaction IDs.
   */
  u64 createTransaction();

  /*
   * Subclasses need to call this when transactions have ended
   */
  void endTransaction(u64 _trans);

 private:
  u32 id_;
  const std::vector<u32> address_;
  Application* app_;
  MessageReceiver* messageReceiver_;

  RateMonitor* supplyMonitor_;
  RateMonitor* injectionMonitor_;
  RateMonitor* deliveredMonitor_;
  RateMonitor* ejectionMonitor_;

  u32 messagesSent_;
  u32 messagesReceived_;
  u32 transactionsCreated_;
  std::unordered_set<Message*> outstandingMessages_;
};

#endif  // APPLICATION_TERMINAL_H_
