// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "rings.h"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "domain.h"
#include "grid.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "shape.h"
#include "techlayer.h"
#include "utl/Logger.h"

namespace pdn {

Rings::Rings(Grid* grid, const Layer& layer0, const Layer& layer1)
    : GridComponent(grid), layer0_(layer0), layer1_(layer1)
{
}

void Rings::checkLayerSpecifications() const
{
  for (const auto& layer : {layer0_, layer1_}) {
    checkLayerWidth(layer.layer, layer.width, layer.layer->getDirection());
    checkLayerSpacing(
        layer.layer, layer.width, layer.spacing, layer.layer->getDirection());
    const TechLayer techlayer(layer.layer);
    techlayer.checkIfManufacturingGrid(layer.width, getLogger(), "Width");
    techlayer.checkIfManufacturingGrid(layer.spacing, getLogger(), "Spacing");
    for (const int off :
         {offset_.left, offset_.bottom, offset_.right, offset_.top}) {
      techlayer.checkIfManufacturingGrid(off, getLogger(), "Core offset");
    }
  }

  checkDieArea();
}

void Rings::checkDieArea() const
{
  int hor_width;
  int ver_width;
  getTotalWidth(hor_width, ver_width);

  // The ring occupies everything between the inner outline and that outline
  // grown by the metal it carries.  On a polygon core the outline follows the
  // core, so this has to be tested as an area and not as a bounding box: a
  // ring that leaves a polygon die at the notch is still inside the bounding
  // box of that die, which is what the test used to compare against and why it
  // never fired there.
  //
  // The vertical layer's stacked metal is what grows X and the horizontal
  // layer's is what grows Y, which is how makeShapes builds the ring: a side
  // is swept outward by the width of its own layer, so the bottom and top --
  // the horizontal layer's sides -- add their thickness to Y.
  const Region ring_outline = getInnerRingOutline().bloat(
      {ver_width, hor_width, ver_width, hor_width});
  // Read straight from the block: Grid::getGridRegion() is the die for a core
  // grid but the macro for an instance grid, and a ring on either has to fit
  // inside the die.
  const Region die_area = Region(getBlock()->getDieAreaPolygon());

  if (die_area.isEmpty() || die_area.contains(ring_outline)) {
    return;
  }

  if (allow_outside_die_) {
    getLogger()->warn(
        utl::PDN, 239, "Ring shape falls outside the die bounds.");
    return;
  }

  const double dbus = getBlock()->getDbUnitsPerMicron();
  const auto [xbounds, ybounds] = getDieAreaDeficit(die_area);
  getLogger()->error(
      utl::PDN,
      351,
      "PDN rings do not fit inside the die area by {} um in X and {} um in "
      "Y. Either reduce the ring area or increase the core to die spacing "
      "to accommodate. Use -allow_out_of_die if this is intentional.",
      xbounds / dbus,
      ybounds / dbus);
}

std::pair<int, int> Rings::getDieAreaDeficit(const Region& die_area) const
{
  int hor_width;
  int ver_width;
  getTotalWidth(hor_width, ver_width);

  // How much the ring overruns the die, reported the way the user can act on
  // it: side by side, the room there is between the core and the die against
  // the room the ring needs there.  Measuring the residual area instead would
  // report the size of the overhang, which on a polygon die says more about
  // how long the offending side is than about how far past the edge it went.
  int xbounds = 0;
  int ybounds = 0;
  for (const auto& edge : getGrid()->getDomainRegion().getEdges()) {
    const bool horizontal = edge.isHorizontal();

    int needed = 0;
    if (horizontal) {
      needed = (edge.normal.y() > 0 ? offset_.top : offset_.bottom) + hor_width;
    } else {
      needed = (edge.normal.x() > 0 ? offset_.right : offset_.left) + ver_width;
    }

    const int deficit = needed - die_area.getMarginBeyond(edge);
    if (deficit <= 0) {
      continue;
    }
    if (horizontal) {
      ybounds = std::max(ybounds, deficit);
    } else {
      xbounds = std::max(xbounds, deficit);
    }
  }

  return {xbounds, ybounds};
}

void Rings::setOffset(const EdgeSpec& offset)
{
  offset_ = offset.transform(getGrid()->getOrientation());
}

void Rings::setPadOffset(const EdgeSpec& offset)
{
  odb::Rect die_area = getBlock()->getDieArea();
  odb::Rect core = getBlock()->getCoreArea();

  odb::Rect pads_inner = die_area;

  // look for placed pads
  for (auto* inst : getBlock()->getInsts()) {
    if (!inst->getPlacementStatus().isPlaced()) {
      continue;
    }

    auto type = inst->getMaster()->getType();
    // only looking for pads
    if (!type.isPad()) {
      continue;
    }

    if (type == odb::dbMasterType::PAD_AREAIO) {
      continue;
    }

    odb::Rect box = inst->getBBox()->getBox();

    const bool is_ns_with_core
        = box.xMin() >= core.xMin() && box.xMax() <= core.xMax();
    const bool is_ew_with_core
        = box.yMin() >= core.yMin() && box.yMax() <= core.yMax();
    const bool is_north = box.yMin() > core.yMax() && is_ns_with_core;
    const bool is_south = box.yMax() < core.yMin() && is_ns_with_core;
    const bool is_west = box.xMax() < core.xMin() && is_ew_with_core;
    const bool is_east = box.xMin() > core.xMax() && is_ew_with_core;

    // find the inner edge of the pad outline
    if (is_north) {
      pads_inner.set_yhi(std::min(pads_inner.yMax(), box.yMin()));
    } else if (is_south) {
      pads_inner.set_ylo(std::max(pads_inner.yMin(), box.yMax()));
    } else if (is_west) {
      pads_inner.set_xlo(std::max(pads_inner.xMin(), box.xMax()));
    } else if (is_east) {
      pads_inner.set_xhi(std::min(pads_inner.xMax(), box.xMin()));
    }
  }

  if (core == pads_inner) {
    getLogger()->warn(utl::PDN,
                      105,
                      "Unable to determine location of pad offset, using die "
                      "boundary instead.");
    pads_inner = getBlock()->getDieArea();
  }

  int hor_width;
  int ver_width;
  getTotalWidth(hor_width, ver_width);

  debugPrint(getLogger(),
             utl::PDN,
             "PadOffset",
             1,
             "Core area: {}",
             Shape::getRectText(core, getBlock()->getDbUnitsPerMicron()));
  debugPrint(getLogger(),
             utl::PDN,
             "PadOffset",
             1,
             "Pads inner: {}",
             Shape::getRectText(pads_inner, getBlock()->getDbUnitsPerMicron()));

  // this is already resolved against the placed pad ring, so it bypasses the
  // as-drawn remapping setOffset does (pads only ever sit on a core grid,
  // which is never flipped, but the intent should not depend on that)
  offset_ = {core.xMin() - pads_inner.xMin() - offset.left - ver_width,
             core.yMin() - pads_inner.yMin() - offset.bottom - hor_width,
             pads_inner.xMax() - core.xMax() - offset.right - ver_width,
             pads_inner.yMax() - core.yMax() - offset.top - hor_width};
}

void Rings::getTotalWidth(int& hor, int& ver) const
{
  const int rings = getNetCount();
  hor = layer0_.width * rings + layer0_.spacing * (rings - 1);
  ver = layer1_.width * rings + layer1_.spacing * (rings - 1);
  if (layer0_.layer->getDirection() != odb::dbTechLayerDir::HORIZONTAL) {
    std::swap(hor, ver);
  }
}

void Rings::setExtendToBoundary(bool value)
{
  extend_to_boundary_ = value;
}

Region Rings::getInnerRingOutline() const
{
  return getGrid()->getDomainRegion().bloat(
      {offset_.left, offset_.bottom, offset_.right, offset_.top});
}

odb::Rect Rings::getInnerRingRect() const
{
  return getInnerRingOutline().getEnclosingRect();
}

void Rings::makeShapes(const Shape::ShapeTreeMap& other_shapes)
{
  debugPrint(getLogger(),
             utl::PDN,
             "Make",
             1,
             "Ring start of make shapes on layers {} and {}",
             layer0_.layer->getName(),
             layer1_.layer->getName());
  clearShapes();

  auto* grid = getGrid();

  const auto nets = getNets();

  Region boundary;
  if (extend_to_boundary_) {
    boundary = grid->getGridBoundaryRegion();
  }

  const Region core = getInnerRingOutline();

  bool single_layer_ring = false;
  if (layer0_.layer == layer1_.layer) {
    single_layer_ring = true;
  }

  using LayerPair = std::pair<Layer*, Layer*>;
  const std::array<LayerPair, 2> build_layers{LayerPair{&layer0_, &layer1_},
                                              LayerPair{&layer1_, &layer0_}};

  bool processed_horizontal = false;
  for (const auto& [layer_def, layer_other] : build_layers) {
    auto* layer = layer_def->layer;
    const int width = layer_def->width;
    const int pitch = layer_def->spacing + width;

    const int other_width = layer_other->width;
    const int other_pitch = layer_other->spacing + other_width;

    const bool horizontal
        = (single_layer_ring && !processed_horizontal)
          || (!single_layer_ring
              && layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL);
    if (horizontal) {
      processed_horizontal = true;
    }

    // The ring of net i is the domain outline pushed out by i pitches: one
    // layer_def pitch along this layer's own direction of travel and one
    // layer_other pitch across it, which is what makes successive rings nest.
    std::vector<std::vector<Region::Edge>> ring_edges;
    ring_edges.reserve(nets.size());
    for (size_t index = 0; index < nets.size(); index++) {
      const int step = static_cast<int>(index) * pitch;
      const int other_step = static_cast<int>(index) * other_pitch;
      const Region::Margin margin
          = horizontal ? Region::Margin{other_step, step, other_step, step}
                       : Region::Margin{step, other_step, step, other_step};
      ring_edges.push_back(core.bloat(margin).getEdges());
    }

    // Sides are walked in the order the rectangular builder emitted them --
    // bottom then top for the horizontal layer, left then right for the
    // vertical one -- so that the shapes reach the database in the same order
    // on a rectangular floorplan as they always have.
    const std::array<odb::Point, 2> sides
        = horizontal
              ? std::array<odb::Point, 2>{odb::Point(0, -1), odb::Point(0, 1)}
              : std::array<odb::Point, 2>{odb::Point(-1, 0), odb::Point(1, 0)};

    for (const odb::Point& side : sides) {
      for (size_t index = 0; index < nets.size(); index++) {
        odb::dbNet* net = nets[index];

        for (const Region::Edge& edge : ring_edges[index]) {
          if (edge.normal != side) {
            continue;
          }

          const int xlo = std::min(edge.start.x(), edge.end.x());
          const int xhi = std::max(edge.start.x(), edge.end.x());
          const int ylo = std::min(edge.start.y(), edge.end.y());
          const int yhi = std::max(edge.start.y(), edge.end.y());

          // The edge is directed with the interior on its left, so which end
          // is the low one flips between opposite sides.
          const bool start_is_low = horizontal ? edge.start.x() < edge.end.x()
                                               : edge.start.y() < edge.end.y();
          const bool convex_low
              = start_is_low ? edge.convex_at_start : edge.convex_at_end;
          const bool convex_high
              = start_is_low ? edge.convex_at_end : edge.convex_at_start;

          // Sweep the edge outward by the width of this layer.
          odb::Rect rect;
          if (horizontal) {
            const int base = side.y() > 0 ? yhi : ylo - width;
            rect = odb::Rect(xlo, base, xhi, base + width);
          } else {
            const int base = side.x() > 0 ? xhi : xlo - width;
            rect = odb::Rect(base, ylo, base + width, yhi);
          }

          // Then run it out along its own direction far enough to meet the
          // sides that join it.  At a convex corner the neighbour turns away,
          // so the two only overlap if this one reaches across the other
          // layer's width; at a concave corner the neighbour turns towards it
          // and the two already share that square, so extending would only
          // push metal back over the domain.
          const odb::Point low_normal
              = horizontal ? odb::Point(-1, 0) : odb::Point(0, -1);
          const odb::Point high_normal
              = horizontal ? odb::Point(1, 0) : odb::Point(0, 1);
          int extend_low = convex_low ? other_width : 0;
          int extend_high = convex_high ? other_width : 0;
          if (extend_to_boundary_) {
            extend_low = boundary.getMarginBeyond(rect, low_normal);
            extend_high = boundary.getMarginBeyond(rect, high_normal);
          }
          if (horizontal) {
            rect.set_xlo(rect.xMin() - extend_low);
            rect.set_xhi(rect.xMax() + extend_high);
          } else {
            rect.set_ylo(rect.yMin() - extend_low);
            rect.set_yhi(rect.yMax() + extend_high);
          }

          addShape(std::make_unique<Shape>(
              layer, net, rect, odb::dbWireShapeType::RING));
        }
      }
    }
  }

  if (single_layer_ring) {
    for (const auto& [layer, shapes] : getShapes()) {
      for (const auto& shape : shapes) {
        shape->setLocked();
      }
    }
  }
}

std::vector<odb::dbTechLayer*> Rings::getLayers() const
{
  std::vector<odb::dbTechLayer*> layers;
  layers.reserve(2);
  for (const auto& layer_def : {layer0_, layer1_}) {
    layers.push_back(layer_def.layer);
  }
  return layers;
}

void Rings::report() const
{
  auto* logger = getLogger();

  const double dbu_per_micron = getBlock()->getDbUnitsPerMicron();

  logger->report("  Core offset:");
  logger->report("    Left: {:.4f}", offset_.left / dbu_per_micron);
  logger->report("    Bottom: {:.4f}", offset_.bottom / dbu_per_micron);
  logger->report("    Right: {:.4f}", offset_.right / dbu_per_micron);
  logger->report("    Top: {:.4f}", offset_.top / dbu_per_micron);

  for (const auto& layer : {layer0_, layer1_}) {
    logger->report("  Layer: {}", layer.layer->getName());
    logger->report("    Width: {:.4f}", layer.width / dbu_per_micron);
    logger->report("    Spacing: {:.4f}", layer.spacing / dbu_per_micron);
  }
}

}  // namespace pdn
