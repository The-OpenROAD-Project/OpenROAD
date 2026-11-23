#include "PlacementDRC.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "boost/functional/hash.hpp"
#include "boost/geometry/index/predicates.hpp"
#include "boost/geometry/index/rtree.hpp"
#include "dpl/Opendp.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/Padding.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "odb/isotropy.h"
#include "utl/Logger.h"

namespace dpl {

using utl::DPL;

namespace cell_edges {
odb::Rect transformEdgeRect(const odb::Rect& edge_rect,
                            const Node* cell,
                            const DbuX x,
                            const DbuY y,
                            const odb::dbOrientType& orient)
{
  odb::Rect bbox;
  odb::dbMaster* db_master = nullptr;
  if (cell->getDbInst() != nullptr) {
    db_master = cell->getDbInst()->getMaster();
  } else if (cell->getMaster() != nullptr) {
    db_master = cell->getMaster()->getDbMaster();
  }
  if (db_master != nullptr) {
    db_master->getPlacementBoundary(bbox);
  }
  odb::dbTransform transform(orient);
  transform.apply(bbox);
  odb::Point offset(x.v - bbox.xMin(), y.v - bbox.yMin());
  transform.setOffset(offset);
  odb::Rect result(edge_rect);
  transform.apply(result);
  return result;
}
static odb::dbMasterEdgeType::EdgeDir transformEdgeDir(
    const odb::dbMasterEdgeType::EdgeDir dir,
    const odb::dbOrientType& orient)
{
  using odb::dbMasterEdgeType;
  using odb::dbOrientType;
  switch (orient.getValue()) {
    case dbOrientType::R0:
      return dir;
    case dbOrientType::R180:
      switch (dir) {
        case dbMasterEdgeType::TOP:
          return dbMasterEdgeType::BOTTOM;
        case dbMasterEdgeType::BOTTOM:
          return dbMasterEdgeType::TOP;
        case dbMasterEdgeType::LEFT:
          return dbMasterEdgeType::RIGHT;
        case dbMasterEdgeType::RIGHT:
          return dbMasterEdgeType::LEFT;
      }
      break;
    case dbOrientType::MY:
      switch (dir) {
        case dbMasterEdgeType::TOP:
        case dbMasterEdgeType::BOTTOM:
          return dir;
        case dbMasterEdgeType::LEFT:
          return dbMasterEdgeType::RIGHT;
        case dbMasterEdgeType::RIGHT:
          return dbMasterEdgeType::LEFT;
      }
      break;
    case dbOrientType::MX:
      switch (dir) {
        case dbMasterEdgeType::TOP:
          return dbMasterEdgeType::BOTTOM;
        case dbMasterEdgeType::BOTTOM:
          return dbMasterEdgeType::TOP;
        case dbMasterEdgeType::LEFT:
        case dbMasterEdgeType::RIGHT:
          return dir;
      }
      break;
    case dbOrientType::R90:
      switch (dir) {
        case dbMasterEdgeType::TOP:
          return dbMasterEdgeType::LEFT;
        case dbMasterEdgeType::BOTTOM:
          return dbMasterEdgeType::RIGHT;
        case dbMasterEdgeType::LEFT:
          return dbMasterEdgeType::BOTTOM;
        case dbMasterEdgeType::RIGHT:
          return dbMasterEdgeType::TOP;
      }
      break;
    case dbOrientType::R270:
      switch (dir) {
        case dbMasterEdgeType::TOP:
          return dbMasterEdgeType::RIGHT;
        case dbMasterEdgeType::BOTTOM:
          return dbMasterEdgeType::LEFT;
        case dbMasterEdgeType::LEFT:
          return dbMasterEdgeType::TOP;
        case dbMasterEdgeType::RIGHT:
          return dbMasterEdgeType::BOTTOM;
      }
      break;
    case dbOrientType::MYR90:
      switch (dir) {
        case dbMasterEdgeType::TOP:
          return dbMasterEdgeType::LEFT;
        case dbMasterEdgeType::BOTTOM:
          return dbMasterEdgeType::RIGHT;
        case dbMasterEdgeType::LEFT:
          return dbMasterEdgeType::TOP;
        case dbMasterEdgeType::RIGHT:
          return dbMasterEdgeType::BOTTOM;
      }
      break;
    case dbOrientType::MXR90:
      switch (dir) {
        case dbMasterEdgeType::TOP:
          return dbMasterEdgeType::RIGHT;
        case dbMasterEdgeType::BOTTOM:
          return dbMasterEdgeType::LEFT;
        case dbMasterEdgeType::LEFT:
          return dbMasterEdgeType::BOTTOM;
        case dbMasterEdgeType::RIGHT:
          return dbMasterEdgeType::TOP;
      }
      break;
  }
  return dir;
}
static odb::Rect getQueryRect(const odb::Rect& edge_box,
                              const int spc,
                              const bool is_vertical_edge)
{
  odb::Rect query_rect(edge_box);
  if (is_vertical_edge) {
    // vertical edge
    query_rect = query_rect.bloat(spc, odb::Orientation2D::Horizontal);
  } else {
    // horizontal edge
    query_rect = query_rect.bloat(spc, odb::Orientation2D::Vertical);
  }
  return query_rect;
}
};  // namespace cell_edges

class PlacementDRC::FixedSupplyVias
{
 public:
  FixedSupplyVias(utl::Logger* logger, Grid* grid, odb::dbBlock* block)
      : logger_(logger), grid_(grid)
  {
    for (odb::dbNet* net : block->getNets()) {
      if (!net->isSpecial() || !net->getSigType().isSupply()) {
        continue;
      }
      for (odb::dbSWire* swire : net->getSWires()) {
        for (odb::dbSBox* sbox : swire->getWires()) {
          if (sbox->isVia()) {
            std::vector<odb::dbShape> shapes;
            sbox->getViaBoxes(shapes);
            for (const odb::dbShape& shape : shapes) {
              addFixedShape(shape.getTechLayer(), shape.getBox(), net);
            }
          }
        }
      }
    }

    // Only fixed supply via layers can contribute placement conflicts.
    if (layers_.empty()) {
      return;
    }

    std::unordered_set<odb::dbMaster*> masters;
    for (odb::dbInst* inst : block->getInsts()) {
      odb::dbMaster* master = inst->getMaster();
      if (master != nullptr && master->isCoreAutoPlaceable()
          && !master->isBlock()) {
        masters.insert(master);
      }
    }
    for (odb::dbMaster* master : masters) {
      collectMasterShapes(master);
    }

    // Layers unused by instantiated masters cannot contribute conflicts.
    for (auto layer_it = layers_.begin(); layer_it != layers_.end();) {
      if (!master_dimensions_.contains(layer_it->first)) {
        layer_it = layers_.erase(layer_it);
      } else {
        ++layer_it;
      }
    }
    if (layers_.empty()) {
      return;
    }

    const bool has_right_angle_row
        = std::ranges::any_of(block->getRows(), [](odb::dbRow* row) {
            return row->getOrient().isRightAngleRotation();
          });
    // Legal row origins bound vertical conflicts without right-angle rotations.
    const LayerVerticalRanges cell_ranges
        = has_right_angle_row ? LayerVerticalRanges{}
                              : cellVerticalRanges(masters, block);
    size_t vertically_safe_shape_count = 0;
    int max_query_halo = 0;
    for (auto& [layer, data] : layers_) {
      const int max_spacing = maximumSpacing(layer, data);
      data.fixed_dimensions.clear();
      data.query_halo = std::max(0, max_spacing - 1);
      max_query_halo = std::max(max_query_halo, data.query_halo);
      if (!has_right_angle_row
          && !hasPotentialVerticalConflict(data, cell_ranges.at(layer))) {
        vertically_safe_shape_count += data.shapes.size();
      }
    }
    master_dimensions_.clear();
    if (vertically_safe_shape_count == fixedShapeCount()) {
      legal_sites_safe_ = true;
      debugPrint(logger_,
                 DPL,
                 "fixed_supply_via",
                 1,
                 "skipped fixed supply via checks at legal sites for {} "
                 "shapes that cannot violate spacing",
                 vertically_safe_shape_count);
      return;
    }

    buildIndexes();
    debugPrint(
        logger_,
        DPL,
        "fixed_supply_via",
        1,
        "indexed {} fixed supply via shapes on {} master-used layers from {} "
        "instantiated masters; maximum query halo={}",
        fixedShapeCount(),
        layers_.size(),
        masters.size(),
        max_query_halo);
  }

  bool check(const Node* cell,
             const GridX x,
             const GridY y,
             const odb::dbOrientType& orient)
  {
    const int site_count = grid_->getRowSiteCount().v;
    const int row_count = grid_->getRowCount().v;
    if (layers_.empty() || cell->getDbInst() == nullptr || x.v < 0
        || x.v >= site_count || y.v < 0 || y.v >= row_count) {
      return true;
    }
    odb::dbMaster* master = cell->getMaster()->getDbMaster();
    if (legal_sites_safe_) {
      const std::optional<odb::dbOrientType> site_orientation
          = grid_->getSiteOrientation(x, y, master->getSite());
      const int64_t origin_y = static_cast<int64_t>(grid_->getCore().yMin())
                               + grid_->gridYToDbu(y).v;
      if (site_orientation.has_value() && site_orientation.value() == orient
          && origin_y + master->getHeight() <= grid_->getCore().yMax()) {
        return true;
      }
      buildIndexes();
    }
    const int page = x.v / page_size_;
    const PhysicalPageKey page_key{master, orient.getValue(), y.v, page};
    auto page_it = physical_pages_.find(page_key);
    if (page_it == physical_pages_.end()) {
      page_it = physical_pages_
                    .emplace(page_key, generatePage(master, orient, y.v, page))
                    .first;
    }

    const PageRecipe& recipe = page_it->second;
    const uint64_t query_mask = uint64_t{1} << (x.v % page_size_);
    if (recipe.unconditional_mask & query_mask) {
      return false;
    }
    // Skip terminal scans when no terminal shape conflicts at this site.
    if ((recipe.conditional_mask & query_mask) == 0) {
      return true;
    }

    for (const RecipeEntry& entry : recipe.entries) {
      if ((entry.mask & query_mask) == 0) {
        continue;
      }
      odb::dbNet* pin_net = cell->getDbInst()->getITerm(entry.term)->getNet();
      if (pin_net != entry.net) {
        if (pin_net == nullptr && entry.term->getSigType().isSupply()
            && !warned_unconnected_supply_) {
          logger_->warn(DPL,
                        502,
                        "Unconnected supply pin {}/{} prevents same-net PDN "
                        "exemptions. Connect supply pins before PDN-aware "
                        "legalization.",
                        cell->getDbInst()->getName(),
                        entry.term->getName());
          warned_unconnected_supply_ = true;
        }
        return false;
      }
    }
    return true;
  }

  bool empty() const { return layers_.empty(); }

 private:
  struct Shape
  {
    odb::Rect rect;
    odb::dbNet* net;
  };

  struct MasterShape
  {
    odb::Rect rect;
    odb::dbTechLayer* layer;
    odb::dbMTerm* term;
  };

  struct RecipeEntry
  {
    odb::dbMTerm* term;
    odb::dbNet* net;
    uint64_t mask;
  };

  struct PageRecipe
  {
    uint64_t unconditional_mask{0};
    uint64_t conditional_mask{0};
    std::vector<RecipeEntry> entries;
  };

  struct VerticalRange
  {
    int y_min{std::numeric_limits<int>::max()};
    int y_max{std::numeric_limits<int>::min()};

    bool empty() const { return y_min > y_max; }

    void merge(const int low, const int high)
    {
      if (low > high) {
        return;
      }
      y_min = std::min(y_min, low);
      y_max = std::max(y_max, high);
    }
  };

  struct RecipeEntryKey
  {
    odb::dbMTerm* term;
    odb::dbNet* net;
  };

  struct RecipeEntryKeyLess
  {
    bool operator()(const RecipeEntryKey& lhs, const RecipeEntryKey& rhs) const
    {
      return std::pair(lhs.term->getId(), lhs.net->getId())
             < std::pair(rhs.term->getId(), rhs.net->getId());
    }
  };

  using DimensionClass = std::pair<int, int>;
  using IndexValue = std::pair<odb::Rect, size_t>;
  using ShapeIndex
      = boost::geometry::index::rtree<IndexValue,
                                      boost::geometry::index::quadratic<16>>;

