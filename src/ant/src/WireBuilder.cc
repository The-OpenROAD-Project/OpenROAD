// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "WireBuilder.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "boost/functional/hash.hpp"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace ant {

WireBuilder::WireBuilder(odb::dbDatabase* db, utl::Logger* logger)
{
  db_ = db;
  block_ = db_->getChip()->getBlock();
  logger_ = logger;
}
WireBuilder::~WireBuilder() = default;

void WireBuilder::makeNetWiresFromGuides()
{
  const int gcell_dimension = block_->getGCellTileSize();
  default_vias_ = block_->getDefaultVias();
  for (odb::dbNet* db_net : block_->getNets()) {
    const bool is_detailed_routed
        = db_net->getWireType() == odb::dbWireType::ROUTED && db_net->getWire();

    if (!db_net->isSpecial() && !db_net->isConnectedByAbutment()
        && db_net->getTermCount() > 1 && !dbNetIsLocal(db_net)
        && !is_detailed_routed) {
      makeNetWire(db_net, gcell_dimension);
    }
  }
}

void WireBuilder::makeNetWiresFromGuides(const std::vector<odb::dbNet*>& nets)
{
  const int gcell_dimension = block_->getGCellTileSize();
  default_vias_ = block_->getDefaultVias();
  for (odb::dbNet* db_net : nets) {
    const bool is_detailed_routed
        = db_net->getWireType() == odb::dbWireType::ROUTED && db_net->getWire();

    if (!db_net->isSpecial() && !db_net->isConnectedByAbutment()
        && db_net->getTermCount() > 1 && !dbNetIsLocal(db_net)
        && !is_detailed_routed) {
      makeNetWire(db_net, gcell_dimension);
    }
  }
}

void WireBuilder::makeNetWire(odb::dbNet* db_net, const int gcell_dimension)
{
  odb::dbWire* wire = odb::dbWire::create(db_net);
  if (wire) {
    odb::dbTech* tech = db_->getTech();
    odb::dbWireEncoder wire_encoder;
    wire_encoder.begin(wire);
    GuidePtPinsMap route_pt_pins;
    std::vector<GuideSegment> route
        = makeWireFromGuides(db_net, route_pt_pins, gcell_dimension);
    std::unordered_set<GuideSegment, GuideSegmentHash> wire_segments;
    int prev_conn_layer = -1;
    for (GuideSegment& seg : route) {
      int l1 = seg.pt1.layer->getRoutingLevel();
      int l2 = seg.pt2.layer->getRoutingLevel();
      odb::dbTechLayer* bottom_tech_layer
          = seg.pt1.layer->getRoutingLevel() < seg.pt2.layer->getRoutingLevel()
                ? seg.pt1.layer
                : seg.pt2.layer;
      odb::dbTechLayer* top_tech_layer
          = seg.pt1.layer->getRoutingLevel() > seg.pt2.layer->getRoutingLevel()
                ? seg.pt1.layer
                : seg.pt2.layer;

      if (std::abs(l1 - l2) > 1) {
        debugPrint(logger_,
                   utl::ANT,
                   "make_net_wire",
                   1,
                   "invalid seg: ({}, {})um to ({}, {})um",
                   block_->dbuToMicrons(seg.pt1.pos.getX()),
                   block_->dbuToMicrons(seg.pt1.pos.getY()),
                   block_->dbuToMicrons(seg.pt2.pos.getX()),
                   block_->dbuToMicrons(seg.pt2.pos.getY()));

        logger_->error(utl::ANT,
                       15,
                       "Global route segment for net {} not "
                       "valid. The layers {} and {} "
                       "are not adjacent.",
                       db_net->getName(),
                       bottom_tech_layer->getName(),
                       top_tech_layer->getName());
      }
      if (wire_segments.find(seg) == wire_segments.end()) {
        int x1 = seg.pt1.pos.getX();
        int y1 = seg.pt1.pos.getY();
        if (seg.isVia()) {
          if (bottom_tech_layer->getRoutingLevel()
              >= block_->getMinRoutingLayer()) {
            if (bottom_tech_layer->getRoutingLevel() == prev_conn_layer) {
              wire_encoder.newPath(bottom_tech_layer, odb::dbWireType::ROUTED);
              prev_conn_layer = std::max(l1, l2);
            } else if (top_tech_layer->getRoutingLevel() == prev_conn_layer) {
              wire_encoder.newPath(top_tech_layer, odb::dbWireType::ROUTED);
              prev_conn_layer = std::min(l1, l2);
            } else {
              // if a via is the first object added to the wire_encoder, or the
              // via starts a new path and is not connected to previous
              // wires create a new path using the bottom layer and do not
              // update the prev_conn_layer. this way, this process is repeated
              // until the first wire is added and properly update the
              // prev_conn_layer
              wire_encoder.newPath(bottom_tech_layer, odb::dbWireType::ROUTED);
            }

            wire_encoder.addPoint(x1, y1);
            wire_encoder.addTechVia(default_vias_[bottom_tech_layer]);
            addWireTerms(db_net,
                         route,
                         x1,
                         y1,
                         bottom_tech_layer,
                         route_pt_pins,
                         wire_encoder,
                         false);
            wire_segments.insert(seg);
          }
        } else {
          // Add wire
          int x2 = seg.pt2.pos.getX();
          int y2 = seg.pt2.pos.getY();
          if (x1 != x2 || y1 != y2) {
            odb::dbTechLayer* tech_layer = tech->findRoutingLayer(l1);
            addWireTerms(db_net,
                         route,
                         x1,
                         y1,
                         tech_layer,
                         route_pt_pins,
                         wire_encoder,
                         true);
            wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
            wire_encoder.addPoint(x1, y1);
            wire_encoder.addPoint(x2, y2);
            addWireTerms(db_net,
                         route,
                         x2,
                         y2,
                         tech_layer,
                         route_pt_pins,
                         wire_encoder,
                         true);
            wire_segments.insert(seg);
            prev_conn_layer = l1;
          }
        }
      }
    }
    wire_encoder.end();
  } else {
    logger_->error(
        utl::ANT, 16, "Cannot create wire for net {}.", db_net->getConstName());
  }
}

