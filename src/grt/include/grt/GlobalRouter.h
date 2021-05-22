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

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "GRoute.h"
#include "opendb/db.h"
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
class RoutingLayer;
class SteinerTree;
class RoutePt;
class GrouteRenderer;


struct RegionAdjustment
{
  odb::Rect region;
  int layer;
  float adjustment;

  RegionAdjustment(int minX, int minY, int maxX, int maxY, int l, float adjst);
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
  GlobalRouter() = default;
  ~GlobalRouter();
  void init(ord::OpenRoad* openroad);
  void init();
  void clear();

  void setAdjustment(const float adjustment);
  void setMinRoutingLayer(const int minLayer);
  void setMaxRoutingLayer(const int maxLayer);
  void setMinLayerForClock(const int minLayer);
  void setMaxLayerForClock(const int maxLayer);
  void setAlpha(const float alpha);
  unsigned getDbId();
  void addLayerAdjustment(int layer, float reductionPercentage);
  void addRegionAdjustment(int minX,
                           int minY,
                           int maxX,
                           int maxY,
                           int layer,
                           float reductionPercentage);
  void addAlphaForNet(char* netName, float alpha);
  void setVerbose(const int v);
  void setOverflowIterations(int iterations);
  void setGridOrigin(long x, long y);
  void setPDRevForHighFanout(int pdRevForHighFanout);
  void setAllowOverflow(bool allowOverflow);
  void setMacroExtension(int macroExtension);
  void printGrid();

  // flow functions
  void writeGuides(const char* fileName);
  std::vector<Net*> startFastRoute(int minRoutingLayer, int maxRoutingLayer, NetType type);
  void estimateRC();
  void run();
  void globalRouteClocksSeparately();
  void globalRoute();
  NetRouteMap& getRoutes() { return _routes; }
  bool haveRoutes() const { return !_routes.empty(); }

  // repair antenna public functions
  void repairAntennas(sta::LibertyPort* diodePort);
  void addDirtyNet(odb::dbNet* net);

  double dbuToMicrons(int64_t dbu);

  // route clock nets public functions
  void routeClockNets();

  // Highlight route in the gui.
  void highlightRoute(const odb::dbNet *net);
  // Report the wire length on each layer.
  void reportLayerWireLengths();

 protected:
  // Net functions
  int getNetCount() const;
  void reserveNets(size_t net_count);
  Net* addNet(odb::dbNet* db_net);
  int getMaxNetDegree();
  friend class AntennaRepair;

 private:
  void makeComponents();
  void deleteComponents();
  void clearObjects();
  void applyAdjustments(int minRoutingLayer, int maxRoutingLayer);
  // main functions
  void initCoreGrid(int maxRoutingLayer);
  void initRoutingLayers();
  std::vector<std::pair<int, int>> calcLayerPitches(int maxLayer);
  void initRoutingTracks(int maxRoutingLayer);
  void setCapacities(int minRoutingLayer, int maxRoutingLayer);
  void setSpacingsAndMinWidths();
  void initializeNets(std::vector<Net*>& nets);
  void computeGridAdjustments(int minRoutingLayer, int maxRoutingLayer);
  void computeTrackAdjustments(int minRoutingLayer, int maxRoutingLayer);
  void computeUserGlobalAdjustments(int minRoutingLayer, int maxRoutingLayer);
  void computeUserLayerAdjustments(int maxRoutingLayer);
  void computeRegionAdjustments(const odb::Rect& region,
                                int layer,
                                float reductionPercentage);
  void computeObstructionsAdjustments();
  void computeWirelength();
  std::vector<Pin*> getAllPorts();
  int computeTrackConsumption(const Net* net, std::vector<int>& edgeCostsPerLayer);

  // aux functions
  void findPins(Net* net);
  void findPins(Net* net, std::vector<RoutePt>& pinsOnGrid);
  RoutingLayer getRoutingLayerByIndex(int index);
  RoutingTracks getRoutingTracksByIndex(int layer);
  void addGuidesForLocalNets(odb::dbNet* db_net, GRoute& route,
                             int minRoutingLayer, int maxRoutingLayer);
  void addGuidesForPinAccess(odb::dbNet* db_net, GRoute& route);
  void addRemainingGuides(NetRouteMap& routes, std::vector<Net*>& nets,
                          int minRoutingLayer, int maxRoutingLayer);
  void connectPadPins(NetRouteMap& routes);
  void mergeBox(std::vector<odb::Rect>& guideBox);
  odb::Rect globalRoutingToBox(const GSegment& route);
  bool segmentsConnect(const GSegment& seg0,
                       const GSegment& seg1,
                       GSegment& newSeg,
                       const std::map<RoutePt, int>& segsAtPoint);
  void mergeSegments(const std::vector<Pin>& pins, GRoute& route);
  bool pinOverlapsWithSingleTrack(const Pin& pin, odb::Point& trackPosition);
  GSegment createFakePin(Pin pin, odb::Point& pinPosition, RoutingLayer layer);
  odb::Point findFakePinPosition(Pin& pin, odb::dbNet* db_net);
  void initAdjustments();
  odb::Point getRectMiddle(const odb::Rect& rect);
  NetRouteMap findRouting(std::vector<Net*>& nets, int minRoutingLayer, int maxRoutingLayer);
  void print(GRoute& route);
  void reportLayerSettings(int minRoutingLayer, int maxRoutingLayer);
  void reportResources();
  void reportCongestion();

