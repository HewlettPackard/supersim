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
#ifndef NETWORK_DRAGONFLY_NETWORK_H_
#define NETWORK_DRAGONFLY_NETWORK_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "interface/Interface.h"
#include "network/Channel.h"
#include "network/Network.h"
#include "router/Router.h"
#include "util/DimensionalArray.h"

namespace Dragonfly {

class Network : public ::Network {
 public:
  Network(const std::string& _name, const Component* _parent,
          MetadataHandler* _metadataHandler, Json::Value _settings);
  ~Network();

  // this is the routing algorithm factory for this network
  ::RoutingAlgorithm* createRoutingAlgorithm(
       u32 _inputPort, u32 _inputVc, const std::string& _name,
       const Component* _parent, Router* _router) override;

  u32 numRouters() const override;
  u32 numInterfaces() const override;
  Router* getRouter(u32 _id) const override;
  Interface* getInterface(u32 _id) const override;
  void translateInterfaceIdToAddress(
      u32 _id, std::vector<u32>* _address) const override;
  u32 translateInterfaceAddressToId(
      const std::vector<u32>* _address) const override;
  void translateRouterIdToAddress(
      u32 _id, std::vector<u32>* _address) const override;
  u32 translateRouterAddressToId(
      const std::vector<u32>* _address) const override;
  u32 computeMinimalHops(const std::vector<u32>* _source,
                         const std::vector<u32>* _destination) const override;
  void computeGlobalToRouterMap(u32 _thisGlobalWeight, u32 _thisGlobalOffset,
                                u32* _globalPort,
                                u32* _localRouter, u32* _localPort);

  u32 computeLocalSrcPort(u32 _portBase, u32 _offset, u32 _weight);
  u32 computeLocalDstPort(u32 _portBase, u32 _offset, u32 _weight);

 protected:
  void collectChannels(std::vector<Channel*>* _channels) override;

 private:
  u32 localWidth_;
  u32 localWeight_;
  u32 concentration_;
  u32 globalWidth_;
  u32 globalWeight_;

  u32 groupRadix_;
  u32 routerRadix_;
  u32 globalPortsPerRouter_;
  u32 routerGlobalPortBase_;

  std::vector<std::vector<Router*> > routers_;
  DimensionalArray<Interface*> interfaces_;
  std::vector<Channel*> globalChannels_;
  std::vector<Channel*> localChannels_;
  std::vector<Channel*> externalChannels_;
};

}  // namespace Dragonfly

#endif  // NETWORK_DRAGONFLY_NETWORK_H_
