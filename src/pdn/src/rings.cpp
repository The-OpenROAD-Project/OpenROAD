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
    for (const auto& off : offset_) {
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

  odb::Rect ring_outline = getInnerRingOutline();
  ring_outline.set_xlo(ring_outline.xMin() - hor_width);
  ring_outline.set_xhi(ring_outline.xMax() + hor_width);
  ring_outline.set_ylo(ring_outline.yMin() - ver_width);
  ring_outline.set_yhi(ring_outline.yMax() + ver_width);

  const odb::Rect die_area = getBlock()->getDieArea();

  if (!die_area.contains(ring_outline)) {
    if (allow_outside_die_) {
      getLogger()->warn(
          utl::PDN, 239, "Ring shape falls outside the die bounds.");
    } else {
      const double dbus = getBlock()->getDbUnitsPerMicron();

      int xbounds = std::max(die_area.xMin() - ring_outline.xMin(),
                             ring_outline.xMax() - die_area.xMax());
      xbounds = std::max(xbounds, 0);
      int ybounds = std::max(die_area.yMin() - ring_outline.yMin(),
                             ring_outline.yMax() - die_area.yMax());
      ybounds = std::max(ybounds, 0);
      getLogger()->error(
          utl::PDN,
          351,
          "PDN rings do not fit inside the die area by {} um in X and {} um in "
          "Y. Either reduce the ring area or increase the core to die spacing "
          "to accommodate. Use -allow_out_of_die if this is intentional.",
          xbounds / dbus,
          ybounds / dbus);
    }
  }
}

void Rings::setOffset(const std::array<int, 4>& offset)
{
  offset_ = offset;
}

void Rings::setPadOffset(const std::array<int, 4>& offset)
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

  std::array<int, 4> core_offset{
      core.xMin() - pads_inner.xMin() - offset[0] - ver_width,
      core.yMin() - pads_inner.yMin() - offset[1] - hor_width,
      pads_inner.xMax() - core.xMax() - offset[2] - ver_width,
      pads_inner.yMax() - core.yMax() - offset[3] - hor_width};

  // apply pad offset as core offset
  setOffset(core_offset);
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

odb::Rect Rings::getInnerRingOutline() const
{
  auto* grid = getGrid();
  odb::Rect core = grid->getDomainArea();
  core.set_xlo(core.xMin() - offset_[0]);
  core.set_ylo(core.yMin() - offset_[1]);
  core.set_xhi(core.xMax() + offset_[2]);
  core.set_yhi(core.yMax() + offset_[3]);

  return core;
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

  odb::Rect boundary;
  if (extend_to_boundary_) {
    boundary = grid->getGridBoundary();
  }

  const odb::Rect core = getInnerRingOutline();

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
    if ((single_layer_ring && !processed_horizontal)
        || (!single_layer_ring
            && layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL)) {
      processed_horizontal = true;

      // bottom
      int x_start = core.xMin() - other_width;
      int x_end = core.xMax() + other_width;
      if (extend_to_boundary_) {
        x_start = boundary.xMin();
        x_end = boundary.xMax();
      }
      int y_start = core.yMin() - width;
      int y_end = core.yMin();
      for (auto net : nets) {
        addShape(
            std::make_unique<Shape>(layer,
                                    net,
                                    odb::Rect(x_start, y_start, x_end, y_end),
                                    odb::dbWireShapeType::RING));
        if (!extend_to_boundary_) {
          x_start -= other_pitch;
          x_end += other_pitch;
        }
        y_start -= pitch;
        y_end -= pitch;
      }
      // top
      if (!extend_to_boundary_) {
        x_start = core.xMin() - other_width;
        x_end = core.xMax() + other_width;
      }
      y_start = core.yMax();
      y_end = y_start + width;
      for (auto net : nets) {
        addShape(
            std::make_unique<Shape>(layer,
                                    net,
                                    odb::Rect(x_start, y_start, x_end, y_end),
                                    odb::dbWireShapeType::RING));
        if (!extend_to_boundary_) {
          x_start -= other_pitch;
          x_end += other_pitch;
        }
        y_start += pitch;
        y_end += pitch;
      }
    } else {
      // left
      int x_start = core.xMin() - width;
      int x_end = core.xMin();
      int y_start = core.yMin() - other_width;
      int y_end = core.yMax() + other_width;
      if (extend_to_boundary_) {
        y_start = boundary.yMin();
        y_end = boundary.yMax();
      }
      for (auto net : nets) {
        addShape(
            std::make_unique<Shape>(layer,
                                    net,
                                    odb::Rect(x_start, y_start, x_end, y_end),
                                    odb::dbWireShapeType::RING));
        x_start -= pitch;
        x_end -= pitch;
        if (!extend_to_boundary_) {
          y_start -= other_pitch;
          y_end += other_pitch;
        }
      }
      // right
      x_start = core.xMax();
      x_end = x_start + width;
      if (!extend_to_boundary_) {
        y_start = core.yMin() - other_width;
        y_end = core.yMax() + other_width;
      }
      for (auto net : nets) {
        addShape(
            std::make_unique<Shape>(layer,
                                    net,
                                    odb::Rect(x_start, y_start, x_end, y_end),
                                    odb::dbWireShapeType::RING));
        x_start += pitch;
        x_end += pitch;
        if (!extend_to_boundary_) {
          y_start -= other_pitch;
          y_end += other_pitch;
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
  logger->report("    Left: {:.4f}", offset_[0] / dbu_per_micron);
  logger->report("    Bottom: {:.4f}", offset_[1] / dbu_per_micron);
  logger->report("    Right: {:.4f}", offset_[2] / dbu_per_micron);
  logger->report("    Top: {:.4f}", offset_[3] / dbu_per_micron);

  for (const auto& layer : {layer0_, layer1_}) {
    logger->report("  Layer: {}", layer.layer->getName());
    logger->report("    Width: {:.4f}", layer.width / dbu_per_micron);
    logger->report("    Spacing: {:.4f}", layer.spacing / dbu_per_micron);
  }
}

}  // namespace pdn
