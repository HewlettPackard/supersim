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
#ifndef TYPES_MESSAGEOWNER_H_
#define TYPES_MESSAGEOWNER_H_

class Message;

class MessageOwner {
 public:
  MessageOwner();
  virtual ~MessageOwner();

  /*
   * This informs the message owner that the message has entered
   * the network interface. This allows the owner to generate messages
   * back-to-back without being concerned about message queuing.
   */
  virtual void messageEnteredInterface(Message* _message) = 0;

  /*
   * This informs the current message owner that a new message owner
   * is being assigned and that this owner doesn't need to worry about
   * memory deallocation.
   */
  virtual void messageDelivered(Message* _message) = 0;
};

#endif  // TYPES_MESSAGEOWNER_H_
