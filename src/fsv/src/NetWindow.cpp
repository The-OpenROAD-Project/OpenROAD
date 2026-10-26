// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/NetWindow.h"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
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

// A shape in database units.
struct Shape
{
  odb::dbTechLayer* layer;
  odb::Rect rect;
};

bool isConductor(odb::dbTechLayer* layer)
{
  return layer != nullptr
         && (layer->getType() == odb::dbTechLayerType::ROUTING
             || layer->getType() == odb::dbTechLayerType::CUT);
}

void add(std::vector<Shape>& shapes,
         odb::dbTechLayer* layer,
         const odb::Rect& rect)
{
  if (isConductor(layer) && rect.dx() > 0 && rect.dy() > 0) {
    shapes.push_back({layer, rect});
  }
}

void addVia(std::vector<Shape>& shapes,
            const std::vector<odb::dbShape>& boxes,
            const odb::dbTransform* transform = nullptr)
{
  for (const odb::dbShape& box : boxes) {
    odb::Rect rect = box.getBox();
    if (transform != nullptr) {
      transform->apply(rect);
    }
    add(shapes, box.getTechLayer(), rect);
  }
}

// A pin or obstruction box, placed by `transform` (null: as it is). A via
// expands to the boxes of its layers; dbITerm::getGeometries would give
// only its bounding box, without a layer.
void addBox(std::vector<Shape>& shapes,
            odb::dbBox* box,
            const odb::dbTransform* transform)
{
  if (box->isVia()) {
    std::vector<odb::dbShape> boxes;
    box->getViaBoxes(boxes);
    addVia(shapes, boxes, transform);
    return;
  }
  odb::Rect rect = box->getBox();
  if (transform != nullptr) {
    transform->apply(rect);
  }
  add(shapes, box->getTechLayer(), rect);
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
void addPin(std::vector<Shape>& shapes, odb::dbITerm* iterm)
{
  checkPlaced(iterm->getInst());
  const odb::dbTransform transform = iterm->getInst()->getTransform();
  for (odb::dbMPin* mpin : iterm->getMTerm()->getMPins()) {
    for (odb::dbBox* box : mpin->getGeometry()) {
      addBox(shapes, box, &transform);
    }
  }
}

// Every shape of a net: its routed wires and vias, special wires, and the
// pins of its instance and block terminals.
std::vector<Shape> netShapes(odb::dbNet* net)
{
  std::vector<Shape> shapes;
  if (odb::dbWire* wire = net->getWire()) {
    odb::dbWireShapeItr it;
    odb::dbShape shape;
    for (it.begin(wire); it.next(shape);) {
      if (shape.isVia()) {
        std::vector<odb::dbShape> boxes;
        odb::dbShape::getViaBoxes(shape, boxes);
        addVia(shapes, boxes);
      } else {
        add(shapes, shape.getTechLayer(), shape.getBox());
      }
    }
  }
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
        addVia(shapes, boxes);
      } else {
        add(shapes, box->getTechLayer(), box->getBox());
      }
    }
  }
  for (odb::dbITerm* iterm : net->getITerms()) {
    addPin(shapes, iterm);
  }
  for (odb::dbBTerm* bterm : net->getBTerms()) {
    for (odb::dbBPin* pin : bterm->getBPins()) {
      if (!pin->getPlacementStatus().isPlaced()) {
        throw std::invalid_argument("fsv: pin " + bterm->getName() + " of net "
                                    + net->getName() + " is not placed");
      }
      for (odb::dbBox* box : pin->getBoxes()) {
        addBox(shapes, box, nullptr);
      }
    }
  }
  return shapes;
}

// The shapes of no net: cell obstructions and pins left unconnected.
std::vector<Shape> unconnectedShapes(odb::dbBlock* block)
{
  std::vector<Shape> shapes;
  for (odb::dbInst* inst : block->getInsts()) {
    checkPlaced(inst);
    const odb::dbTransform transform = inst->getTransform();
    for (odb::dbBox* box : inst->getMaster()->getObstructions()) {
      addBox(shapes, box, &transform);
    }
    for (odb::dbITerm* iterm : inst->getITerms()) {
      if (iterm->getNet() == nullptr) {
        addPin(shapes, iterm);
      }
    }
  }
  return shapes;
}

