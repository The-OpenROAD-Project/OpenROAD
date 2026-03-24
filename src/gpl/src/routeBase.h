// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <memory>
#include <utility>
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

class NesterovBaseCommon;
class NesterovBase;
class GNet;
class Die;
struct PlaceOptions;

// for GGrid
class Tile
{
 public:
  Tile();
  Tile(int x, int y, int lx, int ly, int ux, int uy, int layers);

  // getter funcs
  int x() const { return x_; }
  int y() const { return y_; }

  int lx() const { return lx_; }
  int ly() const { return ly_; }
  int ux() const { return ux_; }
  int uy() const { return uy_; }

  // only area is needed
  int64_t area() const;

  float inflationRatio() const;
  float inflatedRatio() const;

  // setter funcs
  void setInflationRatio(float ratio);

  // accumulated Ratio as iteration goes on
  void setInflatedRatio(float ratio);

 private:
  // the followings will store
  // blockage / capacity / route-ability
  // idx : metalLayer

  int x_ = 0;
  int y_ = 0;

  int lx_ = 0;
  int ly_ = 0;
  int ux_ = 0;
  int uy_ = 0;

  // to bloat cells in tile
  float inflationRatio_ = 1.0;
  float inflatedRatio_ = 0;
};

class TileGrid
{
 public:
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

  const std::vector<Tile*>& tiles() const;

  void initTiles(bool use_rudy);

 private:
  // for traversing layer info!
  utl::Logger* log_ = nullptr;

  std::vector<Tile> tileStor_;
  std::vector<Tile*> tiles_;

  int lx_ = 0;
  int ly_ = 0;
  int tileCntX_ = 0;
  int tileCntY_ = 0;
  int tileSizeX_ = 0;
  int tileSizeY_ = 0;
  int numRoutingLayers_ = 0;
};

struct RouteBaseVars
{
  RouteBaseVars(const PlaceOptions& options);

  const bool useRudy;
  const float targetRC;
  const float inflationRatioCoef;
  const float maxInflationRatio;
  const float maxDensity;
  const float ignoreEdgeRatio;
  const float minInflationRatio;

  // targetRC metric coefficients.
  const float rcK1, rcK2, rcK3, rcK4;
};

class RouteBase
{
 public:
  RouteBase(RouteBaseVars rbVars,
            odb::dbDatabase* db,
            grt::GlobalRouter* grouter,
            std::shared_ptr<NesterovBaseCommon> nbc,
            std::vector<std::shared_ptr<NesterovBase>> nbVec,
            utl::Logger* log);
  ~RouteBase();

  // Functions using fastroute on grt are saved as backup.
  void updateGrtRoute();
  void getGrtResult();
  // TODO: understand why this function is breaking RUDY.
  // Allow for grt heatmap during gpl execution.
  void loadGrt();
  float getGrtRC() const;

  void calculateRudyTiles();
  void updateRudyAverage(bool verbose = true);

  float getRudyAverage() const { return final_average_rc_; }
  int getOverflowedTilesCount() const { return overflowed_tiles_count_; }
  int getTotalTilesCount() const { return tg_->tiles().size(); }
  double getTotalRudyOverflow() const { return total_route_overflow_; }

  bool isMinRc() const { return is_min_rc_; }

  // first: is Routability Need
  // second: reverting procedure need in NesterovPlace
  //         (e.g. calling NesterovPlace's init())
  std::pair<bool, bool> routability(int routability_driven_revert_count);

  std::vector<int64_t> inflatedAreaDelta() const;
  int64_t getTotalInflation() const;
  int getRevertCount() const;

 private:
  RouteBaseVars rbVars_;
  odb::dbDatabase* db_ = nullptr;
  grt::GlobalRouter* grouter_ = nullptr;

  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<NesterovBase>> nbVec_;
  utl::Logger* log_ = nullptr;

  std::unique_ptr<TileGrid> tg_;

  std::vector<int64_t> inflatedAreaDelta_;
  std::vector<int64_t> minRcInflatedAreaDelta_;
  std::vector<int64_t> accumulatedInflatedAreaDelta_;

  int revert_count_ = 0;
  float final_average_rc_ = 0.0;
  int overflowed_tiles_count_ = 0;
  double total_route_overflow_ = 0.0;
  bool is_min_rc_ = false;

  // if solutions are not improved at all,
  // needs to revert back to have the minimized RC values.
  // minRcInflationSize_ will store
  // GCell's width and height
  float minRc_ = 1e30;
  std::vector<float> minRcTargetDensity_;
  int min_RC_violated_cnt_ = 0;
  int max_routability_no_improvement_ = 3;
  int max_routability_revert_ = 50;

  void init();
  void resetRoutabilityResources();
  void revertToMinCongestion();

  // update revert_count_
  void increaseCounter();

  // routability funcs
  void initGCells();
};
}  // namespace gpl