void WireBuilder::addWireTerms(odb::dbNet* db_net,
                               std::vector<GuideSegment>& route,
                               int grid_x,
                               int grid_y,
                               odb::dbTechLayer* tech_layer,
                               GuidePtPinsMap& route_pt_pins,
                               odb::dbWireEncoder& wire_encoder,
                               bool connect_to_segment)
{
  std::vector<int> layers;
  int layer = tech_layer->getRoutingLevel();
  layers.push_back(layer);
  if (layer == block_->getMinRoutingLayer()) {
    layer--;
    layers.push_back(layer);
  }

  odb::dbTech* tech = db_->getTech();
  for (int l : layers) {
    GuidePoint guide_pt;
    guide_pt.pos = odb::Point(grid_x, grid_y);
    guide_pt.layer = tech->findRoutingLayer(l);
    auto itr = route_pt_pins.find(guide_pt);
    if (itr != route_pt_pins.end() && !itr->second.connected) {
      for (odb::dbBTerm* bterm : itr->second.bterms) {
        itr->second.connected = true;
        std::vector<odb::Rect> pin_rects;
        int bterm_top_layer;
        getBTermTopLayerRects(bterm, pin_rects, bterm_top_layer);
        odb::dbTechLayer* conn_layer = tech->findRoutingLayer(bterm_top_layer);
        odb::Point grid_pt = itr->first.pos;
        odb::Point pin_pt = grid_pt;
        makeWireToTerm(wire_encoder,
                       route,
                       tech_layer,
                       conn_layer,
                       pin_rects,
                       grid_pt,
                       pin_pt,
                       connect_to_segment);
      }
      for (odb::dbITerm* iterm : itr->second.iterms) {
        itr->second.connected = true;
        std::vector<odb::Rect> pin_rects;
        int iterm_top_layer;
        getITermTopLayerRects(iterm, pin_rects, iterm_top_layer);
        odb::dbTechLayer* conn_layer = tech->findRoutingLayer(iterm_top_layer);
        odb::Point grid_pt = itr->first.pos;
        odb::Point pin_pt = grid_pt;
        makeWireToTerm(wire_encoder,
                       route,
                       tech_layer,
                       conn_layer,
                       pin_rects,
                       grid_pt,
                       pin_pt,
                       connect_to_segment);
      }
    }
  }
}

