/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include "DBWrapper.h"
#include "Grid.h"
#include "Net.h"

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

namespace FastRoute {

class RoutePt
{
public:
  RoutePt() = default;
  RoutePt(int x,
	  int y,
	  int layer);

  friend bool operator<(const RoutePt &p1,
			const RoutePt &p2);
private:
  int _x;
  int _y;
  int _layer;
};

bool operator<(const RoutePt &p1,
	       const RoutePt &p2);

typedef std::map<RoutePt, sta::ParasiticNode*> NodeRoutePtMap;

class RcTreeBuilder
{
 public:
  RcTreeBuilder(ord::OpenRoad* openroad,
		DBWrapper* dbWrapper,
		Grid* grid);
  void estimateParasitcs(Net* net,
			 std::vector<ROUTE>& routes);

 protected:
  void makeRoutePtMap();
  void reduceParasiticNetwork();
  sta::Pin* staPin(Pin& pin);
  void makeRouteParasitics(std::vector<ROUTE>& routes);
  sta::ParasiticNode *ensureParasiticNode(int x,
					  int y,
					  int layer);
  void makeParasiticsToGrid(Pin& pin,
			    sta::ParasiticNode *pin_node);
  void layerRC(int wire_length_dbu,
	       int layer,
	       // Return values.
	       float &res,
	       float &cap);

  // Variables common to all nets.
  Grid* _grid;
  DBWrapper* _dbWrapper;
  sta::dbSta* _sta;
  sta::dbNetwork* _network;
  sta::Parasitics* _parasitics;
  sta::Corner* _corner;
  sta::MinMax* _min_max;
  sta::ParasiticAnalysisPt* _analysisPoint;
  bool _debug;

  // Net variables 
  Net* _net;
  sta::Net* _sta_net;
  sta::Parasitic* _parasitic;
  // Counter for internal parasitic node IDs.
  int _node_id;
  // x/y/layer -> parasitic node
  NodeRoutePtMap _node_map;
};

}  // namespace FastRoute
