////////////////////////////////////////////////////////////////////////////////
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

#include "opendb/db.h"
#include "opendb/geom.h"

namespace odb {
class dbNet;
class Rect;
}  // namespace odb

namespace grt {

typedef std::vector<std::vector<std::vector<int>>> CapacitiesVec;

struct GSegment
{
  int initX;
  int initY;
  int initLayer;
  int finalX;
  int finalY;
  int finalLayer;
  GSegment() = default;
  GSegment(int x0, int y0, int l0, int x1, int y1, int l1)
      : initX(x0),
        initY(y0),
        initLayer(l0),
        finalX(x1),
        finalY(y1),
        finalLayer(l1)
  {
  }
  bool isVia() { return (initX == finalX && initY == finalY); }
};

class Capacities
{
 public:
  Capacities() = default;
  CapacitiesVec& getHorCapacities() { return hor_capacities_; }
  CapacitiesVec& getVerCapacities() { return ver_capacities_; }
  void setHorCapacities(CapacitiesVec capacities) { hor_capacities_ = capacities; }
  void setVerCapacities(CapacitiesVec capacities) { ver_capacities_ = capacities; }
 private:
  CapacitiesVec hor_capacities_;
  CapacitiesVec ver_capacities_;
};

struct cmpById {
  bool operator()(odb::dbNet* net1, odb::dbNet* net2) const {
    return net1->getId() < net2->getId();
  }
};

// class Route is defined in fastroute core.
typedef std::vector<GSegment> GRoute;
typedef std::map<odb::dbNet*, GRoute, cmpById> NetRouteMap;

void print(GRoute& groute);

}  // namespace grt
