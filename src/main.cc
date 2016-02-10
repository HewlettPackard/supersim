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
#include <jsoncpp/json/json.h>
#include <settings/settings.h>

#include <cstdio>
#include <cassert>
#include <cstring>

#include <string>
#include <vector>

#include "application/Application.h"
#include "application/ApplicationFactory.h"
#include "application/Terminal.h"
#include "event/Simulator.h"
#include "event/VectorQueue.h"
#include "metadata/MetadataHandler.h"
#include "metadata/MetadataHandlerFactory.h"
#include "network/Network.h"
#include "network/NetworkFactory.h"

s32 main(s32 _argc, char** _argv) {
  // turn off buffered output on stdout and stderr
  setbuf(stdout, nullptr);
  setbuf(stderr, nullptr);

  // get JSON settings
  Json::Value settings;
  settings::commandLine(_argc, _argv, &settings);
  printf("%s\n", settings::toString(settings).c_str());

  // enable debugging on select components
  for (u32 i = 0; i < settings["debug"].size(); i++) {
    std::string componentName = settings["debug"][i].asString();
    Component::addDebugName(componentName);
  }

  // initialize the discrete event simulator
  gSim = new VectorQueue(settings["simulator"]);

  // create a metadata handler
  MetadataHandler* metadataHandler = MetadataHandlerFactory::createHandler(
      settings["metadata_handler"]);

  // create a network
  Network* network = NetworkFactory::createNetwork(
      "Network", nullptr, settings["network"]);
  gSim->setNetwork(network);
  u32 numInterfaces = network->numInterfaces();
  u32 numRouters = network->numRouters();
  u32 routerRadix = network->getRouter(0)->numPorts();
  u32 numVcs = network->numVcs();
  u64 numComponents = Component::numComponents();

  printf("Endpoints:    %u\n"
         "Routers:      %u\n"
         "Router radix: %u\n"
         "VCs:          %u\n"
         "Components:   %lu\n\n",
         numInterfaces,
         numRouters,
         routerRadix,
         numVcs,
         numComponents);

  // create an application
  Application* application = ApplicationFactory::createApplication(
      "Application", nullptr, metadataHandler, settings["application"]);
  gSim->setApplication(application);
  assert(application->numTerminals() == numInterfaces);

  // check that all debug names were authentic
  Component::debugCheck();

  // run the simulation!
  printf("Simulation beginning\n");
  gSim->simulate();
  printf("Simulation complete\n");

  // cleanup the elements created here
  delete network;
  delete application;
  delete metadataHandler;

  // cleanup the global simulator components
  delete gSim;

  return 0;
}
