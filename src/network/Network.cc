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
#include "network/Network.h"

#include <factory/ObjectFactory.h>

#include <cassert>

static u32 computeNumVcs(const Json::Value& _protocolClasses) {
  u32 sum = 0;
  for (u32 idx = 0; idx < _protocolClasses.size(); idx++) {
    const Json::Value& protocolClass = _protocolClasses[idx];
    assert(protocolClass.isMember("num_vcs") &&
           protocolClass["num_vcs"].isUInt() &&
           protocolClass["num_vcs"].asUInt() > 0);
    sum += protocolClass["num_vcs"].asUInt();
  }
  return sum;
}

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : Component(_name, _parent),
      numVcs_(computeNumVcs(_settings["protocol_classes"])),
      metadataHandler_(_metadataHandler),
      monitoring_(false) {
  // check settings
  assert(numVcs_ > 0);

  // create a channel log object
  channelLog_ = new ChannelLog(numVcs_, _settings["channel_log"]);
}

Network::~Network() {
  delete channelLog_;
}

Network* Network::create(
    const std::string& _name, const Component* _parent,
    MetadataHandler* _metadataHandler, Json::Value _settings) {
  // retrieve the topology
  const std::string& topology = _settings["topology"].asString();

  // attempt to build the network topology
  Network* network = factory::ObjectFactory<Network, NETWORK_ARGS>::create(
      topology, _name, _parent, _metadataHandler, _settings);

  // check that the factory had the topology
  if (network == nullptr) {
    fprintf(stderr, "unknown network topology: %s\n", topology.c_str());
    assert(false);
  }
  return network;
}

u32 Network::numVcs() const {
  return numVcs_;
}

MetadataHandler* Network::metadataHandler() const {
  return metadataHandler_;
}

void Network::startMonitoring() {
  monitoring_ = true;
  std::vector<Channel*> channels;
  collectChannels(&channels);
  for (auto it = channels.begin(); it != channels.end(); ++it) {
    Channel* c = *it;
    c->startMonitoring();
  }
}

void Network::endMonitoring() {
  monitoring_ = false;
  std::vector<Channel*> channels;
  collectChannels(&channels);
  for (auto it = channels.begin(); it != channels.end(); ++it) {
    Channel* c = *it;
    c->endMonitoring();
    channelLog_->logChannel(c);
  }
}

bool Network::monitoring() const {
  return monitoring_;
}

void Network::loadProtocolClassInfo(Json::Value _settings) {
  // parse the protocol classes description
  std::vector<std::tuple<u32, u32> > protocolClassVcs;
  for (u32 idx = 0, vcs = 0; idx < _settings.size(); idx++) {
    u32 numVcs = _settings[idx]["num_vcs"].asUInt();
    u32 baseVc = vcs;
    protocolClassVcs_.push_back(std::make_tuple(baseVc, numVcs));
    for (u32 vc = 0; vc < numVcs; vc++, vcs++) {
      RoutingAlgorithmInfo info;
      info.baseVc = baseVc;
      info.numVcs = numVcs;
      info.settings = _settings[idx]["routing"];
      routingAlgorithmInfo_.push_back(info);
    }
  }
  assert(routingAlgorithmInfo_.size() == numVcs_);
}

void Network::clearProtocolClassInfo() {
  protocolClassVcs_.clear();
  routingAlgorithmInfo_.clear();
}
