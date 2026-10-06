// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "grt/RoutingCongestion.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "grt/GlobalRouter.h"
#include "grt/Rudy.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace grt {

RoutingCongestion::RoutingCongestion(GlobalRouter* grouter,
                                     odb::dbBlock* block,
                                     utl::Logger* logger)
    : grouter_(grouter), block_(block), logger_(logger)
{
}

void RoutingCongestion::invalidate()
{
  built_ = false;
  source_ = Source::kNone;
  x_lines_.clear();
  y_lines_.clear();
  scores_.clear();
  tile_size_ = 0;
}

void RoutingCongestion::setCongestionThreshold(const float threshold)
{
  threshold_ = std::clamp(threshold, 0.0f, 1.0f);
}

RoutingCongestion::Source RoutingCongestion::source()
{
  ensureBuilt();
  return source_;
}

int RoutingCongestion::tileSize()
{
  ensureBuilt();
  return tile_size_;
}

void RoutingCongestion::ensureBuilt()
{
  if (built_) {
    return;
  }
  // Post-GRT data is preferred; fall back to the RUDY estimate.
  built_ = true;
  if (buildFromGlobalRoutes()) {
    source_ = Source::kGlobalRoute;
  } else if (buildFromRudy()) {
    source_ = Source::kRudy;
  } else {
    source_ = Source::kNone;
  }
  debugPrint(logger_,
             utl::GRT,
             "congestion",
             1,
             "routing congestion map: source {}, {}x{} tiles of {} dbu",
             source_ == Source::kGlobalRoute
                 ? "global route"
                 : (source_ == Source::kRudy ? "rudy" : "none"),
             x_lines_.size(),
             y_lines_.size(),
             tile_size_);
}

bool RoutingCongestion::buildFromGlobalRoutes()
{
  if (block_ == nullptr) {
    return false;
  }
  odb::dbGCellGrid* gcell_grid = block_->getGCellGrid();
  if (gcell_grid == nullptr) {
    return false;
  }
  std::vector<int> x_grid, y_grid;
  gcell_grid->getGridX(x_grid);
  gcell_grid->getGridY(y_grid);
  if (x_grid.empty() || y_grid.empty()) {
    return false;
  }

  int min_layer, max_layer;
  grouter_->getMinMaxLayer(min_layer, max_layer);
  odb::dbTech* tech = block_->getTech();

  const size_t x_count = x_grid.size();
  const size_t y_count = y_grid.size();
  // Horizontal and vertical resources are separate budgets; a GCell is only as
  // good as its worse direction, so they are accumulated apart and combined at
  // the end.
  std::vector<float> h_usage(x_count * y_count, 0.0f);
  std::vector<float> h_capacity(x_count * y_count, 0.0f);
  std::vector<float> v_usage(x_count * y_count, 0.0f);
  std::vector<float> v_capacity(x_count * y_count, 0.0f);
  bool any_capacity = false;

  for (int level = min_layer; level <= max_layer; level++) {
    odb::dbTechLayer* layer = tech->findRoutingLayer(level);
    if (layer == nullptr) {
      continue;
    }
    const bool horizontal
        = layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
    std::vector<float>& usage = horizontal ? h_usage : v_usage;
    std::vector<float>& capacity = horizontal ? h_capacity : v_capacity;
    for (size_t x = 0; x < x_count; x++) {
      for (size_t y = 0; y < y_count; y++) {
        const size_t index = x * y_count + y;
        usage[index] += gcell_grid->getUsage(layer, x, y);
        const float cap = gcell_grid->getCapacity(layer, x, y);
        capacity[index] += cap;
        if (cap > 0.0f) {
          any_capacity = true;
        }
      }
    }
  }
  if (!any_capacity) {
    // The grid exists but GRT never filled it in (e.g. only a floorplan was
    // read); there is no post-GRT signal to report.
    return false;
  }

  scores_.assign(x_count * y_count, 0.0f);
  for (size_t index = 0; index < scores_.size(); index++) {
    float score = 0.0f;
    for (int dir = 0; dir < 2; dir++) {
      const float usage = dir == 0 ? h_usage[index] : v_usage[index];
      const float capacity = dir == 0 ? h_capacity[index] : v_capacity[index];
      if (capacity > 0.0f) {
        score = std::max(score, usage / capacity);
      } else if (usage > 0.0f) {
        // Demand with no capacity at all is as bad as it gets.
        score = 1.0f;
      }
    }
    scores_[index] = std::clamp(score, 0.0f, 1.0f);
  }
  x_lines_ = std::move(x_grid);
  y_lines_ = std::move(y_grid);
  tile_size_ = x_lines_.size() > 1 ? x_lines_[1] - x_lines_[0] : 0;
  return true;
}

