// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/BlockShapes.h"

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "SpatialIndex.h"
#include "fsv/Geometry.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace fsv {

namespace {

bool isConductor(odb::dbTechLayer* layer)
{
  return layer != nullptr
         && (layer->getType() == odb::dbTechLayerType::ROUTING
             || layer->getType() == odb::dbTechLayerType::CUT);
}

void add(std::vector<BlockShape>& shapes,
         odb::dbTechLayer* layer,
         const odb::Rect& rect,
         const BlockShape& origin)
{
  if (isConductor(layer) && rect.dx() > 0 && rect.dy() > 0) {
    BlockShape shape = origin;
    shape.layer = layer;
    shape.rect = rect;
    shapes.push_back(shape);
  }
}

void addVia(std::vector<BlockShape>& shapes,
            const std::vector<odb::dbShape>& boxes,
            const BlockShape& origin,
            const odb::dbTransform* transform = nullptr)
{
  for (const odb::dbShape& box : boxes) {
    odb::Rect rect = box.getBox();
    if (transform != nullptr) {
      transform->apply(rect);
    }
    add(shapes, box.getTechLayer(), rect, origin);
  }
}

// A pin or obstruction box, placed by `transform` (null: as it is). A via
// expands to the boxes of its layers; dbITerm::getGeometries would give
// only its bounding box, without a layer.
void addBox(std::vector<BlockShape>& shapes,
            odb::dbBox* box,
            const BlockShape& origin,
            const odb::dbTransform* transform)
{
  if (box->isVia()) {
    std::vector<odb::dbShape> boxes;
    box->getViaBoxes(boxes);
    addVia(shapes, boxes, origin, transform);
    return;
  }
  odb::Rect rect = box->getBox();
  if (transform != nullptr) {
    transform->apply(rect);
  }
  add(shapes, box->getTechLayer(), rect, origin);
}

// An instance without a placement sits at the origin, where its shapes
// would be phantom conductors.
void checkPlaced(odb::dbInst* inst)
{
  if (!inst->isPlaced()) {
    throw std::invalid_argument("fsv: instance " + inst->getName()
                                + " is not placed");
  }
}

// The shapes of an instance pin: the boxes of its cell's pin, placed.
void addPin(std::vector<BlockShape>& shapes,
            odb::dbITerm* iterm,
            const BlockShape::Source source)
{
  checkPlaced(iterm->getInst());
  const odb::dbTransform transform = iterm->getInst()->getTransform();
  BlockShape origin{.layer = nullptr, .rect = {}, .source = source};
  origin.iterm = iterm;
  for (odb::dbMPin* mpin : iterm->getMTerm()->getMPins()) {
    for (odb::dbBox* box : mpin->getGeometry()) {
      addBox(shapes, box, origin, &transform);
    }
  }
}

}  // namespace

std::vector<BlockShape> netShapes(odb::dbNet* net)
{
  std::vector<BlockShape> shapes;
  if (odb::dbWire* wire = net->getWire()) {
    odb::dbWireShapeItr it;
    odb::dbShape shape;
    for (it.begin(wire); it.next(shape);) {
      BlockShape origin{
          .layer = nullptr,
          .rect = {},
          .source = BlockShape::Source::kWire,
      };
      origin.shape_id = it.getShapeId();
      if (shape.isVia()) {
        std::vector<odb::dbShape> boxes;
        odb::dbShape::getViaBoxes(shape, boxes);
        addVia(shapes, boxes, origin);
      } else {
        add(shapes, shape.getTechLayer(), shape.getBox(), origin);
      }
    }
  }
  const BlockShape special{
      .layer = nullptr,
      .rect = {},
      .source = BlockShape::Source::kSpecialWire,
  };
  for (odb::dbSWire* swire : net->getSWires()) {
    for (odb::dbSBox* box : swire->getWires()) {
      if (box->getDirection() == odb::dbSBox::OCTILINEAR) {
        // Its box would be the bounding box, which overstates it.
        const double dbu = net->getBlock()->getDbUnitsPerMicron();
        char where[64];
        std::snprintf(where,
                      sizeof(where),
                      "(%g, %g)",
                      box->xMin() / dbu,
                      box->yMin() / dbu);
        throw std::invalid_argument("fsv: net " + net->getName()
                                    + " has a non-Manhattan special wire at "
                                    + where + " um");
      }
      if (box->isVia()) {
        std::vector<odb::dbShape> boxes;
        box->getViaBoxes(boxes);
        addVia(shapes, boxes, special);
      } else {
        add(shapes, box->getTechLayer(), box->getBox(), special);
      }
    }
  }
  for (odb::dbITerm* iterm : net->getITerms()) {
    addPin(shapes, iterm, BlockShape::Source::kITermPin);
  }
  for (odb::dbBTerm* bterm : net->getBTerms()) {
    BlockShape origin{
        .layer = nullptr,
        .rect = {},
        .source = BlockShape::Source::kBTermPin,
    };
    origin.bterm = bterm;
    for (odb::dbBPin* pin : bterm->getBPins()) {
      if (!pin->getPlacementStatus().isPlaced()) {
        throw std::invalid_argument("fsv: pin " + bterm->getName() + " of net "
                                    + net->getName() + " is not placed");
      }
      for (odb::dbBox* box : pin->getBoxes()) {
        addBox(shapes, box, origin, nullptr);
      }
    }
  }
  return shapes;
}

