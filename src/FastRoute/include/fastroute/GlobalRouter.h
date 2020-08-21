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
struct ROUTE;
struct PIN;

class GlobalRouter
{
 public:
  struct EST_
  {
    std::string netName;
    int numSegments;
    std::vector<long> initX;
    std::vector<long> initY;
    std::vector<int> initLayer;
    std::vector<long> finalX;
    std::vector<long> finalY;
    std::vector<int> finalLayer;
  };

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
  void setOutputFile(const std::string& outfile);
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
  void setMaxLayerForClock(int maxLayer);

  // flow functions
  void writeGuides(char* fileName);
  void startFastRoute();
  void estimateRC();
  void runFastRoute();
  bool haveRoutes() const { return _result != nullptr; }

  // congestion drive replace functions
  ROUTE_ getRoute();
  std::vector<EST_> getEst();

  // estimate_rc functions
  void getLayerRC(unsigned layerId, float& r, float& c);
  void getCutLayerRes(unsigned belowLayerId, float& r);
  float dbuToMeters(unsigned dbu);

  // repair antenna public functions
  void repairAntenna(char* diodeCellName, char* diodePinName);

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
  void clearFlow();
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
  void addRemainingGuides(std::vector<FastRoute::NET>* globalRoute);
  void connectPadPins(std::vector<FastRoute::NET>* globalRoute);
  void mergeBox(std::vector<Box>& guideBox);
  Box globalRoutingToBox(const FastRoute::ROUTE& route);
  using Point = std::tuple<long, long, int>;  // x, y, layer
  bool segmentsConnect(const ROUTE& seg0,
                       const ROUTE& seg1,
                       ROUTE& newSeg,
                       const std::map<Point, int>& segsAtPoint);
  void mergeSegments(FastRoute::NET& net);
  bool pinOverlapsWithSingleTrack(const Pin& pin, Coordinate& trackPosition);
  ROUTE createFakePin(Pin pin, Coordinate& pinPosition, RoutingLayer layer);
  bool checkSignalType(const Net &net);

  // check functions
  void checkPinPlacement();

  // antenna functions
  void addLocalConnections(std::vector<FastRoute::NET>* globalRoute);
  void mergeResults(const std::vector<FastRoute::NET>* newRoute);
  void runClockNetsRouteFlow();
  void restartFastRoute();
  void getPreviousCapacities(int previousMinLayer);
  void restorePreviousCapacities(int previousMinLayer);

  // db functions
  void initGrid(int maxLayer);
  void initRoutingLayers(std::vector<RoutingLayer>& routingLayers);
  void initRoutingTracks(std::vector<RoutingTracks>& allRoutingTracks,
                         int maxLayer,
                         std::map<int, float> layerPitches);
  void computeCapacities(int maxLayer, std::map<int, float> layerPitches);
  void computeSpacingsAndMinWidth(int maxLayer);
  void initNetlist(bool reroute);
  void addNets(std::vector<odb::dbNet*> nets);
  Net* getNet(odb::dbNet* db_net);
  Net* getNet(NET& net);
  void initObstacles();
  int computeMaxRoutingLayer();
  std::set<int> findTransitionLayers(int maxRoutingLayer);
  std::map<int, odb::dbTechVia*> getDefaultVias(int maxRoutingLayer);
  void makeItermPins(Net* net, odb::dbNet* db_net, Box& dieArea);
  void makeBtermPins(Net* net, odb::dbNet* db_net, Box& dieArea);
  void initClockNets();
  void commitGlobalSegmentsToDB(std::vector<FastRoute::NET> routing,
                                int maxRoutingLayer);
  void setSelectedMetal(int metal) { _layerForGCells = metal; }
  void setDirtyNets(std::vector<odb::dbNet*> dirtyNets) { _dirtyNets = dirtyNets; }

  ord::OpenRoad* _openroad;
  // Objects variables
  FT* _fastRoute = nullptr;
  Coordinate* _gridOrigin = nullptr;
  std::vector<FastRoute::NET>* _result;

  std::vector<Net> *_nets;
  std::map<odb::dbNet*, Net*> _db_net_map;
  Grid* _grid = nullptr;
  std::vector<RoutingLayer>* _routingLayers = nullptr;
  std::vector<RoutingTracks>* _allRoutingTracks = nullptr;

  // Flow variables
  std::string _outfile;
  std::string _congestFile;
  float _adjustment;
  int _minRoutingLayer;
  int _maxRoutingLayer;
  bool _unidirectionalRoute;
  int _fixLayer;
  const int _gcellsOffset = 2;
  int _overflowIterations;
  int _pdRevForHighFanout;
  bool _allowOverflow;
  bool _reportCongest;
  std::vector<int> _vCapacities;
  std::vector<int> _hCapacities;
  unsigned _seed;

  // Layer adjustment variables
  std::vector<int> _layersToAdjust;
  std::vector<float> _layersReductionPercentage;

  // Region adjustment variables
  std::vector<int> _regionsMinX;
  std::vector<int> _regionsMinY;
  std::vector<int> _regionsMaxX;
  std::vector<int> _regionsMaxY;
  std::vector<int> _regionsLayer;
  std::vector<float> _regionsReductionPercentage;

  // Pitches variables
  std::map<int, float> _layerPitches;

  // Clock net routing variables
  bool _pdRev;
  float _alpha;
  int _verbose;
  std::map<std::string, float> _netsAlpha;
  bool _clockNetsRouteFlow = false;
  bool _onlyClockNets = false;
  bool _onlySignalNets = false;
  int _minLayerForClock;
  int _maxLayerForClock;

  // Antenna variables
  std::string _diodeCellName;
  std::string _diodePinName;
  int*** oldHUsages;
  int*** oldVUsages;
  int _reroute = false;

  // temporary for congestion driven replace
  int _numAdjusts = 0;

  // Variables for PADs obstacles handling
  std::map<Net*, std::vector<FastRoute::ROUTE>> _padPinsConnections;

  // db variables
  int _layerForGCells = 3;
  sta::dbSta* _sta;
  odb::dbDatabase* _db;
  odb::dbBlock* _block;
  std::vector<odb::dbNet*> _dirtyNets;
};

std::string getITermName(odb::dbITerm* iterm);
Net *
getNet(NET* net);

}  // namespace FastRoute
