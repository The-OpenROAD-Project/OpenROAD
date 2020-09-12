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

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "opendb/db.h"
#include "GRoute.h"

namespace ord {
class OpenRoad;
}

namespace odb {
class dbDatabase;
class dbTech;
class dbBlock;
class dbDatabase;
}  // namespace odb

namespace sta {
  class dbSta;
}

namespace FastRoute {

class FT;
class AntennaRepair;
class Box;
class Coordinate;
class DBWrapper;
class Grid;
class Pin;
class Net;
class Netlist;
class RoutingTracks;
class RoutingLayer;
class SteinerTree;
struct NET;
struct PIN;

class GlobalRouter
{
 public:
  struct ADJUSTMENT_
  {
    int firstX;
    int firstY;
    int firstLayer;
    int finalX;
    int finalY;
    int finalLayer;
    int edgeCapacity;
  };

  struct ROUTE_
  {
    int gridCountX;
    int gridCountY;
    int numLayers;
    std::vector<int> verticalEdgesCapacities;
    std::vector<int> horizontalEdgesCapacities;
    std::vector<int> minWireWidths;
    std::vector<int> minWireSpacings;
    std::vector<int> viaSpacings;
    long gridOriginX;
    long gridOriginY;
    long tileWidth;
    long tileHeight;
    int blockPorosity;
    int numAdjustments;
    std::vector<ADJUSTMENT_> adjustments;
  };

  GlobalRouter() = default;
  ~GlobalRouter();
  void init(ord::OpenRoad* openroad);
  void init();
  void clear();

  void setAdjustment(const float adjustment);
  void setMinRoutingLayer(const int minLayer);
  void setMaxRoutingLayer(const int maxLayer);
  void setUnidirectionalRoute(const bool unidirRoute);
  void setAlpha(const float alpha);
  void setPitchesInTile(const int pitchesInTile);
  void setSeed(unsigned seed);
  unsigned getDbId();
  void addLayerAdjustment(int layer, float reductionPercentage);
  void addRegionAdjustment(int minX,
                           int minY,
                           int maxX,
                           int maxY,
                           int layer,
                           float reductionPercentage);
  void setLayerPitch(int layer, float pitch);
  void addAlphaForNet(char* netName, float alpha);
  void setVerbose(const int v);
  void setOverflowIterations(int iterations);
  void setGridOrigin(long x, long y);
  void setPDRevForHighFanout(int pdRevForHighFanout);
  void setAllowOverflow(bool allowOverflow);
  void setReportCongestion(char* congestFile);
  void printGrid();
  void printHeader();
  void setClockNetsRouteFlow(bool clockFlow);
  void setMinLayerForClock(int minLayer);

  // flow functions
  void enableAntennaAvoidance(char* diodeCellName, char* diodePinName);
  void writeGuides(const char* fileName);
  void startFastRoute();
  void estimateRC();
  void runFastRoute();
  NetRouteMap& getRoutes() { return _routes; }
  bool haveRoutes() const { return !_routes.empty(); }

  // congestion drive replace functions
  ROUTE_ getRoute();

  // estimate_rc functions
  void getLayerRC(unsigned layerId, float& r, float& c);
  void getCutLayerRes(unsigned belowLayerId, float& r);
  float dbuToMeters(unsigned dbu);

protected:
  // Net functions
  int getNetCount() const;
  void reserveNets(size_t net_count);
  Net* addNet(odb::dbNet* net);
  int getMaxNetDegree();
  friend class AntennaRepair;

 private:
  void makeComponents();
  void deleteComponents();
  // main functions
  void initCoreGrid();
  void initRoutingLayers();
  void initRoutingTracks();
  void setCapacities();
  void setSpacingsAndMinWidths();
  void initializeNets(bool reroute);
  void computeGridAdjustments();
  void computeTrackAdjustments();
  void computeUserGlobalAdjustments();
  void computeUserLayerAdjustments();
  void computeRegionAdjustments(const Coordinate& lowerBound,
                                const Coordinate& upperBound,
                                int layer,
                                float reductionPercentage);
  void computeObstaclesAdjustments();
  void computeWirelength();
  std::vector<Pin*> getAllPorts();


