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
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "GRoute.h"
#include "RoutePt.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "sta/Liberty.hh"

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

namespace ant {
class AntennaChecker;
}

namespace dpl {
class Opendp;
}

namespace rsz {
class Resizer;
}

namespace sta {
class dbSta;
class dbNetwork;
}  // namespace sta

namespace grt {

class FastRouteCore;
class RepairAntennas;
class Grid;
class Pin;
class Net;
class Netlist;
class RoutingTracks;
class SteinerTree;
class RoutePt;
class GrouteRenderer;
class GlobalRouter;
class RoutingCongestionDataSource;

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

struct PinGridLocation
{
  PinGridLocation(odb::dbITerm* iterm, odb::dbBTerm* bterm, odb::Point pt);

  odb::dbITerm* iterm_;
  odb::dbBTerm* bterm_;
  odb::Point pt_;
};

typedef std::vector<std::pair<int, odb::Rect>> Guides;

class GlobalRouter
{
 public:
  GlobalRouter();
  ~GlobalRouter();
  void init(utl::Logger* logger,
            stt::SteinerTreeBuilder* stt_builder,
            odb::dbDatabase* db,
            sta::dbSta* sta,
            rsz::Resizer* resizer,
            ant::AntennaChecker* antenna_checker,
            dpl::Opendp* opendp);
  void clear();

  void setAdjustment(const float adjustment);
  void setMinRoutingLayer(const int min_layer);
  void setMaxRoutingLayer(const int max_layer);
  int getMaxRoutingLayer() const { return max_routing_layer_; }
  void setMinLayerForClock(const int min_layer);
  void setMaxLayerForClock(const int max_layer);
  void setCriticalNetsPercentage(float critical_nets_percentage);
  void addLayerAdjustment(int layer, float reduction_percentage);
  void addRegionAdjustment(int min_x,
                           int min_y,
                           int max_x,
                           int max_y,
                           int layer,
                           float reduction_percentage);
  void setVerbose(const bool v);
  void setOverflowIterations(int iterations);
  void setCongestionReportFile(const char* file_name);
  void setGridOrigin(int x, int y);
  void setAllowCongestion(bool allow_congestion);
  void setMacroExtension(int macro_extension);
  void setPinOffset(int pin_offset);
  int getMinRoutingLayer() const { return min_routing_layer_; }

  // flow functions
  void readGuides(const char* file_name);
  void loadGuidesFromDB();
  void saveGuidesFromFile(std::unordered_map<odb::dbNet*, Guides>& guides);
  void saveGuides();
  bool isCoveringPin(Net* net, GSegment& segment);
  std::vector<Net*> initFastRoute(int min_routing_layer, int max_routing_layer);
  void initFastRouteIncr(std::vector<Net*>& nets);
  void estimateRC();
  void estimateRC(odb::dbNet* db_net);
  // Return GRT layer lengths in dbu's for db_net's route indexed by routing
  // layer.
  std::vector<int> routeLayerLengths(odb::dbNet* db_net);
  void globalRoute(bool save_guides = false);
  void saveCongestion();
  NetRouteMap& getRoutes() { return routes_; }
  bool haveRoutes();
  Net* getNet(odb::dbNet* db_net);
  int getTileSize() const;

  // repair antenna public functions
  void repairAntennas(odb::dbMTerm* diode_mterm,
                      int iterations,
                      float ratio_margin);

  // Incremental global routing functions.
  // See class IncrementalGRoute.
  void addDirtyNet(odb::dbNet* net);
  std::set<odb::dbNet*> getDirtyNets() { return dirty_nets_; }
  // check_antennas
  void makeNetWires();
  void destroyNetWires();

  double dbuToMicrons(int64_t dbu);

  // functions for random grt
  void setSeed(int seed) { seed_ = seed; }
  void setCapacitiesPerturbationPercentage(float percentage);
  void setPerturbationAmount(int perturbation);
  void perturbCapacities();

  void initDebugFastRoute();
  void setDebugNet(const odb::dbNet* net);
  void setDebugSteinerTree(bool steinerTree);
  void setDebugRectilinearSTree(bool rectilinearSTree);
  void setDebugTree2D(bool tree2D);
  void setDebugTree3D(bool tree3D);
  void setSttInputFilename(const char* file_name);