  struct LayerData
  {
    std::vector<Shape> shapes;
    std::set<DimensionClass> fixed_dimensions;
    std::unique_ptr<ShapeIndex> index;
    int query_halo{0};
  };

  using GeometryKey = std::pair<odb::dbMaster*, int>;
  using PhysicalPageKey = std::tuple<odb::dbMaster*, int, int, int>;
  using LayerVerticalRanges
      = std::unordered_map<odb::dbTechLayer*, std::vector<VerticalRange>>;
  using RowOrientations = std::vector<std::vector<odb::dbOrientType>>;
  static constexpr int page_size_ = 64;

  size_t fixedShapeCount() const
  {
    size_t count = 0;
    for (const auto& [layer, data] : layers_) {
      count += data.shapes.size();
    }
    return count;
  }

  void buildIndexes()
  {
    for (auto& [layer, data] : layers_) {
      if (data.index != nullptr) {
        continue;
      }
      std::vector<IndexValue> values;
      values.reserve(data.shapes.size());
      for (size_t i = 0; i < data.shapes.size(); ++i) {
        values.emplace_back(data.shapes[i].rect, i);
      }
      data.index = std::make_unique<ShapeIndex>(values.begin(), values.end());
    }
  }

  static bool isUsedShapeLayer(odb::dbTechLayer* layer)
  {
    return layer != nullptr
           && (layer->getType() == odb::dbTechLayerType::ROUTING
               || layer->getType() == odb::dbTechLayerType::CUT);
  }

  static int clipToInt(const int64_t value)
  {
    return static_cast<int>(
        std::clamp<int64_t>(value,
                            std::numeric_limits<int>::min(),
                            std::numeric_limits<int>::max()));
  }

  static odb::Rect bloatClipped(const odb::Rect& rect, const int amount)
  {
    return {clipToInt(static_cast<int64_t>(rect.xMin()) - amount),
            clipToInt(static_cast<int64_t>(rect.yMin()) - amount),
            clipToInt(static_cast<int64_t>(rect.xMax()) + amount),
            clipToInt(static_cast<int64_t>(rect.yMax()) + amount)};
  }

  static int64_t floorSqrt(const int64_t value)
  {
    if (value <= 0) {
      return 0;
    }
    int64_t low = 0;
    int64_t high = std::min<int64_t>(value, std::numeric_limits<int>::max());
    while (low <= high) {
      const int64_t mid = low + (high - low) / 2;
      if (mid * mid <= value) {
        low = mid + 1;
      } else {
        high = mid - 1;
      }
    }
    return high;
  }

  static int64_t verticalDistance(const odb::Rect& fixed,
                                  const odb::Rect& cell,
                                  const int64_t origin_y)
  {
    const int64_t cell_y_min = origin_y + cell.yMin();
    const int64_t cell_y_max = origin_y + cell.yMax();
    if (cell_y_max < fixed.yMin()) {
      return static_cast<int64_t>(fixed.yMin()) - cell_y_max;
    }
    if (cell_y_min > fixed.yMax()) {
      return cell_y_min - static_cast<int64_t>(fixed.yMax());
    }
    return 0;
  }

  static int64_t horizontalKeepout(const int space, const int64_t vertical_gap)
  {
    if (space <= 0 || vertical_gap >= space) {
      return -1;
    }
    const int64_t space_squared = static_cast<int64_t>(space) * space;
    const int64_t gap_squared = vertical_gap * vertical_gap;
    // Subtract one to preserve strict integer spacing at the square-root bound.
    return floorSqrt(space_squared - gap_squared - 1);
  }

  static uint64_t bitRange(const int first, const int last)
  {
    return (~uint64_t{0} << first) & (~uint64_t{0} >> (page_size_ - 1 - last));
  }

  void collectMasterShapes(odb::dbMaster* master)
  {
    std::vector<MasterShape> shapes;
    auto add_rect = [&](odb::dbTechLayer* layer,
                        const odb::Rect& rect,
                        odb::dbMTerm* term) {
      if (!isUsedShapeLayer(layer) || !layers_.contains(layer)) {
        return;
      }
      master_dimensions_[layer].insert({rect.dx(), rect.dy()});
      shapes.push_back({rect, layer, term});
    };
    auto add_box = [&](odb::dbBox* box, odb::dbMTerm* term) {
      if (!box->isVia()) {
        add_rect(box->getTechLayer(), box->getBox(), term);
        return;
      }
      std::vector<odb::dbShape> via_shapes;
      box->getViaBoxes(via_shapes);
      for (const odb::dbShape& shape : via_shapes) {
        add_rect(shape.getTechLayer(), shape.getBox(), term);
      }
    };
    for (odb::dbBox* obs : master->getObstructions()) {
      add_box(obs, nullptr);
    }
    for (odb::dbMTerm* term : master->getMTerms()) {
      for (odb::dbMPin* pin : term->getMPins()) {
        for (odb::dbBox* box : pin->getGeometry()) {
          add_box(box, term);
        }
      }
    }
    if (!shapes.empty()) {
      source_master_shapes_.emplace(master, std::move(shapes));
    }
  }

  void addFixedShape(odb::dbTechLayer* layer,
                     const odb::Rect& rect,
                     odb::dbNet* net)
  {
    if (!isUsedShapeLayer(layer)) {
      return;
    }
    LayerData& data = layers_[layer];
    data.shapes.push_back({rect, net});
    data.fixed_dimensions.insert({rect.dx(), rect.dy()});
  }

  int maximumSpacing(odb::dbTechLayer* layer, const LayerData& data) const
  {
    int max_spacing = 0;
    const auto dimensions_it = master_dimensions_.find(layer);
    if (dimensions_it == master_dimensions_.end()) {
      return max_spacing;
    }
    for (const DimensionClass& fixed : data.fixed_dimensions) {
      const odb::Rect fixed_rect(0, 0, fixed.first, fixed.second);
      for (const DimensionClass& cell : dimensions_it->second) {
        const odb::Rect cell_rect(0, 0, cell.first, cell.second);
        max_spacing
            = std::max(max_spacing, spacing(layer, fixed_rect, cell_rect));
      }
    }
    return max_spacing;
  }

  RowOrientations rowOrientations(odb::dbBlock* block,
                                  odb::dbSite* master_site) const
  {
    const odb::Rect core = grid_->getCore();
    RowOrientations orientations(grid_->getRowCount().v);
    auto add_orientation = [](std::vector<odb::dbOrientType>& row_orientations,
                              const odb::dbOrientType orient) {
      if (std::ranges::none_of(row_orientations, [&](const auto& existing) {
            return existing == orient;
          })) {
        row_orientations.push_back(orient);
      }
    };
    bool matching_row_found = false;
    for (odb::dbRow* row : block->getRows()) {
      if (row->getSite() != master_site) {
        continue;
      }
      matching_row_found = true;
      const int grid_row = std::clamp(
          grid_->gridSnapDownY(DbuY{row->getOrigin().y() - core.yMin()}).v,
          0,
          grid_->getRowCount().v - 1);
      const odb::dbOrientType orient = row->getOrient();
      if (orient == odb::dbOrientType::R0 || orient == odb::dbOrientType::MY) {
        add_orientation(orientations[grid_row], odb::dbOrientType::R0);
      } else if (orient == odb::dbOrientType::MX
                 || orient == odb::dbOrientType::R180) {
        add_orientation(orientations[grid_row], odb::dbOrientType::MX);
      } else {
        add_orientation(orientations[grid_row], odb::dbOrientType::R0);
        add_orientation(orientations[grid_row], odb::dbOrientType::MX);
      }
    }
    if (!matching_row_found || master_site == nullptr
        || master_site->hasRowPattern()) {
      for (std::vector<odb::dbOrientType>& row_orientations : orientations) {
        add_orientation(row_orientations, odb::dbOrientType::R0);
        add_orientation(row_orientations, odb::dbOrientType::MX);
      }
    }
    return orientations;
  }

  LayerVerticalRanges cellVerticalRanges(
      const std::unordered_set<odb::dbMaster*>& masters,
      odb::dbBlock* block)
  {
    const int row_count = grid_->getRowCount().v;
    LayerVerticalRanges ranges;
    for (const auto& [layer, data] : layers_) {
      ranges.emplace(layer, std::vector<VerticalRange>(row_count));
    }

    const odb::Rect core = grid_->getCore();
    std::unordered_map<odb::dbSite*, RowOrientations> site_orientations;
    for (odb::dbMaster* master : masters) {
      auto [orientations_it, inserted]
          = site_orientations.try_emplace(master->getSite());
      if (inserted) {
        orientations_it->second = rowOrientations(block, master->getSite());
      }
      const RowOrientations& row_orientations = orientations_it->second;
      for (int start_row = 0; start_row < row_count; ++start_row) {
        for (const odb::dbOrientType& orient : row_orientations[start_row]) {
          const std::vector<MasterShape>& shapes = masterShapes(master, orient);
          const int origin_y
              = core.yMin() + grid_->gridYToDbu(GridY{start_row}).v;
          if (static_cast<int64_t>(origin_y) + master->getHeight()
              > core.yMax()) {
            continue;
          }
          for (const MasterShape& shape : shapes) {
            const int shape_y_min
                = clipToInt(static_cast<int64_t>(origin_y) + shape.rect.yMin());
            const int shape_y_max
                = clipToInt(static_cast<int64_t>(origin_y) + shape.rect.yMax());
            const int first_row = std::clamp(
                grid_->gridSnapDownY(DbuY{shape_y_min - core.yMin()}).v,
                0,
                row_count - 1);
            const int last_row
                = std::clamp(grid_->gridEndY(DbuY{shape_y_max - core.yMin()}).v,
                             0,
                             row_count - 1);
            std::vector<VerticalRange>& layer_ranges = ranges.at(shape.layer);
            for (int row = first_row; row <= last_row; ++row) {
              const int row_y_min
                  = core.yMin() + grid_->gridYToDbu(GridY{row}).v;
              const int row_y_max
                  = core.yMin() + grid_->gridYToDbu(GridY{row + 1}).v;
              const int clipped_y_min
                  = row == 0 ? shape_y_min : std::max(shape_y_min, row_y_min);
              const int clipped_y_max = row == row_count - 1
                                            ? shape_y_max
                                            : std::min(shape_y_max, row_y_max);
              layer_ranges[row].merge(clipped_y_min, clipped_y_max);
            }
          }
        }
      }
    }
    return ranges;
  }

  bool hasPotentialVerticalConflict(
      const LayerData& data,
      const std::vector<VerticalRange>& cell_ranges) const
  {
    const odb::Rect core = grid_->getCore();
    const int row_count = grid_->getRowCount().v;
    for (const Shape& fixed : data.shapes) {
      const int y_min = clipToInt(static_cast<int64_t>(fixed.rect.yMin())
                                  - data.query_halo - core.yMin());
      const int y_max = clipToInt(static_cast<int64_t>(fixed.rect.yMax())
                                  + data.query_halo - core.yMin());
      const int first_row
          = std::clamp(grid_->gridSnapDownY(DbuY{y_min}).v, 0, row_count - 1);
      const int last_row
          = std::clamp(grid_->gridEndY(DbuY{y_max}).v, 0, row_count - 1);
      for (int row = first_row; row <= last_row; ++row) {
        const VerticalRange& cell = cell_ranges[row];
        if (cell.empty()) {
          continue;
        }
        if (static_cast<int64_t>(fixed.rect.yMax()) + data.query_halo
                >= cell.y_min
            && static_cast<int64_t>(cell.y_max) + data.query_halo
                   >= fixed.rect.yMin()) {
          return true;
        }
      }
    }
    return false;
  }

