///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#ifndef __REPLACE_ROUTE_BASE__
#define __REPLACE_ROUTE_BASE__

#include <memory>
#include <vector>

namespace odb {
  class dbDatabase;
}

namespace grt {
  class GlobalRouter;
}

namespace utl {
  class Logger;
}

namespace gpl {

class NesterovBase;
class GNet;
class Die;

// for GGrid
class Tile {
  public:
    Tile();
    Tile(int x, int y, int lx, int ly,
        int ux, int uy, int layers);
    ~Tile();

    // getter funcs
    int x() const;
    int y() const;

    int lx() const;
    int ly() const;
    int ux() const;
    int uy() const;

    // only area is needed
    int64_t area() const;

    int blockage(int layer) const;
    int capacity(int layer) const;
    int route(int layer) const;

    int usageHL(int layer) const;
    int usageHU(int layer) const;
    int usageVL(int layer) const;
    int usageVU(int layer) const;

    float inflationRatio() const;
    float inflationArea() const;
    float inflationAreaDelta() const;
    float inflatedRatio() const;

    // setter funcs
    void setBlockage(int layer, int blockage);
    void setCapacity(int layer, int capacity);
    void setCapacity(const std::vector<int>& capacity);
    void setRoute(int layer, int route);

    void setInflationRatioH(float val);
    void setInflationRatioV(float val);
    void setInflationRatio(float ratio);

    void setInflationArea(float area);
    void setInflationAreaDelta(float delta);

    // accumulated Ratio as iteration goes on
    void setInflatedRatio(float ratio);

  private:
    // the followings will store
    // blockage / capacity / route-ability
    // idx : metalLayer

    // Note that the layerNum starts from 0 in this Tile class
    std::vector<int> blockage_;
    std::vector<int> capacity_;
    std::vector<int> route_;

    // H : Horizontal
    // V : Vertical
    //
    // L : Lower
    //    1. horizontal usage/cap -> vertical edge: down edge
    //    2. vertical usage/cap -> horizontal edge: left edge
    // R : Upper
    //    1. horizontal usage/cap -> vertical edge: up edge
    //    2. vertical usage/cap -> horizontal edge: right edge
    //
    std::vector<int> usageHL_;
    std::vector<int> usageHU_;
    std::vector<int> usageVL_;
    std::vector<int> usageVU_;

    int x_;
    int y_;

    int lx_;
    int ly_;
    int ux_;
    int uy_;

    // to bloat cells in tile
    float inflationRatio_;

    float inflationArea_;
    float inflationAreaDelta_;

    float inflatedRatio_;

    void reset();
};

inline int
Tile::x() const {
  return x_;
}

inline int
Tile::y() const {
  return y_;
}

inline int
Tile::lx() const {
  return lx_;
}

inline int
Tile::ly() const {
  return ly_;
}

inline int
Tile::ux() const {
  return ux_;
}

inline int
Tile::uy() const {
  return uy_;
}

inline int64_t
Tile::area() const {
  return
    static_cast<int64_t>(ux_ - lx_) *
    static_cast<int64_t>(uy_ - ly_);
}


class TileGrid {
  public:
    TileGrid();
    ~TileGrid();

    void setLogger(utl::Logger* log);
    void setTileCnt(int tileCntX, int tileCntY);
    void setTileCntX(int tileCntX);
    void setTileCntY(int tileCntY);
    void setTileSize(int tileSizeX, int tileSizeY);
    void setTileSizeX(int tileSizeX);
    void setTileSizeY(int tileSizeY);
    void setNumRoutingLayers(int num);

    void setLx(int lx);
    void setLy(int ly);

    int lx() const;
    int ly() const;
    int ux() const;
    int uy() const;

    int tileCntX() const;
    int tileCntY() const;
    int tileSizeX() const;
    int tileSizeY() const;

    int numRoutingLayers() const;

    const std::vector<Tile*> & tiles() const;

    void initTiles();

  private:
    // for traversing layer info!
    utl::Logger* log_;

    std::vector<Tile> tileStor_;
    std::vector<Tile*> tiles_;

    int lx_;
    int ly_;
    int tileCntX_;
    int tileCntY_;
    int tileSizeX_;
    int tileSizeY_;
    int numRoutingLayers_;

    void reset();
};

inline const std::vector<Tile*> &
TileGrid::tiles() const {
  return tiles_;
}

class RouteBaseVars {
public:
  float inflationRatioCoef;
  float maxInflationRatio;
  float maxDensity;
  float targetRC;
  float ignoreEdgeRatio;
  float minInflationRatio;

  // targetRC metric coefficients.
  float rcK1, rcK2, rcK3, rcK4;

  int maxBloatIter;
  int maxInflationIter;

  RouteBaseVars();
  void reset();
};


class RouteBase {
  public:
    RouteBase();
    RouteBase(RouteBaseVars rbVars,
        odb::dbDatabase* db,
        grt::GlobalRouter* grouter,
        std::shared_ptr<NesterovBase> nb,
        utl::Logger* log);
    ~RouteBase();

    // update Route and Est info
    // from GlobalRouter
    void updateRoute();
    void getGlobalRouterResult();

    // first: is Routability Need
    // second: reverting procedure need in NesterovPlace
    //         (e.g. calling NesterovPlace's init())
    std::pair<bool, bool> routability();

    int64_t inflatedAreaDelta() const;
    int numCall() const;
    int bloatIterCnt() const;
    int inflationIterCnt() const;

    float getRC() const;

    void revertGCellSizeToMinRc();

  private:
    RouteBaseVars rbVars_;
    odb::dbDatabase* db_;
    grt::GlobalRouter* grouter_;

    std::shared_ptr<NesterovBase> nb_;
    utl::Logger* log_;

    std::unique_ptr<TileGrid> tg_;

    // from *.route file
    std::vector<int> verticalCapacity_;
    std::vector<int> horizontalCapacity_;

    // inflationList_ for dynamic Inflation Adjustment
    std::vector<std::pair<Tile*, float>> inflationList_;

    int64_t inflatedAreaDelta_;

    int bloatIterCnt_;
    int inflationIterCnt_;
    int numCall_;

    // if solutions are not improved at all,
    // needs to revert back to have the minimized RC values.
    // minRcInflationSize_ will store
    // GCell's width and height
    float minRc_;
    float minRcTargetDensity_;
    int minRcViolatedCnt_;
    std::vector<std::pair<int, int>> minRcCellSize_;

    void init();
    void reset();
    void resetRoutabilityResources();

    // update inflationIterCnt_, bloatIterCnt_ and numCall_
    void increaseCounter();

    // routability funcs
    void initGCells();
};
}

#endif
