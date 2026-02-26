// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "grt/GRoute.h"
#include "grt/PinGridLocation.h"
#include "grt/RoutePt.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/dbObject.h"
#include "odb/geom.h"

using AdjacencyList = std::vector<std::vector<int>>;

namespace utl {
class Logger;
class CallBackHandler;
}  // namespace utl

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

namespace sta {
class dbSta;
class dbNetwork;
class SpefWriter;
}  // namespace sta

namespace grt {

class FastRouteCore;
class CUGR;
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

struct RoutePointPins
{
  std::vector<Pin*> pins;
  bool connected = false;
};

enum class NetType
{
  Clock,
  Signal,
  Antenna,
  All
};

using Guides = std::vector<std::pair<int, odb::Rect>>;
using LayerId = int;
using TileSet = std::set<std::pair<int, int>>;
using RoutePointToPinsMap = std::map<RoutePt, RoutePointPins>;
using PointPair = std::pair<odb::Point, odb::Point>;

class GlobalRouter
{
 public:
  GlobalRouter(utl::Logger* logger,
               utl::CallBackHandler* callback_handler,
               stt::SteinerTreeBuilder* stt_builder,
               odb::dbDatabase* db,
               sta::dbSta* sta,
               ant::AntennaChecker* antenna_checker,
               dpl::Opendp* opendp);
  ~GlobalRouter();

  void initGui(std::unique_ptr<AbstractRoutingCongestionDataSource>
                   routing_congestion_data_source,
               std::unique_ptr<AbstractRoutingCongestionDataSource>
                   routing_congestion_data_source_rudy);

  void clear();

  void setAdjustment(float adjustment);
  void setMinRoutingLayer(int min_layer);
  void setMaxRoutingLayer(int max_layer);
  int getMinRoutingLayer();
  int getMaxRoutingLayer();
  void setMinLayerForClock(int min_layer);
  void setMaxLayerForClock(int max_layer);
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
  void setVerbose(bool v);
  void setCongestionIterations(int iterations);
  void setCongestionReportIterStep(int congestion_report_iter_step);
  void setCongestionReportFile(const char* file_name);
  void setGridOrigin(int x, int y);
  void setAllowCongestion(bool allow_congestion);
  void setResistanceAware(bool resistance_aware);
  void setMacroExtension(int macro_extension);
  void setUseCUGR(bool use_cugr) { use_cugr_ = use_cugr; };
  void setSkipLargeFanoutNets(int skip_large_fanout)
  {
    skip_large_fanout_ = skip_large_fanout;
  };

  void setInfiniteCapacity(bool infinite_capacity);

  // flow functions
  void readGuides(const char* file_name);
  void loadGuidesFromDB();
  void ensurePinsPositions(odb::dbNet* db_net);
  bool findCoveredAccessPoint(const Net* net, Pin& pin);
  void saveGuidesFromFile(std::unordered_map<odb::dbNet*, Guides>& guides);
  void saveGuides(const std::vector<odb::dbNet*>& nets);
  void writeSegments(const char* file_name);
  void readSegments(const char* file_name);
  bool netIsCovered(odb::dbNet* db_net, std::string& pins_not_covered);
  bool segmentIsLine(const GSegment& segment);
  bool segmentCoversPin(const GSegment& segment, const Pin& pin);
  AdjacencyList buildNetGraph(odb::dbNet* net);
  bool isConnected(odb::dbNet* net);
  bool segmentsConnect(const GSegment& segment1, const GSegment& segment2);
  bool isCoveringPin(Net* net, GSegment& segment);
  std::vector<Net*> initFastRoute(int min_routing_layer,
                                  int max_routing_layer,
                                  bool check_pin_placement = true);
  void initFastRouteIncr(std::vector<Net*>& nets);
  // Return GRT layer lengths in dbu's for db_net's route indexed by routing
  // layer.
  std::vector<int> routeLayerLengths(odb::dbNet* db_net);
  void startIncremental();
  void endIncremental(bool save_guides = false);
  void globalRoute(bool save_guides = false);
  void saveCongestion();
  NetRouteMap& getRoutes();
  NetRouteMap getPartialRoutes();
  Net* getNet(odb::dbNet* db_net);
  int getTileSize() const;
  bool isNonLeafClock(odb::dbNet* db_net);