  odb::Rect pageQueryRect(const MasterShape& cell_shape,
                          const int first_site,
                          const int last_site,
                          const int row,
                          const int halo) const
  {
    const odb::Rect core = grid_->getCore();
    const int site_width = grid_->getSiteWidth().v;
    const int first_x
        = clipToInt(static_cast<int64_t>(core.xMin())
                    + static_cast<int64_t>(first_site) * site_width);
    const int last_x
        = clipToInt(static_cast<int64_t>(core.xMin())
                    + static_cast<int64_t>(last_site) * site_width);
    const int origin_y = clipToInt(static_cast<int64_t>(core.yMin())
                                   + grid_->gridYToDbu(GridY{row}).v);
    const odb::Rect swept(
        clipToInt(static_cast<int64_t>(first_x) + cell_shape.rect.xMin()),
        clipToInt(static_cast<int64_t>(origin_y) + cell_shape.rect.yMin()),
        clipToInt(static_cast<int64_t>(last_x) + cell_shape.rect.xMax()),
        clipToInt(static_cast<int64_t>(origin_y) + cell_shape.rect.yMax()));
    return bloatClipped(swept, halo);
  }

  bool addConflict(const MasterShape& cell_shape,
                   const Shape& fixed,
                   const int row,
                   const int first_site,
                   const int last_site,
                   uint64_t& mask) const
  {
    const odb::Rect core = grid_->getCore();
    const int site_width = grid_->getSiteWidth().v;
    const int space = spacing(cell_shape.layer, fixed.rect, cell_shape.rect);
    if (cell_shape.layer->getType() == odb::dbTechLayerType::CUT) {
      const int row_count = grid_->getRowCount().v;
      const odb::Rect keepout
          = bloatClipped(fixed.rect, std::max(0, space - 1));
      const int origin_y_min
          = clipToInt(static_cast<int64_t>(keepout.yMin())
                      - cell_shape.rect.yMax() - core.yMin());
      const int origin_y_max
          = clipToInt(static_cast<int64_t>(keepout.yMax())
                      - cell_shape.rect.yMin() - core.yMin());
      const int first_origin = grid_->gridYToDbu(GridY{0}).v;
      const int last_origin = grid_->gridYToDbu(GridY{row_count - 1}).v;
      if (origin_y_max < first_origin || origin_y_min > last_origin) {
        return false;
      }
      const int row_begin
          = std::clamp(grid_->gridEndY(DbuY{origin_y_min}).v, 0, row_count - 1);
      const int row_end = std::clamp(
          grid_->gridSnapDownY(DbuY{origin_y_max}).v, 0, row_count - 1);
      if (row < row_begin || row > row_end) {
        return false;
      }
    }

    const int64_t origin_y
        = static_cast<int64_t>(core.yMin()) + grid_->gridYToDbu(GridY{row}).v;
    int low;
    int high;
    if (space == 0) {
      // Zero spacing permits boundary contact, but not positive-area overlap.
      if (origin_y + cell_shape.rect.yMax() <= fixed.rect.yMin()
          || origin_y + cell_shape.rect.yMin() >= fixed.rect.yMax()) {
        return false;
      }
      low = clipToInt(static_cast<int64_t>(fixed.rect.xMin())
                      - cell_shape.rect.xMax() + 1);
      high = clipToInt(static_cast<int64_t>(fixed.rect.xMax())
                       - cell_shape.rect.xMin() - 1);
    } else {
      // Routing and cut spacing use strict Euclidean distance in integer DBU.
      const int64_t vertical_gap
          = verticalDistance(fixed.rect, cell_shape.rect, origin_y);
      const int64_t x_keepout = horizontalKeepout(space, vertical_gap);
      if (x_keepout < 0) {
        return false;
      }
      low = clipToInt(static_cast<int64_t>(fixed.rect.xMin()) - x_keepout
                      - cell_shape.rect.xMax());
      high = clipToInt(static_cast<int64_t>(fixed.rect.xMax()) + x_keepout
                       - cell_shape.rect.xMin());
    }
    if (high < core.xMin()) {
      return false;
    }
    const int begin = divCeil(
        clipToInt(static_cast<int64_t>(low) - core.xMin()), site_width);
    const int end = divFloor(
        clipToInt(static_cast<int64_t>(high) - core.xMin()), site_width);
    const int clipped_begin = std::max(begin, first_site);
    const int clipped_end = std::min(end, last_site);
    if (clipped_begin > clipped_end) {
      return false;
    }
    mask |= bitRange(clipped_begin - first_site, clipped_end - first_site);
    return true;
  }

  PageRecipe generatePage(odb::dbMaster* master,
                          const odb::dbOrientType& orient,
                          const int row,
                          const int page)
  {
    const int site_count = grid_->getRowSiteCount().v;
    const int first_site = page * page_size_;
    const int last_site = static_cast<int>(
        std::min<int64_t>(static_cast<int64_t>(first_site) + page_size_ - 1,
                          static_cast<int64_t>(site_count) - 1));
    std::map<RecipeEntryKey, uint64_t, RecipeEntryKeyLess> entries;
    PageRecipe recipe;
    for (const MasterShape& cell_shape : masterShapes(master, orient)) {
      const auto layer_it = layers_.find(cell_shape.layer);
      if (layer_it == layers_.end()) {
        continue;
      }
      const LayerData& data = layer_it->second;
      const odb::Rect query = pageQueryRect(
          cell_shape, first_site, last_site, row, data.query_halo);
      for (auto it
           = data.index->qbegin(boost::geometry::index::intersects(query));
           it != data.index->qend();
           ++it) {
        const Shape& fixed = data.shapes[it->second];
        uint64_t fixed_mask = 0;
        if (!addConflict(
                cell_shape, fixed, row, first_site, last_site, fixed_mask)) {
          continue;
        }
        if (cell_shape.term == nullptr) {
          recipe.unconditional_mask |= fixed_mask;
        } else {
          entries[{cell_shape.term, fixed.net}] |= fixed_mask;
        }
      }
    }

    recipe.entries.reserve(entries.size());
    for (const auto& [key, mask] : entries) {
      recipe.entries.push_back({key.term, key.net, mask});
      recipe.conditional_mask |= mask;
    }

    return recipe;
  }

  static int spacing(odb::dbTechLayer* layer,
                     const odb::Rect& fixed,
                     const odb::Rect& cell)
  {
    // Scalar rules cover width/PRL routing spacing and default cut spacing.
    if (layer->getType() == odb::dbTechLayerType::CUT) {
      return layer->getSpacing();
    }
    const int fixed_width = std::min(fixed.dx(), fixed.dy());
    const int cell_width = std::min(cell.dx(), cell.dy());
    const int length = std::max({fixed.dx(), fixed.dy(), cell.dx(), cell.dy()});
    return std::max({layer->getSpacing(fixed_width, length),
                     layer->getSpacing(cell_width, length),
                     layer->findTwSpacing(fixed_width, cell_width, length)});
  }

  const std::vector<MasterShape>& masterShapes(odb::dbMaster* master,
                                               const odb::dbOrientType& orient)
  {
    const GeometryKey key{master, orient.getValue()};
    auto [it, inserted] = master_shapes_.try_emplace(key);
    if (!inserted) {
      return it->second;
    }
    std::vector<MasterShape>& shapes = it->second;
    odb::Rect boundary;
    master->getPlacementBoundary(boundary);
    odb::dbTransform transform(orient);
    transform.apply(boundary);
    transform.setOffset({-boundary.xMin(), -boundary.yMin()});
    const auto source_it = source_master_shapes_.find(master);
    if (source_it == source_master_shapes_.end()) {
      return it->second;
    }
    for (const MasterShape& source : source_it->second) {
      if (layers_.contains(source.layer)) {
        odb::Rect rect(source.rect);
        transform.apply(rect);
        shapes.push_back({rect, source.layer, source.term});
      }
    }
    return it->second;
  }

  utl::Logger* logger_;
  Grid* grid_;
  std::unordered_map<odb::dbTechLayer*, std::set<DimensionClass>>
      master_dimensions_;
  std::unordered_map<odb::dbMaster*, std::vector<MasterShape>>
      source_master_shapes_;
  std::unordered_map<odb::dbTechLayer*, LayerData> layers_;
  std::map<GeometryKey, std::vector<MasterShape>> master_shapes_;
  // Lazy caches rely on placement DRC's single-threaded check() contract.
  bool legal_sites_safe_{false};
  bool warned_unconnected_supply_{false};
  std::unordered_map<PhysicalPageKey, PageRecipe, boost::hash<PhysicalPageKey>>
      physical_pages_;
};

class PlacementDRC::ImplantDRC
{
 public:
  ImplantDRC(Grid* grid, odb::dbTech* tech) : grid_(grid), tech_(tech)
  {
    if (tech_ == nullptr) {
      return;
    }
    for (odb::dbTechLayer* layer : tech_->getLayers()) {
      const auto type = layer->getType();
      if (type != odb::dbTechLayerType::IMPLANT) {
        continue;
      }
      const bool is_implant = true;
      const int base_w = is_implant
                             ? static_cast<int>(std::max(layer->getWidth(),
                                                         layer->getMinWidth()))
                             : 0;
      const int base_s = is_implant ? layer->getSpacing() : 0;
      const bool has_v54_s = is_implant && !layer->getV54SpacingRules().empty();
      const bool has_lef58
          = !layer->getTechLayerImplantWidthRules().empty()
            || !layer->getTechLayerImplantSpacingRules().empty()
            || !layer->getTechLayerMinStepRules().empty()
            || !layer->getTechLayerCoreEdgeLengthRules().empty()
            || !layer->getTechLayerCornerSpacingRules().empty()
            || !layer->getTechLayerMaxSpacingRules().empty()
            || !layer->getTechLayerAreaRules().empty()
            || !layer->getMinEncRules().empty();
      if (base_w <= 0 && base_s <= 0 && !has_v54_s && !has_lef58) {
        continue;
      }

      has_rules_ = true;
      rule_layers_.push_back(layer);
      tracked_layers_.insert(layer);

      int local_h = std::max(2 * base_w, base_s);
      int area_h = 0;
      max_interaction_ = std::max(max_interaction_, base_s);
      local_pinch_halo_ = std::max({local_pinch_halo_, base_w, base_s});

      if (is_implant) {
        for (odb::dbTechLayerSpacingRule* sr : layer->getV54SpacingRules()) {
          const int spc = static_cast<int>(sr->getSpacing());
          local_h = std::max(local_h, spc);
          max_interaction_ = std::max(max_interaction_, spc);
          local_pinch_halo_ = std::max(local_pinch_halo_, spc);
          odb::dbTechLayer* sec = nullptr;
          if (sr->getCutLayer4Spacing(sec) && sec != nullptr) {
            tracked_layers_.insert(sec);
            layer_deps_[sec].push_back(layer);
          }
        }
      }

      for (auto* r : layer->getTechLayerImplantWidthRules()) {
        local_h = std::max(
            {local_h, r->getMinWidth() + r->getMaxWidth(), r->getLength()});
        local_pinch_halo_ = std::max(local_pinch_halo_, r->getMinWidth());
        if (r->getSecondLayer() != nullptr) {
          tracked_layers_.insert(r->getSecondLayer());
          layer_deps_[r->getSecondLayer()].push_back(layer);
        }
        if (r->isLayerAny()) {
          for (odb::dbTechLayer* l2 : tech_->getLayers()) {
            if (l2->getType() == odb::dbTechLayerType::IMPLANT) {
              tracked_layers_.insert(l2);
              layer_deps_[l2].push_back(layer);
            }
          }
        }
        for (odb::dbTechLayer* wl : r->getWithinLayers()) {
          if (wl != nullptr) {
            tracked_layers_.insert(wl);
            layer_deps_[wl].push_back(layer);
          }
        }
      }

      for (auto* r : layer->getTechLayerImplantSpacingRules()) {
        const int spc = r->getMinSpacing();
        const int prl_ext = r->isPrlValid() ? std::abs(r->getPrl()) : 0;
        local_h = std::max({local_h, spc + prl_ext, r->getLength() + spc});
        max_interaction_ = std::max(max_interaction_, spc + prl_ext);
        local_pinch_halo_ = std::max(local_pinch_halo_, spc + prl_ext);
        if (r->getSecondLayer() != nullptr) {
          tracked_layers_.insert(r->getSecondLayer());
          layer_deps_[r->getSecondLayer()].push_back(layer);
        }
        for (odb::dbTechLayer* il : r->getIntersectLayers()) {
          if (il != nullptr) {
            tracked_layers_.insert(il);
            layer_deps_[il].push_back(layer);
          }
        }
      }

      for (auto* r : layer->getTechLayerMinStepRules()) {
        const int step = r->getMinStepLength();
        const int adj = r->isMinAdjLength1Valid() ? r->getMinAdjLength1() : 0;
        local_h = std::max(local_h, step + 2 * adj);
        local_pinch_halo_ = std::max(local_pinch_halo_, step + adj);
      }

      for (auto* r : layer->getTechLayerCoreEdgeLengthRules()) {
        local_h = std::max({local_h, r->getMinLength(), r->getMaxLength()});
        local_pinch_halo_ = std::max(local_pinch_halo_, r->getMinLength());
        for (odb::dbTechLayer* wl : r->getWithinLayers()) {
          if (wl != nullptr) {
            tracked_layers_.insert(wl);
            layer_deps_[wl].push_back(layer);
          }
        }
        for (odb::dbTechLayer* el : r->getExceptLayers()) {
          if (el != nullptr) {
            tracked_layers_.insert(el);
            layer_deps_[el].push_back(layer);
          }
        }
      }

      for (auto* r : layer->getTechLayerCornerSpacingRules()) {
        std::vector<int> w_tbl;
        std::vector<std::pair<int, int>> tbl;
        r->getWidthTable(w_tbl);
        r->getSpacingTable(tbl);
        for (const int w : w_tbl) {
          local_h = std::max(local_h, w);
        }
        for (const auto& [spc1, spc2] : tbl) {
          const int spc = std::max(spc1, spc2);
          local_h = std::max(local_h, spc);
          max_interaction_ = std::max(max_interaction_, spc);
          local_pinch_halo_ = std::max(local_pinch_halo_, spc);
        }
      }

      for (auto* r : layer->getTechLayerMaxSpacingRules()) {
        if (r->hasCutClass()) {
          continue;
        }
        local_h = std::max(
            {local_h, r->getMaxSpacing(), r->getWidth(), r->getLength()});
        max_interaction_ = std::max(max_interaction_, r->getMaxSpacing());
        local_pinch_halo_ = std::max(local_pinch_halo_, r->getMaxSpacing());
      }

      for (auto* r : layer->getTechLayerAreaRules()) {
        const int64_t area = r->getArea();
        if (area > 0) {
          int bound = 1;
          while (static_cast<int64_t>(bound) * bound < area
                 && bound < 1000000) {
            bound *= 2;
          }
          area_h = std::max(area_h, bound);
        }
      }

      for (auto* r : layer->getMinEncRules()) {
        int64_t area = 0;
        if (r->getEnclosure(area) && area > 0) {
          int bound = 1;
          while (static_cast<int64_t>(bound) * bound < area
                 && bound < 1000000) {
            bound *= 2;
          }
          area_h = std::max(area_h, bound);
        }
      }

      layer_local_halo_[layer] = local_h;
      layer_area_halo_[layer] = area_h;
      local_pinch_halo_ = std::max(local_pinch_halo_, local_h);
    }

    if (odb::dbDatabase* db = tech_->getDb(); db != nullptr) {
      auto recordMasterPairs = [&](odb::dbMaster* master) {
        std::vector<std::pair<odb::dbTechLayer*, odb::Rect>> imp_boxes;
        for (odb::dbBox* box : master->getObstructions()) {
          odb::dbTechLayer* layer = box->getTechLayer();
          if (layer != nullptr
              && layer->getType() == odb::dbTechLayerType::IMPLANT) {
            imp_boxes.emplace_back(layer, box->getBox());
          }
        }
        for (size_t i = 0; i < imp_boxes.size(); ++i) {
          for (size_t j = i + 1; j < imp_boxes.size(); ++j) {
            if (imp_boxes[i].first != imp_boxes[j].first
                && overlap1D(imp_boxes[i].second.xMin(),
                             imp_boxes[i].second.xMax(),
                             imp_boxes[j].second.xMin(),
                             imp_boxes[j].second.xMax())
                       > 0) {
              compatible_implant_pairs_.emplace(imp_boxes[i].first,
                                                imp_boxes[j].first);
              compatible_implant_pairs_.emplace(imp_boxes[j].first,
                                                imp_boxes[i].first);
            }
          }
        }
      };
      for (odb::dbLib* lib : db->getLibs()) {
        for (odb::dbMaster* master : lib->getMasters()) {
          const std::string_view name(master->getConstName());
          if (!master->isFiller() && !name.starts_with("FILL")) {
            continue;
          }
          for (odb::dbBox* box : master->getObstructions()) {
            odb::dbTechLayer* layer = box->getTechLayer();
            if (layer != nullptr
                && layer->getType() == odb::dbTechLayerType::IMPLANT) {
              filler_layers_.insert(layer);
            }
          }
          recordMasterPairs(master);
        }
      }
      if (filler_layers_.empty()) {
        for (odb::dbLib* lib : db->getLibs()) {
          for (odb::dbMaster* master : lib->getMasters()) {
            recordMasterPairs(master);
          }
        }
      }
    }
  }

