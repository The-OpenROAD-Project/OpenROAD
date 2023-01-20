/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "FastRoute.h"
#include "Grid.h"
#include "Net.h"
#include "db_sta/dbSta.hh"
#include "grt/GlobalRouter.h"
#include "sta/Clock.hh"
#include "sta/Set.hh"

namespace sta {
class Net;
class dbNetwork;
class Parasitics;
class Parasitic;
class Corner;
class OperatingConditions;
class ParasiticAnalysisPt;
class Units;
}  // namespace sta

namespace ord {
class OpenRoad;
}

namespace utl {
class Logger;
}

namespace rsz {
class Resizer;
}

namespace grt {

class MakeWireParasitics
{
 public:
  MakeWireParasitics(utl::Logger* logger,
                     rsz::Resizer* resizer,
                     sta::dbSta* sta,
                     odb::dbTech* tech,
                     GlobalRouter* grouter);
  void estimateParasitcs(odb::dbNet* net,
                         std::vector<Pin>& pins,
                         GRoute& route) const;
  // Return GRT layer lengths in dbu's for db_net's route indexed by routing
  // layer.
  std::vector<int> routeLayerLengths(odb::dbNet* db_net) const;

 private:
  typedef std::map<RoutePt, sta::ParasiticNode*> NodeRoutePtMap;

  sta::Pin* staPin(Pin& pin) const;
  void makeRouteParasitics(odb::dbNet* net,
                           GRoute& route,
                           sta::Net* sta_net,
                           sta::Corner* corner,
                           sta::ParasiticAnalysisPt* analysis_point,
                           sta::Parasitic* parasitic,
                           NodeRoutePtMap& node_map) const;
  sta::ParasiticNode* ensureParasiticNode(int x,
                                          int y,
                                          int layer,
                                          NodeRoutePtMap& node_map,
                                          sta::Parasitic* parasitic,
                                          sta::Net* net) const;
  void makeParasiticsToPins(std::vector<Pin>& pins,
                            NodeRoutePtMap& node_map,
                            sta::Corner* corner,
                            sta::ParasiticAnalysisPt* analysis_point,
                            sta::Parasitic* parasitic) const;
  void makeParasiticsToPin(Pin& pin,
                           NodeRoutePtMap& node_map,
                           sta::Corner* corner,
                           sta::ParasiticAnalysisPt* analysis_point,
                           sta::Parasitic* parasitic) const;
  void layerRC(int wire_length_dbu,
               int layer,
               sta::Corner* corner,
               // Return values.
               float& res,
               float& cap) const;
  float getCutLayerRes(odb::dbTechLayer* cut_layer,
                       sta::Corner* corner,
                       int num_cuts = 1) const;
  double dbuToMeters(int dbu) const;

  // Variables common to all nets.
  GlobalRouter* grouter_;
  odb::dbTech* tech_;
  utl::Logger* logger_;
  rsz::Resizer* resizer_;
  sta::dbSta* sta_;
  sta::dbNetwork* network_;
  sta::Parasitics* parasitics_;
  sta::MinMax* min_max_;
};

}  // namespace grt