void WireBuilder::makeWireToTerm(odb::dbWireEncoder& wire_encoder,
                                 std::vector<GuideSegment>& route,
                                 odb::dbTechLayer* tech_layer,
                                 odb::dbTechLayer* conn_layer,
                                 const std::vector<odb::Rect>& pin_rects,
                                 const odb::Point& grid_pt,
                                 odb::Point& pin_pt,
                                 const bool connect_to_segment)
{
  odb::dbTech* tech = db_->getTech();
  // create the local connection with the pin center only when the global
  // segment doesn't overlap the pin
  if (!pinOverlapsGSegment(grid_pt, conn_layer, pin_rects, route)) {
    int min_dist = std::numeric_limits<int>::max();
    for (const odb::Rect& pin_box : pin_rects) {
      odb::Point pos = pin_box.center();
      int dist = odb::Point::manhattanDistance(pos, pin_pt);
      if (dist < min_dist) {
        min_dist = dist;
        pin_pt = pos;
      }
    }
  }

  if (conn_layer->getRoutingLevel() >= block_->getMinRoutingLayer()) {
    wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
    wire_encoder.addPoint(grid_pt.x(), grid_pt.y());
    wire_encoder.addPoint(pin_pt.x(), grid_pt.y());
    wire_encoder.addPoint(pin_pt.x(), pin_pt.y());
  } else {
    odb::dbTechLayer* min_layer
        = tech->findRoutingLayer(block_->getMinRoutingLayer());

    if (connect_to_segment && tech_layer != min_layer) {
      // create vias to connect the guide segment to the min routing
      // layer. the min routing layer will be used to connect to the pin.
      wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
      wire_encoder.addPoint(grid_pt.x(), grid_pt.y());
      for (int i = min_layer->getRoutingLevel();
           i < tech_layer->getRoutingLevel();
           i++) {
        odb::dbTechLayer* l = tech->findRoutingLayer(i);
        wire_encoder.addTechVia(default_vias_[l]);
      }
    }

    if (min_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      makeWire(wire_encoder,
               min_layer,
               grid_pt,
               odb::Point(grid_pt.x(), pin_pt.y()));

      wire_encoder.addTechVia(default_vias_[min_layer]);
      makeWire(
          wire_encoder, min_layer, odb::Point(grid_pt.x(), pin_pt.y()), pin_pt);
    } else {
      makeWire(wire_encoder,
               min_layer,
               grid_pt,
               odb::Point(pin_pt.x(), grid_pt.y()));
      wire_encoder.addTechVia(default_vias_[min_layer]);
      makeWire(
          wire_encoder, min_layer, odb::Point(pin_pt.x(), grid_pt.y()), pin_pt);
    }

    // create vias to reach the pin
    for (int i = min_layer->getRoutingLevel() - 1;
         i >= conn_layer->getRoutingLevel();
         i--) {
      odb::dbTechLayer* l = tech->findRoutingLayer(i);
      wire_encoder.addTechVia(default_vias_[l]);
    }
  }
}

void WireBuilder::makeWire(odb::dbWireEncoder& wire_encoder,
                           odb::dbTechLayer* layer,
                           const odb::Point& start,
                           const odb::Point& end)
{
  wire_encoder.newPath(layer, odb::dbWireType::ROUTED);
  wire_encoder.addPoint(start.x(), start.y());
  wire_encoder.addPoint(end.x(), end.y());
}