  bool hasRules() const { return has_rules_; }

  bool check(const Node* cell,
             const GridX x,
             const GridY y,
             const odb::dbOrientType& orient,
             const bool strict_empty_sites,
             const bool require_filler_master = false) const
  {
    if (!has_rules_ || cell == nullptr || grid_ == nullptr) {
      return true;
    }
    odb::dbMaster* db_master = getDbMaster(cell);
    if (db_master == nullptr) {
      return true;
    }
    const MasterLayerShapes& cand_shapes = getMasterShapes(db_master, orient);
    if ((strict_empty_sites || require_filler_master) && cand_shapes.empty()) {
      return true;
    }

    const int site_w = grid_->getSiteWidth().v;
    const int row_h
        = (grid_->getRowCount().v > 0) ? rowHeightDbu(0) : cell->getHeight().v;
    const int x_real = x.v * site_w;
    const int y_real = grid_->gridYToDbu(y).v;
    const odb::Rect cell_box(x_real,
                             y_real,
                             x_real + cell->getWidth().v,
                             y_real + cell->getHeight().v);

    int active_halo = strict_empty_sites ? 0 : local_pinch_halo_;
    for (const auto& [cl, _] : cand_shapes) {
      if (auto it = layer_local_halo_.find(cl); it != layer_local_halo_.end()) {
        active_halo = std::max(active_halo, it->second);
      }
      if (auto it = layer_area_halo_.find(cl); it != layer_area_halo_.end()) {
        active_halo = std::max(active_halo, it->second);
      }
      if (auto dit = layer_deps_.find(cl); dit != layer_deps_.end()) {
        for (odb::dbTechLayer* rl : dit->second) {
          active_halo = std::max(active_halo, layer_local_halo_.at(rl));
        }
      }
    }

    const int halo_x = std::max(active_halo, 2 * site_w) + 2 * site_w;
    const int halo_y = std::max(active_halo, 2 * row_h) + 2 * row_h;
    const odb::Rect query_box(cell_box.xMin() - halo_x,
                              cell_box.yMin() - halo_y,
                              cell_box.xMax() + halo_x,
                              cell_box.yMax() + halo_y);

    const int interact_pad = std::max(1, max_interaction_);
    const odb::Rect interact_box(cell_box.xMin() - interact_pad,
                                 cell_box.yMin() - interact_pad,
                                 cell_box.xMax() + interact_pad,
                                 cell_box.yMax() + interact_pad);
    const odb::Rect touch_box(cell_box.xMin() - 1,
                              cell_box.yMin() - 1,
                              cell_box.xMax() + 1,
                              cell_box.yMax() + 1);

    // Collect candidate cell + placed neighbors in the local grid window
    scratch_.local_cells.clear();
    if (!cand_shapes.empty()) {
      scratch_.local_cells.push_back({
          .master = db_master,
          .orient = orient,
          .x = x_real,
          .y = y_real,
      });
    }

    const int site_count = grid_->getRowSiteCount().v;
    const int row_count = grid_->getRowCount().v;
    if (site_count > 0 && row_count > 0) {
      const int gx_min
          = std::clamp(grid_->gridX(DbuX(std::max(0, query_box.xMin()))).v,
                       0,
                       site_count - 1);
      const int gx_max
          = std::clamp(grid_->gridEndX(DbuX(std::max(0, query_box.xMax()))).v,
                       0,
                       site_count - 1);
      const int gy_min = std::clamp(
          grid_->gridSnapDownY(DbuY(std::max(0, query_box.yMin()))).v,
          0,
          row_count - 1);
      const int gy_max
          = std::clamp(grid_->gridEndY(DbuY(std::max(0, query_box.yMax()))).v,
                       0,
                       row_count - 1);
      scratch_.seen.clear();
      scratch_.seen.insert(cell);
      for (int gy = gy_min; gy <= gy_max; ++gy) {
        for (int gx = gx_min; gx <= gx_max; ++gx) {
          const Pixel* pixel = grid_->gridPixel(GridX(gx), GridY(gy));
          if (pixel == nullptr || pixel->cell == nullptr
              || scratch_.seen.contains(pixel->cell)) {
            continue;
          }
          scratch_.seen.insert(pixel->cell);
          const Node* nb = pixel->cell;
          odb::dbMaster* nb_master = getDbMaster(nb);
          if (nb_master != nullptr) {
            scratch_.local_cells.push_back({
                .master = nb_master,
                .orient = nb->getOrient(),
                .x = nb->getLeft().v,
                .y = nb->getBottom().v,
            });
          }
        }
      }
    }

    if (scratch_.local_cells.empty()) {
      return true;
    }

    // Build layer shape map for all tracked layers in the local window
    for (auto& [_, vec] : scratch_.layer_shapes) {
      vec.clear();
    }
    for (const PlacedCell& pc : scratch_.local_cells) {
      const MasterLayerShapes& m_shapes = getMasterShapes(pc.master, pc.orient);
      for (const auto& [layer, rects] : m_shapes) {
        auto& dst = scratch_.layer_shapes[layer];
        for (const odb::Rect& r : rects) {
          dst.push_back({
              .rect = odb::Rect(r.xMin() + pc.x,
                                r.yMin() + pc.y,
                                r.xMax() + pc.x,
                                r.yMax() + pc.y),
          });
        }
      }
    }

    auto& layer_shapes = scratch_.layer_shapes;
    auto getRects = [&](odb::dbTechLayer* l) -> std::vector<odb::Rect> {
      std::vector<odb::Rect> out;
      auto it = layer_shapes.find(l);
      if (it != layer_shapes.end() && !it->second.empty()) {
        out.reserve(it->second.size());
        for (const TaggedRect& tr : it->second) {
          out.push_back(tr.rect);
        }
      }
      return out;
    };

    auto isLayerStrict = [&](odb::dbTechLayer* l) -> bool {
      return strict_empty_sites
             || (require_filler_master && !filler_layers_.contains(l));
    };

    for (odb::dbTechLayer* layer : rule_layers_) {
      auto ls_it = layer_shapes.find(layer);
      if (ls_it == layer_shapes.end() || ls_it->second.empty()) {
        continue;
      }
      const bool layer_strict = isLayerStrict(layer);

      bool cand_affects_layer = cand_shapes.contains(layer);
      if (!cand_affects_layer) {
        for (const auto& [cl, _] : cand_shapes) {
          if (auto dit = layer_deps_.find(cl);
              dit != layer_deps_.end()
              && std::ranges::find(dit->second, layer) != dit->second.end()) {
            cand_affects_layer = true;
            break;
          }
        }
      }
      if (!cand_affects_layer) {
        if (layer_strict) {
          continue;
        }
        const int l_halo = std::max(site_w, layer_local_halo_.at(layer));
        const odb::Rect near_box(cell_box.xMin() - l_halo,
                                 cell_box.yMin() - l_halo,
                                 cell_box.xMax() + l_halo,
                                 cell_box.yMax() + l_halo);
        bool near_cand = false;
        for (const TaggedRect& tr : ls_it->second) {
          if (tr.rect.overlaps(near_box)) {
            near_cand = true;
            break;
          }
        }
        if (!near_cand) {
          continue;
        }
      }

      const std::vector<odb::Rect> raw_rects = getRects(layer);
      const std::vector<odb::Rect> rects_1d = merge1D(raw_rects);

      // 1. Base WIDTH and SPACING (for IMPLANT layers)
      std::vector<odb::Rect> req_rects_1d;
      const std::vector<odb::Rect>* width_exp_rects = &rects_1d;
      if (layer->getType() == odb::dbTechLayerType::IMPLANT) {
        const int base_w = static_cast<int>(
            std::max(layer->getWidth(), layer->getMinWidth()));
        if (base_w > 0) {
          if (!checkPlainWidth(
                  rects_1d, base_w, cell, cell_box, touch_box, layer_strict)) {
            return false;
          }
          if (!layer_strict) {
            req_rects_1d = expandMandatoryMinWidth(
                rects_1d, base_w, cell, cell_box, false);
            width_exp_rects = &req_rects_1d;
            if (!checkMandatorySpanContention(layer,
                                              base_w,
                                              rects_1d,
                                              req_rects_1d,
                                              layer_shapes,
                                              cell,
                                              cell_box,
                                              touch_box,
                                              require_filler_master)) {
              return false;
            }
          }
        }

        int same_s = layer->getSpacing();
        for (odb::dbTechLayerSpacingRule* sr : layer->getV54SpacingRules()) {
          odb::dbTechLayer* sec = nullptr;
          const int spc = static_cast<int>(sr->getSpacing());
          if (sr->getCutLayer4Spacing(sec) && sec != nullptr && sec != layer) {
            const int sec_base_w
                = (sec->getType() == odb::dbTechLayerType::IMPLANT)
                      ? static_cast<int>(
                            std::max(sec->getWidth(), sec->getMinWidth()))
                      : 0;
            const std::vector<odb::Rect> sec_rects
                = expandMandatoryMinWidth(merge1D(getRects(sec)),
                                          sec_base_w,
                                          cell,
                                          cell_box,
                                          isLayerStrict(sec));
            if (!checkInterLayerBaseSpacing(*width_exp_rects,
                                            sec_rects,
                                            spc,
                                            std::max({spc, base_w, sec_base_w}),
                                            cell_box,
                                            touch_box,
                                            layer_strict)) {
              return false;
            }
          } else {
            same_s = std::max(same_s, spc);
          }
        }
        if (same_s > 0
            && !checkSameLayerBaseSpacing(*width_exp_rects,
                                          same_s,
                                          std::max(same_s, base_w),
                                          cell,
                                          cell_box,
                                          touch_box,
                                          layer_strict)) {
          return false;
        }
      }
    }

    return true;
  }

