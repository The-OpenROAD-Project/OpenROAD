// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Coordinates.h"
#include "Objects.h"
#include "boost/geometry/index/rtree.hpp"
#include "boost/icl/interval_map.hpp"
#include "dpl/Opendp.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "odb/isotropy.h"
#include "utl/Logger.h"

namespace dpl {

class Network;

struct GridIntervalX
{
  GridX lo;
  GridX hi;
};

struct GridIntervalY
{
  GridY lo;
  GridY hi;
};

struct Pixel
{
  Node* cell = nullptr;
  float util = 0.0;
  bool is_valid = false;     // false for dummy cells
  bool is_hopeless = false;  // too far from sites for diamond search
  uint8_t blocked_layers = 0;
  // Routing levels with power via or patch metal outside the stripes (plus
  // a min spacing halo) in this pixel; see Grid::overlapsPinBlockage
  uint8_t blocked_pin_layers = 0;
};
static_assert(sizeof(Pixel) <= 2 * sizeof(void*));

// Return value for grid searches.
class PixelPt
{
 public:
  PixelPt() = default;
  PixelPt(Pixel* pixel, GridX grid_x, GridY grid_y);
  Pixel* pixel = nullptr;
  GridX x{0};
  GridY y{0};
};

// The "Grid" is a 2D array of pixels.  The pixels represent the
// single height sites onto which multi-height sites are overlaid.
// The sites are assumed to be of a single width but the rows are of
// variable height in order to support hybrid rows.
class Grid
{
 public:
  void init(utl::Logger* logger) { logger_ = logger; }
  void setCore(const odb::Rect& core) { core_ = core; }
  void initGrid(odb::dbDatabase* db,
                odb::dbBlock* block,
                std::shared_ptr<Padding> padding,
                int max_displacement_x,
                int max_displacement_y);
  void allocateGrid();
  void examineRows(odb::dbBlock* block);
  std::unordered_set<int> getRowCoordinates() const;

  GridX gridX(DbuX x) const;
  GridX gridRoundX(DbuX x) const;
  GridX gridEndX(DbuX x) const;

  GridX gridX(const Node* cell) const;
  GridX gridEndX(const Node* cell) const;
  GridX gridPaddedX(const Node* cell) const;
  GridX gridPaddedEndX(const Node* cell) const;

  GridY gridSnapDownY(DbuY y) const;
  GridY gridRoundY(DbuY y) const;
  GridY gridEndY(DbuY y) const;

  // Snap outwards to fully contain
  GridRect gridCovering(const odb::Rect& rect) const;
  GridRect gridCovering(const Node* cell) const;
  GridRect gridCoveringPadded(const Node* cell) const;

  // Snap inwards to be fully contained
  GridRect gridWithin(const DbuRect& rect) const;

  GridY gridSnapDownY(const Node* cell) const;
  GridY gridRoundY(const Node* cell) const;
  GridY gridEndY(const Node* cell) const;

  DbuY gridYToDbu(GridY y) const;

  GridX gridPaddedWidth(const Node* cell) const;
  GridX gridWidth(const Node* cell) const;
  GridY gridHeight(const Node* cell) const;
  GridY gridHeight(odb::dbMaster* master) const;
  DbuY rowHeight(GridY index);

  void paintPixel(Node* cell, GridX grid_x, GridY grid_y);
  void paintPixel(Node* cell);
  void paintCellPadding(Node* cell);
  void paintCellPadding(Node* cell,
                        GridX grid_x_begin,
                        GridY grid_y_begin,
                        GridX grid_x_end,
                        GridY grid_y_end);
  void erasePixel(Node* cell);
  void visitCellPixels(
      Node& cell,
      bool padded,
      const std::function<void(Pixel* pixel, bool padded)>& visitor) const;
  void visitCellBoundaryPixels(
      Node& cell,
      const std::function<
          void(Pixel* pixel, odb::Direction2D edge, GridX x, GridY y)>& visitor)
      const;

  GridY getRowCount() const { return row_count_; }
  GridX getRowSiteCount() const { return row_site_count_; }
  DbuX getSiteWidth() const { return site_width_; }

  Pixel* gridPixel(GridX x, GridY y) const;
  Pixel& pixel(GridY y, GridX x)
  {
    assert(x >= 0 && x < row_site_count_ && y >= 0 && y < row_count_);
    return pixels_[static_cast<size_t>(y.v) * row_site_count_.v + x.v];
  }
  const Pixel& pixel(GridY y, GridX x) const
  {
    assert(x >= 0 && x < row_site_count_ && y >= 0 && y < row_count_);
    return pixels_[static_cast<size_t>(y.v) * row_site_count_.v + x.v];
  }
  size_t pixelIndex(const Pixel* pixel) const
  {
    assert(pixel != nullptr);
    assert(pixel >= pixels_.data());
    const auto idx = static_cast<size_t>(pixel - pixels_.data());
    assert(idx < pixels_.size());
    return idx;
  }

  Group* pixelGroup(const Pixel* pixel) const
  {
    const size_t idx = pixelIndex(pixel);
    return pixel_groups_.empty() ? nullptr : pixel_groups_[idx];
  }
  void setPixelGroup(Pixel* pixel, Group* group)
  {
    const size_t idx = pixelIndex(pixel);
    if (group == nullptr && pixel_groups_.empty()) {
      return;
    }
    if (pixel_groups_.empty()) {
      pixel_groups_.assign(pixels_.size(), nullptr);
    }
    pixel_groups_[idx] = group;
  }

