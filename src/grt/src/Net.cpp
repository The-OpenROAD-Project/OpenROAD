/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "Net.h"

#include "grt/GlobalRouter.h"
#include "odb/dbShape.h"

namespace grt {

Net::Net(odb::dbNet* net, bool has_wires)
    : net_(net),
      slack_(0),
      has_wires_(has_wires),
      merged_net_(false),
      is_dirty_net_(false)
{
}

const std::string Net::getName() const
{
  return net_->getName();
}

const char* Net::getConstName() const
{
  return net_->getConstName();
}

odb::dbSigType Net::getSignalType() const
{
  return net_->getSigType().getString();
}

void Net::deleteSegment(const int seg_id, GRoute& route)
{
  for (SegmentIndex& parent : parent_segment_indices_) {
    if (parent >= seg_id) {
      parent--;
    }
  }
  parent_segment_indices_.erase(parent_segment_indices_.begin() + seg_id);
  route.erase(route.begin() + seg_id);
}

void Net::addPin(Pin& pin)
{
  pins_.push_back(pin);
}

std::vector<std::vector<SegmentIndex>> Net::buildSegmentsGraph()
{
  std::vector<std::vector<SegmentIndex>> graph(parent_segment_indices_.size(),
                                               std::vector<SegmentIndex>());
  for (int i = 0; i < parent_segment_indices_.size(); i++) {
    graph[i].push_back(parent_segment_indices_[i]);
    graph[parent_segment_indices_[i]].push_back(i);
  }
  return graph;
}

bool Net::isLocal()
{
  if (pins_.empty()) {
    return true;
  }
  odb::Point position = pins_[0].getOnGridPosition();
  for (Pin& pin : pins_) {
    odb::Point pinPos = pin.getOnGridPosition();
    if (pinPos != position) {
      return false;
    }
  }

  return true;
}

void Net::destroyPins()
{
  pins_.clear();
}

void Net::destroyITermPin(odb::dbITerm* iterm)
{
  pins_.erase(std::remove_if(pins_.begin(),
                             pins_.end(),
                             [&](const Pin& pin) {
                               return pin.getName() == getITermName(iterm);
                             }),
              pins_.end());
}

void Net::destroyBTermPin(odb::dbBTerm* bterm)
{
  pins_.erase(std::remove_if(pins_.begin(),
                             pins_.end(),
                             [&](const Pin& pin) {
                               return pin.getName() == bterm->getName();
                             }),
              pins_.end());
}

int Net::getNumBTermsAboveMaxLayer(odb::dbTechLayer* max_routing_layer)
{
  int bterm_count = 0;
  for (auto bterm : net_->getBTerms()) {
    int bterm_bottom_layer_idx = std::numeric_limits<int>::max();
    for (auto bpin : bterm->getBPins()) {
      for (auto box : bpin->getBoxes()) {
        bterm_bottom_layer_idx = std::min(
            bterm_bottom_layer_idx, box->getTechLayer()->getRoutingLevel());
      }
    }
    if (bterm_bottom_layer_idx > max_routing_layer->getRoutingLevel()) {
      bterm_count++;
    }
  }

  return bterm_count;
}

bool Net::hasStackedVias(odb::dbTechLayer* max_routing_layer)
{
  int bterms_above_max_layer = getNumBTermsAboveMaxLayer(max_routing_layer);
  odb::uint wire_cnt = 0, via_cnt = 0;
  net_->getWireCount(wire_cnt, via_cnt);

  if (wire_cnt != 0 || via_cnt == 0) {
    return false;
  }

  odb::dbWirePath path;
  odb::dbWirePathShape pshape;
  odb::dbWire* wire = net_->getWire();

  odb::dbWirePathItr pitr;
  std::set<odb::Point> via_points;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    while (pitr.getNextShape(pshape)) {
      via_points.insert(path.point);
    }
  }

  if (via_points.size() != bterms_above_max_layer) {
    return false;
  }

  return true;
}

void Net::saveLastPinPositions()
{
  if (last_pin_positions_.empty()) {
    for (const Pin& pin : pins_) {
      last_pin_positions_.insert(RoutePt(pin.getOnGridPosition().getX(),
                                         pin.getOnGridPosition().getY(),
                                         pin.getConnectionLayer()));
    }
  }
}

}  // namespace grt