  void saveSttInputFile(Net* net);

  // Highlight route in the gui.
  void highlightRoute(odb::dbNet* net, bool show_pin_locations);

  // Clear routes in the gui
  void clearRouteGui();
  // Report the wire length on each layer.
  void reportNetLayerWirelengths(odb::dbNet* db_net, std::ofstream& out);
  void reportLayerWireLengths();
  odb::Rect globalRoutingToBox(const GSegment& route);
  void boxToGlobalRouting(const odb::Rect& route_bds, int layer, GRoute& route);
  void updateVias();

  // Report wire length
  void reportNetWireLength(odb::dbNet* net,
                           bool global_route,
                           bool detailed_route,
                           bool verbose,
                           const char* file_name);
  void reportNetDetailedRouteWL(odb::dbWire* wire, std::ofstream& out);
  void createWLReportFile(const char* file_name, bool verbose);
  std::vector<PinGridLocation> getPinGridPositions(odb::dbNet* db_net);

  bool pinAccessPointPositions(
      const Pin& pin,
      std::vector<std::pair<odb::Point, odb::Point>>& ap_positions);

 private:
  // Net functions
  Net* addNet(odb::dbNet* db_net);
  void removeNet(odb::dbNet* db_net);

  void applyAdjustments(int min_routing_layer, int max_routing_layer);
  // main functions
  void initCoreGrid(int max_routing_layer);
  void initRoutingLayers();
  std::vector<std::pair<int, int>> calcLayerPitches(int max_layer);
  void initRoutingTracks(int max_routing_layer);
  void averageTrackPattern(odb::dbTrackGrid* grid,
                           bool is_x,
                           int& track_init,
                           int& num_tracks,
                           int& track_step);
  void setCapacities(int min_routing_layer, int max_routing_layer);
  void initNets(std::vector<Net*>& nets);
  bool makeFastrouteNet(Net* net);
  void getNetLayerRange(Net* net, int& min_layer, int& max_layer);
  void computeGridAdjustments(int min_routing_layer, int max_routing_layer);
  void computeTrackAdjustments(int min_routing_layer, int max_routing_layer);
  void computeUserGlobalAdjustments(int min_routing_layer,
                                    int max_routing_layer);
  void computeUserLayerAdjustments(int max_routing_layer);
  void computeRegionAdjustments(const odb::Rect& region,
                                int layer,
                                float reduction_percentage);
  void computePinOffsetAdjustments();
  void applyObstructionAdjustment(const odb::Rect& obstruction,
                                  odb::dbTechLayer* tech_layer);
  int computeNetWirelength(odb::dbNet* db_net);
  void computeWirelength();
  std::vector<Pin*> getAllPorts();
  void computeTrackConsumption(const Net* net,
                               int& track_consumption,
                               std::vector<int>*& edge_costs_per_layer);

  // aux functions
  std::vector<odb::Point> findOnGridPositions(const Pin& pin,
                                              bool& has_access_points,
                                              odb::Point& pos_on_grid);
  int getNetMaxRoutingLayer(const Net* net);
  void findPins(Net* net);
  void findPins(Net* net, std::vector<RoutePt>& pins_on_grid, int& root_idx);
  float getNetSlack(Net* net);
  void computeNetSlacks();
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
  void mergeBox(std::vector<odb::Rect>& guide_box,
                const std::set<odb::Point>& via_positions);
  bool segmentsConnect(const GSegment& seg0,
                       const GSegment& seg1,
                       GSegment& new_seg,
                       const std::map<RoutePt, int>& segs_at_point);
  void mergeSegments(const std::vector<Pin>& pins, GRoute& route);
  bool pinOverlapsWithSingleTrack(const Pin& pin, odb::Point& track_position);
  void createFakePin(Pin pin,
                     odb::Point& pin_position,
                     odb::dbTechLayer* layer,
                     Net* net);
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
  void updateEdgesUsage();
  void updateDbCongestionFromGuides();

  // check functions
  void checkPinPlacement();