 private:
  using MasterKey = std::pair<odb::dbMaster*, int>;
  using LayerPair = std::pair<odb::dbTechLayer*, odb::dbTechLayer*>;
  using MasterLayerShapes
      = std::unordered_map<odb::dbTechLayer*, std::vector<odb::Rect>>;

  struct PlacedCell
  {
    odb::dbMaster* master{nullptr};
    odb::dbOrientType orient;
    int x{0};
    int y{0};
  };

  struct TaggedRect
  {
    odb::Rect rect;
  };

  static odb::dbMaster* getDbMaster(const Node* cell)
  {
    if (cell == nullptr) {
      return nullptr;
    }
    if (cell->getDbInst() != nullptr) {
      return cell->getDbInst()->getMaster();
    }
    if (cell->getMaster() != nullptr) {
      return cell->getMaster()->getDbMaster();
    }
    return nullptr;
  }

  const MasterLayerShapes& getMasterShapes(
      odb::dbMaster* master,
      const odb::dbOrientType& orient) const
  {
    const MasterKey key{master, orient.getValue()};
    auto it = master_cache_.find(key);
    if (it != master_cache_.end()) {
      return it->second;
    }

    MasterLayerShapes shapes;
    odb::Rect bbox;
    master->getPlacementBoundary(bbox);
    odb::dbTransform transform(orient);
    transform.apply(bbox);
    transform.setOffset(odb::Point(-bbox.xMin(), -bbox.yMin()));

    for (odb::dbBox* box : master->getObstructions()) {
      odb::dbTechLayer* layer = box->getTechLayer();
      if (layer == nullptr || !tracked_layers_.contains(layer)) {
        continue;
      }
      odb::Rect r = box->getBox();
      transform.apply(r);
      shapes[layer].push_back(r);
    }

    return master_cache_.emplace(key, std::move(shapes)).first->second;
  }

  static int overlap1D(const int a_lo,
                       const int a_hi,
                       const int b_lo,
                       const int b_hi)
  {
    return std::min(a_hi, b_hi) - std::max(a_lo, b_lo);
  }

  static int gap1D(const int a_lo,
                   const int a_hi,
                   const int b_lo,
                   const int b_hi)
  {
    return std::max(0, std::max(a_lo, b_lo) - std::min(a_hi, b_hi));
  }

  // Merge rectangles that share a vertical span or overlap in Y and touch in X
  // (1D horizontal merge within overlapping Y bands, without merging pure
  // row-boundary yMax == yMin touches across separate 1H rows).
  static std::vector<odb::Rect> merge1D(const std::vector<odb::Rect>& rects)
  {
    if (rects.size() <= 1) {
      return rects;
    }
    const int y0 = rects[0].yMin();
    const int y1 = rects[0].yMax();
    bool uniform_y = true;
    for (size_t i = 1; i < rects.size(); ++i) {
      if (rects[i].yMin() != y0 || rects[i].yMax() != y1) {
        uniform_y = false;
        break;
      }
    }
    if (uniform_y) {
      std::vector<odb::Rect> sorted = rects;
      std::ranges::sort(sorted, [](const odb::Rect& a, const odb::Rect& b) {
        if (a.xMin() != b.xMin()) {
          return a.xMin() < b.xMin();
        }
        return a.xMax() < b.xMax();
      });
      std::vector<odb::Rect> out;
      out.reserve(sorted.size());
      for (const odb::Rect& r : sorted) {
        if (!out.empty() && out.back().xMax() >= r.xMin()) {
          out.back().set_xhi(std::max(out.back().xMax(), r.xMax()));
        } else {
          out.push_back(r);
        }
      }
      return out;
    }

    std::vector<int> ys;
    ys.reserve(2 * rects.size());
    for (const odb::Rect& r : rects) {
      ys.push_back(r.yMin());
      ys.push_back(r.yMax());
    }
    std::ranges::sort(ys);
    const auto [ys_first, ys_last] = std::ranges::unique(ys);
    ys.erase(ys_first, ys_last);

    std::vector<odb::Rect> bands;
    std::vector<std::pair<int, int>> x_ivs;
    for (size_t k = 0; k + 1 < ys.size(); ++k) {
      const int b_lo = ys[k];
      const int b_hi = ys[k + 1];
      if (b_lo >= b_hi) {
        continue;
      }
      x_ivs.clear();
      for (const odb::Rect& r : rects) {
        if (r.yMin() <= b_lo && r.yMax() >= b_hi) {
          x_ivs.emplace_back(r.xMin(), r.xMax());
        }
      }
      if (x_ivs.empty()) {
        continue;
      }
      std::ranges::sort(x_ivs);
      int cur_lo = x_ivs[0].first;
      int cur_hi = x_ivs[0].second;
      for (size_t i = 1; i < x_ivs.size(); ++i) {
        if (cur_hi >= x_ivs[i].first) {
          cur_hi = std::max(cur_hi, x_ivs[i].second);
        } else {
          bands.emplace_back(cur_lo, b_lo, cur_hi, b_hi);
          cur_lo = x_ivs[i].first;
          cur_hi = x_ivs[i].second;
        }
      }
      bands.emplace_back(cur_lo, b_lo, cur_hi, b_hi);
    }

    // Coalesce adjacent elementary Y bands that share identical [xMin, xMax]
    // when an input rectangle actually spans strictly across their shared Y
    // split (preserving full-height rectangles without merging separate rows).
    std::ranges::sort(bands, [](const odb::Rect& a, const odb::Rect& b) {
      if (a.xMin() != b.xMin()) {
        return a.xMin() < b.xMin();
      }
      if (a.xMax() != b.xMax()) {
        return a.xMax() < b.xMax();
      }
      return a.yMin() < b.yMin();
    });
    std::vector<odb::Rect> out;
    out.reserve(bands.size());
    for (const odb::Rect& b : bands) {
      bool can_coalesce = false;
      if (!out.empty() && out.back().xMin() == b.xMin()
          && out.back().xMax() == b.xMax() && out.back().yMax() == b.yMin()) {
        const int y_split = b.yMin();
        for (const odb::Rect& orig : rects) {
          if (orig.yMin() < y_split && orig.yMax() > y_split
              && overlap1D(orig.xMin(), orig.xMax(), b.xMin(), b.xMax()) > 0) {
            can_coalesce = true;
            break;
          }
        }
      }
      if (can_coalesce) {
        out.back().set_yhi(b.yMax());
      } else {
        out.push_back(b);
      }
    }
    return out;
  }

  int rowYLo(const int gy) const { return grid_->gridYToDbu(GridY(gy)).v; }

  int rowYHi(const int gy) const { return grid_->gridYToDbu(GridY(gy + 1)).v; }

  int rowHeightDbu(const int gy) const { return rowYHi(gy) - rowYLo(gy); }

  std::pair<int, int> horizontalExtent(
      const odb::Rect& r,
      const Node* cell,
      const odb::Rect& cell_box,
      const bool strict_empty_sites,
      const std::vector<odb::Rect>* same_layer_rects = nullptr) const
  {
    int lo = r.xMin();
    int hi = r.xMax();
    if (strict_empty_sites || grid_ == nullptr) {
      return {lo, hi};
    }
    const int site_w = grid_->getSiteWidth().v;
    const int site_count = grid_->getRowSiteCount().v;
    const int row_count = grid_->getRowCount().v;
    if (site_w <= 0 || site_count <= 0 || row_count <= 0) {
      return {lo, hi};
    }

    const int gy_start = std::clamp(
        grid_->gridSnapDownY(DbuY(std::max(0, r.yMin()))).v, 0, row_count - 1);
    const int gy_end
        = std::clamp(grid_->gridEndY(DbuY(std::max(0, r.yMax()))).v,
                     gy_start + 1,
                     row_count);

    auto isSiteEmptyAndValid = [&](const int gx) -> bool {
      if (gx < 0 || gx >= site_count) {
        return false;
      }
      const int site_x_lo = gx * site_w;
      const int site_x_hi = site_x_lo + site_w;
      for (int gy = gy_start; gy < gy_end; ++gy) {
        const int row_y_lo = rowYLo(gy);
        const int row_y_hi = rowYHi(gy);
        // If this site overlaps the candidate cell's new footprint, it is
        // occupied by the candidate cell itself (not empty).
        if (overlap1D(site_x_lo, site_x_hi, cell_box.xMin(), cell_box.xMax())
                > 0
            && overlap1D(row_y_lo, row_y_hi, cell_box.yMin(), cell_box.yMax())
                   > 0) {
          return false;
        }
        const Pixel* pixel = grid_->gridPixel(GridX(gx), GridY(gy));
        if (pixel == nullptr || !pixel->is_valid) {
          return false;
        }
        if (pixel->cell != nullptr && pixel->cell != cell) {
          return false;
        }
      }
      return true;
    };

    while (lo > 0 && (lo % site_w == 0)
           && isSiteEmptyAndValid((lo / site_w) - 1)) {
      lo -= site_w;
      if (same_layer_rects != nullptr) {
        for (const odb::Rect& sr : *same_layer_rects) {
          if (sr.xMax() == lo && sr.xMin() < lo && sr.yMin() <= r.yMin()
              && sr.yMax() >= r.yMax()) {
            lo = sr.xMin();
            break;
          }
        }
      }
    }

    while (hi >= 0 && (hi % site_w == 0) && isSiteEmptyAndValid(hi / site_w)) {
      hi += site_w;
      if (same_layer_rects != nullptr) {
        for (const odb::Rect& sr : *same_layer_rects) {
          if (sr.xMin() == hi && sr.xMax() > hi && sr.yMin() <= r.yMin()
              && sr.yMax() >= r.yMax()) {
            hi = sr.xMax();
            break;
          }
        }
      }
    }

    return {lo, hi};
  }

  bool canExtendHorizontally(const odb::Rect& r,
                             const Node* cell,
                             const odb::Rect& cell_box,
                             const bool strict_empty_sites,
                             const int min_w = 0,
                             const std::vector<odb::Rect>* same_layer_rects
                             = nullptr) const
  {
    const auto [lo, hi] = horizontalExtent(
        r, cell, cell_box, strict_empty_sites, same_layer_rects);
    return (lo < r.xMin() || hi > r.xMax()) && (hi - lo) >= min_w;
  }

