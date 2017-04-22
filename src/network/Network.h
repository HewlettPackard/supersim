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
#ifndef NETWORK_NETWORK_H_
#define NETWORK_NETWORK_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <tuple>
#include <vector>

#include "event/Component.h"
#include "interface/Interface.h"
#include "metadata/MetadataHandler.h"
#include "network/Channel.h"
#include "network/RoutingAlgorithm.h"
#include "router/Router.h"
#include "stats/ChannelLog.h"

#define NETWORK_ARGS const std::string&, const Component*, MetadataHandler*, \
    Json::Value

class Network : public Component {
 public:
  Network(const std::string& _name, const Component* _parent,
          MetadataHandler* _metadataHandler, Json::Value _settings);
  virtual ~Network();

  // this is a network factory
  static Network* create(NETWORK_ARGS);

  // this is a routing algorithm factory definition
  virtual RoutingAlgorithm* createRoutingAlgorithm(
      u32 _vc, u32 _port, const std::string& _name, const Component* _parent,
      Router* _router) = 0;

  virtual u32 numRouters() const = 0;
  virtual u32 numInterfaces() const = 0;
  virtual Router* getRouter(u32 _id) const = 0;
  virtual Interface* getInterface(u32 _id) const = 0;
  virtual void translateInterfaceIdToAddress(
      u32 _id, std::vector<u32>* _address) const = 0;
  virtual u32 translateInterfaceAddressToId(
      const std::vector<u32>* _address) const = 0;
  virtual void translateRouterIdToAddress(
      u32 _id, std::vector<u32>* _address) const = 0;
  virtual u32 translateRouterAddressToId(
      const std::vector<u32>* _address) const = 0;
  u32 numVcs() const;
  MetadataHandler* metadataHandler() const;

  void startMonitoring();
  void endMonitoring();

 protected:
  // this is used by network implementations to create routing algorithms
  struct RoutingAlgorithmInfo {
    u32 baseVc;
    u32 numVcs;
    Json::Value settings;
  };

  virtual void collectChannels(std::vector<Channel*>* _channels) = 0;

  // this loads the routing algorithm info vector
  void loadTrafficClassInfo(Json::Value _settings);

  // this clears the routingAlgorithmInfo_ vector
  void clearTrafficClassInfo();

  u32 numVcs_;
  std::vector<std::tuple<u32, u32> > trafficClassVcs_;
  std::vector<RoutingAlgorithmInfo> routingAlgorithmInfo_;

 private:
  ChannelLog* channelLog_;
  MetadataHandler* metadataHandler_;
};

#endif  // NETWORK_NETWORK_H_