  // incremental funcions
  void updateDirtyRoutes();
  void mergeResults(NetRouteMap& routes);
  void updateDirtyNets();
  void updateDbCongestion();

  // db functions
  void initGrid(int max_layer);
  void initRoutingLayers(std::map<int, odb::dbTechLayer*>& routing_layers);
  void computeCapacities(int max_layer);
  void computeSpacingsAndMinWidth(int max_layer);
  std::vector<Net*> initNetlist();
  void computeObstructionsAdjustments();
  void findLayerExtensions(std::vector<int>& layer_extensions);
  int findObstructions(odb::Rect& die_area);
  bool layerIsBlocked(
      int layer,
      odb::dbTechLayerDir& direction,
      const std::unordered_map<int, odb::Rect>& macro_obs_per_layer,
      odb::Rect& extended_obs);
  void extendObstructions(
      std::unordered_map<int, odb::Rect>& macro_obs_per_layer,
      int bottom_layer,
      int top_layer);
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
  int trackSpacing();
  void initGridAndNets();

  utl::Logger* logger_;
  gui::Gui* gui_;
  stt::SteinerTreeBuilder* stt_builder_;
  ant::AntennaChecker* antenna_checker_;
  dpl::Opendp* opendp_;
  rsz::Resizer* resizer_;
  // Objects variables
  FastRouteCore* fastroute_;
  odb::Point grid_origin_;
  GrouteRenderer* groute_renderer_;
  NetRouteMap routes_;

  std::map<odb::dbNet*, Net*, cmpById> db_net_map_;
  Grid* grid_;
  std::map<int, odb::dbTechLayer*> routing_layers_;
  std::vector<RoutingTracks>* routing_tracks_;

  // Flow variables
  float adjustment_;
  int min_routing_layer_;
  int max_routing_layer_;
  int layer_for_guide_dimension_;
  int gcells_offset_;
  int overflow_iterations_;
  bool allow_congestion_;
  std::vector<int> vertical_capacities_;
  std::vector<int> horizontal_capacities_;
  int macro_extension_;

  // Layer adjustment variables
  std::vector<float> adjustments_;

  // Region adjustment variables
  std::vector<RegionAdjustment> region_adjustments_;

  bool verbose_;
  int min_layer_for_clock_;
  int max_layer_for_clock_;
  float critical_nets_percentage_;

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

  RepairAntennas* repair_antennas_;
  std::unique_ptr<RoutingCongestionDataSource> heatmap_;

  // variables congestion report file
  const char* congestion_file_name_;

  friend class IncrementalGRoute;
  friend class GRouteDbCbk;
  friend class RepairAntennas;
};

std::string getITermName(odb::dbITerm* iterm);
std::string getLayerName(int layer_idx, odb::dbDatabase* db);

class GRouteDbCbk : public odb::dbBlockCallBackObj
{
 public:
  GRouteDbCbk(GlobalRouter* grouter);
  virtual void inDbPostMoveInst(odb::dbInst* inst);
  virtual void inDbInstSwapMasterAfter(odb::dbInst* inst);

  virtual void inDbNetDestroy(odb::dbNet* net);
  virtual void inDbNetCreate(odb::dbNet* net);

  virtual void inDbITermPreDisconnect(odb::dbITerm* iterm);
  virtual void inDbITermPostConnect(odb::dbITerm* iterm);

  virtual void inDbBTermPostConnect(odb::dbBTerm* bterm);
  virtual void inDbBTermPreDisconnect(odb::dbBTerm* bterm);

 private:
  void instItermsDirty(odb::dbInst* inst);

  GlobalRouter* grouter_;
};

// Class to save global router state and monitor db updates with callbacks
// to make incremental routing updates.
class IncrementalGRoute
{
 public:
  // Saves global router state and enables db callbacks.
  IncrementalGRoute(GlobalRouter* groute, odb::dbBlock* block);
  // Update global routes for dirty nets.
  void updateRoutes();
  // Disables db callbacks.
  ~IncrementalGRoute();

 private:
  GlobalRouter* groute_;
  GRouteDbCbk db_cbk_;
};

}  // namespace grt