  // Expand each < min_w island to the mandatory horizontal span [req_lo,
  // req_hi] that every valid >= min_w filler completion inside [ext_lo, ext_hi]
  // must cover.
  std::vector<odb::Rect> expandMandatoryMinWidth(
      const std::vector<odb::Rect>& rects,
      const int min_w,
      const Node* cell,
      const odb::Rect& cell_box,
      const bool strict_empty_sites) const
  {
    if (strict_empty_sites || min_w <= 0 || grid_ == nullptr || rects.empty()) {
      return rects;
    }
    const int site_w = grid_->getSiteWidth().v;
    if (site_w <= 0) {
      return rects;
    }
    const int req_aligned_w = ((min_w + site_w - 1) / site_w) * site_w;
    std::vector<odb::Rect> out;
    out.reserve(rects.size());
    for (const odb::Rect& r : rects) {
      const bool aligned
          = (r.xMin() >= 0 && r.xMax() >= 0 && (r.xMin() % site_w == 0)
             && (r.xMax() % site_w == 0));
      const int req_w = aligned ? req_aligned_w : min_w;
      if (r.dx() > 0 && r.dx() < req_w) {
        const auto [ext_lo, ext_hi]
            = horizontalExtent(r, cell, cell_box, false, &rects);
        if (ext_hi - ext_lo >= req_w) {
          const int req_lo = std::min(r.xMin(), ext_hi - req_w);
          const int req_hi = std::max(r.xMax(), ext_lo + req_w);
          out.emplace_back(req_lo, r.yMin(), req_hi, r.yMax());
          continue;
        }
      }
      out.push_back(r);
    }
    return merge1D(out);
  }

  bool canBridgeHorizontally(const odb::Rect& r1,
                             const odb::Rect& r2,
                             const Node* cell,
                             const odb::Rect& cell_box,
                             const bool strict_empty_sites) const
  {
    if (strict_empty_sites || grid_ == nullptr) {
      return false;
    }
    const int site_w = grid_->getSiteWidth().v;
    const int site_count = grid_->getRowSiteCount().v;
    const int row_count = grid_->getRowCount().v;
    if (site_w <= 0 || site_count <= 0 || row_count <= 0) {
      return false;
    }
    const int x_lo = std::min(r1.xMax(), r2.xMax());
    const int x_hi = std::max(r1.xMin(), r2.xMin());
    if (x_lo > x_hi || x_lo < 0 || (x_lo % site_w != 0)
        || (x_hi % site_w != 0)) {
      return false;
    }
    const int gx_start = x_lo / site_w;
    const int gx_end = x_hi / site_w;
    if (gx_start < 0 || gx_end > site_count) {
      return false;
    }

    auto areSitesEmptyInY = [&](const int gx_lo,
                                const int gx_hi,
                                const int y_lo,
                                const int y_hi) -> bool {
      if (gx_lo >= gx_hi || gx_lo < 0 || gx_hi > site_count) {
        return false;
      }
      const int gy_start = std::clamp(
          grid_->gridSnapDownY(DbuY(std::max(0, y_lo))).v, 0, row_count - 1);
      const int gy_end = std::clamp(
          grid_->gridEndY(DbuY(std::max(0, y_hi))).v, gy_start + 1, row_count);
      for (int gy = gy_start; gy < gy_end; ++gy) {
        const int row_y_lo = rowYLo(gy);
        const int row_y_hi = rowYHi(gy);
        for (int gx = gx_lo; gx < gx_hi; ++gx) {
          const int site_x_lo = gx * site_w;
          const int site_x_hi = site_x_lo + site_w;
          if (overlap1D(site_x_lo, site_x_hi, cell_box.xMin(), cell_box.xMax())
                  > 0
              && overlap1D(row_y_lo, row_y_hi, cell_box.yMin(), cell_box.yMax())
                     > 0) {
            return false;
          }
          const Pixel* pixel = grid_->gridPixel(GridX(gx), GridY(gy));
          if (pixel == nullptr || !pixel->is_valid) {
            return false;
          }
          if (pixel->cell != nullptr && pixel->cell != cell) {
            return false;
          }
        }
      }
      return true;
    };

    const int ovl_y = overlap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax());
    if (ovl_y > 0) {
      return gx_start < gx_end
             && areSitesEmptyInY(gx_start,
                                 gx_end,
                                 std::max(r1.yMin(), r2.yMin()),
                                 std::min(r1.yMax(), r2.yMax()));
    }
    if (gap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax()) == 0) {
      // Diagonally separated or corner-touching across an adjacent row
      // boundary: bridging sites must produce positive X overlap across the row
      // boundary rather than stopping at a 0-distance corner touch.
      const odb::Rect& r_left = (r1.xMax() == x_lo) ? r1 : r2;
      const odb::Rect& r_right = (r1.xMax() == x_lo) ? r2 : r1;
      if (gx_end + 1 <= site_count && r_right.xMax() >= (gx_end + 1) * site_w
          && areSitesEmptyInY(
              gx_start, gx_end + 1, r_left.yMin(), r_left.yMax())) {
        return true;
      }
      if (gx_start >= 1 && r_left.xMin() <= (gx_start - 1) * site_w
          && areSitesEmptyInY(
              gx_start - 1, gx_end, r_right.yMin(), r_right.yMax())) {
        return true;
      }
      for (int split = gx_start; split < gx_end; ++split) {
        if (areSitesEmptyInY(gx_start, split + 1, r_left.yMin(), r_left.yMax())
            && areSitesEmptyInY(
                split, gx_end, r_right.yMin(), r_right.yMax())) {
          return true;
        }
      }
    }
    return false;
  }

  struct AlwaysMatchRect
  {
    bool operator()(const odb::Rect&) const { return true; }
  };

  template <typename RectFilter = AlwaysMatchRect>
  bool checkPlainWidth(const std::vector<odb::Rect>& rects_1d,
                       const int min_w,
                       const Node* cell,
                       const odb::Rect& cell_box,
                       const odb::Rect& touch_box,
                       const bool strict_empty_sites,
                       const RectFilter& rect_filter = AlwaysMatchRect{}) const
  {
    // Base IMPLANT WIDTH (LEF 5.8 Fig. 3-1) and plain LEF58_WIDTH measure
    // horizontal width along the row; vertical height thresholds use
    // LEF58_WIDTH ... LENGTH.
    const odb::Rect active_box = strict_empty_sites
                                     ? cell_box
                                     : odb::Rect(cell_box.xMin() - min_w,
                                                 touch_box.yMin(),
                                                 cell_box.xMax() + min_w,
                                                 touch_box.yMax());
    for (const odb::Rect& r : rects_1d) {
      if (!r.overlaps(active_box) || !rect_filter(r)) {
        continue;
      }
      if (r.dx() < min_w
          && !canExtendHorizontally(
              r, cell, cell_box, strict_empty_sites, min_w, &rects_1d)) {
        return false;
      }
    }
    return true;
  }

  bool pairAnchoredToCandidate(const odb::Rect& r1,
                               const odb::Rect& r2,
                               const int max_dist,
                               const odb::Rect& cell_box,
                               const bool allow_bridge,
                               const odb::Rect* anchor_override = nullptr) const
  {
    const int ovl_x = overlap1D(r1.xMin(), r1.xMax(), r2.xMin(), r2.xMax());
    const int iz_x_lo = (ovl_x > 0) ? std::max(r1.xMin(), r2.xMin())
                                    : std::min(r1.xMax(), r2.xMax());
    const int iz_x_hi = (ovl_x > 0) ? std::min(r1.xMax(), r2.xMax())
                                    : std::max(r1.xMin(), r2.xMin());

    const int ovl_y = overlap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax());
    const int iz_y_lo = (ovl_y > 0) ? std::max(r1.yMin(), r2.yMin())
                                    : std::min(r1.yMax(), r2.yMax());
    const int iz_y_hi = (ovl_y > 0) ? std::min(r1.yMax(), r2.yMax())
                                    : std::max(r1.yMin(), r2.yMin());

    const int site_w = (grid_ != nullptr) ? grid_->getSiteWidth().v : 0;
    const odb::Rect& anchor_box
        = (anchor_override != nullptr) ? *anchor_override : cell_box;
    if (r1.overlaps(anchor_box) || r2.overlaps(anchor_box)) {
      const int reach = std::max(1, max_dist) + site_w;
      if (gap1D(cell_box.xMin(), cell_box.xMax(), iz_x_lo, iz_x_hi) <= reach
          && gap1D(cell_box.yMin(), cell_box.yMax(), iz_y_lo, iz_y_hi)
                 <= reach) {
        return true;
      }
    }
    if (allow_bridge) {
      const int dy_gap = gap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax());
      if (dy_gap == 0) {
        const int pad_x = (ovl_y > 0) ? 0 : site_w;
        const int by_lo
            = (ovl_y > 0) ? iz_y_lo : std::min(r1.yMin(), r2.yMin());
        const int by_hi
            = (ovl_y > 0) ? iz_y_hi : std::max(r1.yMax(), r2.yMax());
        const odb::Rect bridge_box(
            iz_x_lo - pad_x, by_lo, iz_x_hi + pad_x, by_hi);
        if (cell_box.overlaps(bridge_box)) {
          return true;
        }
      } else if (ovl_x > 0 && dy_gap > 0) {
        const odb::Rect bridge_box(iz_x_lo, iz_y_lo, iz_x_hi, iz_y_hi);
        if (cell_box.overlaps(bridge_box)) {
          return true;
        }
      }
    }
    return false;
  }

  bool checkMandatorySpanContention(
      odb::dbTechLayer* layer,
      const int base_w,
      const std::vector<odb::Rect>& orig_rects,
      const std::vector<odb::Rect>& req_rects,
      const std::unordered_map<odb::dbTechLayer*, std::vector<TaggedRect>>&
          layer_shapes,
      const Node* cell,
      const odb::Rect& cell_box,
      const odb::Rect& touch_box,
      const bool require_filler_master) const
  {
    if (grid_ == nullptr || req_rects == orig_rects) {
      return true;
    }
    const int row_count = grid_->getRowCount().v;
    auto rowSpan = [&](const odb::Rect& r) -> std::pair<int, int> {
      if (row_count <= 0) {
        return {0, 0};
      }
      const int gy0
          = std::clamp(grid_->gridSnapDownY(DbuY(std::max(0, r.yMin()))).v,
                       0,
                       row_count - 1);
      const int gy1 = std::clamp(
          grid_->gridEndY(DbuY(std::max(0, r.yMax()))).v, gy0 + 1, row_count);
      return {gy0, gy1};
    };
    auto coveredBy = [](const std::vector<odb::Rect>& rects,
                        const int x_lo,
                        const int x_hi,
                        const int y_lo,
                        const int y_hi) -> bool {
      for (const odb::Rect& r : rects) {
        if (r.xMin() <= x_lo && r.xMax() >= x_hi
            && overlap1D(r.yMin(), r.yMax(), y_lo, y_hi) > 0) {
          return true;
        }
      }
      return false;
    };

    for (odb::dbTechLayer* l2 : rule_layers_) {
      if (l2 == layer || l2->getType() != odb::dbTechLayerType::IMPLANT
          || compatible_implant_pairs_.contains({layer, l2})) {
        continue;
      }
      if (require_filler_master && !filler_layers_.contains(l2)) {
        continue;
      }
      const int l2_base_w
          = static_cast<int>(std::max(l2->getWidth(), l2->getMinWidth()));
      if (l2_base_w <= 0) {
        continue;
      }
      auto it2 = layer_shapes.find(l2);
      if (it2 == layer_shapes.end() || it2->second.empty()) {
        continue;
      }
      std::vector<odb::Rect> l2_raw;
      l2_raw.reserve(it2->second.size());
      for (const TaggedRect& tr : it2->second) {
        l2_raw.push_back(tr.rect);
      }
      const std::vector<odb::Rect> l2_orig = merge1D(l2_raw);
      const std::vector<odb::Rect> l2_req
          = expandMandatoryMinWidth(l2_orig, l2_base_w, cell, cell_box, false);
      if (l2_req == l2_orig) {
        continue;
      }
      const int reach = std::max(base_w, l2_base_w);
      const odb::Rect active_box(cell_box.xMin() - reach,
                                 touch_box.yMin(),
                                 cell_box.xMax() + reach,
                                 touch_box.yMax());
      for (const odb::Rect& r1 : req_rects) {
        const auto [gy1_lo, gy1_hi] = rowSpan(r1);
        for (const odb::Rect& r2 : l2_req) {
          if (!r1.overlaps(active_box) && !r2.overlaps(active_box)) {
            continue;
          }
          const int ovl_x
              = overlap1D(r1.xMin(), r1.xMax(), r2.xMin(), r2.xMax());
          if (ovl_x <= 0) {
            continue;
          }
          const auto [gy2_lo, gy2_hi] = rowSpan(r2);
          const bool same_row
              = !filler_layers_.empty()
                && overlap1D(gy1_lo, gy1_hi, gy2_lo, gy2_hi) > 0;
          if (!r1.overlaps(r2) && !same_row) {
            continue;
          }
          const int x_lo = std::max(r1.xMin(), r2.xMin());
          const int x_hi = std::min(r1.xMax(), r2.xMax());
          if (!coveredBy(orig_rects, x_lo, x_hi, r1.yMin(), r1.yMax())
              || !coveredBy(l2_orig, x_lo, x_hi, r2.yMin(), r2.yMax())) {
            return false;
          }
        }
      }
    }
    return true;
  }

  bool checkSameLayerBaseSpacing(const std::vector<odb::Rect>& rects_1d,
                                 const int min_spc,
                                 const int anchor_reach,
                                 const Node* cell,
                                 const odb::Rect& cell_box,
                                 const odb::Rect& touch_box,
                                 const bool strict_empty_sites) const
  {
    // Base IMPLANT SPACING (LEF 5.8 Fig. 3-1 & p. 467) applies horizontally
    // within overlapping Y-bands; cross-row vertical/diagonal spacing uses
    // LEF58_SPACING VERTICAL PRL and LEF58_WIDTH ZEROPRL.
    const odb::Rect* anchor_override
        = strict_empty_sites ? nullptr : &touch_box;
    for (size_t i = 0; i < rects_1d.size(); ++i) {
      const odb::Rect& r1 = rects_1d[i];
      for (size_t j = i + 1; j < rects_1d.size(); ++j) {
        const odb::Rect& r2 = rects_1d[j];
        if (overlap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax()) <= 0) {
          continue;
        }
        const int dx_gap = gap1D(r1.xMin(), r1.xMax(), r2.xMin(), r2.xMax());
        if (dx_gap <= 0 || dx_gap >= min_spc) {
          continue;
        }
        if (!pairAnchoredToCandidate(r1,
                                     r2,
                                     anchor_reach,
                                     cell_box,
                                     !strict_empty_sites,
                                     anchor_override)) {
          continue;
        }
        if (canBridgeHorizontally(r1, r2, cell, cell_box, strict_empty_sites)) {
          continue;
        }
        return false;
      }
    }
    return true;
  }

  bool checkInterLayerBaseSpacing(const std::vector<odb::Rect>& rects1,
                                  const std::vector<odb::Rect>& rects2,
                                  const int min_spc,
                                  const int anchor_reach,
                                  const odb::Rect& cell_box,
                                  const odb::Rect& touch_box,
                                  const bool strict_empty_sites) const
  {
    const odb::Rect* anchor_override
        = strict_empty_sites ? nullptr : &touch_box;
    for (const odb::Rect& r1 : rects1) {
      for (const odb::Rect& r2 : rects2) {
        if (!pairAnchoredToCandidate(
                r1, r2, anchor_reach, cell_box, false, anchor_override)) {
          continue;
        }
        if (r1.overlaps(r2)) {
          return false;
        }
        if (min_spc > 0
            && overlap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax()) > 0
            && gap1D(r1.xMin(), r1.xMax(), r2.xMin(), r2.xMax()) < min_spc) {
          return false;
        }
      }
    }
    return true;
  }

  struct Scratch
  {
    std::vector<PlacedCell> local_cells;
    std::unordered_set<const Node*> seen;
    std::unordered_map<odb::dbTechLayer*, std::vector<TaggedRect>> layer_shapes;
  };

  Grid* grid_{nullptr};
  odb::dbTech* tech_{nullptr};
  bool has_rules_{false};
  int max_interaction_{0};
  int local_pinch_halo_{0};
  std::vector<odb::dbTechLayer*> rule_layers_;
  std::unordered_set<odb::dbTechLayer*> tracked_layers_;
  std::unordered_set<odb::dbTechLayer*> filler_layers_;
  std::unordered_set<LayerPair, boost::hash<LayerPair>>
      compatible_implant_pairs_;
  std::unordered_map<odb::dbTechLayer*, int> layer_local_halo_;
  std::unordered_map<odb::dbTechLayer*, int> layer_area_halo_;
  std::unordered_map<odb::dbTechLayer*, std::vector<odb::dbTechLayer*>>
      layer_deps_;
  mutable std::
      unordered_map<MasterKey, MasterLayerShapes, boost::hash<MasterKey>>
          master_cache_;
  mutable Scratch scratch_;
};