  // check functions
  void checkPinPlacement();

  // antenna functions
  void addLocalConnections(NetRouteMap& routes);
  void mergeResults(NetRouteMap& routes);
  Capacities saveCapacities(int previousMinLayer, int previousMaxLayer);
  void restoreCapacities(Capacities capacities, int previousMinLayer, int previousMaxLayer);
  int getEdgeResource(int x1, int y1, int x2, int y2,
                      odb::dbTechLayer* tech_layer, odb::dbGCellGrid* gcell_grid);
  void removeDirtyNetsRouting();
  void updateDirtyNets();

  // db functions
  void initGrid(int maxLayer);
  void initRoutingLayers(std::vector<RoutingLayer>& routingLayers);
  void initRoutingTracks(std::vector<RoutingTracks>& allRoutingTracks,
                         int maxLayer);
  void computeCapacities(int maxLayer);
  void computeSpacingsAndMinWidth(int maxLayer);
  void initNetlist();
  void addNets(std::set<odb::dbNet*, cmpById>& db_nets);
  Net* getNet(odb::dbNet* db_net);
  void getNetsByType(NetType type, std::vector<Net*>& nets);
  void initObstructions();
  void findLayerExtensions(std::vector<int>& layerExtensions);
  int findObstructions(odb::Rect& dieArea);
  int findInstancesObstructions(odb::Rect& dieArea,
                              const std::vector<int>& layerExtensions);
  void findNetsObstructions(odb::Rect& dieArea);
  int computeMaxRoutingLayer();
  std::map<int, odb::dbTechVia*> getDefaultVias(int maxRoutingLayer);
  void makeItermPins(Net* net, odb::dbNet* db_net, const odb::Rect& dieArea);
  void makeBtermPins(Net* net, odb::dbNet* db_net, const odb::Rect& dieArea);
  void initClockNets();
  bool isClkTerm(odb::dbITerm* iterm, sta::dbNetwork* network);
  bool clockHasLeafITerm(odb::dbNet* db_net);
  void setSelectedMetal(int metal) { selectedMetal = metal; }

  ord::OpenRoad* _openroad;
  utl::Logger *_logger;
  gui::Gui *_gui;
  // Objects variables
  FastRouteCore* _fastRoute;
  odb::Point* _gridOrigin;
  GrouteRenderer *_groute_renderer;
  NetRouteMap _routes;

  std::vector<Net>* _nets;
  std::map<odb::dbNet*, Net*> _db_net_map;
  Grid* _grid;
  std::vector<RoutingLayer>* _routingLayers;
  std::vector<RoutingTracks>* _allRoutingTracks;

  // Flow variables
  float _adjustment;
  int _minRoutingLayer;
  int _maxRoutingLayer;
  const int _selectedMetal = 3;
  const int _gcellsOffset = 2;
  int _overflowIterations;
  int _pdRevForHighFanout;
  bool _allowOverflow;
  std::vector<int> _vCapacities;
  std::vector<int> _hCapacities;
  int _macroExtension;

  // Layer adjustment variables
  std::vector<float> _adjustments;

  // Region adjustment variables
  std::vector<RegionAdjustment> _regionAdjustments;

  // Clock net routing variables
  bool _pdRev;
  float _alpha;
  int _verbose;
  std::map<std::string, float> _netsAlpha;
  int _minLayerForClock = -1;
  int _maxLayerForClock = -2;

  // temporary for congestion driven replace
  int _numAdjusts = 0;

  // Variables for PADs obstructions handling
  std::map<odb::dbNet*, std::vector<GSegment>> _padPinsConnections;

  // db variables
  sta::dbSta* _sta;
  int selectedMetal = 3;
  odb::dbDatabase* _db;
  odb::dbBlock* _block;

  std::set<odb::dbNet*> _dirtyNets;
};

std::string getITermName(odb::dbITerm* iterm);

}  // namespace grt
