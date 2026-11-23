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
        const int max_w_reach
            = r->isZeroPrl() ? 2 * r->getMaxWidth() : r->getMaxWidth();
        local_h = std::max(
            {local_h, base_w + r->getMinWidth() + max_w_reach, r->getLength()});
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
        local_h = std::max({local_h, spc + prl_ext, 2 * r->getLength() + spc});
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
      max_halo_ = std::max({max_halo_, local_h, area_h});
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
          .cell_height = cell->getHeight().v,
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
                .cell_height = nb->getHeight().v,
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
              .cell_height = pc.cell_height,
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
      std::optional<std::vector<Component2D>> comps_2d_opt;
      auto getComps2D = [&]() -> const std::vector<Component2D>& {
        if (!comps_2d_opt.has_value()) {
          comps_2d_opt = merge2D(rects_1d);
        }
        return *comps_2d_opt;
      };

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
      // 2. LEF58_WIDTH
      for (odb::dbTechLayerImplantWidthRule* r :
           layer->getTechLayerImplantWidthRules()) {
        if (!checkImplantWidthRule(layer,
                                   r,
                                   layer_shapes,
                                   rects_1d,
                                   *width_exp_rects,
                                   getComps2D,
                                   cell,
                                   cell_box,
                                   touch_box,
                                   layer_strict,
                                   require_filler_master)) {
          return false;
        }
      }
      // 3. LEF58_SPACING
      for (odb::dbTechLayerImplantSpacingRule* r :
           layer->getTechLayerImplantSpacingRules()) {
        if (!checkImplantSpacingRule(layer,
                                     r,
                                     layer_shapes,
                                     rects_1d,
                                     getComps2D,
                                     cell,
                                     cell_box,
                                     layer_strict)) {
          return false;
        }
      }

      // 4. LEF58_MINSTEP
      for (odb::dbTechLayerMinStepRule* r : layer->getTechLayerMinStepRules()) {
        if (!checkMinStepRule(
                r, getComps2D, cell, cell_box, touch_box, layer_strict)) {
          return false;
        }
      }

      // 5. LEF58_COREEDGELENGTH
      for (odb::dbTechLayerCoreEdgeLengthRule* r :
           layer->getTechLayerCoreEdgeLengthRules()) {
        if (!checkCoreEdgeLengthRule(layer,
                                     r,
                                     layer_shapes,
                                     rects_1d,
                                     getComps2D,
                                     cell,
                                     cell_box,
                                     touch_box,
                                     layer_strict)) {
          return false;
        }
      }

      // 6. LEF58_CORNERSPACING
      for (odb::dbTechLayerCornerSpacingRule* r :
           layer->getTechLayerCornerSpacingRules()) {
        if (!checkCornerSpacingRule(
                r, getComps2D, cell, cell_box, touch_box, layer_strict)) {
          return false;
        }
      }

      // 7. LEF58_MAXSPACING
      for (odb::dbTechLayerMaxSpacingRule* r :
           layer->getTechLayerMaxSpacingRules()) {
        if (!checkMaxSpacingRule(
                r, rects_1d, cell, cell_box, touch_box, layer_strict)) {
          return false;
        }
      }

      // 8. LEF58_AREA
      for (odb::dbTechLayerAreaRule* r : layer->getTechLayerAreaRules()) {
        if (!checkAreaRule(layer,
                           r,
                           getComps2D,
                           cell,
                           cell_box,
                           touch_box,
                           query_box,
                           layer_strict)) {
          return false;
        }
      }
    }

    return true;
  }

 private:
  using BoostPolygon = boost::polygon::polygon_90_with_holes_data<int>;
  using BoostPolygonSet = boost::polygon::polygon_90_set_data<int>;
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
    int cell_height{0};
  };

  struct TaggedRect
  {
    odb::Rect rect;
    int cell_height{0};
  };

  struct Component2D
  {
    std::vector<odb::Rect> rects;
    std::vector<BoostPolygon> polys;
    odb::Rect bbox;
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

  static int64_t rectDistSq(const odb::Rect& a, const odb::Rect& b)
  {
    const int64_t dx = gap1D(a.xMin(), a.xMax(), b.xMin(), b.xMax());
    const int64_t dy = gap1D(a.yMin(), a.yMax(), b.yMin(), b.yMax());
    return dx * dx + dy * dy;
  }

  static bool isEdgeConnected(const odb::Rect& a, const odb::Rect& b)
  {
    const int ovl_x = overlap1D(a.xMin(), a.xMax(), b.xMin(), b.xMax());
    const int ovl_y = overlap1D(a.yMin(), a.yMax(), b.yMin(), b.yMax());
    return (ovl_x > 0 && ovl_y >= 0) || (ovl_y > 0 && ovl_x >= 0);
  }

  static bool isCornerTouchOnly(const odb::Rect& a, const odb::Rect& b)
  {
    const int ovl_x = overlap1D(a.xMin(), a.xMax(), b.xMin(), b.xMax());
    const int ovl_y = overlap1D(a.yMin(), a.yMax(), b.yMin(), b.yMax());
    return ovl_x == 0 && ovl_y == 0;
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

  // Group rectangles into 2D edge-connected components (so diagonal corner
  // touches remain separate components) and extract Boost 90-degree polygons
  // WITH holes preserved (fracture_holes = false via pset.get()).
  static std::vector<Component2D> merge2D(const std::vector<odb::Rect>& rects)
  {
    std::vector<Component2D> comps;
    const size_t n = rects.size();
    std::vector<bool> visited(n, false);
    for (size_t i = 0; i < n; ++i) {
      if (visited[i]) {
        continue;
      }
      visited[i] = true;
      Component2D comp;
      comp.bbox = rects[i];
      std::vector<size_t> queue = {i};
      for (size_t q = 0; q < queue.size(); ++q) {
        const odb::Rect& cur = rects[queue[q]];
        comp.rects.push_back(cur);
        comp.bbox.merge(cur);
        for (size_t j = 0; j < n; ++j) {
          if (!visited[j] && isEdgeConnected(cur, rects[j])) {
            visited[j] = true;
            queue.push_back(j);
          }
        }
      }
      comp.rects = merge1D(comp.rects);
      BoostPolygonSet pset;
      using boost::polygon::operators::operator+=;
      for (const odb::Rect& r : comp.rects) {
        pset += r;
      }
      pset.get(comp.polys);
      comps.push_back(std::move(comp));
    }
    return comps;
  }

  int rowYLo(const int gy) const { return grid_->gridYToDbu(GridY(gy)).v; }

  int rowYHi(const int gy) const { return grid_->gridYToDbu(GridY(gy + 1)).v; }

  int rowHeightDbu(const int gy) const { return rowYHi(gy) - rowYLo(gy); }

  // True when `r` overlaps at least one placement site and every site it
  // overlaps is a valid, unoccupied site (or one still registered to the
  // candidate itself) that is not under the candidate's new position. Such
  // a region can be closed by filler insertion.
  bool sitesFillable(const odb::Rect& r,
                     const Node* cell,
                     const odb::Rect& cell_box) const
  {
    if (grid_ == nullptr) {
      return false;
    }
    const int site_w = grid_->getSiteWidth().v;
    const int site_count = grid_->getRowSiteCount().v;
    const int row_count = grid_->getRowCount().v;
    if (site_w <= 0 || site_count <= 0 || row_count <= 0) {
      return false;
    }
    const int gx0 = std::clamp(
        grid_->gridX(DbuX(std::max(0, r.xMin()))).v, 0, site_count - 1);
    const int gx1 = std::clamp(
        grid_->gridEndX(DbuX(std::max(0, r.xMax()))).v - 1, 0, site_count - 1);
    const int gy0 = std::clamp(
        grid_->gridSnapDownY(DbuY(std::max(0, r.yMin()))).v, 0, row_count - 1);
    const int gy1 = std::clamp(
        grid_->gridEndY(DbuY(std::max(0, r.yMax()))).v - 1, 0, row_count - 1);
    bool any = false;
    for (int gy = gy0; gy <= gy1; ++gy) {
      const int ry_lo = rowYLo(gy);
      const int ry_hi = rowYHi(gy);
      if (overlap1D(ry_lo, ry_hi, r.yMin(), r.yMax()) <= 0) {
        continue;
      }
      for (int gx = gx0; gx <= gx1; ++gx) {
        const int sx_lo = gx * site_w;
        const int sx_hi = sx_lo + site_w;
        if (overlap1D(sx_lo, sx_hi, r.xMin(), r.xMax()) <= 0) {
          continue;
        }
        any = true;
        // Under the candidate itself: no filler can ever land there.
        if (overlap1D(sx_lo, sx_hi, cell_box.xMin(), cell_box.xMax()) > 0
            && overlap1D(ry_lo, ry_hi, cell_box.yMin(), cell_box.yMax()) > 0) {
          return false;
        }
        const Pixel* pixel = grid_->gridPixel(GridX(gx), GridY(gy));
        if (pixel == nullptr || !pixel->is_valid
            || (pixel->cell != nullptr && pixel->cell != cell)) {
          return false;
        }
      }
    }
    return any;
  }

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

  bool canExtendVertically(const odb::Rect& vr,
                           const int min_w,
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

    const int gx_start = std::clamp(
        grid_->gridX(DbuX(std::max(0, vr.xMin()))).v, 0, site_count - 1);
    const int gx_end
        = std::clamp(grid_->gridEndX(DbuX(std::max(0, vr.xMax()))).v,
                     gx_start + 1,
                     site_count);

    auto isRowEmptyAcrossX = [&](const int gy) -> bool {
      if (gy < 0 || gy >= row_count) {
        return false;
      }
      const int row_y_lo = rowYLo(gy);
      const int row_y_hi = rowYHi(gy);
      for (int gx = gx_start; gx < gx_end; ++gx) {
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
      return true;
    };

    int total_h = vr.dy();
    int ext_h = 0;

    const int gy_lo = std::clamp(
        grid_->gridSnapDownY(DbuY(std::max(0, vr.yMin()))).v, 0, row_count - 1);
    if (vr.yMin() <= rowYLo(gy_lo)) {
      for (int gy = gy_lo - 1; gy >= 0 && isRowEmptyAcrossX(gy); --gy) {
        const int rh = rowHeightDbu(gy);
        ext_h += rh;
        total_h += rh;
        if (total_h >= min_w) {
          return true;
        }
      }
    }

    const int gy_hi = std::clamp(
        grid_->gridEndY(DbuY(std::max(0, vr.yMax()))).v, 1, row_count);
    const int top_row_y_hi = rowYHi(gy_hi - 1);
    if (vr.yMax() >= top_row_y_hi) {
      for (int gy = gy_hi; gy < row_count && isRowEmptyAcrossX(gy); ++gy) {
        const int rh = rowHeightDbu(gy);
        ext_h += rh;
        total_h += rh;
        if (total_h >= min_w) {
          return true;
        }
      }
    }

    return ext_h > 0 && total_h >= min_w;
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

  bool canBridgeVertically(const odb::Rect& v1,
                           const odb::Rect& v2,
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
    const int x_lo = std::max(v1.xMin(), v2.xMin());
    const int x_hi = std::min(v1.xMax(), v2.xMax());
    const int y_lo = std::min(v1.yMax(), v2.yMax());
    const int y_hi = std::max(v1.yMin(), v2.yMin());
    if (x_lo >= x_hi || y_lo >= y_hi) {
      return false;
    }
    const int gy_start = std::clamp(
        grid_->gridSnapDownY(DbuY(std::max(0, y_lo))).v, 0, row_count - 1);
    const int gy_end = std::clamp(
        grid_->gridEndY(DbuY(std::max(0, y_hi))).v, gy_start + 1, row_count);
    const int rows_y_lo = rowYLo(gy_start);
    const int rows_y_hi = rowYHi(gy_end - 1);
    // Vertical gap must align with full placement row(s) to be fillable.
    if (rows_y_lo != y_lo || rows_y_hi != y_hi) {
      return false;
    }
    const int gx_start = std::clamp(
        grid_->gridX(DbuX(std::max(0, x_lo))).v, 0, site_count - 1);
    const int gx_end = std::clamp(
        grid_->gridEndX(DbuX(std::max(0, x_hi))).v, gx_start + 1, site_count);
    for (int gy = gy_start; gy < gy_end; ++gy) {
      const int row_y_lo = rowYLo(gy);
      const int row_y_hi = rowYHi(gy);
      for (int gx = gx_start; gx < gx_end; ++gx) {
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

  template <typename GetComps2D>
  bool checkImplantWidthRule(
      odb::dbTechLayer* layer,
      odb::dbTechLayerImplantWidthRule* rule,
      const std::unordered_map<odb::dbTechLayer*, std::vector<TaggedRect>>&
          layer_shapes,
      const std::vector<odb::Rect>& rects_1d,
      const std::vector<odb::Rect>& width_exp_rects,
      const GetComps2D& get_comps_2d,
      const Node* cell,
      const odb::Rect& cell_box,
      const odb::Rect& touch_box,
      const bool strict_empty_sites,
      const bool require_filler_master) const
  {
    // Skip implant-group-scoped rules until group membership is modeled on
    // masters.
    if (!rule->getCheckImplantGroup().empty()) {
      return true;
    }

    const int min_w = rule->getMinWidth();
    const int max_w = rule->getMaxWidth();
    const int min_len = rule->getLength();
    const int req_cell_h = rule->getCellHeight();
    const bool zero_prl = rule->isZeroPrl();
    const bool except_corner = rule->isExceptCornerTouch();
    odb::dbTechLayer* sec_layer = rule->getSecondLayer();
    const bool layer_any = rule->isLayerAny();
    const auto within_layers = rule->getWithinLayers();
    const int base_w
        = static_cast<int>(std::max(layer->getWidth(), layer->getMinWidth()));
    const int reach_x = std::max({base_w, min_w, max_w});
    const odb::Rect active_box = strict_empty_sites
                                     ? cell_box
                                     : odb::Rect(cell_box.xMin() - reach_x,
                                                 touch_box.yMin(),
                                                 cell_box.xMax() + reach_x,
                                                 touch_box.yMax());

    auto ls_it = layer_shapes.find(layer);
    if (ls_it == layer_shapes.end() || ls_it->second.empty()) {
      return true;
    }

    // Apply WITHINLAYER clipping before stitching, but evaluate CELLHEIGHT
    // AFTER stitching so mixed-height continuous implant islands stay intact.
    std::vector<odb::Rect> filtered_rects_1d;
    std::optional<std::vector<Component2D>> filtered_comps_2d;
    const bool has_within = !within_layers.empty();
    if (has_within) {
      BoostPolygonSet within_set;
      using boost::polygon::operators::operator+=;
      using boost::polygon::operators::operator&=;
      for (odb::dbTechLayer* wl : within_layers) {
        auto wit = layer_shapes.find(wl);
        if (wit != layer_shapes.end()) {
          for (const TaggedRect& tr : wit->second) {
            within_set += tr.rect;
          }
        }
      }
      BoostPolygonSet cand_set;
      for (const odb::Rect& r : rects_1d) {
        cand_set += r;
      }
      cand_set &= within_set;
      std::vector<odb::Rect> clipped;
      cand_set.get_rectangles(clipped, boost::polygon::HORIZONTAL);
      if (clipped.empty()) {
        return true;
      }
      filtered_rects_1d = merge1D(clipped);
    }

    const std::vector<odb::Rect>& eff_rects_1d
        = has_within ? filtered_rects_1d : rects_1d;
    const std::vector<odb::Rect>& eff_req_rects_1d
        = has_within ? filtered_rects_1d : width_exp_rects;
    auto getEffComps2D = [&]() -> const std::vector<Component2D>& {
      if (!has_within) {
        return get_comps_2d();
      }
      if (!filtered_comps_2d.has_value()) {
        filtered_comps_2d = merge2D(filtered_rects_1d);
      }
      return *filtered_comps_2d;
    };

    auto hasMatchingCellHeight = [&](const odb::Rect& target) -> bool {
      if (req_cell_h <= 0) {
        return true;
      }
      for (const TaggedRect& tr : ls_it->second) {
        if (tr.cell_height == req_cell_h && tr.rect.overlaps(target)) {
          return true;
        }
      }
      return false;
    };

    auto expandSecLayer
        = [&](odb::dbTechLayer* l2, std::vector<odb::Rect> l2_merged) {
            if (has_within || strict_empty_sites || l2 == nullptr
                || l2->getType() != odb::dbTechLayerType::IMPLANT
                || (require_filler_master && !filler_layers_.contains(l2))) {
              return l2_merged;
            }
            const int l2_base_w
                = static_cast<int>(std::max(l2->getWidth(), l2->getMinWidth()));
            return expandMandatoryMinWidth(
                l2_merged, l2_base_w, cell, cell_box, false);
          };

    // Helper to collect 1D merged shapes on the target secondary layer(s),
    // merging each secondary layer independently so LAYER ANY never merges
    // distinct secondary layers (e.g. IMP2 + IMP3) across MAXWIDTH.
    auto getSecRects = [&]() -> std::vector<odb::Rect> {
      std::vector<odb::Rect> sec_merged;
      if (sec_layer != nullptr) {
        auto it = layer_shapes.find(sec_layer);
        if (it != layer_shapes.end()) {
          std::vector<odb::Rect> sec_raw;
          sec_raw.reserve(it->second.size());
          for (const TaggedRect& tr : it->second) {
            sec_raw.push_back(tr.rect);
          }
          sec_merged = expandSecLayer(sec_layer, merge1D(sec_raw));
        }
      } else if (layer_any && tech_ != nullptr) {
        for (odb::dbTechLayer* l2 : tech_->getLayers()) {
          if (l2 != layer && l2->getType() == odb::dbTechLayerType::IMPLANT) {
            auto it = layer_shapes.find(l2);
            if (it != layer_shapes.end() && !it->second.empty()) {
              std::vector<odb::Rect> l2_raw;
              l2_raw.reserve(it->second.size());
              for (const TaggedRect& tr : it->second) {
                l2_raw.push_back(tr.rect);
              }
              std::vector<odb::Rect> l2_merged
                  = expandSecLayer(l2, merge1D(l2_raw));
              sec_merged.insert(
                  sec_merged.end(), l2_merged.begin(), l2_merged.end());
            }
          }
        }
      }
      return sec_merged;
    };

    const bool has_sec = (sec_layer != nullptr || layer_any);

    // Case 1: ZEROPRL [LAYER {secondLayer | ANY}] [MAXWIDTH maxWidth]
    // [LENGTH length] [EXCEPTCORNERTOUCH]
    // Per LEF 5.8 p. 471: applies across adjacent rows with zero vertical PRL.
    // LENGTH selects shapes with vertical height < min_len; WIDTH specifies
    // minimum horizontal width min_w. When LAYER {sec | ANY} is set, shapes on
    // `layer` are merged one by one with each secondary implant layer.
    if (zero_prl) {
      auto extendAcrossPlaced = [](odb::Rect cand,
                                   const std::vector<odb::Rect>& placed) {
        bool changed = true;
        while (changed) {
          changed = false;
          for (const odb::Rect& pr : placed) {
            if (pr.yMin() <= cand.yMin() && pr.yMax() >= cand.yMax()
                && overlap1D(cand.xMin(), cand.xMax(), pr.xMin(), pr.xMax())
                       >= 0) {
              if (pr.xMin() < cand.xMin()) {
                cand.set_xlo(pr.xMin());
                changed = true;
              }
              if (pr.xMax() > cand.xMax()) {
                cand.set_xhi(pr.xMax());
                changed = true;
              }
            }
          }
        }
        return cand;
      };

      // Enumerate all minimal >= base_w filler completions [lo, lo + req_w]
      // inside [ext_lo, ext_hi] for < base_w islands, since wide-first
      // non-implant fillers can seal either open side and force any minimal
      // 1D-legal completion (e.g. [r.xMin(), r.xMin() + req_w]).
      auto collectZeroPrlSpans
          = [&](odb::dbTechLayer* l, const std::vector<odb::Rect>& merged_rects)
          -> std::vector<odb::Rect> {
        if (has_within || strict_empty_sites || grid_ == nullptr || l == nullptr
            || l->getType() != odb::dbTechLayerType::IMPLANT
            || (require_filler_master && !filler_layers_.contains(l))) {
          return merged_rects;
        }
        const int l_base_w
            = static_cast<int>(std::max(l->getWidth(), l->getMinWidth()));
        const int site_w = grid_->getSiteWidth().v;
        if (l_base_w <= 0 || site_w <= 0) {
          return merged_rects;
        }
        const int req_aligned_w = ((l_base_w + site_w - 1) / site_w) * site_w;
        std::vector<odb::Rect> spans;
        spans.reserve(merged_rects.size() * 2);
        for (const odb::Rect& r : merged_rects) {
          const bool aligned
              = (r.xMin() >= 0 && r.xMax() >= 0 && (r.xMin() % site_w == 0)
                 && (r.xMax() % site_w == 0));
          const int req_w = aligned ? req_aligned_w : l_base_w;
          if (r.dx() > 0 && r.dx() < req_w) {
            const auto [ext_lo, ext_hi]
                = horizontalExtent(r, cell, cell_box, false, &merged_rects);
            const int lo_min = std::max(ext_lo, r.xMax() - req_w);
            const int lo_max = std::min(r.xMin(), ext_hi - req_w);
            if (lo_min <= lo_max) {
              if (aligned) {
                for (int lo = lo_min; lo <= lo_max; lo += site_w) {
                  spans.push_back(extendAcrossPlaced(
                      odb::Rect(lo, r.yMin(), lo + req_w, r.yMax()),
                      merged_rects));
                }
              } else {
                spans.push_back(extendAcrossPlaced(
                    odb::Rect(lo_min, r.yMin(), lo_min + req_w, r.yMax()),
                    merged_rects));
                if (lo_max > lo_min) {
                  spans.push_back(extendAcrossPlaced(
                      odb::Rect(lo_max, r.yMin(), lo_max + req_w, r.yMax()),
                      merged_rects));
                }
              }
              continue;
            }
          }
          spans.push_back(r);
        }
        return spans;
      };

      // z_rects holds the primary-layer spans first (primary_count of them),
      // then any secondary-layer spans. Only pairs with at least one primary
      // span are checked: the rule belongs to `layer`, so two secondary-layer
      // shapes meeting each other are not its business.
      auto evalZeroPrlRects = [&](const std::vector<odb::Rect>& z_rects,
                                  const size_t primary_count) -> bool {
        for (size_t i = 0; i < std::min(primary_count, z_rects.size()); ++i) {
          const odb::Rect& r1 = z_rects[i];
          for (size_t j = i + 1; j < z_rects.size(); ++j) {
            const odb::Rect& r2 = z_rects[j];
            if (!r1.overlaps(active_box) && !r2.overlaps(active_box)) {
              continue;
            }
            if (!hasMatchingCellHeight(r1) && !hasMatchingCellHeight(r2)) {
              continue;
            }
            if (gap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax()) != 0
                || overlap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax()) != 0) {
              continue;
            }
            const int y_shared
                = (r1.yMax() == r2.yMin()) ? r1.yMax() : r1.yMin();
            if (grid_ != nullptr && y_shared > 0
                && grid_->gridSnapDownY(DbuY(y_shared - 1))
                       == grid_->gridSnapDownY(DbuY(y_shared))) {
              continue;
            }
            const int ovl_x
                = overlap1D(r1.xMin(), r1.xMax(), r2.xMin(), r2.xMax());
            if (ovl_x < 0) {
              continue;
            }
            if (ovl_x == 0) {
              if (except_corner) {
                continue;
              }
              const bool r1_short
                  = (min_len <= 0 || r1.dy() < min_len) && r1.dx() < min_w;
              if ((max_w <= 0 || r2.dx() <= max_w) && r1_short
                  && !canExtendHorizontally(r1,
                                            cell,
                                            cell_box,
                                            strict_empty_sites,
                                            min_w,
                                            &z_rects)) {
                return false;
              }
              const bool r2_short
                  = (min_len <= 0 || r2.dy() < min_len) && r2.dx() < min_w;
              if ((max_w <= 0 || r1.dx() <= max_w) && r2_short
                  && !canExtendHorizontally(r2,
                                            cell,
                                            cell_box,
                                            strict_empty_sites,
                                            min_w,
                                            &z_rects)) {
                return false;
              }
            } else if (ovl_x < min_w
                       && (min_len <= 0 || r1.dy() < min_len
                           || r2.dy() < min_len)
                       && (max_w <= 0 || r1.dx() <= max_w
                           || r2.dx() <= max_w)) {
              return false;
            }
          }
        }
        return true;
      };

      const std::vector<odb::Rect> l1_spans
          = collectZeroPrlSpans(layer, eff_rects_1d);
      if (!has_sec) {
        return evalZeroPrlRects(l1_spans, l1_spans.size());
      }
      auto evalWithSecLayer = [&](odb::dbTechLayer* l2) -> bool {
        if (l2 == nullptr || l2 == layer) {
          return true;
        }
        auto it = layer_shapes.find(l2);
        if (it == layer_shapes.end() || it->second.empty()) {
          return evalZeroPrlRects(l1_spans, l1_spans.size());
        }
        std::vector<odb::Rect> l2_raw;
        l2_raw.reserve(it->second.size());
        for (const TaggedRect& tr : it->second) {
          l2_raw.push_back(tr.rect);
        }
        const std::vector<odb::Rect> l2_merged = merge1D(l2_raw);
        std::vector<odb::Rect> row_placed = eff_rects_1d;
        row_placed.insert(row_placed.end(), l2_merged.begin(), l2_merged.end());
        row_placed = merge1D(row_placed);

        const std::vector<odb::Rect> l2_spans
            = collectZeroPrlSpans(l2, l2_merged);
        std::vector<odb::Rect> combined;
        combined.reserve(l1_spans.size() + l2_spans.size());
        for (const odb::Rect& s : l1_spans) {
          combined.push_back(extendAcrossPlaced(s, row_placed));
        }
        for (const odb::Rect& s : l2_spans) {
          combined.push_back(extendAcrossPlaced(s, row_placed));
        }
        return evalZeroPrlRects(combined, l1_spans.size());
      };
      if (sec_layer != nullptr) {
        return evalWithSecLayer(sec_layer);
      }
      if (layer_any && tech_ != nullptr) {
        bool checked_any = false;
        for (odb::dbTechLayer* l2 : tech_->getLayers()) {
          if (l2 != layer && l2->getType() == odb::dbTechLayerType::IMPLANT
              && layer_shapes.contains(l2) && !layer_shapes.at(l2).empty()) {
            checked_any = true;
            if (!evalWithSecLayer(l2)) {
              return false;
            }
          }
        }
        if (!checked_any) {
          return evalZeroPrlRects(l1_spans, l1_spans.size());
        }
      }
      return true;
    }

    // Case 2: WIDTH minWidth LAYER {secondLayer | ANY} [MAXWIDTH maxWidth]
    // [LENGTH length] [EXCEPTCORNERTOUCH]
    // LENGTH has the same meaning as in the ZEROPRL form: it selects the
    // shapes the rule applies to (vertical height < length); WIDTH is the
    // minimum horizontal width of a shape on `layer` that abuts a shape on
    // the secondary layer.
    if (has_sec) {
      const std::vector<odb::Rect> sec_rects = getSecRects();
      for (const odb::Rect& r1 : eff_req_rects_1d) {
        if (!hasMatchingCellHeight(r1)) {
          continue;
        }
        if (min_len > 0 && r1.dy() >= min_len) {
          continue;
        }
        const bool r1_violates = r1.dx() < min_w
                                 && !canExtendHorizontally(r1,
                                                           cell,
                                                           cell_box,
                                                           strict_empty_sites,
                                                           min_w,
                                                           &eff_req_rects_1d);
        if (!r1_violates) {
          continue;
        }
        for (const odb::Rect& r2 : sec_rects) {
          if (!r1.overlaps(active_box) && !r2.overlaps(active_box)) {
            continue;
          }
          if (max_w > 0 && r2.dx() > max_w) {
            continue;
          }
          const int dx_gap = gap1D(r1.xMin(), r1.xMax(), r2.xMin(), r2.xMax());
          const int dy_gap = gap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax());
          if (dx_gap > 0 || dy_gap > 0) {
            continue;
          }
          if (except_corner && isCornerTouchOnly(r1, r2)) {
            continue;
          }
          return false;
        }
      }
      return true;
    }

    // Case 3: WIDTH width LENGTH minLength [WITHINLAYER ...] [CELLHEIGHT ...]
    if (min_len > 0) {
      for (const Component2D& comp : getEffComps2D()) {
        if (!comp.bbox.overlaps(active_box)) {
          continue;
        }
        bool comp_matches_h = (req_cell_h <= 0);
        bool comp_touches_cand = false;
        for (const odb::Rect& r : comp.rects) {
          if (r.overlaps(active_box)) {
            comp_touches_cand = true;
          }
          if (!comp_matches_h && hasMatchingCellHeight(r)) {
            comp_matches_h = true;
          }
        }
        if (!comp_touches_cand || !comp_matches_h) {
          continue;
        }
        BoostPolygonSet pset;
        using boost::polygon::operators::operator+=;
        for (const odb::Rect& r : comp.rects) {
          pset += r;
        }
        std::vector<odb::Rect> h_rects;
        pset.get_rectangles(h_rects, boost::polygon::HORIZONTAL);
        for (const odb::Rect& hr : h_rects) {
          if (!hr.overlaps(active_box) || !hasMatchingCellHeight(hr)) {
            continue;
          }
          if (hr.dy() <= min_w && hr.dx() < min_len
              && !canExtendHorizontally(hr,
                                        cell,
                                        cell_box,
                                        strict_empty_sites,
                                        min_len,
                                        &eff_rects_1d)
              && !canExtendVertically(
                  hr, min_w + 1, cell, cell_box, strict_empty_sites)) {
            return false;
          }
        }
        std::vector<odb::Rect> v_rects;
        pset.get_rectangles(v_rects, boost::polygon::VERTICAL);
        for (const odb::Rect& vr : v_rects) {
          if (!vr.overlaps(active_box) || !hasMatchingCellHeight(vr)) {
            continue;
          }
          if (vr.dx() <= min_w && vr.dy() < min_len
              && !canExtendHorizontally(vr,
                                        cell,
                                        cell_box,
                                        strict_empty_sites,
                                        min_w + 1,
                                        &eff_rects_1d)
              && !canExtendVertically(
                  vr, min_len, cell, cell_box, strict_empty_sites)) {
            return false;
          }
        }
      }
      return true;
    }

    // Case 4: Plain LEF58_WIDTH minWidth [WITHINLAYER ...] [CELLHEIGHT ...]
    return checkPlainWidth(eff_rects_1d,
                           min_w,
                           cell,
                           cell_box,
                           touch_box,
                           strict_empty_sites,
                           hasMatchingCellHeight);
  }

  template <typename GetComps2D>
  bool checkImplantSpacingRule(
      odb::dbTechLayer* layer,
      odb::dbTechLayerImplantSpacingRule* rule,
      const std::unordered_map<odb::dbTechLayer*, std::vector<TaggedRect>>&
          layer_shapes,
      const std::vector<odb::Rect>& rects_1d,
      const GetComps2D& get_comps_2d,
      const Node* cell,
      const odb::Rect& cell_box,
      const bool strict_empty_sites) const
  {
    const int min_spc = rule->getMinSpacing();
    if (min_spc <= 0) {
      return true;
    }
    odb::dbTechLayer* sec_layer = rule->getSecondLayer();
    const bool same_layer = (sec_layer == nullptr || sec_layer == layer);
    const bool is_horiz = rule->isHorizontal();
    const bool is_vert = rule->isVertical();
    const bool has_prl = rule->isPrlValid();
    const int rule_prl = rule->getPrl();
    const bool except_abutted = rule->isExceptAbutted();
    const bool except_corner = rule->isExceptCornerTouch();
    const int max_len = rule->getLength();
    const auto intersect_layers = rule->getIntersectLayers();
    const int anchor_reach = min_spc + (has_prl ? std::abs(rule_prl) : 0);

    std::vector<odb::Rect> sec_rects;
    if (!same_layer) {
      auto it = layer_shapes.find(sec_layer);
      if (it != layer_shapes.end()) {
        sec_rects.reserve(it->second.size());
        for (const TaggedRect& tr : it->second) {
          sec_rects.push_back(tr.rect);
        }
      }
      sec_rects = merge1D(sec_rects);
      if (sec_rects.empty()) {
        return true;
      }
    }

    auto computeGapSpan
        = [](const odb::Rect& r1, const odb::Rect& r2) -> odb::Rect {
      if (r1.overlaps(r2)) {
        return r1.intersect(r2);
      }
      const int ovl_x = overlap1D(r1.xMin(), r1.xMax(), r2.xMin(), r2.xMax());
      const int sx_lo = (ovl_x > 0) ? std::max(r1.xMin(), r2.xMin())
                                    : std::min(r1.xMax(), r2.xMax());
      const int sx_hi = (ovl_x > 0) ? std::min(r1.xMax(), r2.xMax())
                                    : std::max(r1.xMin(), r2.xMin());

      const int ovl_y = overlap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax());
      const int sy_lo = (ovl_y > 0) ? std::max(r1.yMin(), r2.yMin())
                                    : std::min(r1.yMax(), r2.yMax());
      const int sy_hi = (ovl_y > 0) ? std::min(r1.yMax(), r2.yMax())
                                    : std::max(r1.yMin(), r2.yMin());
      return odb::Rect(sx_lo, sy_lo, sx_hi, sy_hi);
    };

    // INTERSECTLAYERS: the spacing applies only where the gap between the two
    // shapes is also covered by a shape on at least one of the listed layers.
    auto checkIntersectLayers
        = [&](const odb::Rect& r1, const odb::Rect& r2) -> bool {
      if (intersect_layers.empty()) {
        return true;
      }
      const odb::Rect span = computeGapSpan(r1, r2);

      for (odb::dbTechLayer* il : intersect_layers) {
        auto it = layer_shapes.find(il);
        if (it == layer_shapes.end() || it->second.empty()) {
          continue;
        }
        for (const TaggedRect& tr : it->second) {
          const odb::Rect& ir = tr.rect;
          bool hits = false;
          if (span.dx() > 0 && span.dy() > 0) {
            hits = ir.overlaps(span);
          } else if (span.dx() == 0 && span.dy() > 0) {
            hits = (ir.xMin() <= span.xMin() && span.xMin() <= ir.xMax()
                    && overlap1D(ir.yMin(), ir.yMax(), span.yMin(), span.yMax())
                           > 0);
          } else if (span.dy() == 0 && span.dx() > 0) {
            hits = (ir.yMin() <= span.yMin() && span.yMin() <= ir.yMax()
                    && overlap1D(ir.xMin(), ir.xMax(), span.xMin(), span.xMax())
                           > 0);
          } else {
            hits = ir.intersects(span);
          }
          if (hits) {
            return true;
          }
        }
      }
      return false;
    };

    auto evalPair = [&](const odb::Rect& r1,
                        const odb::Rect& r2,
                        const int v_len1,
                        const int v_len2,
                        const bool check_horiz_dir,
                        const bool check_vert_dir,
                        const bool check_omni_dir) -> bool {
      const bool anchored
          = pairAnchoredToCandidate(r1,
                                    r2,
                                    anchor_reach,
                                    cell_box,
                                    same_layer && !strict_empty_sites)
            || (!intersect_layers.empty()
                && cell_box.intersects(computeGapSpan(r1, r2)));
      if (!anchored) {
        return true;
      }
      if (!same_layer && r1.overlaps(r2)) {
        const bool len_ok_h
            = (max_len <= 0 || r1.dx() <= max_len || r2.dx() <= max_len);
        const bool len_ok_v
            = (max_len <= 0 || v_len1 <= max_len || v_len2 <= max_len);
        return !(((is_horiz && len_ok_h) || (is_vert && len_ok_v)
                  || (!is_horiz && !is_vert && (len_ok_h || len_ok_v)))
                 && checkIntersectLayers(r1, r2));
      }

      const int dx_gap = gap1D(r1.xMin(), r1.xMax(), r2.xMin(), r2.xMax());
      const int dy_gap = gap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax());
      const int prl_x = overlap1D(r1.xMin(), r1.xMax(), r2.xMin(), r2.xMax());
      const int prl_y = overlap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax());

      if (except_corner && dx_gap == 0 && dy_gap == 0 && prl_x == 0
          && prl_y == 0) {
        return true;
      }

      // HORIZONTAL spacing applies only when rects do NOT overlap in X (prl_x
      // <= 0).
      if (check_horiz_dir && is_horiz && prl_x <= 0) {
        const bool both_long
            = max_len > 0 && r1.dx() > max_len && r2.dx() > max_len;
        const bool abutted = dx_gap == 0 && dy_gap == 0 && prl_y > 0;
        if (!both_long && !((except_abutted || same_layer) && abutted)) {
          bool prl_match;
          if (has_prl) {
            prl_match = rule_prl > 0 ? (dy_gap == 0 && prl_y >= rule_prl)
                                     : (prl_y >= rule_prl);
          } else {
            // Without PRL the shapes must face each other (positive run
            // length); a corner-only touch is a corner, not a facing pair.
            prl_match = dy_gap == 0 && prl_y > 0;
          }
          const bool bridged = same_layer && dy_gap == 0
                               && canBridgeHorizontally(
                                   r1, r2, cell, cell_box, strict_empty_sites);
          if (prl_match && dx_gap < min_spc && !bridged
              && checkIntersectLayers(r1, r2)) {
            return false;
          }
        }
      }

      // VERTICAL spacing applies only when rects do NOT overlap in Y (prl_y <=
      // 0).
      if (check_vert_dir && is_vert && prl_y <= 0) {
        const bool both_long
            = max_len > 0 && v_len1 > max_len && v_len2 > max_len;
        const bool abutted = dy_gap == 0 && dx_gap == 0 && prl_x > 0;
        if (!both_long && !((except_abutted || same_layer) && abutted)) {
          bool prl_match;
          if (has_prl) {
            prl_match = rule_prl > 0 ? (dx_gap == 0 && prl_x >= rule_prl)
                                     : (prl_x >= rule_prl);
          } else {
            // See the HORIZONTAL case: no PRL means a facing pair is required.
            prl_match = dx_gap == 0 && prl_x > 0;
          }
          const bool bridged
              = same_layer
                && ((dy_gap == 0
                     && canBridgeHorizontally(
                         r1, r2, cell, cell_box, strict_empty_sites))
                    || (prl_x > 0
                        && canBridgeVertically(
                            r1, r2, cell, cell_box, strict_empty_sites)));
          if (prl_match && dy_gap < min_spc && !bridged
              && checkIntersectLayers(r1, r2)) {
            return false;
          }
        }
      }

      // Omnidirectional Euclidean spacing when neither HORIZONTAL nor VERTICAL.
      if (check_omni_dir && !is_horiz && !is_vert) {
        const bool both_long = max_len > 0 && r1.dx() > max_len
                               && r2.dx() > max_len && v_len1 > max_len
                               && v_len2 > max_len;
        if (!both_long
            && !((except_abutted || same_layer) && isEdgeConnected(r1, r2))
            && (!has_prl || std::max(prl_x, prl_y) >= rule_prl)) {
          const int64_t dist_sq = static_cast<int64_t>(dx_gap) * dx_gap
                                  + static_cast<int64_t>(dy_gap) * dy_gap;
          const bool bridged
              = same_layer
                && ((dy_gap == 0
                     && canBridgeHorizontally(
                         r1, r2, cell, cell_box, strict_empty_sites))
                    || (prl_x > 0
                        && canBridgeVertically(
                            r1, r2, cell, cell_box, strict_empty_sites)));
          if (dist_sq < static_cast<int64_t>(min_spc) * min_spc && !bridged
              && checkIntersectLayers(r1, r2)) {
            return false;
          }
        }
      }
      return true;
    };

    auto buildVertSlices = [](const std::vector<odb::Rect>& src) {
      std::vector<odb::Rect> v_rects;
      if (src.empty()) {
        return v_rects;
      }
      BoostPolygonSet pset;
      using boost::polygon::operators::operator+=;
      for (const odb::Rect& r : src) {
        pset += r;
      }
      pset.get_rectangles(v_rects, boost::polygon::VERTICAL);
      return v_rects;
    };

    auto lookupVertLen
        = [](const odb::Rect& r, const std::vector<odb::Rect>& v_slices) {
            if (v_slices.empty()) {
              return r.dy();
            }
            int best = r.dy();
            for (const odb::Rect& vr : v_slices) {
              if (overlap1D(r.xMin(), r.xMax(), vr.xMin(), vr.xMax()) > 0
                  && vr.yMin() <= r.yMin() && vr.yMax() >= r.yMax()) {
                best = std::max(best, vr.dy());
              }
            }
            return best;
          };

    const bool need_vert_len = max_len > 0 && (is_vert || !is_horiz);
    const std::vector<odb::Rect> v_slices1
        = need_vert_len ? buildVertSlices(rects_1d) : std::vector<odb::Rect>{};

    if (!same_layer) {
      const std::vector<odb::Rect> v_slices2 = need_vert_len
                                                   ? buildVertSlices(sec_rects)
                                                   : std::vector<odb::Rect>{};
      for (const odb::Rect& r1 : rects_1d) {
        const int vl1 = lookupVertLen(r1, v_slices1);
        for (const odb::Rect& r2 : sec_rects) {
          const int vl2 = lookupVertLen(r2, v_slices2);
          if (!evalPair(r1, r2, vl1, vl2, true, true, true)) {
            return false;
          }
        }
      }
      return true;
    }

    // Same-layer:
    // 1. Inter-component pairs across distinct 2D components.
    const std::vector<Component2D>& comps = get_comps_2d();
    for (size_t i = 0; i < comps.size(); ++i) {
      for (size_t j = i + 1; j < comps.size(); ++j) {
        for (const odb::Rect& r1 : comps[i].rects) {
          const int vl1 = lookupVertLen(r1, v_slices1);
          for (const odb::Rect& r2 : comps[j].rects) {
            const int vl2 = lookupVertLen(r2, v_slices1);
            if (!evalPair(r1, r2, vl1, vl2, true, true, true)) {
              return false;
            }
          }
        }
      }
    }

    // 2. Intra-component notches (horizontal notches & figure-8 corner touches
    //    on Y-band comp.rects, plus vertical notches on X-band v_rects).
    for (const Component2D& comp : comps) {
      if (comp.rects.size() < 2) {
        continue;
      }
      for (size_t a = 0; a < comp.rects.size(); ++a) {
        for (size_t b = a + 1; b < comp.rects.size(); ++b) {
          const odb::Rect& r1 = comp.rects[a];
          const odb::Rect& r2 = comp.rects[b];
          if (isEdgeConnected(r1, r2)) {
            continue;
          }
          // Skip any vertically separated pair on horizontal strips (may be
          // solid through an intermediate row such as a 3-row staircase/Z);
          // vertical notches are checked on v_rects below.
          if (gap1D(r1.yMin(), r1.yMax(), r2.yMin(), r2.yMax()) > 0) {
            continue;
          }
          if (isCornerTouchOnly(r1, r2)) {
            const int cx = (r1.xMax() == r2.xMin()) ? r1.xMax() : r1.xMin();
            const int cy = (r1.yMax() == r2.yMin()) ? r1.yMax() : r1.yMin();
            bool third_filled = false;
            for (size_t c = 0; c < comp.rects.size(); ++c) {
              if (c == a || c == b) {
                continue;
              }
              const odb::Rect& r3 = comp.rects[c];
              if (r3.xMin() <= cx && cx <= r3.xMax() && r3.yMin() <= cy
                  && cy <= r3.yMax() && isEdgeConnected(r1, r3)
                  && isEdgeConnected(r2, r3)) {
                third_filled = true;
                break;
              }
            }
            if (third_filled) {
              continue;
            }
          }
          const int vl1 = lookupVertLen(r1, v_slices1);
          const int vl2 = lookupVertLen(r2, v_slices1);
          if (!evalPair(r1, r2, vl1, vl2, true, true, true)) {
            return false;
          }
        }
      }

      if (is_vert || !is_horiz) {
        const std::vector<odb::Rect> v_rects = buildVertSlices(comp.rects);
        for (size_t a = 0; a < v_rects.size(); ++a) {
          for (size_t b = a + 1; b < v_rects.size(); ++b) {
            const odb::Rect& v1 = v_rects[a];
            const odb::Rect& v2 = v_rects[b];
            if (overlap1D(v1.xMin(), v1.xMax(), v2.xMin(), v2.xMax()) <= 0
                || gap1D(v1.yMin(), v1.yMax(), v2.yMin(), v2.yMax()) <= 0) {
              continue;
            }
            if (!evalPair(v1, v2, v1.dy(), v2.dy(), false, true, true)) {
              return false;
            }
          }
        }
      }
    }
    return true;
  }

  template <typename PolyT>
  static std::vector<odb::Point> extractCleanContour(const PolyT& poly)
  {
    std::vector<odb::Point> pts;
    for (auto it = boost::polygon::begin_points(poly);
         it != boost::polygon::end_points(poly);
         ++it) {
      const auto pt = *it;
      const odb::Point p(pt.x(), pt.y());
      if (pts.empty() || pts.back() != p) {
        pts.push_back(p);
      }
    }
    if (pts.size() > 1 && pts.front() == pts.back()) {
      pts.pop_back();
    }
    bool changed = true;
    while (changed && pts.size() >= 4) {
      changed = false;
      std::vector<odb::Point> next_pts;
      const size_t m = pts.size();
      for (size_t i = 0; i < m; ++i) {
        const odb::Point& prev = pts[(i + m - 1) % m];
        const odb::Point& cur = pts[i];
        const odb::Point& nxt = pts[(i + 1) % m];
        const bool collinear_x
            = (prev.getX() == cur.getX() && cur.getX() == nxt.getX());
        const bool collinear_y
            = (prev.getY() == cur.getY() && cur.getY() == nxt.getY());
        if (collinear_x || collinear_y) {
          changed = true;
        } else {
          next_pts.push_back(cur);
        }
      }
      pts = std::move(next_pts);
    }
    return pts;
  }

  static int64_t contourSignedArea2(const std::vector<odb::Point>& pts)
  {
    int64_t area2 = 0;
    const size_t m = pts.size();
    for (size_t i = 0; i < m; ++i) {
      const odb::Point& a = pts[i];
      const odb::Point& b = pts[(i + 1) % m];
      area2 += static_cast<int64_t>(a.getX()) * b.getY()
               - static_cast<int64_t>(b.getX()) * a.getY();
    }
    return area2;
  }

  template <typename GetComps2D>
  bool checkMinStepRule(odb::dbTechLayerMinStepRule* rule,
                        const GetComps2D& get_comps_2d,
                        const Node* cell,
                        const odb::Rect& cell_box,
                        const odb::Rect& touch_box,
                        const bool strict_empty_sites) const
  {
    const int min_step = rule->getMinStepLength();
    if (min_step <= 0) {
      return true;
    }
    // Skip routing-only MINSTEP clauses not applicable to standard-cell
    // implants.
    if (rule->isMinAdjLength2Valid() || rule->isConvexCorner()
        || rule->isConcaveCorner() || rule->isExceptSameCorners()
        || rule->isMinBetweenLengthValid() || rule->isNoBetweenEol()
        || rule->isNoAdjacentEol() || rule->getEolWidth() > 0) {
      return true;
    }

    const bool has_adj = rule->isMinAdjLength1Valid();
    const int min_adj = rule->getMinAdjLength1();
    const bool has_max_edges = rule->isMaxEdgesValid();
    const uint32_t max_edges = rule->getMaxEdges();
    const bool except_rect = rule->isExceptRectangle();

    auto makeEdgeInteriorRect = [](const odb::Point& pa,
                                   const odb::Point& pb,
                                   const int64_t signed_area2,
                                   const bool is_hole) -> odb::Rect {
      if (pa.getY() == pb.getY()) {
        const int y = pa.getY();
        const bool interior_above
            = ((pb.getX() > pa.getX()) == ((signed_area2 > 0) != is_hole));
        return odb::Rect(std::min(pa.getX(), pb.getX()),
                         interior_above ? y : y - 1,
                         std::max(pa.getX(), pb.getX()),
                         interior_above ? y + 1 : y);
      }
      const int x = pa.getX();
      return odb::Rect(
          x, std::min(pa.getY(), pb.getY()), x, std::max(pa.getY(), pb.getY()));
    };

    auto canExtendStep = [&](const odb::Point& p_prev,
                             const odb::Point& p0,
                             const odb::Point& p1,
                             const odb::Point& p_next,
                             const int64_t signed_area2,
                             const bool is_hole) -> bool {
      if (strict_empty_sites) {
        return false;
      }
      const odb::Rect r_prev
          = makeEdgeInteriorRect(p_prev, p0, signed_area2, is_hole);
      const odb::Rect r_curr
          = makeEdgeInteriorRect(p0, p1, signed_area2, is_hole);
      const odb::Rect r_next
          = makeEdgeInteriorRect(p1, p_next, signed_area2, is_hole);
      return canExtendHorizontally(r_prev, cell, cell_box, strict_empty_sites)
             || canExtendHorizontally(
                 r_curr, cell, cell_box, strict_empty_sites)
             || canExtendHorizontally(
                 r_next, cell, cell_box, strict_empty_sites);
    };

    auto checkContour
        = [&](const std::vector<odb::Point>& pts, const bool is_hole) -> bool {
      const size_t m = pts.size();
      if (m < 4) {
        return true;
      }
      const int64_t signed_area2 = contourSignedArea2(pts);

      if (has_max_edges && !has_adj) {
        // Count consecutive runs of unextendable < min_step edges; violate
        // only when a run touching touch_box exceeds max_edges.
        std::vector<bool> short_edge(m, false);
        std::vector<bool> near_cand(m, false);
        bool all_short = true;
        for (size_t i = 0; i < m; ++i) {
          const odb::Point& p_prev = pts[(i + m - 1) % m];
          const odb::Point& p0 = pts[i];
          const odb::Point& p1 = pts[(i + 1) % m];
          const odb::Point& p_next = pts[(i + 2) % m];
          const int len_curr = std::abs(p1.getX() - p0.getX())
                               + std::abs(p1.getY() - p0.getY());
          const odb::Rect edge_box(std::min(p0.getX(), p1.getX()),
                                   std::min(p0.getY(), p1.getY()),
                                   std::max(p0.getX(), p1.getX()),
                                   std::max(p0.getY(), p1.getY()));
          near_cand[i] = edge_box.intersects(touch_box);
          if (len_curr < min_step
              && !canExtendStep(
                  p_prev, p0, p1, p_next, signed_area2, is_hole)) {
            short_edge[i] = true;
          } else {
            all_short = false;
          }
        }
        if (all_short) {
          bool any_near = false;
          for (const bool nc : near_cand) {
            any_near = any_near || nc;
          }
          return !any_near || m <= max_edges;
        }
        size_t start = 0;
        while (start < m && short_edge[start]) {
          ++start;
        }
        uint32_t run_len = 0;
        bool run_near = false;
        for (size_t step = 0; step <= m; ++step) {
          const size_t idx = (start + step) % m;
          if (short_edge[idx]) {
            ++run_len;
            run_near = run_near || near_cand[idx];
          } else {
            if (run_near && run_len > max_edges) {
              return false;
            }
            run_len = 0;
            run_near = false;
          }
        }
        return true;
      }

      for (size_t i = 0; i < m; ++i) {
        const odb::Point& p_prev = pts[(i + m - 1) % m];
        const odb::Point& p0 = pts[i];
        const odb::Point& p1 = pts[(i + 1) % m];
        const odb::Point& p_next = pts[(i + 2) % m];

        odb::Rect gate_box(std::min(p0.getX(), p1.getX()),
                           std::min(p0.getY(), p1.getY()),
                           std::max(p0.getX(), p1.getX()),
                           std::max(p0.getY(), p1.getY()));
        if (has_adj) {
          // The staircase is a 3-edge pattern; the cell that creates or
          // shortens a tread usually does not touch the riser itself.
          gate_box.merge(odb::Rect(std::min(p_prev.getX(), p0.getX()),
                                   std::min(p_prev.getY(), p0.getY()),
                                   std::max(p_prev.getX(), p0.getX()),
                                   std::max(p_prev.getY(), p0.getY())));
          gate_box.merge(odb::Rect(std::min(p1.getX(), p_next.getX()),
                                   std::min(p1.getY(), p_next.getY()),
                                   std::max(p1.getX(), p_next.getX()),
                                   std::max(p1.getY(), p_next.getY())));
        }
        if (!gate_box.intersects(touch_box)) {
          continue;
        }

        const int len_prev = std::abs(p0.getX() - p_prev.getX())
                             + std::abs(p0.getY() - p_prev.getY());
        const int len_curr
            = std::abs(p1.getX() - p0.getX()) + std::abs(p1.getY() - p0.getY());
        const int len_next = std::abs(p_next.getX() - p1.getX())
                             + std::abs(p_next.getY() - p1.getY());

        if (has_adj) {
          // LEF 5.8 p. 466 Fig. 3-7 (Implant Min Step): vertical middle step
          // of a 3-row staircase must exceed minStepLength when BOTH adjacent
          // horizontal edges are <= minAdjLength.
          if (p0.getX() != p1.getX() || len_curr > min_step) {
            continue;
          }
          const int64_t dx_prev = p0.getX() - p_prev.getX();
          const int64_t dx_next = p_next.getX() - p1.getX();
          if (dx_prev * dx_next > 0 && len_prev <= min_adj
              && len_next <= min_adj
              && !canExtendStep(
                  p_prev, p0, p1, p_next, signed_area2, is_hole)) {
            return false;
          }
        } else if (len_curr < min_step
                   && !canExtendStep(
                       p_prev, p0, p1, p_next, signed_area2, is_hole)) {
          return false;
        }
      }
      return true;
    };

    for (const Component2D& comp : get_comps_2d()) {
      if (!comp.bbox.intersects(touch_box)) {
        continue;
      }
      for (const BoostPolygon& poly : comp.polys) {
        const std::vector<odb::Point> outer_pts = extractCleanContour(poly);
        const bool has_holes = (poly.begin_holes() != poly.end_holes());
        if (!except_rect || outer_pts.size() != 4 || has_holes) {
          if (!checkContour(outer_pts, false)) {
            return false;
          }
        }
        for (auto hit = poly.begin_holes(); hit != poly.end_holes(); ++hit) {
          if (!checkContour(extractCleanContour(*hit), true)) {
            return false;
          }
        }
      }
    }
    return true;
  }

  template <typename GetComps2D>
  bool checkCoreEdgeLengthRule(
      odb::dbTechLayer* layer,
      odb::dbTechLayerCoreEdgeLengthRule* rule,
      const std::unordered_map<odb::dbTechLayer*, std::vector<TaggedRect>>&
          layer_shapes,
      const std::vector<odb::Rect>& rects_1d,
      const GetComps2D& get_comps_2d,
      const Node* cell,
      const odb::Rect& cell_box,
      const odb::Rect& touch_box,
      const bool strict_empty_sites) const
  {
    (void) layer;
    const int min_len = rule->getMinLength();
    if (min_len <= 0) {
      return true;
    }
    const int max_len = rule->getMaxLength();
    std::vector<int> adj_edge_lengths;
    rule->getExceptAdjacentEdgeLengths(adj_edge_lengths);
    std::vector<int> adj_lengths;
    rule->getExceptAdjacentLengths(adj_lengths);
    std::vector<int> adj_exact;
    rule->getExceptAdjacentExact(adj_exact);
    const auto within_layers = rule->getWithinLayers();
    const auto except_layers = rule->getExceptLayers();

    std::vector<odb::Rect> clipped_rects_1d;
    std::vector<Component2D> clipped_comps;
    const bool has_within = !within_layers.empty();
    if (has_within) {
      BoostPolygonSet within_set;
      using boost::polygon::operators::operator+=;
      using boost::polygon::operators::operator&=;
      for (odb::dbTechLayer* wl : within_layers) {
        auto it = layer_shapes.find(wl);
        if (it != layer_shapes.end()) {
          for (const TaggedRect& tr : it->second) {
            within_set += tr.rect;
          }
        }
      }
      BoostPolygonSet cand_set;
      for (const odb::Rect& r : rects_1d) {
        cand_set += r;
      }
      cand_set &= within_set;
      std::vector<odb::Rect> clipped;
      cand_set.get_rectangles(clipped, boost::polygon::HORIZONTAL);
      if (clipped.empty()) {
        return true;
      }
      clipped_rects_1d = merge1D(clipped);
      clipped_comps = merge2D(clipped_rects_1d);
    }

    const std::vector<odb::Rect>& eff_rects_1d
        = has_within ? clipped_rects_1d : rects_1d;
    const std::vector<Component2D>& comps
        = has_within ? clipped_comps : get_comps_2d();
    if (comps.empty()) {
      return true;
    }

    std::vector<odb::Rect> except_rects;
    for (odb::dbTechLayer* el : except_layers) {
      auto it = layer_shapes.find(el);
      if (it != layer_shapes.end()) {
        for (const TaggedRect& tr : it->second) {
          except_rects.push_back(tr.rect);
        }
      }
    }

    const odb::Rect active_box = strict_empty_sites ? cell_box : touch_box;
    const size_t adj_n = std::min(
        {adj_edge_lengths.size(), adj_lengths.size(), adj_exact.size()});

    auto checkContour
        = [&](const std::vector<odb::Point>& pts, const bool is_hole) -> bool {
      const size_t m = pts.size();
      if (m < 4) {
        return true;
      }
      const int64_t signed_area2 = contourSignedArea2(pts);
      for (size_t i = 0; i < m; ++i) {
        const odb::Point& p_prev = pts[(i + m - 1) % m];
        const odb::Point& p0 = pts[i];
        const odb::Point& p1 = pts[(i + 1) % m];
        const odb::Point& p_next = pts[(i + 2) % m];

        // Core edge length applies to horizontal (core-row parallel) edges.
        if (p0.getY() != p1.getY()) {
          continue;
        }
        const int x_lo = std::min(p0.getX(), p1.getX());
        const int x_hi = std::max(p0.getX(), p1.getX());
        const int y_edge = p0.getY();
        const int len_curr = x_hi - x_lo;
        if (len_curr >= min_len) {
          continue;
        }

        const bool interior_above
            = ((p1.getX() > p0.getX()) == ((signed_area2 > 0) != is_hole));
        const odb::Rect interior_strip(x_lo,
                                       interior_above ? y_edge : y_edge - 1,
                                       x_hi,
                                       interior_above ? y_edge + 1 : y_edge);
        if (!interior_strip.overlaps(active_box)) {
          continue;
        }
        if (canExtendHorizontally(interior_strip,
                                  cell,
                                  cell_box,
                                  strict_empty_sites,
                                  min_len,
                                  &eff_rects_1d)) {
          continue;
        }
        // Before filler insertion a short edge that faces empty, fillable
        // sites (a gap under or over the island) disappears once a same-layer
        // filler lands there, so it is not final yet.
        if (!strict_empty_sites) {
          const odb::Rect exterior_strip(x_lo,
                                         interior_above ? y_edge - 1 : y_edge,
                                         x_hi,
                                         interior_above ? y_edge : y_edge + 1);
          if (sitesFillable(exterior_strip, cell, cell_box)) {
            continue;
          }
        }

        bool is_except_layer = false;
        for (const odb::Rect& er : except_rects) {
          if (er.overlaps(interior_strip)) {
            is_except_layer = true;
            break;
          }
        }
        if (is_except_layer) {
          continue;
        }

        const int len_prev = std::abs(p0.getY() - p_prev.getY())
                             + std::abs(p0.getX() - p_prev.getX());
        const int len_next = std::abs(p_next.getY() - p1.getY())
                             + std::abs(p_next.getX() - p1.getX());

        bool exempt = false;
        for (size_t k = 0; k < adj_n; ++k) {
          if (len_curr != adj_edge_lengths[k]) {
            continue;
          }
          const int req_adj = adj_lengths[k];
          if (adj_exact[k] != 0) {
            if (len_prev == req_adj || len_next == req_adj) {
              exempt = true;
              break;
            }
          } else {
            if (len_prev >= req_adj || len_next >= req_adj) {
              exempt = true;
              break;
            }
          }
        }
        if (!exempt) {
          return false;
        }
      }
      return true;
    };

    for (const Component2D& comp : comps) {
      if (!comp.bbox.overlaps(active_box)) {
        continue;
      }
      if (max_len > 0 && comp.bbox.dx() > max_len) {
        continue;
      }
      for (const BoostPolygon& poly : comp.polys) {
        if (!checkContour(extractCleanContour(poly), false)) {
          return false;
        }
        for (auto hit = poly.begin_holes(); hit != poly.end_holes(); ++hit) {
          if (!checkContour(extractCleanContour(*hit), true)) {
            return false;
          }
        }
      }
    }
    return true;
  }

  // Before filler insertion, a corner-touch junction between two shapes is
  // not final if a same-layer filler can land in one of the two empty
  // quadrants at the junction: the filler edge-connects the shapes and the
  // junction disappears. Returns true when such a site exists.
  bool junctionFillable(const odb::Rect& r1,
                        const odb::Rect& r2,
                        const odb::Point& pt,
                        const Node* cell,
                        const odb::Rect& cell_box) const
  {
    if (grid_ == nullptr) {
      return false;
    }
    for (const int sx : {-1, 1}) {
      for (const int sy : {-1, 1}) {
        const odb::Point probe(pt.getX() + sx, pt.getY() + sy);
        if (r1.intersects(probe) || r2.intersects(probe)
            || cell_box.intersects(probe) || probe.getX() < 0
            || probe.getY() < 0) {
          continue;
        }
        const int gx = grid_->gridX(DbuX(probe.getX())).v;
        const int gy = grid_->gridSnapDownY(DbuY(probe.getY())).v;
        if (gx < 0 || gy < 0 || gx >= grid_->getRowSiteCount().v
            || gy >= grid_->getRowCount().v) {
          continue;
        }
        const Pixel* pixel = grid_->gridPixel(GridX(gx), GridY(gy));
        if (pixel != nullptr && pixel->is_valid
            && (pixel->cell == nullptr || pixel->cell == cell)) {
          return true;
        }
      }
    }
    return false;
  }

  // LEF58_CORNERSPACING on IMPLANT layers: the distance between two
  // corner-touch junctions (points where two implant shapes meet only at a
  // corner, diagonally) must be at least the rule spacing. Concave corners of
  // a single merged shape (notches, holes) are not junctions and are not
  // measured.
  template <typename GetComps2D>
  bool checkCornerSpacingRule(odb::dbTechLayerCornerSpacingRule* rule,
                              const GetComps2D& get_comps_2d,
                              const Node* cell,
                              const odb::Rect& cell_box,
                              const odb::Rect& touch_box,
                              const bool strict_empty_sites) const
  {
    if (rule->isCheckImplantGroupOnly()
        || rule->getType() == odb::dbTechLayerCornerSpacingRule::CONCAVECORNER
        || rule->isCornerOnly() || rule->isCornerToCorner()
        || rule->isSameMask() || rule->isExceptEol()
        || rule->isExceptJogLength() || rule->isEdgeLengthValid()
        || rule->isIncludeShape() || rule->isMinLengthValid()
        || rule->isExceptNotch() || rule->isExceptNotchLengthValid()
        || rule->isExceptSameNet() || rule->isExceptSameMetal()) {
      return true;
    }

    std::vector<int> width_table;
    std::vector<std::pair<int, int>> spc_table;
    rule->getWidthTable(width_table);
    rule->getSpacingTable(spc_table);
    if (spc_table.empty()) {
      return true;
    }

    int rule_max_spc = 0;
    for (const auto& [s1, s2] : spc_table) {
      rule_max_spc = std::max({rule_max_spc, s1, s2});
    }
    if (rule_max_spc <= 0) {
      return true;
    }

    const bool aligned_only = rule->isAlignedOnly();
    const odb::Rect search_box = touch_box.bloat(rule_max_spc);
    const std::vector<Component2D>& comps_2d = get_comps_2d();

    struct CornerInfo
    {
      odb::Point pt;
      int adj_len{0};
    };
    std::vector<CornerInfo> corners;

    // 1. True exterior diagonal corner touches between distinct 2D components
    //    (plus figure-8 self-touches where neither bridging quadrant is
    //    filled).
    for (size_t a = 0; a < comps_2d.size(); ++a) {
      if (!comps_2d[a].bbox.intersects(search_box)) {
        continue;
      }
      for (size_t b = a; b < comps_2d.size(); ++b) {
        if (!comps_2d[b].bbox.intersects(search_box)) {
          continue;
        }
        if (a != b && rectDistSq(comps_2d[a].bbox, comps_2d[b].bbox) > 0) {
          continue;
        }
        for (size_t i = 0; i < comps_2d[a].rects.size(); ++i) {
          const odb::Rect& r1 = comps_2d[a].rects[i];
          const size_t j_start = (a == b) ? (i + 1) : 0;
          for (size_t j = j_start; j < comps_2d[b].rects.size(); ++j) {
            const odb::Rect& r2 = comps_2d[b].rects[j];
            if (!isCornerTouchOnly(r1, r2)) {
              continue;
            }
            const int cx = (r1.xMax() == r2.xMin() || r1.xMax() == r2.xMax())
                               ? r1.xMax()
                               : r1.xMin();
            const int cy = (r1.yMax() == r2.yMin() || r1.yMax() == r2.yMax())
                               ? r1.yMax()
                               : r1.yMin();
            const odb::Point pt(cx, cy);
            if (!search_box.intersects(pt)) {
              continue;
            }
            if (!strict_empty_sites && a != b
                && junctionFillable(r1, r2, pt, cell, cell_box)) {
              continue;
            }
            if (a == b) {
              bool third_filled = false;
              for (size_t c = 0; c < comps_2d[a].rects.size(); ++c) {
                if (c == i || c == j) {
                  continue;
                }
                const odb::Rect& r3 = comps_2d[a].rects[c];
                if (r3.xMin() <= cx && cx <= r3.xMax() && r3.yMin() <= cy
                    && cy <= r3.yMax() && isEdgeConnected(r1, r3)
                    && isEdgeConnected(r2, r3)) {
                  third_filled = true;
                  break;
                }
              }
              if (third_filled) {
                continue;
              }
            }
            corners.push_back({pt, std::min(r1.dx(), r2.dx())});
          }
        }
      }
    }

    for (size_t i = 0; i < corners.size(); ++i) {
      for (size_t j = i + 1; j < corners.size(); ++j) {
        const CornerInfo& c1 = corners[i];
        const CornerInfo& c2 = corners[j];
        if (c1.pt == c2.pt) {
          continue;
        }
        if (!touch_box.intersects(c1.pt) && !touch_box.intersects(c2.pt)) {
          continue;
        }
        const int max_adj = std::max(c1.adj_len, c2.adj_len);
        int req_spc_x = 0;
        int req_spc_y = 0;
        for (size_t k = 0; k < spc_table.size(); ++k) {
          const int thresh = (k < width_table.size()) ? width_table[k] : 0;
          if (max_adj >= thresh) {
            const int sx = spc_table[k].first;
            const int sy = (spc_table[k].second > 0) ? spc_table[k].second : sx;
            req_spc_x = std::max(req_spc_x, sx);
            req_spc_y = std::max(req_spc_y, sy);
          }
        }
        const int req_spc = std::max(req_spc_x, req_spc_y);
        if (req_spc <= 0) {
          continue;
        }
        const int dx = std::abs(c1.pt.getX() - c2.pt.getX());
        const int dy = std::abs(c1.pt.getY() - c2.pt.getY());
        if (aligned_only) {
          if ((dx == 0 && dy > 0 && dy < req_spc_y)
              || (dy == 0 && dx > 0 && dx < req_spc_x)) {
            return false;
          }
        } else {
          if (req_spc_x == req_spc_y) {
            const int64_t dist_sq
                = static_cast<int64_t>(dx) * dx + static_cast<int64_t>(dy) * dy;
            if (dist_sq < static_cast<int64_t>(req_spc) * req_spc) {
              return false;
            }
          } else if (dx < req_spc_x && dy < req_spc_y) {
            return false;
          }
        }
      }
    }
    return true;
  }

  static bool intervalsCoverSpan(const int span_lo,
                                 const int span_hi,
                                 std::vector<std::pair<int, int>>& intervals)
  {
    if (span_lo >= span_hi) {
      return true;
    }
    if (intervals.empty()) {
      return false;
    }
    std::ranges::sort(intervals);
    int covered_up_to = span_lo;
    for (const auto& [lo, hi] : intervals) {
      if (lo > covered_up_to) {
        return false;
      }
      covered_up_to = std::max(covered_up_to, hi);
      if (covered_up_to >= span_hi) {
        return true;
      }
    }
    return covered_up_to >= span_hi;
  }

  bool checkMaxSpacingRule(odb::dbTechLayerMaxSpacingRule* rule,
                           const std::vector<odb::Rect>& rects_1d,
                           const Node* cell,
                           const odb::Rect& cell_box,
                           const odb::Rect& touch_box,
                           const bool strict_empty_sites) const
  {
    if (rule->hasCutClass()) {
      return true;
    }
    const int max_spc = rule->getMaxSpacing();
    if (max_spc <= 0 || rects_1d.empty()) {
      return true;
    }
    const int rule_w = rule->getWidth();
    const int rule_l = rule->getLength();

    // Slice vertically so vertically stacked cells across rows report their
    // true 2D merged height against rule_w.
    std::vector<odb::Rect> v_slices;
    if (rule_w > 0) {
      BoostPolygonSet pset;
      using boost::polygon::operators::operator+=;
      for (const odb::Rect& r : rects_1d) {
        pset += r;
      }
      pset.get_rectangles(v_slices, boost::polygon::VERTICAL);
    }

    const odb::Rect search_box = strict_empty_sites
                                     ? cell_box
                                     : odb::Rect(touch_box.xMin(),
                                                 cell_box.yMin() - max_spc - 1,
                                                 touch_box.xMax(),
                                                 cell_box.yMax() + max_spc + 1);

    const int site_w = (grid_ != nullptr) ? grid_->getSiteWidth().v : 0;
    const int site_count = (grid_ != nullptr) ? grid_->getRowSiteCount().v : 0;
    const int row_count = (grid_ != nullptr) ? grid_->getRowCount().v : 0;
    const int core_y_min = (row_count > 0) ? rowYLo(0) : 0;
    const int core_y_max = (row_count > 0) ? rowYHi(row_count - 1) : 0;

    for (const odb::Rect& r : rects_1d) {
      if (!r.overlaps(search_box)) {
        continue;
      }
      if (rule_w > 0 && r.dy() > rule_w) {
        continue;
      }
      if (rule_l > 0 && r.dx() >= rule_l) {
        continue;
      }
      if (!strict_empty_sites && rule_l > 0
          && canExtendHorizontally(
              r, cell, cell_box, strict_empty_sites, rule_l, &rects_1d)) {
        continue;
      }

      std::vector<std::pair<int, int>> above_intervals;
      std::vector<std::pair<int, int>> below_intervals;

      // Sub-spans where 2D merged vertical height already exceeds rule_w are
      // exempt.
      if (rule_w > 0) {
        for (const odb::Rect& vr : v_slices) {
          if (vr.dy() <= rule_w || vr.yMin() > r.yMin()
              || vr.yMax() < r.yMax()) {
            continue;
          }
          const int x_lo = std::max(r.xMin(), vr.xMin());
          const int x_hi = std::min(r.xMax(), vr.xMax());
          if (x_lo < x_hi) {
            above_intervals.emplace_back(x_lo, x_hi);
            below_intervals.emplace_back(x_lo, x_hi);
          }
        }
      }

      // Boundary rows at bottom/top of multi-row core abut outer boundary cap
      // rows.
      if (row_count > 1 && r.yMin() <= core_y_min) {
        below_intervals.emplace_back(r.xMin(), r.xMax());
      }
      if (row_count > 1 && r.yMax() >= core_y_max) {
        above_intervals.emplace_back(r.xMin(), r.xMax());
      }

      for (const odb::Rect& other : rects_1d) {
        if (&other == &r) {
          continue;
        }
        const int x_lo = std::max(r.xMin(), other.xMin());
        const int x_hi = std::min(r.xMax(), other.xMax());
        if (x_lo >= x_hi) {
          continue;
        }
        if (other.yMax() > r.yMax() && other.yMin() - r.yMax() <= max_spc) {
          above_intervals.emplace_back(x_lo, x_hi);
        }
        if (other.yMin() < r.yMin() && r.yMin() - other.yMax() <= max_spc) {
          below_intervals.emplace_back(x_lo, x_hi);
        }
      }

      // During incremental legalization (!strict_empty_sites), valid empty
      // vertical sites within max_spc above/below r can still host
      // fillers/cells.
      if (!strict_empty_sites && site_w > 0 && site_count > 0
          && row_count > 0) {
        const int gx_lo = std::clamp(
            grid_->gridX(DbuX(std::max(0, r.xMin()))).v, 0, site_count - 1);
        const int gx_hi
            = std::clamp(grid_->gridEndX(DbuX(std::max(0, r.xMax()))).v,
                         gx_lo + 1,
                         site_count);
        const int gy_lo = std::clamp(
            grid_->gridSnapDownY(DbuY(std::max(0, r.yMin() - max_spc - 1))).v,
            0,
            row_count - 1);
        const int gy_hi = std::clamp(
            grid_->gridEndY(DbuY(std::max(0, r.yMax() + max_spc + 1))).v,
            gy_lo + 1,
            row_count);
        for (int gx = gx_lo; gx < gx_hi; ++gx) {
          const int sx_lo = gx * site_w;
          const int sx_hi = sx_lo + site_w;
          const int x_lo = std::max(r.xMin(), sx_lo);
          const int x_hi = std::min(r.xMax(), sx_hi);
          if (x_lo >= x_hi) {
            continue;
          }
          bool empty_above = false;
          bool empty_below = false;
          for (int gy = gy_lo; gy < gy_hi && !(empty_above && empty_below);
               ++gy) {
            const int ry_lo = rowYLo(gy);
            const int ry_hi = rowYHi(gy);
            const bool cand_blocks
                = overlap1D(sx_lo, sx_hi, cell_box.xMin(), cell_box.xMax()) > 0
                  && overlap1D(ry_lo, ry_hi, cell_box.yMin(), cell_box.yMax())
                         > 0;
            if (cand_blocks) {
              continue;
            }
            const Pixel* pixel = grid_->gridPixel(GridX(gx), GridY(gy));
            if (pixel == nullptr || !pixel->is_valid
                || (pixel->cell != nullptr && pixel->cell != cell)) {
              continue;
            }
            if (ry_hi > r.yMax() && ry_lo - r.yMax() <= max_spc) {
              empty_above = true;
            }
            if (ry_lo < r.yMin() && r.yMin() - ry_hi <= max_spc) {
              empty_below = true;
            }
          }
          if (empty_above) {
            above_intervals.emplace_back(x_lo, x_hi);
          }
          if (empty_below) {
            below_intervals.emplace_back(x_lo, x_hi);
          }
        }
      }

      if (!intervalsCoverSpan(r.xMin(), r.xMax(), above_intervals)
          || !intervalsCoverSpan(r.xMin(), r.xMax(), below_intervals)) {
        return false;
      }
    }
    return true;
  }

  static bool isPolygonExemptFromAreaRule(odb::dbTechLayerAreaRule* rule,
                                          const BoostPolygon& poly)
  {
    const std::vector<odb::Point> pts = extractCleanContour(poly);
    if (pts.size() < 4) {
      return true;
    }
    const bool has_holes = (poly.begin_holes() != poly.end_holes());
    const bool is_rect = (pts.size() == 4 && !has_holes);

    int x_lo = pts[0].getX();
    int x_hi = pts[0].getX();
    int y_lo = pts[0].getY();
    int y_hi = pts[0].getY();
    int min_edge = std::numeric_limits<int>::max();
    int max_edge = 0;
    const size_t m = pts.size();
    for (size_t i = 0; i < m; ++i) {
      const odb::Point& p0 = pts[i];
      const odb::Point& p1 = pts[(i + 1) % m];
      x_lo = std::min(x_lo, p0.getX());
      x_hi = std::max(x_hi, p0.getX());
      y_lo = std::min(y_lo, p0.getY());
      y_hi = std::max(y_hi, p0.getY());
      const int elen
          = std::abs(p1.getX() - p0.getX()) + std::abs(p1.getY() - p0.getY());
      min_edge = std::min(min_edge, elen);
      max_edge = std::max(max_edge, elen);
    }
    for (auto hit = poly.begin_holes(); hit != poly.end_holes(); ++hit) {
      const std::vector<odb::Point> hpts = extractCleanContour(*hit);
      const size_t hm = hpts.size();
      for (size_t i = 0; i < hm; ++i) {
        const odb::Point& p0 = hpts[i];
        const odb::Point& p1 = hpts[(i + 1) % hm];
        const int elen
            = std::abs(p1.getX() - p0.getX()) + std::abs(p1.getY() - p0.getY());
        min_edge = std::min(min_edge, elen);
        max_edge = std::max(max_edge, elen);
      }
    }
    const int bbox_dx = x_hi - x_lo;
    const int bbox_dy = y_hi - y_lo;

    const int rect_w = rule->getRectWidth();
    if (rect_w > 0 && (!is_rect || std::min(bbox_dx, bbox_dy) > rect_w)) {
      return true;
    }
    if (rule->isExceptRectangle() && is_rect) {
      return true;
    }
    const auto [ms_w, ms_l] = rule->getExceptMinSize();
    if (ms_w > 0 && ms_l > 0
        && std::min(bbox_dx, bbox_dy) >= std::min(ms_w, ms_l)
        && std::max(bbox_dx, bbox_dy) >= std::max(ms_w, ms_l)) {
      return true;
    }
    const int eel = rule->getExceptEdgeLength();
    if (eel > 0 && max_edge >= eel) {
      return true;
    }
    const auto [eel_a, eel_b] = rule->getExceptEdgeLengths();
    if ((eel_a > 0 || eel_b > 0) && min_edge >= std::min(eel_a, eel_b)
        && max_edge >= std::max(eel_a, eel_b)) {
      return true;
    }
    const int emw = rule->getExceptMinWidth();
    if (emw > 0) {
      BoostPolygonSet pset;
      using boost::polygon::operators::operator+=;
      pset += poly;
      std::vector<boost::polygon::rectangle_data<int>> max_rects;
      boost::polygon::get_max_rectangles(max_rects, pset);
      bool all_wide = !max_rects.empty();
      for (const auto& mr : max_rects) {
        const int mw = static_cast<int>(
            boost::polygon::delta(mr, boost::polygon::HORIZONTAL));
        const int mh = static_cast<int>(
            boost::polygon::delta(mr, boost::polygon::VERTICAL));
        if (std::min(mw, mh) < emw) {
          all_wide = false;
          break;
        }
      }
      if (all_wide) {
        return true;
      }
    }
    return false;
  }

  Component2D expandComponentOnGrid(odb::dbTechLayer* layer,
                                    const Component2D& seed,
                                    const Node* cand_cell,
                                    const int64_t min_area,
                                    bool& satisfied_early) const
  {
    satisfied_early = false;
    if (grid_ == nullptr) {
      return seed;
    }
    const int site_count = grid_->getRowSiteCount().v;
    const int row_count = grid_->getRowCount().v;
    if (site_count <= 0 || row_count <= 0) {
      return seed;
    }

    std::vector<odb::Rect> cur_rects = seed.rects;
    std::vector<odb::Rect> frontier = seed.rects;
    std::unordered_set<const Node*> fully_used;
    fully_used.insert(cand_cell);
    using NodeRectKey = std::pair<const Node*, size_t>;
    std::unordered_set<NodeRectKey, boost::hash<NodeRectKey>> used_node_rects;
    size_t next_check_sz = 64;

    for (size_t fi = 0; fi < frontier.size(); ++fi) {
      const odb::Rect fr = frontier[fi];
      const int gx_lo = std::clamp(
          grid_->gridX(DbuX(std::max(0, fr.xMin() - 1))).v, 0, site_count - 1);
      const int gx_hi
          = std::clamp(grid_->gridEndX(DbuX(std::max(0, fr.xMax() + 1))).v,
                       0,
                       site_count - 1);
      const int gy_lo
          = std::clamp(grid_->gridSnapDownY(DbuY(std::max(0, fr.yMin() - 1))).v,
                       0,
                       row_count - 1);
      const int gy_hi
          = std::clamp(grid_->gridEndY(DbuY(std::max(0, fr.yMax() + 1))).v,
                       0,
                       row_count - 1);

      std::unordered_set<const Node*> local_nb_seen;
      for (int gy = gy_lo; gy <= gy_hi; ++gy) {
        for (int gx = gx_lo; gx <= gx_hi; ++gx) {
          const Pixel* pixel = grid_->gridPixel(GridX(gx), GridY(gy));
          if (pixel == nullptr || pixel->cell == nullptr
              || fully_used.contains(pixel->cell)
              || !local_nb_seen.insert(pixel->cell).second) {
            continue;
          }
          const Node* nb = pixel->cell;
          odb::dbMaster* nb_master = getDbMaster(nb);
          if (nb_master == nullptr) {
            fully_used.insert(nb);
            continue;
          }
          const MasterLayerShapes& m_shapes
              = getMasterShapes(nb_master, nb->getOrient());
          auto mit = m_shapes.find(layer);
          if (mit == m_shapes.end() || mit->second.empty()) {
            fully_used.insert(nb);
            continue;
          }
          const int nx = nb->getLeft().v;
          const int ny = nb->getBottom().v;
          const size_t m_cnt = mit->second.size();
          std::vector<odb::Rect> nb_rects;
          nb_rects.reserve(m_cnt);
          for (const odb::Rect& mr : mit->second) {
            nb_rects.emplace_back(
                mr.xMin() + nx, mr.yMin() + ny, mr.xMax() + nx, mr.yMax() + ny);
          }

          std::vector<odb::Rect> newly_added;
          for (size_t idx = 0; idx < m_cnt; ++idx) {
            if (used_node_rects.contains({nb, idx})) {
              continue;
            }
            if (isEdgeConnected(nb_rects[idx], fr)) {
              used_node_rects.insert({nb, idx});
              cur_rects.push_back(nb_rects[idx]);
              frontier.push_back(nb_rects[idx]);
              newly_added.push_back(nb_rects[idx]);
            }
          }
          for (size_t ni = 0; ni < newly_added.size(); ++ni) {
            // Copy: the inner loop appends to newly_added and may reallocate.
            const odb::Rect base_r = newly_added[ni];
            for (size_t idx = 0; idx < m_cnt; ++idx) {
              if (used_node_rects.contains({nb, idx})) {
                continue;
              }
              if (isEdgeConnected(nb_rects[idx], base_r)) {
                used_node_rects.insert({nb, idx});
                cur_rects.push_back(nb_rects[idx]);
                frontier.push_back(nb_rects[idx]);
                newly_added.push_back(nb_rects[idx]);
              }
            }
          }

          bool all_in = true;
          for (size_t idx = 0; idx < m_cnt; ++idx) {
            if (!used_node_rects.contains({nb, idx})) {
              all_in = false;
              break;
            }
          }
          if (all_in) {
            fully_used.insert(nb);
          }
        }
      }

      if (min_area < std::numeric_limits<int64_t>::max()
          && cur_rects.size() >= next_check_sz) {
        next_check_sz *= 2;
        const std::vector<Component2D> tmp_comps = merge2D(merge1D(cur_rects));
        if (!tmp_comps.empty()) {
          bool all_ge = true;
          for (const BoostPolygon& p : tmp_comps[0].polys) {
            if (boost::polygon::area(p) < min_area) {
              all_ge = false;
              break;
            }
          }
          if (all_ge) {
            satisfied_early = true;
            return tmp_comps[0];
          }
        }
      }
    }

    std::vector<Component2D> final_comps = merge2D(merge1D(cur_rects));
    if (!final_comps.empty()) {
      return std::move(final_comps[0]);
    }
    return seed;
  }

  template <typename GetComps2D>
  bool checkAreaRule(odb::dbTechLayer* layer,
                     odb::dbTechLayerAreaRule* rule,
                     const GetComps2D& get_comps_2d,
                     const Node* cell,
                     const odb::Rect& cell_box,
                     const odb::Rect& touch_box,
                     const odb::Rect& query_box,
                     const bool strict_empty_sites) const
  {
    const int64_t min_area = rule->getArea();
    if (min_area <= 0) {
      return true;
    }
    if (rule->getTrimLayer() != nullptr || rule->getMask() != 0
        || rule->getExceptStep() != std::pair<int, int>(0, 0)) {
      return true;
    }

    const int site_w = grid_->getSiteWidth().v;
    const int site_count = grid_->getRowSiteCount().v;
    const int row_count = grid_->getRowCount().v;
    int gx_min = 0;
    int gx_max = 0;
    int gy_min = 0;
    int gy_max = 0;
    if (site_w > 0 && site_count > 0 && row_count > 0) {
      gx_min = std::clamp(grid_->gridX(DbuX(std::max(0, query_box.xMin()))).v,
                          0,
                          site_count - 1);
      gx_max
          = std::clamp(grid_->gridEndX(DbuX(std::max(0, query_box.xMax()))).v,
                       0,
                       site_count - 1);
      gy_min = std::clamp(
          grid_->gridSnapDownY(DbuY(std::max(0, query_box.yMin()))).v,
          0,
          row_count - 1);
      gy_max
          = std::clamp(grid_->gridEndY(DbuY(std::max(0, query_box.yMax()))).v,
                       0,
                       row_count - 1);
    }

    auto mayBeWindowClipped = [&](const odb::Rect& bbox) -> bool {
      if (site_w <= 0 || site_count <= 0 || row_count <= 0) {
        return false;
      }
      return (gx_min > 0 && bbox.xMin() <= (gx_min + 1) * site_w)
             || (gx_max + 1 < site_count && bbox.xMax() >= gx_max * site_w)
             || (gy_min > 0 && bbox.yMin() <= rowYHi(gy_min))
             || (gy_max + 1 < row_count && bbox.yMax() >= rowYLo(gy_max));
    };

    auto evalComponent = [&](const Component2D& c) -> bool {
      for (const BoostPolygon& poly : c.polys) {
        const int64_t poly_area = boost::polygon::area(poly);
        if (poly_area >= min_area || isPolygonExemptFromAreaRule(rule, poly)) {
          continue;
        }
        bool extendable = false;
        if (!strict_empty_sites) {
          for (const odb::Rect& r : c.rects) {
            if (canExtendHorizontally(
                    r, cell, cell_box, strict_empty_sites, 0, &c.rects)) {
              extendable = true;
              break;
            }
          }
        }
        if (!extendable) {
          return false;
        }
      }
      return true;
    };

    const odb::Rect active_box = strict_empty_sites ? cell_box : touch_box;
    const std::vector<Component2D>& comps_2d = get_comps_2d();
    for (const Component2D& comp : comps_2d) {
      if (!comp.bbox.overlaps(active_box)) {
        continue;
      }
      bool touches_active = false;
      for (const odb::Rect& r : comp.rects) {
        if (r.overlaps(active_box)) {
          touches_active = true;
          break;
        }
      }
      if (!touches_active) {
        continue;
      }

      if (mayBeWindowClipped(comp.bbox)) {
        bool all_polys_meet_min_area = true;
        for (const BoostPolygon& poly : comp.polys) {
          if (boost::polygon::area(poly) < min_area) {
            all_polys_meet_min_area = false;
            break;
          }
        }
        if (all_polys_meet_min_area) {
          continue;
        }
        bool satisfied_early = false;
        const Component2D full_comp = expandComponentOnGrid(
            layer, comp, cell, min_area, satisfied_early);
        if (satisfied_early || evalComponent(full_comp)) {
          continue;
        }
        return false;
      }
      if (evalComponent(comp)) {
        continue;
      }
      return false;
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
  int max_halo_{0};
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
