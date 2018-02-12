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
#ifndef ROUTING_REDUCTION_H_
#define ROUTING_REDUCTION_H_

#include <colhash/tuplehash.h>
#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <tuple>
#include <unordered_set>

#include "architecture/PortedDevice.h"
#include "event/Component.h"
#include "routing/mode.h"

#define REDUCTION_ARGS const std::string&, const Component*,  \
    const PortedDevice*, RoutingMode, Json::Value

class Reduction : public Component {
 public:
  Reduction(const std::string& _name, const Component* _parent,
            const PortedDevice* _device, RoutingMode _mode,
            Json::Value _settings);
  ~Reduction();

  // this is a reduction factory
  static Reduction* create(REDUCTION_ARGS);

  // add an option
  void add(u32 _port, u32 _vc, u32 _hops, f64 _congestion);

  // compute the results
  //  'allMinimal' can be nullptr if the user doesn't care. The flag indicates
  //  if any routes are non-minimal
  const std::unordered_set<std::tuple<u32, u32> >* reduce(bool* _allMinimal);

 protected:
  // subclasses must implement this function
  //  _inputs  = {port, vc, hops, congestion}
  //  _outputs = {port, vc}
  virtual void process(
      u32 _minHops,
      const std::unordered_set<std::tuple<u32, u32, u32, f64> >& _minimal,
      const std::unordered_set<std::tuple<u32, u32, u32, f64> >& _nonMinimal,
      std::unordered_set<std::tuple<u32, u32> >* _outputs,
      bool* _allMinimal) = 0;

 private:
  const PortedDevice* device_;
  const RoutingMode mode_;
  const u32 maxOutputs_;

  bool start_;
  std::unordered_set<u32> check_;
  std::unordered_set<std::tuple<u32, u32, u32, f64> > minimal_;
  std::unordered_set<std::tuple<u32, u32, u32, f64> > nonMinimal_;
  u32 minHops_;
  std::unordered_set<std::tuple<u32, u32> > intermediate_;
  std::unordered_set<std::tuple<u32, u32> > outputs_;
  bool allMinimal_;
};

#endif  // ROUTING_REDUCTION_H_