  bool hasAvailableResources(bool is_horizontal,
                             const int& pos_x,
                             const int& pos_y,
                             const int& layer_level,
                             odb::dbNet* db_net);
  odb::Point getPositionOnGrid(const odb::Point& real_position);
  int repairAntennas(odb::dbMTerm* diode_mterm,
                     int iterations,
                     float ratio_margin,
                     bool jumper_only,
                     bool diode_only,
                     int num_threads = 1);
  void updateResources(const int& init_x,
                       const int& init_y,
                       const int& final_x,
                       const int& final_y,
                       const int& layer_level,
                       int used,
                       odb::dbNet* db_net);
  void updateFastRouteGridsLayer(const int& init_x,
                                 const int& init_y,
                                 const int& final_x,
                                 const int& final_y,
                                 const int& layer_level,
                                 const int& new_layer_level,
                                 odb::dbNet* db_net);
  // Incremental global routing functions.
  // See class IncrementalGRoute.
  void addDirtyNet(odb::dbNet* net);
  std::set<odb::dbNet*> getDirtyNets() { return dirty_nets_; }
  // check_antennas
  bool haveRoutes();
  bool haveDbGuides();
  bool designIsPlaced();
  bool haveDetailedRoutes();
  bool haveDetailedRoutes(const std::vector<odb::dbNet*>& db_nets);

  void addNetToRoute(odb::dbNet* db_net);
  std::vector<odb::dbNet*> getNetsToRoute();
  void mergeNetsRouting(odb::dbNet* db_net1, odb::dbNet* db_net2);
  bool connectRouting(odb::dbNet* db_net1, odb::dbNet* db_net2);
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
  void reportLayerWireLengths(bool global_route, bool detailed_route);
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

  // Report wire resistance
  float getLayerResistance(int layer, int length, odb::dbNet* net);
  float getViaResistance(int from_layer, int to_layer);
  double dbuToMicrons(int dbu);
  float estimatePathResistance(odb::dbObject* pin1,
                               odb::dbObject* pin2,
                               bool verbose = false);
  float estimatePathResistance(odb::dbObject* pin1,
                               odb::dbObject* pin2,
                               odb::dbTechLayer* layer1,
                               odb::dbTechLayer* layer2,
                               bool verbose = false);

  bool findPinAccessPointPositions(
      const Pin& pin,
      std::map<int, std::vector<PointPair>>& ap_positions,
      bool all_access_points = false);
  void getNetLayerRange(odb::dbNet* db_net, int& min_layer, int& max_layer);
  void getGridSize(int& x_grids, int& y_grids);
  int getGridTileSize();
  void getMinMaxLayer(int& min_layer, int& max_layer);
  void getCapacityReductionData(CapacityReductionData& cap_red_data);
  bool isInitialized() const { return initialized_; }
  bool isCongested() const { return is_congested_; }
  void setDbBlock(odb::dbBlock* block) { block_ = block; }

  void setRenderer(std::unique_ptr<AbstractGrouteRenderer> groute_renderer);
  AbstractGrouteRenderer* getRenderer();

  odb::dbDatabase* db() const { return db_; }
  FastRouteCore* fastroute() const { return fastroute_; }
  Rudy* getRudy();

  void writePinLocations(const char* file_name);

 private:
  void finishGlobalRouting(bool save_guides = false);
  // Net functions
  Net* addNet(odb::dbNet* db_net);
  void removeNet(odb::dbNet* db_net);
  void updateNetPins(Net* net);

  void getCongestionNets(std::set<odb::dbNet*>& congestion_nets);
  void applyAdjustments(int min_routing_layer, int max_routing_layer);
  // main functions
  void initCoreGrid(int max_routing_layer);
  void initRoutingLayers(int min_routing_layer, int max_routing_layer);
  void checkAdjacentLayersDirection(int min_routing_layer,
                                    int max_routing_layer);
  std::vector<std::pair<int, int>> calcLayerPitches(int max_layer);
  void initRoutingTracks(int max_routing_layer);
  void setCapacities(int min_routing_layer, int max_routing_layer);
  int computeGCellCapacity(int x,
                           int y,
                           int track_init,
                           int track_pitch,
                           int track_count,
                           bool horizontal);
  odb::Rect getGCellRect(int x, int y);
  void initNetlist(std::vector<Net*>& nets, bool incremental = false);
  void makeFastrouteNet(Net* net);
  bool pinPositionsChanged(Net* net);
  bool newPinOnGrid(Net* net, std::multiset<RoutePt>& last_pos);
  std::vector<LayerId> findTransitionLayers();
  void adjustTransitionLayers(
      const std::vector<LayerId>& transition_layers,
      std::map<int, std::vector<odb::Rect>>& layer_obs_map);
  void adjustTileSet(const TileSet& tiles_to_reduce,
                     odb::dbTechLayer* tech_layer);
  void computeUserGlobalAdjustments(int min_routing_layer,
                                    int max_routing_layer);
  void computeUserLayerAdjustments(int min_routing_layer,
                                   int max_routing_layer);
  void computeRegionAdjustments(const odb::Rect& region,
                                int layer,
                                float reduction_percentage);
  void applyObstructionAdjustment(const odb::Rect& obstruction,
                                  odb::dbTechLayer* tech_layer,
                                  bool is_macro = false,
                                  bool release = false);
  void savePositionWithReducedResources(const odb::Rect& rect,
                                        odb::dbTechLayer* tech_layer,
                                        odb::dbNet* db_net);
  void addResourcesForPinAccess(const std::vector<Net*>& nets);
  bool isPinReachable(const Pin& pin, const odb::Point& pos_on_grid);
  int computeNetWirelength(odb::dbNet* db_net);
  void computeWirelength();
  std::vector<Pin*> getAllPorts();
  void computeTrackConsumption(const Net* net,
                               int8_t& track_consumption,
                               std::vector<int8_t>*& edge_costs_per_layer);