bool RoutingCongestion::buildFromRudy()
{
  if (block_ == nullptr || block_->getTrackGrids().empty()) {
    // RUDY needs a routing grid, which needs tracks.  A design without them
    // (a bare floorplan, most unit tests) simply has no congestion signal.
    return false;
  }
  Rudy* rudy = nullptr;
  try {
    // Building the RUDY grid initializes global routing, which can still fail
    // on an incomplete floorplan.  Congestion is advisory, so a failure must
    // not abort the caller.
    rudy = grouter_->getRudy();
    if (rudy != nullptr) {
      rudy->calculateRudy();
    }
  } catch (const std::runtime_error&) {
    return false;
  }
  if (rudy == nullptr) {
    return false;
  }
  const auto [x_count, y_count] = rudy->getGridSize();
  if (x_count <= 0 || y_count <= 0) {
    return false;
  }

  x_lines_.resize(x_count);
  y_lines_.resize(y_count);
  scores_.assign(static_cast<size_t>(x_count) * y_count, 0.0f);
  for (int x = 0; x < x_count; x++) {
    for (int y = 0; y < y_count; y++) {
      const Rudy::Tile& tile = rudy->getTile(x, y);
      if (y == 0) {
        x_lines_[x] = tile.getRect().xMin();
      }
      if (x == 0) {
        y_lines_[y] = tile.getRect().yMin();
      }
      // RUDY reports demand as a percentage of the available resources.
      scores_[static_cast<size_t>(x) * y_count + y]
          = std::clamp(tile.getRudy() / 100.0f, 0.0f, 1.0f);
    }
  }
  tile_size_ = rudy->getTileSize();
  return true;
}

int RoutingCongestion::xIndex(const int x) const
{
  if (x_lines_.empty()) {
    return -1;
  }
  const auto it = std::ranges::upper_bound(x_lines_, x);
  return std::clamp(static_cast<int>(it - x_lines_.begin()) - 1,
                    0,
                    static_cast<int>(x_lines_.size()) - 1);
}

int RoutingCongestion::yIndex(const int y) const
{
  if (y_lines_.empty()) {
    return -1;
  }
  const auto it = std::ranges::upper_bound(y_lines_, y);
  return std::clamp(static_cast<int>(it - y_lines_.begin()) - 1,
                    0,
                    static_cast<int>(y_lines_.size()) - 1);
}

float RoutingCongestion::tileScore(const int x_idx, const int y_idx) const
{
  if (x_idx < 0 || y_idx < 0) {
    return 0.0f;
  }
  return scores_[static_cast<size_t>(x_idx) * y_lines_.size() + y_idx];
}

float RoutingCongestion::congestion(const odb::Point& at)
{
  ensureBuilt();
  if (source_ == Source::kNone) {
    return 0.0f;
  }
  return tileScore(xIndex(at.x()), yIndex(at.y()));
}

float RoutingCongestion::congestion(const odb::Rect& region,
                                    const Aggregate agg)
{
  ensureBuilt();
  if (source_ == Source::kNone) {
    return 0.0f;
  }
  const int x_lo = xIndex(region.xMin());
  const int x_hi = xIndex(region.xMax());
  const int y_lo = yIndex(region.yMin());
  const int y_hi = yIndex(region.yMax());
  if (x_lo < 0 || y_lo < 0) {
    return 0.0f;
  }

  float peak = 0.0f;
  double sum = 0.0;
  int count = 0;
  for (int x = x_lo; x <= x_hi; x++) {
    for (int y = y_lo; y <= y_hi; y++) {
      const float score = tileScore(x, y);
      peak = std::max(peak, score);
      sum += score;
      count++;
    }
  }
  if (count == 0) {
    return 0.0f;
  }
  return agg == Aggregate::kPeak ? peak : static_cast<float>(sum / count);
}

bool RoutingCongestion::isCongested(const odb::Rect& region)
{
  return congestion(region, Aggregate::kPeak) >= threshold_;
}

std::vector<RoutingCongestion::GCell> RoutingCongestion::gcellsByCongestion(
    const odb::Rect& region,
    const odb::Point& prefer_near,
    const float bucket)
{
  ensureBuilt();
  std::vector<GCell> gcells;
  if (source_ == Source::kNone) {
    return gcells;
  }
  const int x_lo = xIndex(region.xMin());
  const int x_hi = xIndex(region.xMax());
  const int y_lo = yIndex(region.yMin());
  const int y_hi = yIndex(region.yMax());
  if (x_lo < 0 || y_lo < 0) {
    return gcells;
  }

  const int x_count = static_cast<int>(x_lines_.size());
  const int y_count = static_cast<int>(y_lines_.size());
  gcells.reserve(static_cast<size_t>(x_hi - x_lo + 1) * (y_hi - y_lo + 1));
  for (int x = x_lo; x <= x_hi; x++) {
    // The last line is the lower edge of the final tile, whose upper edge is
    // not in the grid; fall back to one tile width.
    const int x_max = x + 1 < x_count ? x_lines_[x + 1]
                                      : x_lines_[x] + std::max(tile_size_, 1);
    for (int y = y_lo; y <= y_hi; y++) {
      const int y_max = y + 1 < y_count ? y_lines_[y + 1]
                                        : y_lines_[y] + std::max(tile_size_, 1);
      gcells.push_back(GCell{odb::Rect(x_lines_[x], y_lines_[y], x_max, y_max),
                             tileScore(x, y)});
    }
  }

  const auto rank = [bucket](const float congestion) {
    return bucket > 0.0f ? std::floor(congestion / bucket) : congestion;
  };
  std::ranges::sort(
      gcells, [&prefer_near, &rank](const GCell& a, const GCell& b) {
        const float rank_a = rank(a.congestion);
        const float rank_b = rank(b.congestion);
        if (rank_a != rank_b) {
          return rank_a < rank_b;
        }
        const int dist_a = odb::manhattanDistance(a.rect, prefer_near);
        const int dist_b = odb::manhattanDistance(b.rect, prefer_near);
        if (dist_a != dist_b) {
          return dist_a < dist_b;
        }
        // Keep the order stable across runs.
        if (a.rect.yMin() != b.rect.yMin()) {
          return a.rect.yMin() < b.rect.yMin();
        }
        return a.rect.xMin() < b.rect.xMin();
      });
  return gcells;
}

}  // namespace grt
