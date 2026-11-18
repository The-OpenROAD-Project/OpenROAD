// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "PinGeometry.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace grt {

PinEdge findClosestEdge(const odb::Rect& bounds, const odb::Rect& box)
{
  const int n_dist = bounds.yMax() - box.yMax();
  const int s_dist = box.yMin() - bounds.yMin();
  const int e_dist = bounds.xMax() - box.xMax();
  const int w_dist = box.xMin() - bounds.xMin();
  const int min_dist = std::min({n_dist, s_dist, w_dist, e_dist});
  if (n_dist == min_dist) {
    return PinEdge::north;
  }
  if (s_dist == min_dist) {
    return PinEdge::south;
  }
  if (e_dist == min_dist) {
    return PinEdge::east;
  }
  return PinEdge::west;
}

PinEdge findPinEdge(const odb::Rect& bounds,
                    const std::vector<odb::Rect>& boxes)
{
  int n_count = 0;
  int s_count = 0;
  int e_count = 0;
  int w_count = 0;
  for (const odb::Rect& box : boxes) {
    const PinEdge edge = findClosestEdge(bounds, box);
    if (edge == PinEdge::north) {
      n_count += 1;
    } else if (edge == PinEdge::south) {
      s_count += 1;
    } else if (edge == PinEdge::east) {
      e_count += 1;
    } else {
      w_count += 1;
    }
  }

  // voting system to determine the pin edge after checking all pin boxes
  const int edge_count = std::max({n_count, s_count, e_count, w_count});
  if (edge_count == n_count) {
    return PinEdge::north;
  }
  if (edge_count == s_count) {
    return PinEdge::south;
  }
  if (edge_count == e_count) {
    return PinEdge::east;
  }
  return PinEdge::west;
}

PinEdge findITermEdge(odb::dbITerm* iterm)
{
  odb::dbInst* inst = iterm->getInst();
  const odb::dbTransform transform = inst->getTransform();
  std::vector<odb::Rect> boxes;
  for (odb::dbMPin* mpin : iterm->getMTerm()->getMPins()) {
    for (odb::dbBox* box : mpin->getGeometry()) {
      // Via boxes have no layer.
      odb::dbTechLayer* layer = box->getTechLayer();
      if (layer == nullptr
          || layer->getType() != odb::dbTechLayerType::ROUTING) {
        continue;
      }
      odb::Rect rect = box->getBox();
      transform.apply(rect);
      boxes.push_back(rect);
    }
  }
  return findPinEdge(inst->getBBox()->getBox(), boxes);
}

int coordinateTowardEdge(const PinEdge edge, const odb::Point& point)
{
  switch (edge) {
    case PinEdge::north:
      return point.y();
    case PinEdge::south:
      return -point.y();
    case PinEdge::east:
      return point.x();
    case PinEdge::west:
      return -point.x();
    case PinEdge::none:
      break;
  }
  return 0;
}

odb::dbITerm* findCoveringITerm(odb::dbBTerm* bterm)
{
  odb::dbNet* net = bterm->getNet();
  if (net == nullptr) {
    return nullptr;
  }

  std::vector<std::pair<odb::dbTechLayer*, odb::Rect>> port_shapes;
  for (odb::dbBPin* bpin : bterm->getBPins()) {
    if (!bpin->getPlacementStatus().isPlaced()) {
      continue;
    }
    for (odb::dbBox* box : bpin->getBoxes()) {
      odb::dbTechLayer* layer = box->getTechLayer();
      if (layer->getType() == odb::dbTechLayerType::ROUTING) {
        port_shapes.emplace_back(layer, box->getBox());
      }
    }
  }
  if (port_shapes.empty()) {
    return nullptr;
  }

  for (odb::dbITerm* iterm : net->getITerms()) {
    odb::dbInst* inst = iterm->getInst();
    if (!inst->isPlaced()) {
      continue;
    }
    const odb::dbTransform transform = inst->getTransform();
    std::vector<std::pair<odb::dbTechLayer*, odb::Rect>> pin_shapes;
    for (odb::dbMPin* mpin : iterm->getMTerm()->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::Rect rect = box->getBox();
        transform.apply(rect);
        pin_shapes.emplace_back(box->getTechLayer(), rect);
      }
    }
    const bool covers_port
        = std::ranges::all_of(port_shapes, [&](const auto& port_shape) {
            return std::ranges::any_of(pin_shapes, [&](const auto& pin_shape) {
              return pin_shape.first == port_shape.first
                     && pin_shape.second.contains(port_shape.second);
            });
          });
    if (covers_port) {
      return iterm;
    }
  }

  return nullptr;
}

}  // namespace grt
