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

namespace grt {

typedef std::map<RoutePt, sta::ParasiticNode*> NodeRoutePtMap;

class MakeWireParasitics
{
 public:
  MakeWireParasitics(ord::OpenRoad* openroad, GlobalRouter* grouter);
  void estimateParasitcs(odb::dbNet* net,
                         std::vector<Pin>& pins,
                         std::vector<GSegment>& routes);

 protected:
  void makePinRoutePts(std::vector<Pin>& pins);
  RoutePt routePt(Pin& pin);
  sta::Pin* staPin(Pin& pin);
  void makeRouteParasitics(odb::dbNet* net, std::vector<GSegment>& routes);
  sta::ParasiticNode* ensureParasiticNode(int x, int y, int layer);
  void makeParasiticsToGrid(std::vector<Pin>& pins);
  void makeParasiticsToGrid(Pin& pin, sta::ParasiticNode* pin_node);
  void reduceParasiticNetwork();
  void layerRC(int wire_length_dbu,
               int layer,
               // Return values.
               float& res,
               float& cap);
  float getCutLayerRes(unsigned belowLayerId);
  double dbuToMeters(int dbu);

  // Variables common to all nets.
  GlobalRouter* _grouter;
  odb::dbTech* _tech;
  utl::Logger* _logger;
  sta::dbSta* _sta;
  sta::dbNetwork* _network;
  sta::Parasitics* _parasitics;
  sta::Corner* _corner;
  sta::MinMax* _min_max;
  sta::ParasiticAnalysisPt* _analysisPoint;

  // Net variables
  sta::Net* _sta_net;
  sta::Parasitic* _parasitic;
  // Counter for internal parasitic node IDs.
  int _node_id;
  // x/y/layer -> parasitic node
  NodeRoutePtMap _node_map;
};

}  // namespace grt
