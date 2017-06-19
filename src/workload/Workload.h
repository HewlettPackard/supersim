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
#ifndef WORKLOAD_WORKLOAD_H_
#define WORKLOAD_WORKLOAD_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "stats/MessageLog.h"
#include "workload/MessageDistributor.h"

class Application;
class MetadataHandler;

class Workload : public Component {
 public:
  Workload(const std::string& _name, const Component* _parent,
           MetadataHandler* _metadataHandler, Json::Value _settings);
  ~Workload();

  u32 numApplications() const;
  Application* application(u32 _index) const;
  MessageDistributor* messageDistributor(u32 _index) const;
  MessageLog* messageLog() const;
  bool monitoring() const;

  // OPERATION: The Workload class signals the applications to keep them
  //  synchronized. After all applications report 'ready', the workload
  //  calls the start() function on each application. After all applications
  //  report 'complete', the workload calls the stop() function on each
  //  application. After all application report 'done', the workload calls the
  //  kill() function on each application. At this point the application should
  //  stop injecting traffic into the network and the simulation will end.

  // MONITORING: The Workload class signals the application and the network when
  //  to perform monitoring. The startMonitoring() function is called when all
  //  applications report 'ready'. The endMonitoring() function is called when
  //  all applications report 'done'. The monitoring may not happen if
  //  applications transition immediately from ready to complete to done.

  // This function indicates that an application is ready to run.
  void applicationReady(u32 _index);

  // This function indicates that an application is complete. This does not mean
  //  that the application has stopped sending. Some applications will continue
  //  sending traffic after it completes.
  void applicationComplete(u32 _index);

  // This function indicates that an application is done. A done application is
  //  one that has completed and is ready to stop sending traffic.
  void applicationDone(u32 _index);

 private:
  enum class Fsm {READY, COMPLETE, DONE, KILLED};

  std::vector<Application*> applications_;
  std::vector<MessageDistributor*> distributors_;
  MessageLog* messageLog_;

  Fsm fsm_;
  u32 readyCount_;
  u32 completeCount_;
  u32 doneCount_;
  bool monitoring_;
};

#endif  // WORKLOAD_WORKLOAD_H_