  Node* paddingReservedBy(const Pixel* pixel) const
  {
    const size_t idx = pixelIndex(pixel);
    return padding_reserved_by_.empty() ? nullptr : padding_reserved_by_[idx];
  }
  void setPaddingReservedBy(Pixel* pixel, Node* cell)
  {
    const size_t idx = pixelIndex(pixel);
    if (cell == nullptr && padding_reserved_by_.empty()) {
      return;
    }
    if (padding_reserved_by_.empty()) {
      padding_reserved_by_.assign(pixels_.size(), nullptr);
    }
    padding_reserved_by_[idx] = cell;
  }
  void clearPaddingReservations()
  {
    std::vector<Node*>().swap(padding_reserved_by_);
  }

  std::optional<odb::dbOrientType> getSiteOrientation(GridX x,
                                                      GridY y,
                                                      odb::dbSite* site) const;
  std::pair<odb::dbSite*, odb::dbOrientType> getShortestSite(GridX grid_x,
                                                             GridY grid_y);

  void clear();
  // Frees the pixels and keeps the rows, which gridX() and the other
  // coordinate conversions read; allocateGrid() makes them again.
  void releasePixels();
  bool hasPixels() const { return !pixels_.empty(); }
  // The rows are there and the pixels were released.
  bool pixelsReleased() const { return row_count_ > 0 && pixels_.empty(); }

  bool hasHybridRows() const { return has_hybrid_rows_; }

  GridY getRowCount(DbuY row_height) const;

  odb::Rect getCore() const { return core_; }

  // True if rect (core relative) is within min spacing of power via or
  // patch metal outside the stripes on the given routing level that belongs
  // to a net other than pin_net
  bool overlapsPinBlockage(int level,
                           const odb::Rect& rect,
                           const odb::dbNet* pin_net) const;

  bool cellFitsInCore(Node* cell) const;

  bool isMultiHeight(odb::dbMaster* master) const;

  // Utilization-aware placement support
  void computeUtilizationMap(Network* network,
                             float area_weight,
                             float pin_weight);
  void updateUtilizationMap(Node* node, DbuX x, DbuY y, bool add);
  float getUtilizationDensity(int pixel_idx) const;
  void normalizeUtilization();

 private:
  // Maps a site to the right orientation to use in a given row
  using SiteToOrientation = odb::PtrMap<odb::dbSite, odb::dbOrientType>;

  // Used to combine the SiteToOrientation for two intervals when merged
  template <typename MapType>
  struct SitesCombiner
  {
    using first_argument_type = MapType&;
    using second_argument_type = const MapType&;

    static MapType identity_element() { return MapType(); }

    void operator()(MapType& target, const MapType& source) const
    {
      target.insert(source.begin(), source.end());
    }
  };

  // Map intervals in rows to the site/orientation mapping
  using RowSitesMap = boost::icl::interval_map<int,
                                               SiteToOrientation,
                                               boost::icl::total_absorber,
                                               std::less,
                                               SitesCombiner>;

  using Pixels = std::vector<Pixel>;
  using NetRect = std::pair<odb::Rect, odb::dbNet*>;
  using NetRectTree
      = boost::geometry::index::rtree<NetRect,
                                      boost::geometry::index::quadratic<16>>;

  void markHopeless(odb::dbBlock* block,
                    int max_displacement_x,
                    int max_displacement_y);
  void markBlocked(odb::dbBlock* block);
  void visitDbRows(odb::dbBlock* block,
                   const std::function<void(odb::dbRow*)>& func) const;

  utl::Logger* logger_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  std::shared_ptr<Padding> padding_;
  Pixels pixels_;
  // Lazily allocated only when placement regions or cell padding exist.
  std::vector<Group*> pixel_groups_;
  std::vector<Node*> padding_reserved_by_;
  // Contains all the rows' yLo plus the yHi of the last row.  The extra
  // value is useful for operations like region snapping to rows
  std::map<DbuY, GridY> row_y_dbu_to_index_;
  std::vector<DbuY> row_index_to_y_dbu_;         // index is GridY
  std::vector<DbuY> row_index_to_pixel_height_;  // index is GridY

  // Indexed by row (GridY)
  std::vector<RowSitesMap> row_sites_;

  // Power via/patch metal outside the stripes (core relative) and its net,
  // by level
  std::array<NetRectTree, kMaxPinLevel + 1> pin_blockages_;

  bool has_hybrid_rows_ = false;
  odb::Rect core_;

  std::optional<DbuY> uniform_row_height_;  // unset if hybrid
  bool contiguous_uniform_rows_ = false;
  DbuX site_width_{0};

  GridY row_count_{0};
  GridX row_site_count_{0};

  // Utilization density map
  std::vector<float> utilization_density_;
  std::vector<float> total_area_;
  std::vector<float> total_pins_;
  float area_weight_ = 0.0f;
  float pin_weight_ = 0.0f;
  bool utilization_dirty_ = false;
  float last_max_area_ = 1.0f;
  float last_max_pins_ = 1.0f;
  float last_max_utilization_ = 1.0f;

  int countValidPixels(GridX x_begin,
                       GridY y_begin,
                       GridX x_end,
                       GridY y_end) const;
  void applyCellContribution(Node* node,
                             GridX x_begin,
                             GridY y_begin,
                             GridX x_end,
                             GridY y_end,
                             float scale);
};

}  // namespace dpl