  // aux functions
  std::vector<RoutePt> findOnGridPositions(const Pin& pin,
                                           bool& has_access_points,
                                           odb::Point& pos_on_grid,
                                           bool ignore_db_access_points
                                           = false);
  int getNetMaxRoutingLayer(const Net* net);
  void findPins(Net* net);
  void computePinPositionOnGrid(std::vector<RoutePt>& pin_positions_on_grid,
                                Pin& pin,
                                odb::Point& pos_on_grid,
                                bool has_access_points);
  void updatePinAccessPoints();
  void suggestAdjustment();
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
  RoutePointToPinsMap findRoutePtPins(Net* net);
  void addPinsConnectedToGuides(RoutePointToPinsMap& point_to_pins,
                                const RoutePt& route_pt,
                                odb::dbGuide* guide);

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
  void findTrackPitches(int max_layer);
  std::vector<Net*> findNets(bool init_clock_nets);
  void findClockNets(const std::vector<Net*>& nets,
                     std::set<odb::dbNet*>& clock_nets);
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
  void makeItermPins(Net* net, odb::dbNet* db_net, const odb::Rect& die_area);
  void makeBtermPins(Net* net, odb::dbNet* db_net, const odb::Rect& die_area);
  void initClockNets();
  bool isClkTerm(odb::dbITerm* iterm, sta::dbNetwork* network);
  void initGridAndNets();
  void configFastRoute();

  utl::Logger* logger_;
  utl::CallBackHandler* callback_handler_;
  stt::SteinerTreeBuilder* stt_builder_;
  ant::AntennaChecker* antenna_checker_;
  dpl::Opendp* opendp_;
  // Objects variables
  FastRouteCore* fastroute_;
  CUGR* cugr_;
  odb::Point grid_origin_;
  std::unique_ptr<AbstractGrouteRenderer> groute_renderer_;
  NetRouteMap routes_;
  NetRouteMap partial_routes_;

  std::map<odb::dbNet*, Net*> db_net_map_;
  Grid* grid_;
  std::map<int, odb::dbTechLayer*> routing_layers_;
  std::vector<RoutingTracks> routing_tracks_;

  // Flow variables
  bool infinite_capacity_;
  float adjustment_;
  int congestion_iterations_{50};
  int congestion_report_iter_step_;
  bool allow_congestion_;
  bool resistance_aware_{false};
  std::vector<int> vertical_capacities_;
  std::vector<int> horizontal_capacities_;
  int macro_extension_;
  bool initialized_;
  int total_diodes_count_;
  bool is_congested_{false};
  bool use_cugr_{false};
  int skip_large_fanout_{std::numeric_limits<int>::max()};
  bool has_macros_or_pads_{false};

  // Region adjustment variables
  std::vector<RegionAdjustment> region_adjustments_;

  bool verbose_;

  // variables for random grt
  int seed_;
  float caps_perturbation_percentage_;
  int perturbation_amount_;

  // Variables for PADs obstructions handling
  std::map<odb::dbNet*, std::vector<GSegment>> pad_pins_connections_;

  // Saving the positions used by nets
  std::map<odb::Point, std::vector<odb::dbNet*>> h_nets_in_pos_;
  std::map<odb::Point, std::vector<odb::dbNet*>> v_nets_in_pos_;

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
  bool is_incremental_;

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
  void inDbNetPostMerge(odb::dbNet* preserved_net,
                        odb::dbNet* removed_net) override;

  void inDbITermPreDisconnect(odb::dbITerm* iterm) override;
  void inDbITermPostConnect(odb::dbITerm* iterm) override;
  void inDbITermPostSetAccessPoints(odb::dbITerm* iterm) override;

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
