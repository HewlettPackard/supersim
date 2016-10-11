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
#ifndef ROUTER_COMMON_CREDITWATCHER_H_
#define ROUTER_COMMON_CREDITWATCHER_H_

#include <prim/prim.h>

/*
 * This class defines the interface for devices that want to listen to the
 *  activity of credits. Most likely this is only used in conjuntion with a
 *  congestion status algorithm.
 */
class CreditWatcher {
 public:
  CreditWatcher();
  virtual ~CreditWatcher();
  virtual void initCredits(u32 _vcIdx, u32 _credits) = 0;
  virtual void incrementCredit(u32 _vcIdx) = 0;
  virtual void decrementCredit(u32 _vcIdx) = 0;
};

#endif  // ROUTER_COMMON_CREDITWATCHER_H_
