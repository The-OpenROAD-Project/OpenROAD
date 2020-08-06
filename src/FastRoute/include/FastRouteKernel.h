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

#include "FastRoute.h"

namespace ord {
class OpenRoad;
}

namespace FastRoute {

class FT;
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

class FastRouteKernel
{
 public:
  struct EST_
  {
    std::string netName;
    int netId;
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

  FastRouteKernel() = default;
  ~FastRouteKernel();
  void init(ord::OpenRoad* openroad);
  void init();
  void reset();
  void resetResources();

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
  void setEstimateRC(bool estimateRC);
  void setReportCongestion(char* congestFile);
  void printGrid();
  void printHeader();
  void setMaxLength(float maxLength);
  void addLayerMaxLength(int layer, float length);
  void setClockNetsRouteFlow(bool clockFlow);
  void setMinLayerForClock(int minLayer);

  // flow functions
  void enableAntennaAvoidance(char* diodeCellName, char* diodePinName);
  void writeGuides();
  void startFastRoute();
  void estimateRC();
  void runFastRoute();

  // congestion drive replace functions
  ROUTE_ getRoute();
  std::vector<EST_> getEst();

 private:
  void makeComponents();
  void deleteComponents();
  // main functions
  void initGrid();
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

  // check functions
  void checkPinPlacement();
  void checkSinksAndSource();

  // antenna functions
  bool checkResource(ROUTE segment);
  bool breakSegment(ROUTE segment,
                    long maxLength,
                    std::vector<ROUTE>& newSegments);
  void fixLongSegments();
  SteinerTree createSteinerTree(std::vector<ROUTE>& route,
                                const std::vector<Pin>& pins);
  bool checkSteinerTree(SteinerTree sTree);
  void addLocalConnections(std::vector<FastRoute::NET>& globalRoute);
  void mergeResults(std::vector<FastRoute::NET> newRoute);
  void runAntennaAvoidanceFlow();
  void runClockNetsRouteFlow();
  void restartFastRoute();
  void getPreviousCapacities(int previousMinLayer);
  void restorePreviousCapacities(int previousMinLayer);

  Netlist* _netlist = nullptr;
  Grid* _grid = nullptr;
  std::vector<RoutingLayer>* _routingLayers = nullptr;
  std::vector<RoutingTracks>* _allRoutingTracks = nullptr;

  ord::OpenRoad* _openroad;
  // Objects variables
  DBWrapper* _dbWrapper = nullptr;
  FT* _fastRoute = nullptr;
  Coordinate* _gridOrigin = nullptr;
  std::vector<FastRoute::NET>* _result;

  // Flow variables
  std::string _outfile;
  std::string _congestFile;
  float _adjustment;
  int _minRoutingLayer;
  int _maxRoutingLayer;
  bool _unidirectionalRoute;
  int _fixLayer;
  bool _clockNetRouting;
  const int _selectedMetal = 3;
  const float transitionLayerAdjust = 0.6;
  const int _gcellsOffset = 2;
  int _overflowIterations;
  int _pdRevForHighFanout;
  bool _allowOverflow;
  bool _estimateRC;
  bool _reportCongest;
  std::vector<int> _vCapacities;
  std::vector<int> _hCapacities;
  unsigned _seed;

  // Layer adjustment variables
  std::vector<int> _layersToAdjust;
  std::vector<float> _layersReductionPercentage;

  // Region adjustment variables
  std::vector<int> regionsMinX;
  std::vector<int> regionsMinY;
  std::vector<int> regionsMaxX;
  std::vector<int> regionsMaxY;
  std::vector<int> regionsLayer;
  std::vector<float> regionsReductionPercentage;

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

  // Antenna variables
  float _maxLengthMicrons = -1;
  std::map<int, float> _layersMaxLengthMicrons;
  long _maxLengthDBU = -1;
  std::map<int, long> _layersMaxLengthDBU;
  bool _enableAntennaFlow = false;
  std::string _diodeCellName;
  std::string _diodePinName;
  int*** oldHUsages;
  int*** oldVUsages;
  int _reroute = false;

  // temporary for congestion driven replace
  int _numAdjusts = 0;

  // Variables for PADs obstacles handling
  std::map<Net*, std::vector<FastRoute::ROUTE>> _padPinsConnections;
};

}  // namespace FastRoute
