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
#include <utility>
#include <vector>

#include "SpatialIndex.h"
#include "fsv/BlockShapes.h"
#include "fsv/Geometry.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace fsv {

namespace {

// Shapes of different nets may touch but not overlap.
void checkShorts(const Geometry& geometry)
{
  const std::vector<Box>& boxes = geometry.boxes();
  const SpatialIndex<3> index(boxes);
  for (int i = 0; std::cmp_less(i, boxes.size()); i++) {
    index.forEachOverlapping(boxes[i], [&](const int j) {
      if (j <= i || geometry.boxNet(i) == geometry.boxNet(j)) {
        return;
      }
      for (int k = 0; k < 3; k++) {
        if (boxes[i].lo[k] >= boxes[j].hi[k]
            || boxes[j].lo[k] >= boxes[i].hi[k]) {
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
  auto toBox = [&](const BlockShape& shape) {
    const std::string name = shape.layer->getName();
    auto it = extents.find(name);
    if (it == extents.end()) {
      it = extents.emplace(name, layer_extent(name)).first;
    }
    return Box{
        .lo
        = {shape.rect.xMin() / dbu, shape.rect.yMin() / dbu, it->second.first},
        .hi
        = {shape.rect.xMax() / dbu, shape.rect.yMax() / dbu, it->second.second},
    };
  };

  NetWindow result;
  Geometry& geometry = result.geometry;
  // The targets, whole.
  AlignedBox<2> bounds{
      .lo={std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
      .hi={std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest(),},};
  std::set<int> target_ids;
  for (odb::dbNet* net : targets) {
    if (!target_ids.insert(net->getId()).second) {
      continue;  // named twice
    }
    const std::vector<BlockShape> shapes = netShapes(net);
    if (shapes.empty()) {
      throw std::invalid_argument("fsv: net " + net->getName()
                                  + " has no shapes on conductor layers");
    }
    const int index = geometry.addNet(net->getName());
    result.targets.push_back(index);
    for (const BlockShape& shape : shapes) {
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
  auto clipToWindow = [&](const BlockShape& shape, Box& box) {
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
    for (const BlockShape& shape : netShapes(net)) {
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
  for (const BlockShape& shape : unconnectedShapes(block)) {
    Box box;
    if (clipToWindow(shape, box)) {
      obstructions.push_back(obstruction(shape, box, dbu));
    }
  }
  const std::vector<Box> pieces
      = trimObstructions(geometry.boxes(), obstructions, result.trimmed);
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