std::vector<BlockShape> unconnectedShapes(odb::dbBlock* block)
{
  std::vector<BlockShape> shapes;
  const BlockShape obstruction{
      .layer = nullptr,
      .rect = {},
      .source = BlockShape::Source::kObstruction,
  };
  for (odb::dbInst* inst : block->getInsts()) {
    checkPlaced(inst);
    const odb::dbTransform transform = inst->getTransform();
    for (odb::dbBox* box : inst->getMaster()->getObstructions()) {
      addBox(shapes, box, obstruction, &transform);
    }
    for (odb::dbITerm* iterm : inst->getITerms()) {
      if (iterm->getNet() == nullptr) {
        addPin(shapes, iterm, BlockShape::Source::kUnconnectedPin);
      }
    }
  }
  const BlockShape fill{
      .layer = nullptr,
      .rect = {},
      .source = BlockShape::Source::kFill,
  };
  for (odb::dbFill* f : block->getFills()) {
    odb::Rect rect;
    f->getRect(rect);
    add(shapes, f->getTechLayer(), rect, fill);
  }
  return shapes;
}

Obstruction obstruction(const BlockShape& shape,
                        const Box& box,
                        const double dbu)
{
  const double width = shape.layer->getWidth() / dbu;
  const double spacing = shape.layer->getSpacing() / dbu;
  return {
      .box = box,
      .spacing = spacing > 0 ? spacing : std::max(width, 1 / dbu),
      .min_width = width,
  };
}

namespace {

// The parts of rect outside cut, in x-y (z is kept).
void subtract(const Box& rect, const Box& cut, std::vector<Box>& out)
{
  for (int k = 0; k < 2; k++) {
    if (rect.lo[k] >= cut.hi[k] || cut.lo[k] >= rect.hi[k]) {
      out.push_back(rect);
      return;
    }
  }
  Box rest = rect;
  // Slabs beside cut along x, then along y within its x extent.
  for (int k = 0; k < 2; k++) {
    if (rest.lo[k] < cut.lo[k]) {
      Box slab = rest;
      slab.hi[k] = cut.lo[k];
      out.push_back(slab);
      rest.lo[k] = cut.lo[k];
    }
    if (rest.hi[k] > cut.hi[k]) {
      Box slab = rest;
      slab.lo[k] = cut.hi[k];
      out.push_back(slab);
      rest.hi[k] = cut.hi[k];
    }
  }
}

}  // namespace

std::vector<Box> trimObstructions(const std::vector<Box>& net_boxes,
                                  const std::vector<Obstruction>& obstructions,
                                  int& trimmed)
{
  const SpatialIndex<3> index(net_boxes);
  std::vector<Box> result;
  std::vector<int> cutters;
  for (const Obstruction& o : obstructions) {
    Box reach = o.box;
    for (int k = 0; k < 2; k++) {
      reach.lo[k] -= o.spacing;
      reach.hi[k] += o.spacing;
    }
    // On the same layer: the same z extent.
    cutters.clear();
    index.forEachOverlapping(reach, [&](const int i) {
      if (net_boxes[i].lo[2] == o.box.lo[2]
          && net_boxes[i].hi[2] == o.box.hi[2]) {
        cutters.push_back(i);
      }
    });
    if (cutters.empty()) {
      result.push_back(o.box);
      continue;
    }
    std::ranges::sort(cutters);
    std::vector<Box> pieces{o.box};
    std::vector<Box> next;
    for (const int i : cutters) {
      Box cut = net_boxes[i];
      for (int k = 0; k < 2; k++) {
        cut.lo[k] -= o.spacing;
        cut.hi[k] += o.spacing;
      }
      next.clear();
      for (const Box& piece : pieces) {
        subtract(piece, cut, next);
      }
      pieces.swap(next);
    }
    if (pieces.size() == 1 && pieces[0].lo == o.box.lo
        && pieces[0].hi == o.box.hi) {
      result.push_back(o.box);
      continue;
    }
    trimmed++;
    // Only the pieces a cut left are checked against the width (within
    // rounding); the obstruction itself is kept as drawn.
    constexpr double kTolerance = 1e-9;
    for (const Box& piece : pieces) {
      if (std::min(piece.hi[0] - piece.lo[0], piece.hi[1] - piece.lo[1])
          >= o.min_width - kTolerance) {
        result.push_back(piece);
      }
    }
  }
  return result;
}

}  // namespace fsv