bool WireBuilder::pinOverlapsGSegment(const odb::Point& pin_position,
                                      const odb::dbTechLayer* pin_layer,
                                      const std::vector<odb::Rect>& pin_rects,
                                      const std::vector<GuideSegment>& route)
{
  // check if pin position on grid overlaps with the pin shape
  for (const odb::Rect& box : pin_rects) {
    if (box.overlaps(pin_position)) {
      return true;
    }
  }

  // check if pin position on grid overlaps with at least one GSegment
  for (const odb::Rect& box : pin_rects) {
    for (const GuideSegment& seg : route) {
      if (seg.pt1.layer == seg.pt2.layer &&  // ignore vias
          seg.pt1.layer == pin_layer) {
        auto [x0, x1] = std::minmax({seg.pt1.pos.getX(), seg.pt2.pos.getX()});
        auto [y0, y1] = std::minmax({seg.pt1.pos.getY(), seg.pt2.pos.getY()});
        odb::Rect seg_rect(x0, y0, x1, y1);

        if (box.intersects(seg_rect)) {
          return true;
        }
      }
    }
  }

  return false;
}

std::vector<GuideSegment> WireBuilder::makeWireFromGuides(
    odb::dbNet* db_net,
    GuidePtPinsMap& route_pt_pins,
    const int gcell_dimension)
{
  std::vector<GuideSegment> route;
  for (odb::dbGuide* guide : db_net->getGuides()) {
    odb::dbTechLayer* layer = guide->getLayer();
    odb::dbTechLayer* via_layer = guide->getViaLayer();
    std::pair<GuidePoint, GuidePoint> endpoints;
    std::pair<odb::Point, odb::Point> box_limits;
    boxToGuideSegment(guide->getBox(),
                      layer,
                      via_layer,
                      route,
                      endpoints,
                      box_limits,
                      gcell_dimension);

    if (guide->isConnectedToTerm()) {
      for (odb::dbITerm* iterm : db_net->getITerms()) {
        if (checkGuideITermConnection(
                endpoints.first, iterm, box_limits.first, gcell_dimension)) {
          route_pt_pins[endpoints.first].iterms.push_back(iterm);
        }
        if (checkGuideITermConnection(
                endpoints.second, iterm, box_limits.second, gcell_dimension)) {
          route_pt_pins[endpoints.second].iterms.push_back(iterm);
        }
      }

      for (odb::dbBTerm* bterm : db_net->getBTerms()) {
        if (checkGuideBTermConnection(
                endpoints.first, bterm, box_limits.first, gcell_dimension)) {
          route_pt_pins[endpoints.first].bterms.push_back(bterm);
        }
        if (checkGuideBTermConnection(
                endpoints.second, bterm, box_limits.second, gcell_dimension)) {
          route_pt_pins[endpoints.second].bterms.push_back(bterm);
        }
      }
    }
  }

  return route;
}