// Constructor
PlacementDRC::PlacementDRC(utl::Logger* logger,
                           Grid* grid,
                           odb::dbTech* tech,
                           Padding* padding,
                           bool disallow_one_site_gap)
    : logger_(logger),
      grid_(grid),
      padding_(padding),
      disallow_one_site_gap_(disallow_one_site_gap)
{
  makeCellEdgeSpacingTable(tech);
  auto implant_drc = std::make_unique<ImplantDRC>(grid, tech);
  if (implant_drc->hasRules()) {
    implant_drc_ = std::move(implant_drc);
  }
}

PlacementDRC::~PlacementDRC() = default;

void PlacementDRC::initFixedSupplyVias(odb::dbBlock* block)
{
  fixed_supply_vias_.reset();
  auto fixed_supply_vias
      = std::make_unique<FixedSupplyVias>(logger_, grid_, block);
  if (!fixed_supply_vias->empty()) {
    fixed_supply_vias_ = std::move(fixed_supply_vias);
  }
}

void PlacementDRC::clearFixedSupplyVias()
{
  fixed_supply_vias_.reset();
}

bool PlacementDRC::checkEdgeSpacing(const Node* cell) const
{
  const GridX x = grid_->gridX(cell);
  const GridY y = grid_->gridRoundY(cell);
  return checkEdgeSpacing(cell, x, y, cell->getOrient());
}

