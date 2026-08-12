#include "PlacementDRC.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "boost/functional/hash.hpp"
#include "boost/geometry/index/predicates.hpp"
#include "boost/geometry/index/rtree.hpp"
#include "dpl/Opendp.h"
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
  cell->getDbInst()->getMaster()->getPlacementBoundary(bbox);
  odb::dbTransform transform(orient);
  transform.apply(bbox);
  odb::Point offset(x.v - bbox.xMin(), y.v - bbox.yMin());
  transform.setOffset(offset);
  odb::Rect result(edge_rect);
  transform.apply(result);
  return result;
}
odb::Rect getQueryRect(const odb::Rect& edge_box, const int spc)
{
  odb::Rect query_rect(edge_box);
  bool is_vertical_edge = edge_box.getDir() == odb::vertical;
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

    // Fixed supply via constituent layers are the only master geometry that
    // can affect placement.  Avoid collecting master geometry when there are
    // no fixed supply via shapes.
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

    // Do not retain fixed layers for which no instantiated master has
    // geometry.  They cannot contribute a placement conflict.
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
    // Standard cells can only be placed at legal row origins. For rows without
    // right-angle rotations, merge cell geometry vertically per physical row
    // to prove when no via can conflict.
    const LayerVerticalRanges cell_ranges
        = has_right_angle_row ? LayerVerticalRanges{}
                              : cellVerticalRanges(masters, block);
    size_t vertically_safe_shape_count = 0;
    for (auto& [layer, data] : layers_) {
      const int max_spacing = maximumSpacing(layer, data);
      data.query_halo = std::max(0, max_spacing - 1);
      if (max_spacing == 0
          || (!has_right_angle_row
              && !hasPotentialVerticalConflict(data, cell_ranges.at(layer)))) {
        vertically_safe_shape_count += data.shapes.size();
      }
    }
    if (vertically_safe_shape_count == fixedShapeCount()) {
      legal_sites_safe_ = true;
      if (logger_->debugCheck(DPL, "fixed_supply_via", 1)) {
        debugPrint(logger_,
                   DPL,
                   "fixed_supply_via",
                   1,
                   "skipped fixed supply via checks at legal sites for {} "
                   "shapes that cannot violate spacing",
                   vertically_safe_shape_count);
      }
      return;
    }

    size_t fixed_shape_count = 0;
    int max_query_halo = 0;
    buildIndexes();
    for (const auto& [layer, data] : layers_) {
      fixed_shape_count += data.shapes.size();
      max_query_halo = std::max(max_query_halo, data.query_halo);
    }

    if (logger_->debugCheck(DPL, "fixed_supply_via", 1)) {
      debugPrint(
          logger_,
          DPL,
          "fixed_supply_via",
          1,
          "indexed {} fixed supply via shapes on {} master-used layers from {} "
          "instantiated masters; maximum query halo={}",
          fixed_shape_count,
          layers_.size(),
          masters.size(),
          max_query_halo);
    }
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
    // Avoid scanning terminal entries for pages whose terminal shapes do not
    // conflict at the queried site.
    if ((recipe.conditional_mask & query_mask) == 0) {
      return true;
    }

    for (const RecipeEntry& entry : recipe.entries) {
      if ((entry.mask & query_mask) != 0
          && cell->getDbInst()->getITerm(entry.term)->getNet() != entry.net) {
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
      if ((lhs.term == nullptr) != (rhs.term == nullptr)) {
        return lhs.term == nullptr;
      }
      if (lhs.term != nullptr && lhs.term->getId() != rhs.term->getId()) {
        return lhs.term->getId() < rhs.term->getId();
      }
      if ((lhs.net == nullptr) != (rhs.net == nullptr)) {
        return lhs.net == nullptr;
      }
      return lhs.net != nullptr && lhs.net->getId() < rhs.net->getId();
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
    // Distances are integer DBU values and spacing is strict.  Subtracting
    // one makes the square-root bound exact without floating-point rounding.
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
    for (odb::dbMaster* master : masters) {
      const RowOrientations row_orientations
          = rowOrientations(block, master->getSite());
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
    odb::Rect query = bloatClipped(swept, halo);
    // divFloor() historically truncates a negative right endpoint toward zero.
    // Include that possible site-zero interval in the spatial candidate query;
    // the exact span calculation below still decides the mask.
    query.set_xlo(clipToInt(static_cast<int64_t>(query.xMin())
                            - std::max(0, site_width - 1)));
    return query;
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

    // Routing and cut spacing use strict Euclidean distance in integer DBU.
    const int64_t origin_y
        = static_cast<int64_t>(core.yMin()) + grid_->gridYToDbu(GridY{row}).v;
    const int64_t vertical_gap
        = verticalDistance(fixed.rect, cell_shape.rect, origin_y);
    const int64_t x_keepout = horizontalKeepout(space, vertical_gap);
    if (x_keepout < 0) {
      return false;
    }
    const int low = clipToInt(static_cast<int64_t>(fixed.rect.xMin())
                              - x_keepout - cell_shape.rect.xMax());
    const int high = clipToInt(static_cast<int64_t>(fixed.rect.xMax())
                               + x_keepout - cell_shape.rect.xMin());
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
    // Fixed supply via constituent checks cover width/PRL routing spacing and
    // default cut spacing. Detailed routing remains authoritative for other
    // LEF58 rules.
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
  // Placement DRC currently calls check() from one thread. These lazy caches
  // intentionally rely on that contract and need synchronization if it changes.
  bool legal_sites_safe_{false};
  std::unordered_map<PhysicalPageKey, PageRecipe, boost::hash<PhysicalPageKey>>
      physical_pages_;
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
    bool is_vertical_edge = edge1_box.getDir() == odb::vertical;
    odb::Rect query_rect = cell_edges::getQueryRect(edge1_box, max_spc);
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
        if (checked_cells.find(cell2) != checked_cells.end()) {
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
          if (edge1_box.getDir() != edge2_box.getDir()) {
            // Skip if edges are not parallel.
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
  return checkBlockedLayers(cell, grid_->gridX(cell), grid_->gridRoundY(cell));
}

bool PlacementDRC::checkBlockedLayers(const Node* cell,
                                      const GridX x,
                                      const GridY y) const
{
  const GridX x_begin = x;
  const GridY y_begin = y;
  const GridX x_end = x + grid_->gridWidth(cell);
  const GridY y_end = grid_->gridEndY(grid_->gridYToDbu(y) + cell->getHeight());
  for (GridY y1 = y_begin; y1 < y_end; y1++) {
    for (GridX x1 = x_begin; x1 < x_end; x1++) {
      const Pixel* pixel = grid_->gridPixel(x1, y1);
      if (pixel != nullptr && pixel->blocked_layers & cell->getUsedLayers()) {
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
    return checkBlockedLayers(cell, x, y) && checkOneSiteGap(cell, x, y)
           && checkPadding(cell, x, y) && checkEdgeSpacing(cell, x, y, orient)
           && checkFixedSupplyVias(cell, x, y, orient);
  }

  // Debug path: evaluate every check so the report shows each one.
  const bool edge_ok = checkEdgeSpacing(cell, x, y, orient);
  const bool padding_ok = checkPadding(cell, x, y);
  const bool blocked_ok = checkBlockedLayers(cell, x, y);
  const bool supply_vias_ok = checkFixedSupplyVias(cell, x, y, orient);
  const bool gap_ok = checkOneSiteGap(cell, x, y);

  const bool all_ok
      = edge_ok && padding_ok && blocked_ok && supply_vias_ok && gap_ok;

  if (!all_ok) {
    const std::string cell_name = cell->name();
    debugPrint(logger_,
               DPL,
               "checkDRC",
               1,
               "cell {} at ({}, {}): ok?={} edge={} padding={} blocked={} "
               "supply_vias={} gap={}",
               cell_name,
               x.v,
               y.v,
               all_ok ? 1 : 0,
               edge_ok ? 1 : 0,
               padding_ok ? 1 : 0,
               blocked_ok ? 1 : 0,
               supply_vias_ok ? 1 : 0,
               gap_ok ? 1 : 0);
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
  if (!checkBlockedLayers(cell, x, y)) {
    ++count;
  }
  if (!checkOneSiteGap(cell, x, y)) {
    ++count;
  }
  if (include_fixed_supply_vias && !checkFixedSupplyVias(cell, x, y, orient)) {
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
  return checkPadding(cell, grid_->gridX(cell), grid_->gridRoundY(cell));
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
  const GridX cell_x_end = x + grid_->gridWidth(cell);
  const GridY cell_y_end
      = grid_->gridEndY(grid_->gridYToDbu(y) + cell->getHeight());

  // Get the cell's padding requirements
  const GridX left_pad = padding_->padLeft(cell);
  const GridX right_pad = padding_->padRight(cell);
  for (GridX grid_x{x - left_pad}; grid_x < cell_x_end + right_pad; grid_x++) {
    for (GridY grid_y{y}; grid_y < cell_y_end; grid_y++) {
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
