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
#include "ant/AntennaChecker.hh"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "sta/Liberty.hh"

using AdjacencyList = std::vector<std::vector<int>>;

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
class SpefWriter;
}  // namespace rsz

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
class RoutePt;
class AbstractGrouteRenderer;
class AbstractFastRouteRenderer;
class GlobalRouter;
class AbstractRoutingCongestionDataSource;
class GRouteDbCbk;
class Rudy;

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
using LayerId = int;
using TileSet = std::set<std::pair<int, int>>;

class GlobalRouter : public ant::GlobalRouteSource
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
            dpl::Opendp* opendp,
            std::unique_ptr<AbstractRoutingCongestionDataSource>
                routing_congestion_data_source,
            std::unique_ptr<AbstractRoutingCongestionDataSource>
                routing_congestion_data_source_rudy);

  void clear();

  void setAdjustment(const float adjustment);
  void setMinRoutingLayer(const int min_layer);
  void setMaxRoutingLayer(const int max_layer);
  int getMinRoutingLayer();
  int getMaxRoutingLayer();
  void setMinLayerForClock(const int min_layer);
  void setMaxLayerForClock(const int max_layer);
  int getMinLayerForClock();
  int getMaxLayerForClock();
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
  void setCongestionReportIterStep(int congestion_report_iter_step);
  void setCongestionReportFile(const char* file_name);
  void setGridOrigin(int x, int y);
  void setAllowCongestion(bool allow_congestion);
  void setMacroExtension(int macro_extension);

  // flow functions
  void readGuides(const char* file_name);
  void loadGuidesFromDB();
  void saveGuidesFromFile(std::unordered_map<odb::dbNet*, Guides>& guides);
  void saveGuides();
  void writeSegments(const char* file_name);
  void readSegments(const char* file_name);
  bool netIsCovered(odb::dbNet* db_net, std::string& pins_not_covered);
  bool segmentIsLine(const GSegment& segment);
  bool segmentCoversPin(const GSegment& segment, const Pin& pin);
  AdjacencyList buildNetGraph(odb::dbNet* net);
  bool isConnected(odb::dbNet* net);
  bool segmentsConnect(const GSegment& segment1, const GSegment& segment2);
  bool isCoveringPin(Net* net, GSegment& segment);
  std::vector<Net*> initFastRoute(int min_routing_layer, int max_routing_layer);
  void initFastRouteIncr(std::vector<Net*>& nets);
  void estimateRC(rsz::SpefWriter* spef_writer = nullptr);
  void estimateRC(odb::dbNet* db_net);
  // Return GRT layer lengths in dbu's for db_net's route indexed by routing
  // layer.
  std::vector<int> routeLayerLengths(odb::dbNet* db_net);
  void globalRoute(bool save_guides = false,
                   bool start_incremental = false,
                   bool end_incremental = false);
  void saveCongestion();
  NetRouteMap& getRoutes() { return routes_; }
  Net* getNet(odb::dbNet* db_net);
  int getTileSize() const;
  bool isNonLeafClock(odb::dbNet* db_net);

  // repair antenna public functions
  int repairAntennas(odb::dbMTerm* diode_mterm,
                     int iterations,
                     float ratio_margin,
                     int num_threads = 1);

  // Incremental global routing functions.
  // See class IncrementalGRoute.
  void addDirtyNet(odb::dbNet* net);
  std::set<odb::dbNet*> getDirtyNets() { return dirty_nets_; }
  // check_antennas
  bool haveRoutes() override;
  bool haveDetailedRoutes();
  bool haveDetailedRoutes(const std::vector<odb::dbNet*>& db_nets);
  void makeNetWires() override;
  void destroyNetWires() override;

  void addNetToRoute(odb::dbNet* db_net);
  std::vector<odb::dbNet*> getNetsToRoute();
  void mergeNetsRouting(odb::dbNet* db_net1, odb::dbNet* db_net2);
  void connectRouting(odb::dbNet* db_net1, odb::dbNet* db_net2);
  void findBufferPinPostions(Net* net1,
                             Net* net2,
                             odb::Point& pin_pos1,
                             odb::Point& pin_pos2);
  int findTopLayerOverPosition(const odb::Point& pin_pos, const GRoute& route);
  std::vector<GSegment> createConnectionForPositions(const odb::Point& pin_pos1,
                                                     const odb::Point& pin_pos2,
                                                     int layer1,
                                                     int layer2);
  void insertViasForConnection(std::vector<GSegment>& connection,
                               const odb::Point& via_pos,
                               int layer,
                               int conn_layer);

  void getBlockage(odb::dbTechLayer* layer,
                   int x,
                   int y,
                   uint8_t& blockage_h,
                   uint8_t& blockage_v);

  // functions for random grt
  void setSeed(int seed) { seed_ = seed; }
  void setCapacitiesPerturbationPercentage(float percentage);
  void setPerturbationAmount(int perturbation);
  void perturbCapacities();

  void initDebugFastRoute(std::unique_ptr<AbstractFastRouteRenderer> renderer);
  AbstractFastRouteRenderer* getDebugFastRoute() const;

  void setDebugNet(const odb::dbNet* net);
  void setDebugSteinerTree(bool steinerTree);
  void setDebugRectilinearSTree(bool rectilinearSTree);
  void setDebugTree2D(bool tree2D);
  void setDebugTree3D(bool tree3D);
  void setSttInputFilename(const char* file_name);

  void saveSttInputFile(Net* net);

  // Report the wire length on each layer.
  void reportNetLayerWirelengths(odb::dbNet* db_net, std::ofstream& out);
  void reportLayerWireLengths();
  odb::Rect globalRoutingToBox(const GSegment& route);
  void boxToGlobalRouting(const odb::Rect& route_bds,
                          int layer,
                          int via_layer,
                          GRoute& route);
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

  bool findPinAccessPointPositions(
      const Pin& pin,
      std::vector<std::pair<odb::Point, odb::Point>>& ap_positions);
  void getNetLayerRange(odb::dbNet* db_net, int& min_layer, int& max_layer);
  void getGridSize(int& x_grids, int& y_grids);
  int getGridTileSize();
  void getMinMaxLayer(int& min_layer, int& max_layer);
  void getCapacityReductionData(CapacityReductionData& cap_red_data);
  bool isInitialized() const { return initialized_; }
  void setDbBlock(odb::dbBlock* block) { block_ = block; }

  void setRenderer(std::unique_ptr<AbstractGrouteRenderer> groute_renderer);
  AbstractGrouteRenderer* getRenderer();

  odb::dbDatabase* db() const { return db_; }
  FastRouteCore* fastroute() const { return fastroute_; }
  Rudy* getRudy();

 private:
  // Net functions
  Net* addNet(odb::dbNet* db_net);
  void removeNet(odb::dbNet* db_net);

  void applyAdjustments(int min_routing_layer, int max_routing_layer);
  // main functions
  void initCoreGrid(int max_routing_layer);
  void initRoutingLayers(int min_routing_layer, int max_routing_layer);
  void checkAdjacentLayersDirection(int min_routing_layer,
                                    int max_routing_layer);
  std::vector<std::pair<int, int>> calcLayerPitches(int max_layer);
  void initRoutingTracks(int max_routing_layer);
  void setCapacities(int min_routing_layer, int max_routing_layer);
  void initNetlist(std::vector<Net*>& nets);
  bool makeFastrouteNet(Net* net);
  bool pinPositionsChanged(Net* net);
  bool newPinOnGrid(Net* net, std::multiset<RoutePt>& last_pos);
  std::vector<LayerId> findTransitionLayers();
  void adjustTransitionLayers(
      const std::vector<LayerId>& transition_layers,
      std::map<int, std::vector<odb::Rect>>& layer_obs_map);
  void adjustTileSet(const TileSet& tiles_to_reduce,
                     odb::dbTechLayer* tech_layer);
  void computeGridAdjustments(int min_routing_layer, int max_routing_layer);
  void computeTrackAdjustments(int min_routing_layer, int max_routing_layer);
  void computeUserGlobalAdjustments(int min_routing_layer,
                                    int max_routing_layer);
  void computeUserLayerAdjustments(int max_routing_layer);
  void computeRegionAdjustments(const odb::Rect& region,
                                int layer,
                                float reduction_percentage);
  void applyObstructionAdjustment(const odb::Rect& obstruction,
                                  odb::dbTechLayer* tech_layer,
                                  bool is_macro = false);
  void addResourcesForPinAccess();
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
  void findFastRoutePins(Net* net,
                         std::vector<RoutePt>& pins_on_grid,
                         int& root_idx);
  float getNetSlack(Net* net);
  odb::dbTechLayer* getRoutingLayerByIndex(int index);
  RoutingTracks getRoutingTracksByIndex(int layer);
  void addGuidesForLocalNets(odb::dbNet* db_net,
                             GRoute& route,
                             int min_routing_layer,
                             int max_routing_layer);
  void connectTopLevelPins(odb::dbNet* db_net, GRoute& route);
  void addRemainingGuides(NetRouteMap& routes,
                          std::vector<Net*>& nets,
                          int min_routing_layer,
                          int max_routing_layer);
  void connectPadPins(NetRouteMap& routes);
  bool segmentsConnect(const GSegment& seg0,
                       const GSegment& seg1,
                       GSegment& new_seg,
                       const std::map<RoutePt, int>& segs_at_point);
  void mergeSegments(const std::vector<Pin>& pins, GRoute& route);
  bool pinOverlapsWithSingleTrack(const Pin& pin, odb::Point& track_position);
  odb::Point getRectMiddle(const odb::Rect& rect);
  NetRouteMap findRouting(std::vector<Net*>& nets,
                          int min_routing_layer,
                          int max_routing_layer);
  void print(GRoute& route);
  void printSegment(const GSegment& segment);
  void reportLayerSettings(int min_routing_layer, int max_routing_layer);
  void reportResources();
  void reportCongestion();
  void updateEdgesUsage();
  void updateDbCongestionFromGuides();
  void computeGCellGridPatternFromGuides(
      std::unordered_map<odb::dbNet*, Guides>& guides);
  void fillTileSizeMaps(std::unordered_map<odb::dbNet*, Guides>& net_guides,
                        std::map<int, int>& tile_size_x_map,
                        std::map<int, int>& tile_size_y_map,
                        int& min_loc_x,
                        int& min_loc_y);
  void findTileSize(const std::map<int, int>& tile_size_x_map,
                    const std::map<int, int>& tile_size_y_map,
                    int& tile_size_x,
                    int& tile_size_y);

  // check functions
  void checkPinPlacement();

  // incremental funcions
  std::vector<Net*> updateDirtyRoutes(bool save_guides = false);
  void mergeResults(NetRouteMap& routes);
  void updateDirtyNets(std::vector<Net*>& dirty_nets);
  void shrinkNetRoute(odb::dbNet* db_net);
  void deleteSegment(Net* net, GRoute& segments, int seg_id);
  void destroyNetWire(Net* net);
  void removeWireUsage(odb::dbWire* wire);
  void removeRectUsage(const odb::Rect& rect, odb::dbTechLayer* tech_layer);
  bool isDetailedRouted(odb::dbNet* db_net);
  void updateDbCongestion();

  // db functions
  void initGrid(int max_layer);
  void computeCapacities(int max_layer);
  void findTrackPitches(int max_layer);
  std::vector<Net*> findNets();
  void computeObstructionsAdjustments();
  void findLayerExtensions(std::vector<int>& layer_extensions);
  int findObstructions(odb::Rect& die_area);
  bool layerIsBlocked(int layer,
                      const std::unordered_map<int, std::vector<odb::Rect>>&
                          macro_obs_per_layer,
                      std::vector<odb::Rect>& extended_obs);
  void extendObstructions(
      std::unordered_map<int, std::vector<odb::Rect>>& macro_obs_per_layer,
      int bottom_layer,
      int top_layer);
  int findInstancesObstructions(
      odb::Rect& die_area,
      const std::vector<int>& layer_extensions,
      std::map<int, std::vector<odb::Rect>>& layer_obs_map);
  void findNetsObstructions(odb::Rect& die_area);
  void applyNetObstruction(const odb::Rect& rect,
                           odb::dbTechLayer* tech_layer,
                           const odb::Rect& die_area,
                           odb::dbNet* db_net);
  int computeMaxRoutingLayer();
  std::map<int, odb::dbTechVia*> getDefaultVias(int max_routing_layer);
  void makeItermPins(Net* net, odb::dbNet* db_net, const odb::Rect& die_area);
  void makeBtermPins(Net* net, odb::dbNet* db_net, const odb::Rect& die_area);
  void initClockNets();
  bool isClkTerm(odb::dbITerm* iterm, sta::dbNetwork* network);
  void initGridAndNets();
  void ensureLayerForGuideDimension(int max_routing_layer);
  void configFastRoute();
  void checkOverflow();

  utl::Logger* logger_;
  stt::SteinerTreeBuilder* stt_builder_;
  ant::AntennaChecker* antenna_checker_;
  dpl::Opendp* opendp_;
  rsz::Resizer* resizer_;
  // Objects variables
  FastRouteCore* fastroute_;
  odb::Point grid_origin_;
  std::unique_ptr<AbstractGrouteRenderer> groute_renderer_;
  NetRouteMap routes_;

  std::map<odb::dbNet*, Net*> db_net_map_;
  Grid* grid_;
  std::map<int, odb::dbTechLayer*> routing_layers_;
  std::vector<RoutingTracks> routing_tracks_;

  // Flow variables
  float adjustment_;
  int layer_for_guide_dimension_;
  int overflow_iterations_;
  int congestion_report_iter_step_;
  bool allow_congestion_;
  std::vector<int> vertical_capacities_;
  std::vector<int> horizontal_capacities_;
  int macro_extension_;
  bool initialized_;
  int total_diodes_count_;
  bool incremental_;

  // Region adjustment variables
  std::vector<RegionAdjustment> region_adjustments_;

  bool verbose_;

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
  std::vector<odb::dbNet*> nets_to_route_;

  RepairAntennas* repair_antennas_;
  Rudy* rudy_;
  std::unique_ptr<AbstractRoutingCongestionDataSource> heatmap_;
  std::unique_ptr<AbstractRoutingCongestionDataSource> heatmap_rudy_;

  // variables congestion report file
  const char* congestion_file_name_;

  // incremental grt
  GRouteDbCbk* grouter_cbk_;

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
  void inDbPostMoveInst(odb::dbInst* inst) override;
  void inDbInstSwapMasterAfter(odb::dbInst* inst) override;

  void inDbNetDestroy(odb::dbNet* net) override;
  void inDbNetCreate(odb::dbNet* net) override;
  void inDbNetPreMerge(odb::dbNet* preserved_net,
                       odb::dbNet* removed_net) override;

  void inDbITermPreDisconnect(odb::dbITerm* iterm) override;
  void inDbITermPostConnect(odb::dbITerm* iterm) override;

  void inDbBTermPostConnect(odb::dbBTerm* bterm) override;
  void inDbBTermPreDisconnect(odb::dbBTerm* bterm) override;

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
  std::vector<Net*> updateRoutes(bool save_guides = false);
  // Disables db callbacks.
  ~IncrementalGRoute();

 private:
  GlobalRouter* groute_;
  GRouteDbCbk db_cbk_;
};

}  // namespace grt
