// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace grt {

class GlobalRouter;

// Shared routing congestion service owned by GlobalRouter, for consumers
// choosing where to put something (e.g., a buffer) without making routing
// worse.
//
// Answers from the post-GRT per-GCell usage/capacity map in ODB when it
// exists, otherwise from RUDY's pre-GRT estimate.  Callers get the best
// available signal; source() reports which one answered.
class RoutingCongestion
{
 public:
  enum class Source
  {
    kNone,        // no congestion information available
    kRudy,        // pre-GRT estimate
    kGlobalRoute  // post-GRT usage/capacity
  };

  enum class Aggregate
  {
    kMean,
    kPeak
  };

  RoutingCongestion(GlobalRouter* grouter,
                    odb::dbBlock* block,
                    utl::Logger* logger);

  // Congestion score in [0,1] at a point (0 = free, 1 = saturated).
  // Uses GRT data if routed, otherwise the RUDY estimate.
  float congestion(const odb::Point& at);

  // Aggregate congestion over a region.
  float congestion(const odb::Rect& region, Aggregate agg = Aggregate::kMean);

  // True if the region is at/above the congestion threshold.
  bool isCongested(const odb::Rect& region);

  // One GCell of the congestion map.
  struct GCell
  {
    odb::Rect rect;
    float congestion = 0.0f;
  };

  // Every GCell overlapping `region`, least congested first, ties broken by
  // distance from `prefer_near`.  Empty means no congestion information, i.e.
  // "no preference", not "nowhere is acceptable".
  //
  // `bucket` quantizes the congestion key so near-equal GCells rank by
  // distance instead; 0 ranks on the exact value, which lets an arbitrarily
  // small congestion difference outrank an arbitrarily large distance.
  std::vector<GCell> gcellsByCongestion(const odb::Rect& region,
                                        const odb::Point& prefer_near,
                                        float bucket = 0.0f);

  void setCongestionThreshold(float threshold);
  float congestionThreshold() const { return threshold_; }

  // Which signal the last/next query uses.  Builds the map if needed.
  Source source();

  // Drop the cached map; the next query rebuilds it.
  void invalidate();

  // Tile size of the cached map in DBU, or 0 when there is no map.
  int tileSize();

 private:
  void ensureBuilt();
  bool buildFromGlobalRoutes();
  bool buildFromRudy();
  // Index of the tile containing `coord`, clamped into the grid.
  int xIndex(int x) const;
  int yIndex(int y) const;
  float tileScore(int x_idx, int y_idx) const;

  GlobalRouter* grouter_;
  odb::dbBlock* block_;
  utl::Logger* logger_;

  bool built_ = false;
  Source source_ = Source::kNone;
  // Lower edge of each tile, ascending.  scores_ is x-major.
  std::vector<int> x_lines_;
  std::vector<int> y_lines_;
  std::vector<float> scores_;
  int tile_size_ = 0;

  // 90%: a GCell this full already detours nets, before any real overflow.
  float threshold_ = 0.9f;
};

}  // namespace grt
