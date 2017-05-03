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
#ifndef CONGESTION_CONGESTIONSTATUS_H_
#define CONGESTION_CONGESTIONSTATUS_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "architecture/CreditWatcher.h"
#include "architecture/PortedDevice.h"
#include "event/Component.h"

#define CONGESTIONSTATUS_ARGS const std::string&, const Component*, \
    PortedDevice*, Json::Value

class CongestionStatus : public Component, public CreditWatcher {
 public:
  CongestionStatus(const std::string& _name, const Component* _parent,
                   PortedDevice* _device, Json::Value _settings);
  virtual ~CongestionStatus();

  // this is a congestion status factory
  static CongestionStatus* create(CONGESTIONSTATUS_ARGS);

  // CreditWatcher interface
  void initCredits(u32 _vcIdx, u32 _credits) override;  // called per source
  void incrementCredit(u32 _vcIdx) override;  // a credit came from downstream
  void decrementCredit(u32 _vcIdx) override;  // a credit was consumed locally

  // this returns congestion status (i.e. 0=empty 1=congested)
  f64 status(u32 _port, u32 _vc) const;  // (must be epsilon >= 1)

  void processEvent(void* _event, s32 _type);

  // this class NEEDS to use these two event types. subclasses also using
  //  events must pass these two types on to this class.
  static const s32 INCR = 0x50;
  static const s32 DECR = 0xAF;

 protected:
  virtual void performInitCredits(u32 _port, u32 _vc, u32 _credits) = 0;
  virtual void performIncrementCredit(u32 _port, u32 _vc) = 0;
  virtual void performDecrementCredit(u32 _port, u32 _vc) = 0;

  // this MUST return a value >= 0.0 and <= 1.0
  virtual f64 computeStatus(u32 _port, u32 _vc) const = 0;

  PortedDevice* device_;
  const u32 numPorts_;
  const u32 numVcs_;

 private:
  void createEvent(u32 _vcIdx, s32 _type);

  const u32 latency_;
  const u32 granularity_;
};

#endif  // CONGESTION_CONGESTIONSTATUS_H_