bool WireBuilder::checkGuideITermConnection(const GuidePoint& guide_pt,
                                            odb::dbITerm* iterm,
                                            const odb::Point& box_limit,
                                            const int gcell_dimension)
{
  odb::Point ll(guide_pt.pos.getX() - gcell_dimension / 2,
                guide_pt.pos.getY() - gcell_dimension / 2);
  odb::Point ur(guide_pt.pos.getX() + gcell_dimension / 2,
                guide_pt.pos.getY() + gcell_dimension / 2);
  if (ll != box_limit && ur != box_limit) {
    ur = box_limit;
  }
  odb::Rect guide_box(ll, ur);
  std::vector<odb::dbAccessPoint*> aps = iterm->getPrefAccessPoints();

  odb::dbTechLayer* iterm_top_layer = nullptr;
  odb::dbMTerm* mterm = iterm->getMTerm();
  for (odb::dbMPin* mpin : mterm->getMPins()) {
    for (odb::dbBox* box : mpin->getGeometry()) {
      odb::dbTechLayer* tech_layer = box->getTechLayer();
      if (tech_layer->getType() == odb::dbTechLayerType::ROUTING
          && (iterm_top_layer == nullptr
              || tech_layer->getRoutingLevel()
                     > iterm_top_layer->getRoutingLevel())) {
        iterm_top_layer = tech_layer;
      }
    }
  }

  if (!aps.empty()) {
    for (auto ap : aps) {
      odb::Point ap_position = ap->getPoint();
      odb::dbTransform xform;
      int x, y;
      iterm->getInst()->getLocation(x, y);
      xform.setOffset({x, y});
      xform.setOrient(odb::dbOrientType(odb::dbOrientType::R0));
      xform.apply(ap_position);
      if (guide_box.intersects(ap_position)
          && (ap->getLayer() == guide_pt.layer
              || iterm_top_layer == guide_pt.layer)) {
        return true;
      }
    }
  } else {
    odb::Rect iterm_bbox = iterm->getBBox();
    if (guide_box.overlaps(iterm_bbox) && iterm_top_layer == guide_pt.layer) {
      return true;
    }
  }

  return false;
}
bool WireBuilder::checkGuideBTermConnection(const GuidePoint& guide_pt,
                                            odb::dbBTerm* bterm,
                                            const odb::Point& box_limit,
                                            const int gcell_dimension)
{
  odb::Point ll(guide_pt.pos.getX() - gcell_dimension / 2,
                guide_pt.pos.getY() - gcell_dimension / 2);
  odb::Point ur(guide_pt.pos.getX() + gcell_dimension / 2,
                guide_pt.pos.getY() + gcell_dimension / 2);
  if (ll != box_limit && ur != box_limit) {
    ur = box_limit;
  } else {
  }
  odb::Rect guide_box(ll, ur);

  odb::dbTechLayer* bterm_top_layer = nullptr;
  std::vector<odb::dbAccessPoint*> aps;
  for (odb::dbBPin* bpin : bterm->getBPins()) {
    for (odb::dbBox* bpin_box : bpin->getBoxes()) {
      odb::dbTechLayer* tech_layer = bpin_box->getTechLayer();
      if (tech_layer->getType() == odb::dbTechLayerType::ROUTING
          && (bterm_top_layer == nullptr
              || tech_layer->getRoutingLevel()
                     > bterm_top_layer->getRoutingLevel())) {
        bterm_top_layer = tech_layer;
      }
      const std::vector<odb::dbAccessPoint*>& bpin_pas
          = bpin->getAccessPoints();
      aps.insert(aps.begin(), bpin_pas.begin(), bpin_pas.end());
    }
  }

  if (!aps.empty()) {
    for (auto ap : aps) {
      odb::Point ap_position = ap->getPoint();
      if (guide_box.intersects(ap_position)
          && ap->getLayer() == guide_pt.layer) {
        return true;
      }
    }
  } else {
    odb::Rect bterm_bbox = bterm->getBBox();
    if (guide_box.overlaps(bterm_bbox) && bterm_top_layer == guide_pt.layer) {
      return true;
    }
  }

  return false;
}

void WireBuilder::boxToGuideSegment(
    const odb::Rect& guide_box,
    odb::dbTechLayer* layer,
    odb::dbTechLayer* via_layer,
    std::vector<GuideSegment>& route,
    std::pair<GuidePoint, GuidePoint>& endpoints,
    std::pair<odb::Point, odb::Point>& box_limits,
    const int gcell_dimension)
{
  int x0 = (gcell_dimension * (guide_box.xMin() / gcell_dimension))
           + (gcell_dimension / 2);
  int y0 = (gcell_dimension * (guide_box.yMin() / gcell_dimension))
           + (gcell_dimension / 2);

  const int x1 = (gcell_dimension * (guide_box.xMax() / gcell_dimension))
                 - (gcell_dimension / 2);
  const int y1 = (gcell_dimension * (guide_box.yMax() / gcell_dimension))
                 - (gcell_dimension / 2);

  if (x0 == x1 && y0 == y1) {
    const GuideSegment seg
        = GuideSegment{GuidePoint{odb::Point(x0, y0), layer},
                       GuidePoint{odb::Point(x1, y1), via_layer}};
    route.push_back(seg);
    endpoints.first = GuidePoint{odb::Point(x0, y0), layer};
    endpoints.second = GuidePoint{odb::Point(x1, y1), via_layer};
    box_limits.first = guide_box.ur();
    box_limits.second = guide_box.ur();
  } else {
    endpoints.first = GuidePoint{odb::Point(x0, y0), layer};
    endpoints.second = GuidePoint{odb::Point(x1, y1), layer};
    box_limits.first = guide_box.ll();
    box_limits.second = guide_box.ur();
  }

  while (y0 == y1 && (x0 + gcell_dimension) <= x1) {
    const GuideSegment seg
        = GuideSegment{GuidePoint{odb::Point(x0, y0), layer},
                       GuidePoint{odb::Point(x0 + gcell_dimension, y0), layer}};
    route.push_back(seg);
    x0 += gcell_dimension;
  }

  while (x0 == x1 && (y0 + gcell_dimension) <= y1) {
    const GuideSegment seg
        = GuideSegment{GuidePoint{odb::Point(x0, y0), layer},
                       GuidePoint{odb::Point(x0, y0 + gcell_dimension), layer}};
    route.push_back(seg);
    y0 += gcell_dimension;
  }
}

