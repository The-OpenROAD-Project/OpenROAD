////////////////////////////////////////////////////////////////////////////////
// Authors: Vitor Bandeira, Eder Matheus Monteiro e Isadora Oliveira
//          (Advisor: Ricardo Reis)
//
// BSD 3-Clause License
//
// Copyright (c) 2019, Federal University of Rio Grande do Sul (UFRGS)
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
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <string>
#include <vector>

#include "fastroute/GRoute.h"

namespace utl {
class Logger;
}

namespace grt {

class FastRouteCore
{
 public:
  FastRouteCore(utl::Logger* log);
  ~FastRouteCore();

  void deleteComponents();
  void clear();
  void setGridsAndLayers(int x, int y, int nLayers);
  void addVCapacity(int verticalCapacity, int layer);
  void addHCapacity(int horizontalCapacity, int layer);
  void addMinWidth(int width, int layer);
  void addMinSpacing(int spacing, int layer);
  void addViaSpacing(int spacing, int layer);
  void setNumberNets(int nNets);
  void setLowerLeft(int x, int y);
  void setTileSize(int width, int height);
  void setLayerOrientation(int x);
  void addPin(int netID, int x, int y, int layer);
  int addNet(odb::dbNet* db_net,
             int nPins,
             int validPins,
             float alpha,
             bool isClock,
             int cost);
  void initEdges();
  void setNumAdjustments(int nAdjustements);
  void addAdjustment(long x1,
                     long y1,
                     int l1,
                     long x2,
                     long y2,
                     int l2,
                     int reducedCap,
                     bool isReduce = true);
  void initAuxVar();
  NetRouteMap run();
  void writeCongestionReport2D(std::string fileName);
  void writeCongestionReport3D(std::string fileName);
  void findCongestionInformation(std::vector<GCellCongestion>& congestionInfo);

  int getEdgeCapacity(long x1, long y1, int l1, long x2, long y2, int l2);
  int getEdgeCurrentResource(long x1,
                             long y1,
                             int l1,
                             long x2,
                             long y2,
                             int l2);
  int getEdgeCurrentUsage(long x1, long y1, int l1, long x2, long y2, int l2);
  void setEdgeUsage(long x1,
                    long y1,
                    int l1,
                    long x2,
                    long y2,
                    int l2,
                    int newUsage);
  void setEdgeCapacity(long x1,
                       long y1,
                       int l1,
                       long x2,
                       long y2,
                       int l2,
                       int newCap);
  void setMaxNetDegree(int);
  void setAlpha(float a);
  void setVerbose(int v);
  void setOverflowIterations(int iterations);
  void setPDRevForHighFanout(int pdRevHihgFanout);
  void setAllowOverflow(bool allow);

 private:
  NetRouteMap getRoutes();
  int maxNetDegree;
};

}  // namespace grt