// Check edge spacing for a cell at a given location and orientation
bool PlacementDRC::checkEdgeSpacing(const Node* cell,
                                    const GridX x,
                                    const GridY y,
                                    const odb::dbOrientType& orient) const
{
  if (!hasCellEdgeSpacingTable()) {
    return true;
  }
  auto master = cell->getMaster();
  if (master == nullptr) {
    // Filler Cell
    return true;
  }
  // Get the real grid coordinates from the grid indices.
  DbuX x_real = gridToDbu(x, grid_->getSiteWidth());
  DbuY y_real = grid_->gridYToDbu(y);
  for (const auto& edge1 : master->getEdges()) {
    int max_spc = getMaxSpacing(edge1.getEdgeType())
                  + 1;  // +1 to account for EXACT rules
    odb::Rect edge1_box = cell_edges::transformEdgeRect(
        edge1.getBBox(), cell, x_real, y_real, orient);
    const auto dir1 = cell_edges::transformEdgeDir(edge1.getEdgeDir(), orient);
    const bool is_vertical_edge = (dir1 == odb::dbMasterEdgeType::LEFT
                                   || dir1 == odb::dbMasterEdgeType::RIGHT);
    odb::Rect query_rect
        = cell_edges::getQueryRect(edge1_box, max_spc, is_vertical_edge);
    GridX xMin = grid_->gridX(DbuX(query_rect.xMin()));
    GridX xMax = grid_->gridEndX(DbuX(query_rect.xMax()));
    GridY yMin = grid_->gridEndY(DbuY(query_rect.yMin())) - 1;
    GridY yMax = grid_->gridEndY(DbuY(query_rect.yMax()));
    std::set<Node*> checked_cells;
    // Loop over the area covered by queryRect to find neighboring edges and
    // check violations.
    for (GridY y1 = yMin; y1 <= yMax; y1++) {
      for (GridX x1 = xMin; x1 <= xMax; x1++) {
        const Pixel* pixel = grid_->gridPixel(x1, y1);
        if (pixel == nullptr || pixel->cell == nullptr || pixel->cell == cell) {
          // Skip if pixel is empty or occupied only by the current cell.
          continue;
        }
        auto cell2 = static_cast<Node*>(pixel->cell);
        if (checked_cells.contains(cell2)) {
          // Skip if cell was already checked
          continue;
        }
        checked_cells.insert(cell2);
        auto master2 = cell2->getMaster();
        if (master2 == nullptr) {
          continue;
        }
        for (const auto& edge2 : master2->getEdges()) {
          auto spc_entry
              = edge_spacing_table_[edge1.getEdgeType()][edge2.getEdgeType()];
          int spc = spc_entry.spc;
          odb::Rect edge2_box
              = cell_edges::transformEdgeRect(edge2.getBBox(),
                                              cell2,
                                              cell2->getLeft(),
                                              cell2->getBottom(),
                                              cell2->getOrient());
          const auto dir2 = cell_edges::transformEdgeDir(edge2.getEdgeDir(),
                                                         cell2->getOrient());
          const bool facing = (dir1 == odb::dbMasterEdgeType::RIGHT
                               && dir2 == odb::dbMasterEdgeType::LEFT
                               && edge1_box.xMax() <= edge2_box.xMin())
                              || (dir1 == odb::dbMasterEdgeType::LEFT
                                  && dir2 == odb::dbMasterEdgeType::RIGHT
                                  && edge2_box.xMax() <= edge1_box.xMin())
                              || (dir1 == odb::dbMasterEdgeType::TOP
                                  && dir2 == odb::dbMasterEdgeType::BOTTOM
                                  && edge1_box.yMax() <= edge2_box.yMin())
                              || (dir1 == odb::dbMasterEdgeType::BOTTOM
                                  && dir2 == odb::dbMasterEdgeType::TOP
                                  && edge2_box.yMax() <= edge1_box.yMin());
          if (!facing) {
            continue;
          }
          if (!query_rect.overlaps(edge2_box)) {
            // Skip if there is no PRL between the edges.
            continue;
          }
          odb::Rect test_rect(edge1_box);
          // Generalized intersection between the two edges.
          test_rect.merge(edge2_box);
          int dist = is_vertical_edge ? test_rect.dx() : test_rect.dy();
          if (dist == 0 && spc_entry.except_abutted) {
            continue;
          }
          if (spc_entry.is_exact) {
            if (dist == spc) {
              // Violation only if the distance between the edges is exactly the
              // specified spacing.
              return false;
            }
          } else if (dist < spc) {
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool PlacementDRC::checkBlockedLayers(const Node* cell) const
{
  return checkBlockedLayers(
      cell, grid_->gridX(cell), grid_->gridRoundY(cell), cell->getOrient());
}

bool PlacementDRC::checkBlockedLayers(const Node* cell,
                                      const GridX x,
                                      const GridY y,
                                      const odb::dbOrientType& orient) const
{
  const GridX x_begin = x;
  const GridY y_begin = y;
  const GridX x_end = x + grid_->gridWidth(cell);
  const GridY y_end = grid_->gridEndY(grid_->gridYToDbu(y) + cell->getHeight());
  // Pin layers whose power via/patch metal lies somewhere in the footprint
  uint8_t pin_layers = 0;
  for (GridY y1 = y_begin; y1 < y_end; y1++) {
    for (GridX x1 = x_begin; x1 < x_end; x1++) {
      const Pixel* pixel = grid_->gridPixel(x1, y1);
      if (pixel == nullptr) {
        continue;
      }
      if (pixel->blocked_layers & cell->getUsedLayers()) {
        return false;
      }
      pin_layers |= pixel->blocked_pin_layers & cell->getPinLayers();
    }
  }
  if (pin_layers == 0) {
    return true;
  }

  // The via/patch metal is much smaller than a pixel, so check the pin
  // shapes on those layers against the actual metal.
  const Master* master = cell->getMaster();
  odb::dbInst* inst = cell->getDbInst();
  const DbuX x_real = gridToDbu(x, grid_->getSiteWidth());
  const DbuY y_real = grid_->gridYToDbu(y);
  for (int level = 1; level <= kMaxPinLevel; level++) {
    if ((pin_layers & (1 << level)) == 0) {
      continue;
    }
    for (const Master::PinShape& shape : master->getPinShapes(level)) {
      const odb::Rect pin_rect = cell_edges::transformEdgeRect(
          shape.rect, cell, x_real, y_real, orient);
      // A pin may touch power metal of the net it is connected to
      const odb::dbNet* pin_net
          = inst ? inst->getITerm(shape.mterm)->getNet() : nullptr;
      if (grid_->overlapsPinBlockage(level, pin_rect, pin_net)) {
        return false;
      }
    }
  }
  return true;
}

bool PlacementDRC::checkFixedSupplyVias(const Node* cell) const
{
  return checkFixedSupplyVias(
      cell, grid_->gridX(cell), grid_->gridRoundY(cell), cell->getOrient());
}

bool PlacementDRC::checkFixedSupplyVias(const Node* cell,
                                        const GridX x,
                                        const GridY y,
                                        const odb::dbOrientType& orient) const
{
  return fixed_supply_vias_ == nullptr
         || fixed_supply_vias_->check(cell, x, y, orient);
}

bool PlacementDRC::hasImplantRules() const
{
  return implant_drc_ != nullptr && implant_drc_->hasRules();
}

bool PlacementDRC::checkImplantRules(const Node* cell,
                                     const bool strict_empty_sites,
                                     const bool require_filler_master) const
{
  return checkImplantRules(cell,
                           grid_->gridX(cell),
                           grid_->gridSnapDownY(cell),
                           cell->getOrient(),
                           strict_empty_sites,
                           require_filler_master);
}

bool PlacementDRC::checkImplantRules(const Node* cell,
                                     const GridX x,
                                     const GridY y,
                                     const odb::dbOrientType& orient,
                                     const bool strict_empty_sites,
                                     const bool require_filler_master) const
{
  return implant_drc_ == nullptr
         || implant_drc_->check(
             cell, x, y, orient, strict_empty_sites, require_filler_master);
}

bool PlacementDRC::checkDRC(const Node* cell) const
{
  return checkDRC(
      cell, grid_->gridX(cell), grid_->gridRoundY(cell), cell->getOrient());
}

bool PlacementDRC::checkDRC(const Node* cell,
                            const GridX x,
                            const GridY y,
                            const odb::dbOrientType& orient) const
{
  if (!logger_->debugCheck(DPL, "checkDRC", 1)) {
    // Fast path: bail on the first failing check, cheapest first.
    return checkBlockedLayers(cell, x, y, orient) && checkOneSiteGap(cell, x, y)
           && checkPadding(cell, x, y) && checkEdgeSpacing(cell, x, y, orient)
           && checkFixedSupplyVias(cell, x, y, orient)
           && checkImplantRules(cell, x, y, orient);
  }

  // Debug path: evaluate every check so the report shows each one.
  const bool edge_ok = checkEdgeSpacing(cell, x, y, orient);
  const bool padding_ok = checkPadding(cell, x, y);
  const bool blocked_ok = checkBlockedLayers(cell, x, y, orient);
  const bool supply_vias_ok = checkFixedSupplyVias(cell, x, y, orient);
  const bool gap_ok = checkOneSiteGap(cell, x, y);
  const bool implant_ok = checkImplantRules(cell, x, y, orient);

  const bool all_ok = edge_ok && padding_ok && blocked_ok && supply_vias_ok
                      && gap_ok && implant_ok;

  if (!all_ok) {
    const std::string cell_name = cell->name();
    debugPrint(logger_,
               DPL,
               "checkDRC",
               1,
               "cell {} at ({}, {}): ok?={} edge={} padding={} blocked={} "
               "supply_vias={} gap={} implant={}",
               cell_name,
               x.v,
               y.v,
               all_ok ? 1 : 0,
               edge_ok ? 1 : 0,
               padding_ok ? 1 : 0,
               blocked_ok ? 1 : 0,
               supply_vias_ok ? 1 : 0,
               gap_ok ? 1 : 0,
               implant_ok ? 1 : 0);
  }

  return all_ok;
}

int PlacementDRC::countDRCViolations(const Node* cell) const
{
  return countDRCViolations(
      cell, grid_->gridX(cell), grid_->gridRoundY(cell), cell->getOrient());
}

int PlacementDRC::countDRCViolations(const Node* cell,
                                     const GridX x,
                                     const GridY y,
                                     const odb::dbOrientType& orient,
                                     const bool include_fixed_supply_vias) const
{
  int count = 0;
  if (!checkEdgeSpacing(cell, x, y, orient)) {
    ++count;
  }
  if (!checkPadding(cell, x, y)) {
    ++count;
  }
  if (!checkBlockedLayers(cell, x, y, orient)) {
    ++count;
  }
  if (!checkOneSiteGap(cell, x, y)) {
    ++count;
  }
  if (include_fixed_supply_vias && !checkFixedSupplyVias(cell, x, y, orient)) {
    ++count;
  }
  if (!checkImplantRules(cell, x, y, orient)) {
    ++count;
  }
  return count;
}

namespace {
bool isCrWtBlClass(const Node* cell)
{
  using odb::dbMasterType;

  dbMasterType type = cell->getDbInst()->getMaster()->getType();
  // Use switch so if new types are added we get a compiler warning.
  switch (type.getValue()) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
      return true;
    case dbMasterType::CORE_SPACER:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
      // These classes are completely ignored by the placer.
    case dbMasterType::COVER:
    case dbMasterType::COVER_BUMP:
    case dbMasterType::RING:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
      return false;
  }
  return false;
}

bool isWellTap(const Node* cell)
{
  odb::dbMasterType type = cell->getDbInst()->getMaster()->getType();
  return type == odb::dbMasterType::CORE_WELLTAP;
}

bool allowOverlap(const Node* cell1, const Node* cell2)
{
  return cell1->isBlock() && cell2->isBlock();
}

bool allowPaddingOverlap(const Node* cell1, const Node* cell2)
{
  return !isCrWtBlClass(cell1) || !isCrWtBlClass(cell2)
         || (isWellTap(cell1) && isWellTap(cell2));
}

}  // namespace

bool PlacementDRC::hasPaddingConflict(const Node* cell,
                                      const Node* padding_cell) const
{
  return cell != nullptr && padding_cell != nullptr && cell != padding_cell
         && !allowPaddingOverlap(cell, padding_cell)
         && !allowOverlap(cell, padding_cell);
}

bool PlacementDRC::checkPadding(const Node* cell) const
{
  // Use the same approach as grid pixel painting, using gridCovering.
  return checkPadding(cell, grid_->gridCovering(cell));
}

// CLASSes are grouped as follows
// CR = {CORE, CORE FEEDTHRU, CORE TIEHIGH, CORE TIELOW, CORE ANTENNACELL}
// WT = CORE WELLTAP
// SP = CORE SPACER, ENDCAP *
// BL = BLOCK *

//    CR WT BL SP
// CR  P  P  P  O
// WT  P  O  P  O
// BL  P  P  -  O
// SP  O  O  O  O
//
// P = no padded overlap
// O = no overlap (padding ignored)
// - = no overlap check (overlap allowed)
// The rules apply to both FIXED or PLACED instances

bool PlacementDRC::checkPadding(const Node* cell,
                                const GridX x,
                                const GridY y) const
{
  return checkPadding(
      cell,
      GridRect{
          .xlo = x,
          .ylo = y,
          .xhi = x + grid_->gridWidth(cell),
          .yhi = grid_->gridEndY(grid_->gridYToDbu(y) + cell->getHeight()),
      });
}

bool PlacementDRC::checkPadding(const Node* cell, const GridRect& rect) const
{
  // Get the cell's padding requirements
  const GridX left_pad = padding_->padLeft(cell);
  const GridX right_pad = padding_->padRight(cell);
  for (GridX grid_x{rect.xlo - left_pad}; grid_x < rect.xhi + right_pad;
       grid_x++) {
    for (GridY grid_y{rect.ylo}; grid_y < rect.yhi; grid_y++) {
      const Pixel* pixel = grid_->gridPixel(grid_x, grid_y);
      if (pixel == nullptr) {  // at the core edge
        continue;
      }
      if (hasPaddingConflict(cell, pixel->cell)) {
        return false;
      }
      if (hasPaddingConflict(cell, pixel->padding_reserved_by)) {
        return false;
      }
    }
  }
  return true;  // No padding conflicts found
}

bool PlacementDRC::checkOneSiteGap(const Node* cell) const
{
  return checkOneSiteGap(cell, grid_->gridX(cell), grid_->gridRoundY(cell));
}

bool PlacementDRC::checkOneSiteGap(const Node* cell,
                                   const GridX x,
                                   const GridY y) const
{
  if (!disallow_one_site_gap_) {
    return true;
  }
  const GridX x_begin = x - 1;
  const GridY y_begin = y;
  // inclusive search, so we don't add 1 to the end
  const GridX x_finish = x + grid_->gridWidth(cell);
  const GridY y_finish
      = grid_->gridEndY(grid_->gridYToDbu(y) + cell->getHeight());

  auto isAbutted = [this](const GridX x, const GridY y) {
    const Pixel* pixel = grid_->gridPixel(x, y);
    return (pixel == nullptr || pixel->cell);
  };

  auto cellAtSite = [this](const GridX x, const GridY y) {
    const Pixel* pixel = grid_->gridPixel(x, y);
    return (pixel == nullptr || pixel->cell);
  };
  for (GridY y = y_begin; y < y_finish; ++y) {
    // left side
    if (!isAbutted(x_begin, y) && cellAtSite(x_begin - 1, y)) {
      return false;
    }
    // right side
    if (!isAbutted(x_finish, y) && cellAtSite(x_finish + 1, y)) {
      return false;
    }
  }
  return true;
}

// Initialize the edge spacing table from the technology
void PlacementDRC::makeCellEdgeSpacingTable(odb::dbTech* tech)
{
  auto spacing_rules = tech->getCellEdgeSpacingTable();
  if (spacing_rules.empty()) {
    return;
  }
  for (auto rule : spacing_rules) {
    edge_types_indices_.try_emplace(rule->getFirstEdgeType(),
                                    edge_types_indices_.size());
    edge_types_indices_.try_emplace(rule->getSecondEdgeType(),
                                    edge_types_indices_.size());
  }
  // Resize
  const size_t size = edge_types_indices_.size();
  edge_spacing_table_.resize(size);
  for (size_t i = 0; i < size; i++) {
    edge_spacing_table_[i].resize(size, EdgeSpacingEntry(0, false, false));
  }
  // Fill Table
  for (auto rule : spacing_rules) {
    std::string first_edge = rule->getFirstEdgeType();
    std::string second_edge = rule->getSecondEdgeType();
    const int spc = rule->getSpacing();
    const bool exact = rule->isExact();
    const bool except_abutted = rule->isExceptAbutted();
    const EdgeSpacingEntry entry(spc, exact, except_abutted);
    const int idx1 = edge_types_indices_[first_edge];
    const int idx2 = edge_types_indices_[second_edge];
    edge_spacing_table_[idx1][idx2] = entry;
    edge_spacing_table_[idx2][idx1] = entry;
  }
}

// Check if the edge spacing table is populated
bool PlacementDRC::hasCellEdgeSpacingTable() const
{
  return !edge_spacing_table_.empty();
}

// Get the maximum spacing for a given edge type index
int PlacementDRC::getMaxSpacing(const int edge_type_idx) const
{
  return std::max_element(edge_spacing_table_[edge_type_idx].begin(),
                          edge_spacing_table_[edge_type_idx].end())
      ->spc;
}

// Get the index of an edge type from its name
int PlacementDRC::getEdgeTypeIdx(const std::string& edge_type) const
{
  auto it = edge_types_indices_.find(edge_type);
  if (it != edge_types_indices_.end()) {
    return it->second;
  }
  return -1;  // Edge type not found
}

// Convert grid coordinates to DBU coordinates
DbuX PlacementDRC::gridToDbu(const GridX grid_x, const DbuX site_width) const
{
  return DbuX(grid_x.v * site_width.v);
}

}  // namespace dpl
