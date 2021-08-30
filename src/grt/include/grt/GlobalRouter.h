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

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "GRoute.h"
#include "odb/db.h"
#include "sta/Liberty.hh"

namespace ord {
class OpenRoad;
}

namespace gui {
class Gui;
}

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
class dbTech;
class dbBlock;
class dbDatabase;
class dbTechLayer;
}  // namespace odb

namespace stt {
class SteinerTreeBuilder;
}

namespace sta {
class dbSta;
class dbNetwork;
}  // namespace sta

namespace grt {

class FastRouteCore;
class AntennaRepair;
class Grid;
class Pin;
class Net;
class Netlist;
class RoutingTracks;
class SteinerTree;
class RoutePt;
class GrouteRenderer;

struct RegionAdjustment
{
  odb::Rect region;
  int layer;
  float adjustment;

  RegionAdjustment(int min_x,
                   int min_y,
                   int max_x,
                   int max_y,
                   int l,
                   float adjst);
  odb::Rect getRegion() { return region; }
  int getLayer() { return layer; }
  float getAdjustment() { return adjustment; }
};

enum class NetType
{
  Clock,
  Signal,
  Antenna,
  All
};

class RoutePt
{
 public:
  RoutePt() = default;
  RoutePt(int x, int y, int layer);
  int x() { return _x; };
  int y() { return _y; };
  int layer() { return _layer; };

  friend bool operator<(const RoutePt& p1, const RoutePt& p2);

 private:
  int _x;
  int _y;
  int _layer;
};

bool operator<(const RoutePt& p1, const RoutePt& p2);

class GlobalRouter
{
 public:
  GlobalRouter();
  ~GlobalRouter();
  void init(ord::OpenRoad* openroad);
  void clear();

  void setAdjustment(const float adjustment);
  void setMinRoutingLayer(const int min_layer);
  void setMaxRoutingLayer(const int max_layer);
  void setMinLayerForClock(const int min_layer);
  void setMaxLayerForClock(const int max_layer);
  unsigned getDbId();
  void addLayerAdjustment(int layer, float reduction_percentage);
  void addRegionAdjustment(int min_x,
                           int min_y,
                           int max_x,
                           int max_y,
                           int layer,
                           float reduction_percentage);
  void setVerbose(const int v);
  void setOverflowIterations(int iterations);
  void setGridOrigin(long x, long y);
  void setAllowCongestion(bool allow_congestion);
  void setMacroExtension(int macro_extension);
  void printGrid();

  // flow functions
  void writeGuides(const char* file_name);
  std::vector<Net*> startFastRoute(int min_routing_layer,
                                   int max_routing_layer,
                                   NetType type);
  void estimateRC();
  void globalRoute();
  NetRouteMap& getRoutes() { return routes_; }
  bool haveRoutes() const { return !routes_.empty(); }

  // repair antenna public functions
  void repairAntennas(sta::LibertyPort* diode_port, int iterations);
  void addDirtyNet(odb::dbNet* net);

  double dbuToMicrons(int64_t dbu);

  // route clock nets public functions
  void routeClockNets();

  // functions for random grt
  void setSeed(int seed) { seed_ = seed; }
  void setCapacitiesPerturbationPercentage(float percentage);
  void setPerturbationAmount(int perturbation)
  {
    perturbation_amount_ = perturbation;
  };
  void perturbCapacities();

  // Highlight route in the gui.
  void highlightRoute(const odb::dbNet* net);

  void highlightSteinerTreeBuilder(const odb::dbNet* net);
  void highlightRectilinearSteinerTree(const odb::dbNet* net, int design);

  // Clear routes in the gui
  void clearRouteGui();
  void clearFastRouteGui();
  void clearSteinerTreeGui();
  // Report the wire length on each layer.
  void reportLayerWireLengths();
  odb::Rect globalRoutingToBox(const GSegment& route);

 protected:
  // Net functions
  int getNetCount() const;
  void reserveNets(size_t net_count);
  Net* addNet(odb::dbNet* db_net);
  int getMaxNetDegree();
  friend class AntennaRepair;

 private:
  void clearObjects();
  void applyAdjustments(int min_routing_layer, int max_routing_layer);
  // main functions
  void initCoreGrid(int max_routing_layer);
  void initRoutingLayers();
  std::vector<std::pair<int, int>> calcLayerPitches(int max_layer);
  void initRoutingTracks(int max_routing_layer);
  void setCapacities(int min_routing_layer, int max_routing_layer);
  void initializeNets(std::vector<Net*>& nets);
  void computeGridAdjustments(int min_routing_layer, int max_routing_layer);
  void computeTrackAdjustments(int min_routing_layer, int max_routing_layer);
  void computeUserGlobalAdjustments(int min_routing_layer,
                                    int max_routing_layer);
  void computeUserLayerAdjustments(int max_routing_layer);
  void computeRegionAdjustments(const odb::Rect& region,
                                int layer,
                                float reduction_percentage);
  void computeObstructionsAdjustments();
  void computeWirelength();
  std::vector<Pin*> getAllPorts();
  int computeTrackConsumption(const Net* net,
                              std::vector<int>& edge_costs_per_layer);

