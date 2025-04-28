// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "Pin.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "grt/GlobalRouter.h"

namespace grt {

Pin::Pin(
    odb::dbITerm* iterm,
    const odb::Point& position,
    const std::vector<odb::dbTechLayer*>& layers,
    const std::map<odb::dbTechLayer*, std::vector<odb::Rect>>& boxes_per_layer,
    bool connected_to_pad_or_macro)
    : iterm_(iterm),
      position_(position),
      edge_(PinEdge::none),
      is_port_(false),
      connected_to_pad_or_macro_(connected_to_pad_or_macro)
{
  for (auto layer : layers) {
    layers_.push_back(layer->getRoutingLevel());
  }
  std::sort(layers_.begin(), layers_.end());

  for (auto& [layer, boxes] : boxes_per_layer) {
    boxes_per_layer_[layer->getRoutingLevel()] = boxes;
  }

  if (connected_to_pad_or_macro) {
    determineEdge(iterm->getInst()->getBBox()->getBox(), layers);
  } else {
    connection_layer_ = layers_.back();
  }
}

Pin::Pin(
    odb::dbBTerm* bterm,
    const odb::Point& position,
    const std::vector<odb::dbTechLayer*>& layers,
    const std::map<odb::dbTechLayer*, std::vector<odb::Rect>>& boxes_per_layer,
    const odb::Point& die_center)
    : bterm_(bterm),
      position_(position),
      edge_(PinEdge::none),
      is_port_(true),
      connected_to_pad_or_macro_(false)
{
  for (auto layer : layers) {
    layers_.push_back(layer->getRoutingLevel());
  }
  std::sort(layers_.begin(), layers_.end());

  for (auto& [layer, boxes] : boxes_per_layer) {
    boxes_per_layer_[layer->getRoutingLevel()] = boxes;
  }

  determineEdge(bterm->getBlock()->getDieArea(), layers);
}

void Pin::determineEdge(const odb::Rect& bounds,
                        const std::vector<odb::dbTechLayer*>& layers)
{
  odb::Point lower_left;
  odb::Point upper_right;
  int n_count = 0;
  int s_count = 0;
  int e_count = 0;
  int w_count = 0;
  for (const auto& [layer, boxes] : boxes_per_layer_) {
    for (const auto& box : boxes) {
      lower_left = box.ll();
      upper_right = box.ur();
      // Determine which edge is closest to the pin box
      const int n_dist = bounds.yMax() - upper_right.y();
      const int s_dist = lower_left.y() - bounds.yMin();
      const int e_dist = bounds.xMax() - upper_right.x();
      const int w_dist = lower_left.x() - bounds.xMin();
      const int min_dist = std::min({n_dist, s_dist, w_dist, e_dist});
      if (n_dist == min_dist) {
        n_count += 1;
      } else if (s_dist == min_dist) {
        s_count += 1;
      } else if (e_dist == min_dist) {
        e_count += 1;
      } else {
        w_count += 1;
      }
    }
  }

  // voting system to determine the pin edge after checking all pin boxes
  int edge_count = std::max({n_count, s_count, e_count, w_count});
  if (edge_count == n_count) {
    edge_ = PinEdge::north;
  } else if (edge_count == s_count) {
    edge_ = PinEdge::south;
  } else if (edge_count == e_count) {
    edge_ = PinEdge::east;
  } else if (edge_count == w_count) {
    edge_ = PinEdge::west;
  }

  // Find the best connection layer as the top layer may not be in the
  // right direction (eg horizontal layer for a south pin).
  odb::dbTechLayerDir dir = (edge_ == PinEdge::north || edge_ == PinEdge::south)
                                ? odb::dbTechLayerDir::VERTICAL
                                : odb::dbTechLayerDir::HORIZONTAL;
  odb::dbTechLayer* best_layer = nullptr;
  for (auto layer : layers) {
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
    connection_layer_ = layers_.back();
  }
}

odb::dbITerm* Pin::getITerm() const
{
  if (is_port_)
    return nullptr;

  return iterm_;
}

odb::dbBTerm* Pin::getBTerm() const
{
  if (is_port_)
    return bterm_;

  return nullptr;
}

std::string Pin::getName() const
{
  if (is_port_)
    return bterm_->getName();

  return getITermName(iterm_);
}

bool Pin::isDriver()
{
  if (is_port_) {
    return (bterm_->getIoType() == odb::dbIoType::INPUT);
  }
  odb::dbMTerm* mterm = iterm_->getMTerm();
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

int Pin::getConnectionLayer() const
{
  return connection_layer_;
}

}  // namespace grt