bool GuidePoint::operator<(const GuidePoint& pt) const
{
  if (pos != pt.pos) {
    return pos < pt.pos;
  }
  return layer->getRoutingLevel() < pt.layer->getRoutingLevel();
}

bool GuideSegment::operator==(const GuideSegment& segment) const
{
  return pt1.layer == segment.pt1.layer && pt2.layer == segment.pt2.layer
         && pt1.pos == segment.pt1.pos && pt2.pos == segment.pt2.pos;
}

std::size_t GuideSegmentHash::operator()(const GuideSegment& seg) const
{
  return boost::hash<std::tuple<int, int, int, int, int, int>>()(
      {seg.pt1.pos.getX(),
       seg.pt1.pos.getY(),
       seg.pt1.layer->getRoutingLevel(),
       seg.pt2.pos.getX(),
       seg.pt2.pos.getY(),
       seg.pt2.layer->getRoutingLevel()});
}

void WireBuilder::getBTermTopLayerRects(odb::dbBTerm* bterm,
                                        std::vector<odb::Rect>& rects,
                                        int& top_layer_idx)
{
  const std::string& pin_name = bterm->getName();
  std::map<int, std::vector<odb::Rect>> rects_per_layer;
  top_layer_idx = 0;
  for (odb::dbBPin* bterm_pin : bterm->getBPins()) {
    for (odb::dbBox* bpin_box : bterm_pin->getBoxes()) {
      odb::dbTechLayer* tech_layer = bpin_box->getTechLayer();
      if (tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
        continue;
      }
      const int tech_layer_idx = tech_layer->getRoutingLevel();
      top_layer_idx = std::max(top_layer_idx, tech_layer_idx);

      odb::Rect rect = bpin_box->getBox();
      rects_per_layer[tech_layer_idx].push_back(rect);
    }
  }
  rects = rects_per_layer[top_layer_idx];
}

void WireBuilder::getITermTopLayerRects(odb::dbITerm* iterm,
                                        std::vector<odb::Rect>& rects,
                                        int& top_layer_idx)
{
  odb::dbInst* inst = iterm->getInst();
  if (!inst->isPlaced()) {
    logger_->error(utl::ANT, 17, "Instance {} is not placed.", inst->getName());
  }
  const odb::dbTransform transform = inst->getTransform();

  std::map<int, std::vector<odb::Rect>> rects_per_layer;
  odb::dbMTerm* mterm = iterm->getMTerm();
  top_layer_idx = 0;
  for (odb::dbMPin* mterm : mterm->getMPins()) {
    for (odb::dbBox* box : mterm->getGeometry()) {
      odb::dbTechLayer* tech_layer = box->getTechLayer();
      if (tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
        continue;
      }
      const int tech_layer_idx = tech_layer->getRoutingLevel();
      top_layer_idx = std::max(top_layer_idx, tech_layer_idx);

      odb::Rect rect = box->getBox();
      transform.apply(rect);
      rects_per_layer[tech_layer_idx].push_back(rect);
    }
  }
  rects = rects_per_layer[top_layer_idx];
}

bool WireBuilder::dbNetIsLocal(odb::dbNet* db_net)
{
  bool is_local = true;
  odb::Rect last_box = (*(db_net->getGuides().begin()))->getBox();
  for (odb::dbGuide* guide : db_net->getGuides()) {
    if (last_box != guide->getBox()) {
      return false;
    }
    last_box = guide->getBox();
  }

  return is_local;
}

}  // namespace ant
