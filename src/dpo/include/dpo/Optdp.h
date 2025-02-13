///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>  // pair
#include <vector>

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace dpl {
class Opendp;
}

namespace dpo {

class RoutingParams;
class Architecture;
class Network;
class Node;
class Edge;
class Pin;

using dpl::Opendp;
using odb::dbDatabase;
using odb::dbOrientType;
using utl::Logger;

class Optdp
{
 public:
  Optdp() = default;

  Optdp(const Optdp&) = delete;
  Optdp& operator=(const Optdp&) = delete;
  Optdp(const Optdp&&) = delete;
  Optdp& operator=(const Optdp&&) = delete;

  void init(odb::dbDatabase* db, utl::Logger* logger, dpl::Opendp* opendp);

  void improvePlacement(int seed,
                        int max_displacement_x,
                        int max_displacement_y,
                        bool disallow_one_site_gaps = false);

 private:
  void import();
  void updateDbInstLocations();

  void initPadding();
  void createLayerMap();
  void createNdrMap();
  void setupMasterPowers();
  void createNetwork();
  void createArchitecture();
  void createRouteInformation();
  void setUpNdrRules();
  void setUpPlacementRegions();
  unsigned dbToDpoOrient(const dbOrientType& orient);

  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
  dpl::Opendp* opendp_ = nullptr;

  // My stuff.
  Architecture* arch_ = nullptr;  // Information about rows, etc.
  Network* network_ = nullptr;    // The netlist, cells, etc.
  RoutingParams* routeinfo_
      = nullptr;  // Route info we might consider (future).

  // Some maps.
  std::unordered_map<odb::dbInst*, Node*> instMap_;
  std::unordered_map<odb::dbNet*, Edge*> netMap_;
  std::unordered_map<odb::dbBTerm*, Node*> termMap_;

  // For monitoring power alignment.
  std::unordered_set<odb::dbTechLayer*> pwrLayers_;
  std::unordered_set<odb::dbTechLayer*> gndLayers_;
  std::unordered_map<odb::dbMaster*, std::pair<int, int>>
      masterPwrs_;  // top,bot
};

}  // namespace dpo
