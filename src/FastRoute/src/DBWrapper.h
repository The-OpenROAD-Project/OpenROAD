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

#include <boost/function_output_iterator.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <string>

#include "FastRoute.h"
#include "Grid.h"
#include "Netlist.h"
#include "RoutingLayer.h"
#include "RoutingTracks.h"
#include "antennachecker/AntennaChecker.hh"
#include "db_sta/dbSta.hh"
#include "opendb/db.h"
#include "opendb/dbShape.h"
#include "opendb/wOrder.h"
#include "opendp/Opendp.h"
#include "sta/Clock.hh"
#include "sta/Set.hh"

// Forward declaration protects FastRoute code from any
// header file from the DB. FastRoute code keeps independent.
namespace odb {
class dbDatabase;
class dbChip;
class dbTech;
}  // namespace odb

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace FastRoute {

class DBWrapper
{
 public:
  DBWrapper(odb::dbDatabase* db, Netlist* netlist, Grid* grid);

  void initGrid(int maxLayer);
  void initRoutingLayers(std::vector<RoutingLayer>& routingLayers);
  void initRoutingTracks(std::vector<RoutingTracks>& allRoutingTracks,
                         int maxLayer,
                         std::map<int, float> layerPitches);
  void computeCapacities(int maxLayer, std::map<int, float> layerPitches);
  void computeSpacingsAndMinWidth(int maxLayer);
  void initNetlist(bool reroute);
  void addNet(odb::dbNet* net, Box dieArea, bool isClock);
  void initClockNets();
  void initObstacles();
  int computeMaxRoutingLayer();
  void getLayerRC(unsigned layerId, float& r, float& c);
  void getCutLayerRes(unsigned belowLayerId, float& r);
  float dbuToMeters(unsigned dbu);
  std::set<int> findTransitionLayers(int maxRoutingLayer);
  std::map<int, odb::dbTechVia*> getDefaultVias(int maxRoutingLayer);
  void commitGlobalSegmentsToDB(std::vector<FastRoute::NET> routing,
                                int maxRoutingLayer);
  int checkAntennaViolations(std::vector<FastRoute::NET> routing,
                             int maxRoutingLayer);
  void fixAntennas(std::string antennaCellName, std::string antennaPinName);
  void legalizePlacedCells();
  void setDB(unsigned idx) { _db = odb::dbDatabase::getDatabase(idx); }
  void setSelectedMetal(int metal) { selectedMetal = metal; }

 private:
  typedef int coord_type;
  typedef bg::cs::cartesian coord_sys_type;
  typedef bg::model::point<coord_type, 2, coord_sys_type> point;
  typedef bg::model::box<point> box;
  typedef std::pair<box, int> value;
  typedef bgi::rtree<value, bgi::quadratic<8, 4>> r_tree;

  void makeItermPins(Net* net, odb::dbNet* db_net, Box& dieArea);
  void makeBtermPins(Net* net, odb::dbNet* db_net, Box& dieArea);
  void insertDiode(odb::dbNet* net,
                   std::string antennaCellName,
                   std::string antennaPinName,
                   odb::dbInst* sinkInst,
                   odb::dbITerm* sinkITerm,
                   std::string antennaInstName,
                   int siteWidth,
                   r_tree& fixedInsts);
  void getFixedInstances(r_tree& fixedInsts);

  std::set<odb::dbNet*> _clockNets;
  int selectedMetal = 3;
  odb::dbDatabase* _db;
  odb::dbChip* _chip;
  Netlist* _netlist;
  Grid* _grid;
  bool _verbose = false;
  antenna_checker::AntennaChecker* _arc = nullptr;
  opendp::Opendp* _opendp = nullptr;

  std::map<std::string, odb::dbNet*> dbNets;
  std::map<std::string, std::vector<VINFO>>
      antennaViolations;
  std::vector<odb::dbNet*> dirtyNets;
};

std::string getITermName(odb::dbITerm* iterm);

}  // namespace FastRoute