// An obstruction or unconnected pin box, which joins the grounded net once
// it is trimmed clear of every net. Distances are in µm.
struct Obstruction
{
  Box box;
  double spacing;    // the layer's minimum spacing (or width)
  double min_width;  // pieces narrower than this are not metal
};

// The parts of rect outside cut, in x-y (z is kept).
void subtract(const Box& rect, const Box& cut, std::vector<Box>& out)
{
  for (int k = 0; k < 2; k++) {
    if (!(rect.lo[k] < cut.hi[k] && cut.lo[k] < rect.hi[k])) {
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

// Obstructions trimmed clear of the nets' shapes: a LEF obstruction is often
// a routing blockage over a whole macro, its own pins included. Each one
// loses, on its layer, every net shape grown by the layer's spacing, which
// is as close as real metal could come; leftovers narrower than the layer's
// width are dropped. Counts the obstructions that changed.
std::vector<Box> trimObstructions(const Geometry& geometry,
                                  const std::vector<Obstruction>& obstructions,
                                  int& trimmed)
{
  const std::vector<Box>& boxes = geometry.boxes();
  const SpatialIndex<3> index(boxes);
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
      if (boxes[i].lo[2] == o.box.lo[2] && boxes[i].hi[2] == o.box.hi[2]) {
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
      Box cut = boxes[i];
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

// Shapes of different nets may touch but not overlap.
void checkShorts(const Geometry& geometry)
{
  const std::vector<Box>& boxes = geometry.boxes();
  const SpatialIndex<3> index(boxes);
  for (int i = 0; i < static_cast<int>(boxes.size()); i++) {
    index.forEachOverlapping(boxes[i], [&](const int j) {
      if (j <= i || geometry.boxNet(i) == geometry.boxNet(j)) {
        return;
      }
      for (int k = 0; k < 3; k++) {
        if (!(boxes[i].lo[k] < boxes[j].hi[k]
              && boxes[j].lo[k] < boxes[i].hi[k])) {
          return;
        }
      }
      char where[96];
      std::snprintf(where,
                    sizeof(where),
                    "(%g, %g, %g)",
                    std::max(boxes[i].lo[0], boxes[j].lo[0]),
                    std::max(boxes[i].lo[1], boxes[j].lo[1]),
                    std::max(boxes[i].lo[2], boxes[j].lo[2]));
      throw std::invalid_argument(
          "fsv: shapes of nets " + geometry.netName(geometry.boxNet(i))
          + " and " + geometry.netName(geometry.boxNet(j)) + " overlap at "
          + where + " um");
    });
  }
}

}  // namespace

NetWindow readNetWindow(odb::dbBlock* block,
                        const std::vector<odb::dbNet*>& targets,
                        const LayerExtent& layer_extent,
                        const NetWindowOptions& options)
{
  if (targets.empty()) {
    throw std::invalid_argument("fsv: no target nets");
  }
  const double dbu = block->getDbUnitsPerMicron();
  // By layer name: odb objects have no ordering.
  std::map<std::string, std::pair<double, double>> extents;
  auto toBox = [&](const Shape& shape) {
    const std::string name = shape.layer->getName();
    auto it = extents.find(name);
    if (it == extents.end()) {
      it = extents.emplace(name, layer_extent(name)).first;
    }
    return Box{
        {shape.rect.xMin() / dbu, shape.rect.yMin() / dbu, it->second.first},
        {shape.rect.xMax() / dbu, shape.rect.yMax() / dbu, it->second.second}};
  };

  NetWindow result;
  Geometry& geometry = result.geometry;
  // The targets, whole.
  AlignedBox<2> bounds{
      {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
      {std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest()}};
  std::set<int> target_ids;
  for (odb::dbNet* net : targets) {
    if (!target_ids.insert(net->getId()).second) {
      continue;  // named twice
    }
    const std::vector<Shape> shapes = netShapes(net);
    if (shapes.empty()) {
      throw std::invalid_argument("fsv: net " + net->getName()
                                  + " has no shapes on conductor layers");
    }
    const int index = geometry.addNet(net->getName());
    result.targets.push_back(index);
    for (const Shape& shape : shapes) {
      const Box box = toBox(shape);
      geometry.addBox(index, box);
      for (int k = 0; k < 2; k++) {
        bounds.lo[k] = std::min(bounds.lo[k], box.lo[k]);
        bounds.hi[k] = std::max(bounds.hi[k], box.hi[k]);
      }
      result.height = std::max(result.height, box.hi[2]);
    }
  }
  result.halo = options.halo >= 0 ? options.halo : 3 * result.height;
  result.margin = options.margin >= 0 ? options.margin : 0.5 * result.height;
  AlignedBox<2> window = bounds;
  AlignedBox<2> clip = bounds;
  for (int k = 0; k < 2; k++) {
    window.lo[k] -= result.halo;
    window.hi[k] += result.halo;
    clip.lo[k] = window.lo[k] - result.margin;
    clip.hi[k] = window.hi[k] + result.margin;
  }
  // Whether a shape overlaps the window with positive area, in µm as
  // converted by toBox: a shape that only touches it would clip to nothing.
  // Inside, its clipped box keeps a positive area, as clip contains window.
  auto inWindow = [&](const odb::Rect& rect) {
    return rect.xMax() / dbu > window.lo[0] && rect.xMin() / dbu < window.hi[0]
           && rect.yMax() / dbu > window.lo[1]
           && rect.yMin() / dbu < window.hi[1];
  };

  // The neighbours: shapes reaching the window, clipped at the margin.
  // False if the shape does not reach the window.
  auto clipToWindow = [&](const Shape& shape, Box& box) {
    if (!inWindow(shape.rect)) {
      return false;
    }
    box = toBox(shape);
    bool cut = false;
    for (int k = 0; k < 2; k++) {
      if (box.lo[k] < clip.lo[k] || box.hi[k] > clip.hi[k]) {
        box.lo[k] = std::max(box.lo[k], clip.lo[k]);
        box.hi[k] = std::min(box.hi[k], clip.hi[k]);
        cut = true;
      }
    }
    result.clipped += cut ? 1 : 0;
    return true;
  };
  for (odb::dbNet* net : block->getNets()) {
    if (target_ids.contains(net->getId())) {
      continue;
    }
    int index = -1;
    for (const Shape& shape : netShapes(net)) {
      Box box;
      if (!clipToWindow(shape, box)) {
        continue;
      }
      if (index < 0) {
        index = geometry.addNet(net->getName());
        result.neighbours++;
      }
      geometry.addBox(index, box);
    }
  }
  std::vector<Obstruction> obstructions;
  for (const Shape& shape : unconnectedShapes(block)) {
    Box box;
    if (clipToWindow(shape, box)) {
      const double width = shape.layer->getWidth() / dbu;
      const double spacing = shape.layer->getSpacing() / dbu;
      obstructions.push_back(
          {box, spacing > 0 ? spacing : std::max(width, 1 / dbu), width});
    }
  }
  const std::vector<Box> pieces
      = trimObstructions(geometry, obstructions, result.trimmed);
  if (!pieces.empty()) {
    const int index = geometry.addNet(kObstructionNet);
    geometry.setGrounded(index);
    for (const Box& piece : pieces) {
      geometry.addBox(index, piece);
    }
  }

  // Nothing lies beyond the die, so the window is open on any side that
  // reaches past it; it is set only if some side stops inside.
  const odb::Rect die = block->getDieArea();
  const double die_lo[2] = {die.xMin() / dbu, die.yMin() / dbu};
  const double die_hi[2] = {die.xMax() / dbu, die.yMax() / dbu};
  bool closed = false;
  for (int k = 0; k < 2; k++) {
    if (window.lo[k] <= die_lo[k]) {
      window.lo[k] = -std::numeric_limits<double>::infinity();
    } else {
      closed = true;
    }
    if (window.hi[k] >= die_hi[k]) {
      window.hi[k] = std::numeric_limits<double>::infinity();
    } else {
      closed = true;
    }
  }
  if (closed) {
    geometry.setWindow(window);
  }
  checkShorts(geometry);
  return result;
}

}  // namespace fsv