  // aux functions
  void findPins(Net* net);
  void findPins(Net* net, std::vector<RoutePt>& pins_on_grid, int& root_idx);
  odb::dbTechLayer* getRoutingLayerByIndex(int index);
  RoutingTracks getRoutingTracksByIndex(int layer);
  void addGuidesForLocalNets(odb::dbNet* db_net,
                             GRoute& route,
                             int min_routing_layer,
                             int max_routing_layer);
  void addGuidesForPinAccess(odb::dbNet* db_net, GRoute& route);
  void addRemainingGuides(NetRouteMap& routes,
                          std::vector<Net*>& nets,
                          int min_routing_layer,
                          int max_routing_layer);
  void connectPadPins(NetRouteMap& routes);
  void mergeBox(std::vector<odb::Rect>& guide_box);
  bool segmentsConnect(const GSegment& seg0,
                       const GSegment& seg1,
                       GSegment& new_seg,
                       const std::map<RoutePt, int>& segs_at_point);
  void mergeSegments(const std::vector<Pin>& pins, GRoute& route);
  bool pinOverlapsWithSingleTrack(const Pin& pin, odb::Point& track_position);
  GSegment createFakePin(Pin pin, odb::Point& pin_position, odb::dbTechLayer* layer);
  odb::Point findFakePinPosition(Pin& pin, odb::dbNet* db_net);
  void initAdjustments();
  odb::Point getRectMiddle(const odb::Rect& rect);
  NetRouteMap findRouting(std::vector<Net*>& nets,
                          int min_routing_layer,
                          int max_routing_layer);
  void print(GRoute& route);
  void reportLayerSettings(int min_routing_layer, int max_routing_layer);
  void reportResources();
  void reportCongestion();

  // check functions
  void checkPinPlacement();

  // antenna functions
  void addLocalConnections(NetRouteMap& routes);
  void mergeResults(NetRouteMap& routes);
  Capacities saveCapacities(int previous_min_layer, int previous_max_layer);
  void restoreCapacities(Capacities capacities,
                         int previous_min_layer,
                         int previous_max_layer);
  int getEdgeResource(int x1,
                      int y1,
                      int x2,
                      int y2,
                      odb::dbTechLayer* tech_layer,
                      odb::dbGCellGrid* gcell_grid);
  void removeDirtyNetsRouting();
  void updateDirtyNets();

  // db functions
  void initGrid(int max_layer);
  void initRoutingLayers(std::map<int, odb::dbTechLayer*>& routing_layers);
  void initRoutingTracks(std::vector<RoutingTracks>& routing_tracks,
                         int max_layer);
  void computeCapacities(int max_layer);
  void computeSpacingsAndMinWidth(int max_layer);
  void initNetlist();
  void addNets(std::set<odb::dbNet*, cmpById>& db_nets);
  Net* getNet(odb::dbNet* db_net);
  void getNetsByType(NetType type, std::vector<Net*>& nets);
  void initObstructions();
  void findLayerExtensions(std::vector<int>& layer_extensions);
  int findObstructions(odb::Rect& die_area);
  int findInstancesObstructions(odb::Rect& die_area,
                                const std::vector<int>& layer_extensions);
  void findNetsObstructions(odb::Rect& die_area);
  int computeMaxRoutingLayer();
  std::map<int, odb::dbTechVia*> getDefaultVias(int max_routing_layer);
  void makeItermPins(Net* net, odb::dbNet* db_net, const odb::Rect& die_area);
  void makeBtermPins(Net* net, odb::dbNet* db_net, const odb::Rect& die_area);
  void initClockNets();
  bool isClkTerm(odb::dbITerm* iterm, sta::dbNetwork* network);
  bool isNonLeafClock(odb::dbNet* db_net);

  ord::OpenRoad* openroad_;
  utl::Logger* logger_;
  gui::Gui* gui_;
  stt::SteinerTreeBuilder* stt_builder_;
  // Objects variables
  FastRouteCore* fastroute_;
  odb::Point grid_origin_;
  GrouteRenderer* groute_renderer_;
  GrouteRenderer* fastroute_renderer_;
  GrouteRenderer* steinertree_renderer_;
  NetRouteMap routes_;

  std::vector<Net>* nets_;
  std::map<odb::dbNet*, Net*> db_net_map_;
  Grid* grid_;
  std::map<int, odb::dbTechLayer*> routing_layers_;
  std::vector<RoutingTracks>* routing_tracks_;

  // Flow variables
  float adjustment_;
  int min_routing_layer_;
  int max_routing_layer_;
  int layer_for_guide_dimension_;
  const int gcells_offset_ = 2;
  int overflow_iterations_;
  bool allow_congestion_;
  std::vector<int> vertical_capacities_;
  std::vector<int> horizontal_capacities_;
  int macro_extension_;

  // Layer adjustment variables
  std::vector<float> adjustments_;

  // Region adjustment variables
  std::vector<RegionAdjustment> region_adjustments_;

  int verbose_;
  int min_layer_for_clock_;
  int max_layer_for_clock_;

  // variables for random grt
  int seed_;
  float caps_perturbation_percentage_;
  int perturbation_amount_;

  // Variables for PADs obstructions handling
  std::map<odb::dbNet*, std::vector<GSegment>> pad_pins_connections_;

  // db variables
  sta::dbSta* sta_;
  odb::dbDatabase* db_;
  odb::dbBlock* block_;

  std::set<odb::dbNet*> dirty_nets_;
};

std::string getITermName(odb::dbITerm* iterm);

}  // namespace grt
