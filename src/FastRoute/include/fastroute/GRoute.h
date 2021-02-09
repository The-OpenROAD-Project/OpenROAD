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

// #include "opendb/db.h"

namespace odb {
class dbNet;
class Rect;
}  // namespace odb

namespace grt {

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
};

class GCellCongestion
{
 public:
  GCellCongestion(int min_x, int min_y,
                  int max_x, int max_y,
                  int layer, short h_cap, short v_cap,
                  short h_usage, short v_usage);
  GCellCongestion(odb::Rect *rect, int layer,
                  short h_cap, short v_cap,
                  short h_usage, short v_usage);

  odb::Rect* getGCellRect() { return gcell_rect_; }
  int getLayer() { return layer_; }
  short getHorCapacity() { return hor_capacity_; }
  short getVerCapacity() { return ver_capacity_; }
  short getHorUsage() { return hor_usage_; }
  short getVerUsage() { return ver_usage_; }

 private:
  odb::Rect* gcell_rect_;
  int layer_;
  short hor_capacity_;
  short ver_capacity_;
  short hor_usage_;
  short ver_usage_;
};

// class Route is defined in fastroute core.
typedef std::vector<GSegment> GRoute;
typedef std::map<odb::dbNet*, GRoute> NetRouteMap;

void print(GRoute& groute);

}  // namespace grt
