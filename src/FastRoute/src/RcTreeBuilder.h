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
#include "SteinerTree.h"

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

class RcTreeBuilder
{
 public:
  RcTreeBuilder(ord::OpenRoad* openroad, DBWrapper* dbWrapper);
  void run(Net* net, SteinerTree* steinerTree, Grid* grid);
  void reportParasitics();

 protected:
  void initStaData();
  void makeParasiticNetwork();
  void createSteinerNodes();
  void computeGlobalParasitics();
  void computeLocalParasitics();
  void reduceParasiticNetwork();
  int findNodeToConnect(const Pin& pin,
                        const std::vector<unsigned>& pinNodes) const;
  unsigned computeDist(const Node& n1, const Node& n2) const;
  unsigned computeDist(const odb::Point& pt, const Node& n) const;

  Net* _net = nullptr;
  Grid* _grid = nullptr;
  DBWrapper* _dbWrapper = nullptr;
  sta::Net* _staNet = nullptr;
  SteinerTree* _steinerTree = nullptr;
  sta::Parasitic* _parasitic = nullptr;
  sta::Parasitics* _parasitics = nullptr;
  sta::Corner* _corner = nullptr;
  sta::OperatingConditions* _op_cond = nullptr;
  sta::ParasiticAnalysisPt* _analysisPoint = nullptr;
  sta::dbNetwork* _network = nullptr;
  sta::Units* _units = nullptr;
  bool _debug = false;
};

}  // namespace FastRoute
