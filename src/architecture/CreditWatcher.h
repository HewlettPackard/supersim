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
#ifndef ARCHITECTURE_CREDITWATCHER_H_
#define ARCHITECTURE_CREDITWATCHER_H_

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

#endif  // ARCHITECTURE_CREDITWATCHER_H_