  // aux functions
  RoutingLayer getRoutingLayerByIndex(int index);
  RoutingTracks getRoutingTracksByIndex(int layer);
  void addRemainingGuides(NetRouteMap& routes);
  void connectPadPins(NetRouteMap& routes);
  void mergeBox(std::vector<Box>& guideBox);
  Box globalRoutingToBox(const FastRoute::GSegment& route);
  using Point = std::tuple<long, long, int>;  // x, y, layer
  bool segmentsConnect(const GSegment& seg0,
                       const GSegment& seg1,
                       GSegment& newSeg,
                       const std::map<Point, int>& segsAtPoint);
  void mergeSegments();
  void mergeSegments(GRoute& route);
  bool pinOverlapsWithSingleTrack(const Pin& pin, Coordinate& trackPosition);
  GSegment createFakePin(Pin pin, Coordinate& pinPosition, RoutingLayer layer);
  bool checkSignalType(const Net &net);
  void initAdjustments();
  void initPitches();

  // check functions
  void checkPinPlacement();

  // antenna functions
  void addLocalConnections(NetRouteMap& routes);
  void mergeResults(NetRouteMap& routes);
  void runAntennaAvoidanceFlow();
  void runClockNetsRouteFlow();
  void restartFastRoute();
  void getPreviousCapacities(int previousMinLayer);
  void restorePreviousCapacities(int previousMinLayer);

  // db functions
  void initGrid(int maxLayer);
  void initRoutingLayers(std::vector<RoutingLayer>& routingLayers);
  void initRoutingTracks(std::vector<RoutingTracks>& allRoutingTracks,
                         int maxLayer,
                         std::vector<float> layerPitches);
  void computeCapacities(int maxLayer, std::vector<float> layerPitches);
  void computeSpacingsAndMinWidth(int maxLayer);
  void initNetlist(bool reroute);
  void addNets(std::vector<odb::dbNet*> nets);
  Net* getNet(odb::dbNet* db_net);
  void initObstacles();
  int computeMaxRoutingLayer();
  std::set<int> findTransitionLayers(int maxRoutingLayer);
  std::map<int, odb::dbTechVia*> getDefaultVias(int maxRoutingLayer);
  void makeItermPins(Net* net, odb::dbNet* db_net, Box& dieArea);
  void makeBtermPins(Net* net, odb::dbNet* db_net, Box& dieArea);
  void initClockNets();
  void setSelectedMetal(int metal) { selectedMetal = metal; }
  void setDirtyNets(std::vector<odb::dbNet*> dirtyNets) { _dirtyNets = dirtyNets; }

  ord::OpenRoad* _openroad;
  // Objects variables
  FT* _fastRoute = nullptr;
  Coordinate* _gridOrigin = nullptr;
  NetRouteMap _routes;

  std::vector<Net> *_nets;
  std::map<odb::dbNet*, Net*> _db_net_map;
  Grid* _grid = nullptr;
  std::vector<RoutingLayer>* _routingLayers = nullptr;
  std::vector<RoutingTracks>* _allRoutingTracks = nullptr;

  // Flow variables
  std::string _congestFile;
  float _adjustment;
  int _minRoutingLayer;
  int _maxRoutingLayer;
  bool _unidirectionalRoute;
  int _fixLayer;
  const int _selectedMetal = 3;
  const float transitionLayerAdjust = 0.6;
  const int _gcellsOffset = 2;
  int _overflowIterations;
  int _pdRevForHighFanout;
  bool _allowOverflow;
  bool _reportCongest;
  std::vector<int> _vCapacities;
  std::vector<int> _hCapacities;
  unsigned _seed;

  // Layer adjustment variables
  std::vector<float> _adjustments;

  // Region adjustment variables
  std::vector<int> regionsMinX;
  std::vector<int> regionsMinY;
  std::vector<int> regionsMaxX;
  std::vector<int> regionsMaxY;
  std::vector<int> regionsLayer;
  std::vector<float> regionsReductionPercentage;

  // Pitches variables
  std::vector<float> _layerPitches;

  // Clock net routing variables
  bool _pdRev;
  float _alpha;
  int _verbose;
  std::map<std::string, float> _netsAlpha;
  bool _clockNetsRouteFlow = false;
  bool _onlyClockNets = false;
  bool _onlySignalNets = false;
  int _minLayerForClock;

  // Antenna variables
  bool _enableAntennaFlow = false;
  std::string _diodeCellName;
  std::string _diodePinName;
  int*** oldHUsages;
  int*** oldVUsages;
  int _reroute = false;

  // temporary for congestion driven replace
  int _numAdjusts = 0;

  // Variables for PADs obstacles handling
  std::map<Net*, std::vector<FastRoute::GSegment>> _padPinsConnections;

  // db variables
  sta::dbSta* _openSta;
  int selectedMetal = 3;
  odb::dbDatabase* _db = nullptr;
  odb::dbBlock* _block;

  std::vector<odb::dbNet*> _dirtyNets;
};

std::string getITermName(odb::dbITerm* iterm);
Net *
getNet(NET* net);

}  // namespace FastRoute
