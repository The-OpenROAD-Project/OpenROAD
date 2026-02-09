// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

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
#include "boost/icl/interval_map.hpp"
#include "dpl/Opendp.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
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
  Group* group = nullptr;
  float util = 0.0;
  bool is_valid = false;     // false for dummy cells
  bool is_hopeless = false;  // too far from sites for diamond search
  uint8_t blocked_layers = 0;
  // Cell that reserved this pixel for padding
  Node* padding_reserved_by = nullptr;
};

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
  Pixel& pixel(GridY y, GridX x) { return pixels_[y.v][x.v]; }
  const Pixel& pixel(GridY y, GridX x) const { return pixels_[y.v][x.v]; }

  std::optional<odb::dbOrientType> getSiteOrientation(GridX x,
                                                      GridY y,
                                                      odb::dbSite* site) const;
  std::pair<odb::dbSite*, odb::dbOrientType> getShortestSite(GridX grid_x,
                                                             GridY grid_y);

  void resize(int size) { pixels_.resize(size); }
  void resize(GridY size) { pixels_.resize(size.v); }
  void resize(GridY y, GridX size) { pixels_[y.v].resize(size.v); }
  void clear();

  bool hasHybridRows() const { return has_hybrid_rows_; }

  GridY getRowCount(DbuY row_height) const;

  odb::Rect getCore() const { return core_; }
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
  using SiteToOrientation = std::map<odb::dbSite*, odb::dbOrientType>;

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

  using Pixels = std::vector<std::vector<Pixel>>;

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
  // Contains all the rows' yLo plus the yHi of the last row.  The extra
  // value is useful for operations like region snapping to rows
  std::map<DbuY, GridY> row_y_dbu_to_index_;
  std::vector<DbuY> row_index_to_y_dbu_;         // index is GridY
  std::vector<DbuY> row_index_to_pixel_height_;  // index is GridY

  // Indexed by row (GridY)
  std::vector<RowSitesMap> row_sites_;

  bool has_hybrid_rows_ = false;
  odb::Rect core_;

  std::optional<DbuY> uniform_row_height_;  // unset if hybrid
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
