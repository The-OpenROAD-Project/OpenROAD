/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu
// Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.
//
// Copyright (c) 2019, The Regents of the University of California
// Copyright (c) 2018, SangGi Do and Mingyu Woo
// All rights reserved.
//
// BSD 3-Clause License
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
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <functional>
#include <map>
#include <set>
#include <vector>
#include <utility> // pair
#include <tuple> // pair
#include <unordered_map>
#include <unordered_set>

#include "odb/db.h"


namespace utl {
class Logger;
}

namespace ord {
class OpenRoad;
}

namespace aak {

class Node;
class Edge;
class Pin;


using std::map;
using std::set;
using std::string;
using std::vector;
using std::pair;
using std::tuple;

using utl::Logger;

using odb::dbBlock;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbNet;
using odb::dbLib;
using odb::dbMaster;
using odb::dbMasterType;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbBTerm;
using odb::dbOrientType;
using odb::dbRow;
using odb::dbSite;
using odb::Point;
using odb::Rect;

using ord::OpenRoad;


class RoutingParams;
class Architecture;
class Network;

class UWdp
{
public:
  UWdp();
  ~UWdp();

  UWdp(const UWdp &) = delete;
  UWdp &operator=(const UWdp &) = delete;
  UWdp(const UWdp &&) = delete;
  UWdp &operator=(const UWdp &&) = delete;

  void clear();
  void init(ord::OpenRoad* openroad);

  void improvePlacement();

protected:
  void import();
  void updateDbInstLocations();

  void initEdgeTypes();
  void initCellSpacingTable();
  void initPadding();
  void createLayerMap();
  void createNdrMap();
  void createNetwork();
  void createArchitecture();
  void createRouteGrid();
  void setUpNdrRules();
  void setUpPlacementRegions();

protected:
  ord::OpenRoad *openroad_;
  utl::Logger *logger_;
  odb::dbDatabase *db_;

  // My stuff.
  Architecture *arch_;
  Network *nw_;
  RoutingParams *rt_;
  std::unordered_map<odb::dbInst*, Node*> instMap_;
  std::unordered_map<odb::dbNet*, Edge*> netMap_;
  std::unordered_map<odb::dbBTerm*, Node*> pinMap_;
  std::unordered_set<odb::dbTechLayer*> pwrLayers_;
  std::unordered_set<odb::dbTechLayer*> gndLayers_;
};

}  // namespace
