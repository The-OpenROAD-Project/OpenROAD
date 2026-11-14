// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "Pin.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "PinGeometry.h"
#include "grt/GlobalRouter.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace grt {

Pin::Pin(odb::dbITerm* iterm,
         const odb::Point& position,
         const std::vector<odb::dbTechLayer*>& layers,
         const odb::PtrMap<odb::dbTechLayer, std::vector<odb::Rect>>&
             boxes_per_layer,
         bool connected_to_pad_or_macro)
    : iterm(iterm),
      position_(position),
      edge_(PinEdge::none),
      is_port_(false),
      connected_to_pad_or_macro_(connected_to_pad_or_macro)
{
  for (auto layer : layers) {
    layers_.push_back(layer->getRoutingLevel());
  }
  std::ranges::sort(layers_);

  for (auto& [layer, boxes] : boxes_per_layer) {
    boxes_per_layer_[layer->getRoutingLevel()] = boxes;
  }

  if (connected_to_pad_or_macro) {
    determineEdge(iterm->getInst()->getBBox()->getBox(), layers);
  } else {
    connection_layer_ = layers_.back();
  }
}

Pin::Pin(odb::dbBTerm* bterm,
         const odb::Point& position,
         const std::vector<odb::dbTechLayer*>& layers,
         const odb::PtrMap<odb::dbTechLayer, std::vector<odb::Rect>>&
             boxes_per_layer,
         const odb::Point& die_center)
    : bterm(bterm),
      position_(position),
      edge_(PinEdge::none),
      is_port_(true),
      connected_to_pad_or_macro_(false),
      covering_iterm_(findCoveringITerm(bterm))
{
  for (auto layer : layers) {
    layers_.push_back(layer->getRoutingLevel());
  }
  std::ranges::sort(layers_);

  for (auto& [layer, boxes] : boxes_per_layer) {
    boxes_per_layer_[layer->getRoutingLevel()] = boxes;
  }

  determineEdge(bterm->getBlock()->getDieArea(), layers);
}

void Pin::determineEdge(const odb::Rect& bounds,
                        const std::vector<odb::dbTechLayer*>& layers)
{
  std::vector<odb::Rect> boxes;
  for (const auto& [layer, layer_boxes] : boxes_per_layer_) {
    boxes.insert(boxes.end(), layer_boxes.begin(), layer_boxes.end());
  }
  edge_ = findPinEdge(bounds, boxes);

  // Only layers with a shape at the edge connect from outside the cell.
  std::vector<odb::dbTechLayer*> edge_layers;
  for (odb::dbTechLayer* layer : layers) {
    const std::vector<odb::Rect>& layer_boxes
        = boxes_per_layer_.at(layer->getRoutingLevel());
    if (std::ranges::any_of(layer_boxes, [&](const odb::Rect& box) {
          return findClosestEdge(bounds, box) == edge_;
        })) {
      edge_layers.push_back(layer);
    }
  }
  const std::vector<odb::dbTechLayer*>& candidate_layers
      = edge_layers.empty() ? layers : edge_layers;

  // Find the best connection layer as the top layer may not be in the
  // right direction (eg horizontal layer for a south pin).
  odb::dbTechLayerDir dir = (edge_ == PinEdge::north || edge_ == PinEdge::south)
                                ? odb::dbTechLayerDir::VERTICAL
                                : odb::dbTechLayerDir::HORIZONTAL;
  odb::dbTechLayer* best_layer = nullptr;
  odb::dbTechLayer* top_layer = nullptr;
  for (auto layer : candidate_layers) {
    if (!top_layer || layer->getRoutingLevel() > top_layer->getRoutingLevel()) {
      top_layer = layer;
    }
    if (layer->getDirection() == dir) {
      if (!best_layer
          || layer->getRoutingLevel() > best_layer->getRoutingLevel()) {
        best_layer = layer;
      }
    }
  }
  if (best_layer) {
    connection_layer_ = best_layer->getRoutingLevel();
  } else {
    connection_layer_ = top_layer->getRoutingLevel();
  }
}

odb::dbITerm* Pin::getITerm() const
{
  if (is_port_) {
    return nullptr;
  }

  return iterm;
}

odb::dbBTerm* Pin::getBTerm() const
{
  if (is_port_) {
    return bterm;
  }

  return nullptr;
}

std::string Pin::getName() const
{
  if (is_port_) {
    return bterm->getName();
  }

  return getITermName(iterm);
}

bool Pin::isDriver()
{
  if (is_port_) {
    return (bterm->getIoType() == odb::dbIoType::INPUT);
  }
  odb::dbMTerm* mterm = iterm->getMTerm();
  odb::dbIoType type = mterm->getIoType();
  return type == odb::dbIoType::OUTPUT || type == odb::dbIoType::INOUT;
}

odb::Point Pin::getPositionNearInstEdge(const odb::Rect& pin_box,
                                        const odb::Point& rect_middle) const
{
  odb::Point pin_pos = rect_middle;
  if (getEdge() == PinEdge::north) {
    pin_pos.setY(pin_box.yMax());
  } else if (getEdge() == PinEdge::south) {
    pin_pos.setY(pin_box.yMin());
  } else if (getEdge() == PinEdge::east) {
    pin_pos.setX(pin_box.xMax());
  } else if (getEdge() == PinEdge::west) {
    pin_pos.setX(pin_box.xMin());
  }

  return pin_pos;
}

bool Pin::isCorePin() const
{
  if (iterm == nullptr) {
    return false;
  }

  odb::dbInst* inst = iterm->getInst();
  return inst->isCore();
  ;
}

}  // namespace grt
