// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "MakeWireParasitics.h"

#include <cmath>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/SpefWriter.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "grt/PinGridLocation.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/MinMax.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Sdc.hh"
#include "sta/StaState.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace est {

using sta::Net;
using sta::Parasitic;
using sta::Parasitics;
using sta::Scene;

using utl::EST;

using std::abs;
using std::min;

MakeWireParasitics::MakeWireParasitics(
    utl::Logger* logger,
    est::EstimateParasitics* estimate_parasitics,
    sta::dbSta* sta,
    odb::dbTech* tech,
    odb::dbBlock* block,
    grt::GlobalRouter* grouter)
    : global_router_(grouter),
      estimate_parasitics_(estimate_parasitics),
      tech_(tech),
      block_(block),
      logger_(logger),
      sta_(sta),
      network_(sta_->getDbNetwork()),
      arc_delay_calc_(sta_->arcDelayCalc()),
      min_max_(sta::MinMax::max()),
      resistor_id_(1)
{
}

void MakeWireParasitics::estimateParasitics(odb::dbNet* net,
                                            grt::GRoute& route,
                                            sta::SpefWriter* spef_writer)
{
  debugPrint(logger_, EST, "est_rc", 1, "net {}", net->getConstName());
  if (logger_->debugCheck(EST, "est_rc", 2)) {
    for (grt::GSegment& segment : route) {
      logger_->report(
          "({:.2f}, {:.2f}) {:2d} -> ({:.2f}, {:.2f}) {:2d} l={:.2f}",
          block_->dbuToMicrons(segment.init_x),
          block_->dbuToMicrons(segment.init_y),
          segment.init_layer,
          block_->dbuToMicrons(segment.final_x),
          block_->dbuToMicrons(segment.final_y),
          segment.final_layer,
          block_->dbuToMicrons(segment.length()));
    }
  }

  Net* sta_net = network_->dbToSta(net);
  std::vector<grt::PinGridLocation> pin_grid_locs
      = global_router_->getPinGridPositions(net);

  for (Scene* corner : sta_->scenes()) {
    NodeRoutePtMap node_map;
    Parasitics* parasitics = corner->parasitics(min_max_);
    Parasitic* parasitic = parasitics->makeParasiticNetwork(sta_net, false);

    resistor_id_ = 1;
    makeRouteParasitics(
        parasitics, net, route, sta_net, corner, parasitic, node_map);
    makeParasiticsToPins(
        parasitics, net, pin_grid_locs, node_map, corner, parasitic);

    if (spef_writer) {
      spef_writer->writeNet(corner, sta_net, parasitic, parasitics);
    }

    arc_delay_calc_->reduceParasitic(
        parasitic, sta_net, corner, sta::MinMaxAll::all());
    parasitics->deleteParasiticNetwork(sta_net);
  }
}

void MakeWireParasitics::estimateParasitics(odb::dbNet* net, grt::GRoute& route)
{
  debugPrint(logger_, EST, "est_rc", 1, "net {}", net->getConstName());
  if (logger_->debugCheck(EST, "est_rc", 2)) {
    for (grt::GSegment& segment : route) {
      logger_->report(
          "({:.2f}, {:.2f}) {:2d} -> ({:.2f}, {:.2f}) {:2d} l={:.2f}",
          block_->dbuToMicrons(segment.init_x),
          block_->dbuToMicrons(segment.init_y),
          segment.init_layer,
          block_->dbuToMicrons(segment.final_x),
          block_->dbuToMicrons(segment.final_y),
          segment.final_layer,
          block_->dbuToMicrons(segment.length()));
    }
  }

  sta::Net* sta_net = network_->dbToSta(net);
  std::vector<grt::PinGridLocation> pin_grid_locs
      = global_router_->getPinGridPositions(net);

  for (Scene* corner : sta_->scenes()) {
    NodeRoutePtMap node_map;

    Parasitics* parasitics = corner->parasitics(min_max_);
    Parasitic* parasitic = parasitics->makeParasiticNetwork(sta_net, false);

    resistor_id_ = 1;
    makeRouteParasitics(
        parasitics, net, route, sta_net, corner, parasitic, node_map);
    makePartialParasiticsToPins(
        parasitics, pin_grid_locs, node_map, corner, parasitic, net);

    arc_delay_calc_->reduceParasitic(
        parasitic, sta_net, corner, sta::MinMaxAll::all());
    parasitics->deleteParasiticNetwork(sta_net);
  }
}

void MakeWireParasitics::clearParasitics()
{
  sta_->deleteParasitics();
  for (Scene* scene : sta_->scenes()) {
    Parasitics* parasitics = sta_->makeConcreteParasitics(scene->name(), "");
    scene->setParasitics(parasitics, sta::MinMaxAll::minMax());
  }
}

sta::Pin* MakeWireParasitics::staPin(odb::dbBTerm* bterm) const
{
  return network_->dbToSta(bterm);
}

sta::Pin* MakeWireParasitics::staPin(odb::dbITerm* iterm) const
{
  return network_->dbToSta(iterm);
}

void MakeWireParasitics::makeRouteParasitics(sta::Parasitics* parasitics,
                                             odb::dbNet* net,
                                             grt::GRoute& route,
                                             sta::Net* sta_net,
                                             sta::Scene* corner,
                                             sta::Parasitic* parasitic,
                                             NodeRoutePtMap& node_map)
{
  const int min_routing_layer = global_router_->getMinRoutingLayer();

  for (grt::GSegment& segment : route) {
    const int wire_length_dbu = segment.length();

    const int init_layer = segment.init_layer;
    bool is_valid_layer = init_layer >= min_routing_layer || segment.isVia();
    sta::ParasiticNode* n1 = is_valid_layer
                                 ? ensureParasiticNode(segment.init_x,
                                                       segment.init_y,
                                                       init_layer,
                                                       node_map,
                                                       parasitics,
                                                       parasitic,
                                                       sta_net)
                                 : nullptr;

    const int final_layer = segment.final_layer;
    is_valid_layer = final_layer >= min_routing_layer || segment.isVia();
    sta::ParasiticNode* n2 = is_valid_layer
                                 ? ensureParasiticNode(segment.final_x,
                                                       segment.final_y,
                                                       final_layer,
                                                       node_map,
                                                       parasitics,
                                                       parasitic,
                                                       sta_net)
                                 : nullptr;
    if (!n1 || !n2) {
      continue;
    }

    sta::Units* units = sta_->units();
    float res = 0.0;
    float cap = 0.0;
    if (wire_length_dbu == 0) {
      // via
      int lower_layer = min(segment.init_layer, segment.final_layer);
      odb::dbTechLayer* cut_layer
          = tech_->findRoutingLayer(lower_layer)->getUpperLayer();
      res = getCutLayerRes(cut_layer, corner);
      debugPrint(logger_,
                 EST,
                 "est_rc",
                 1,
                 "{} -> {} via {}-{} r={}",
                 parasitics->name(n1),
                 parasitics->name(n2),
                 segment.init_layer,
                 segment.final_layer,
                 units->resistanceUnit()->asString(res));
    } else if (segment.init_layer == segment.final_layer) {
      layerRC(wire_length_dbu, segment.init_layer, corner, net, res, cap);
      debugPrint(logger_,
                 EST,
                 "est_rc",
                 1,
                 "{} -> {} {:.2f}u layer={} r={} c={}",
                 parasitics->name(n1),
                 parasitics->name(n2),
                 dbuToMeters(wire_length_dbu) * 1e+6,
                 segment.init_layer,
                 units->resistanceUnit()->asString(res),
                 units->capacitanceUnit()->asString(cap));
    } else {
      logger_->warn(EST,
                    25,
                    "Non wire or via route found on net {}.",
                    net->getConstName());
    }
    parasitics->incrCap(n1, cap / 2.0);
    parasitics->makeResistor(parasitic, resistor_id_++, res, n1, n2);
    parasitics->incrCap(n2, cap / 2.0);
  }

  // Fix A: implicit-via post-pass.
  //
  // GRT does not always emit an explicit isVia=true segment at every
  // layer transition along a route; coincident endpoints on adjacent
  // routing levels are treated as implicitly connected. EST has historically
  // taken GRoute literally, so any layer transition without an explicit via
  // segment ends up as two distinct ParasiticNodes at the same (x, y) with
  // no resistor between them -- a disconnected island in the parasitic
  // network. That goes unnoticed by pi-Elmore-based delay calculators but
  // produces a singular conductance matrix in Prima/CCS.
  //
  // Close the contract gap here: scan node_map for coincident endpoints on
  // adjacent routing levels and, when no resistor already bridges them,
  // insert one with the appropriate cut-layer resistance.
  std::map<std::pair<int, int>, std::map<int, sta::ParasiticNode*>>
      xy_to_layer_node;
  for (const auto& [route_pt, node] : node_map) {
    xy_to_layer_node[{route_pt.x(), route_pt.y()}][route_pt.layer()] = node;
  }

  std::set<std::pair<sta::ParasiticNode*, sta::ParasiticNode*>> connected_pairs;
  for (sta::ParasiticResistor* r : parasitics->resistors(parasitic)) {
    sta::ParasiticNode* a = parasitics->node1(r);
    sta::ParasiticNode* b = parasitics->node2(r);
    connected_pairs.insert(std::minmax(a, b));
  }

  for (auto& [xy, layer_node] : xy_to_layer_node) {
    if (layer_node.size() < 2) {
      continue;
    }
    auto prev = layer_node.begin();
    for (auto it = std::next(prev); it != layer_node.end(); ++prev, ++it) {
      const int lo_level = prev->first;
      const int hi_level = it->first;
      if (hi_level != lo_level + 1) {
        // Not adjacent routing levels -- skipping signals that an
        // intermediate routing-level node is missing entirely, which is a
        // different bug worth surfacing rather than silently bridging.
        continue;
      }
      sta::ParasiticNode* lo_node = prev->second;
      sta::ParasiticNode* hi_node = it->second;
      if (connected_pairs.count(std::minmax(lo_node, hi_node))) {
        continue;
      }
      odb::dbTechLayer* lower_routing = tech_->findRoutingLayer(lo_level);
      odb::dbTechLayer* cut_layer
          = lower_routing ? lower_routing->getUpperLayer() : nullptr;
      if (cut_layer == nullptr) {
        // Top of stack or unresolved -- do not silently invent a resistor.
        continue;
      }
      const float via_R = getCutLayerRes(cut_layer, corner);
      const size_t this_id = resistor_id_;
      parasitics->makeResistor(
          parasitic, resistor_id_++, via_R, lo_node, hi_node);
      connected_pairs.insert(std::minmax(lo_node, hi_node));
    }
  }

}

void MakeWireParasitics::makeParasiticsToPins(
    sta::Parasitics* parasitics,
    odb::dbNet* net,
    std::vector<grt::PinGridLocation>& pin_grid_locs,
    NodeRoutePtMap& node_map,
    sta::Scene* corner,
    sta::Parasitic* parasitic)
{
  for (grt::PinGridLocation& pin_loc : pin_grid_locs) {
    makeParasiticsToPin(parasitics, net, pin_loc, node_map, corner, parasitic);
  }
}

// Make parasitics for the wire from the pin to the grid location of the pin.
void MakeWireParasitics::makeParasiticsToPin(sta::Parasitics* parasitics,
                                             odb::dbNet* net,
                                             grt::PinGridLocation& pin_loc,
                                             NodeRoutePtMap& node_map,
                                             sta::Scene* corner,
                                             sta::Parasitic* parasitic)
{
  sta::Pin* sta_pin = pin_loc.bterm != nullptr ? staPin(pin_loc.bterm)
                                               : staPin(pin_loc.iterm);

  sta::ParasiticNode* pin_node
      = parasitics->ensureParasiticNode(parasitic, sta_pin, network_);

  odb::Point pt = pin_loc.pt;
  odb::Point grid_pt = pin_loc.grid_pt;

  // Use the route layer above the pin layer if there is a via
  // to the pin.
  int layer = pin_loc.conn_layer + 1;
  grt::RoutePt grid_route(grid_pt.getX(), grid_pt.getY(), layer);
  sta::ParasiticNode* grid_node = node_map[grid_route];
  float via_res = 0;

  // Use the pin layer for the connection.
  if (grid_node == nullptr) {
    layer--;
    grid_route = grt::RoutePt(grid_pt.getX(), grid_pt.getY(), layer);
    grid_node = node_map[grid_route];
  } else {
    odb::dbTechLayer* cut_layer
        = tech_->findRoutingLayer(layer)->getLowerLayer();
    via_res = getCutLayerRes(cut_layer, corner);
  }

  const std::string pin_name = pin_loc.bterm != nullptr
                                   ? pin_loc.bterm->getName()
                                   : pin_loc.iterm->getName();
  if (grid_node) {
    // Make wire from pin to gcell center on pin layer.
    int wire_length_dbu
        = abs(pt.getX() - grid_pt.getX()) + abs(pt.getY() - grid_pt.getY());
    float res, cap;
    layerRC(wire_length_dbu, layer, corner, net, res, cap);
    sta::Units* units = sta_->units();
    debugPrint(
        logger_,
        EST,
        "est_rc",
        1,
        "{} -> {} ({:.2f}, {:.2f}) {:.2f}u layer={} r={} via_res={} c={}",
        parasitics->name(grid_node),
        parasitics->name(pin_node),
        block_->dbuToMicrons(pt.getX()),
        block_->dbuToMicrons(pt.getY()),
        block_->dbuToMicrons(wire_length_dbu),
        layer,
        units->resistanceUnit()->asString(res),
        units->resistanceUnit()->asString(via_res),
        units->capacitanceUnit()->asString(cap));

    debugPrint(logger_,
               EST,
               "est_rc",
               1,
               "pin {} -> to grid {}u layer={} r={} via_res={} c={}",
               pin_name,
               static_cast<int>(dbuToMeters(wire_length_dbu) * 1e+6),
               layer,
               units->resistanceUnit()->asString(res),
               units->resistanceUnit()->asString(via_res),
               units->capacitanceUnit()->asString(cap));

    // We could added the via resistor before the segment pi-model
    // but that would require an extra node and the accuracy of all
    // this is not that high.  Instead we just lump them together.
    parasitics->incrCap(pin_node, cap / 2.0);
    parasitics->makeResistor(
        parasitic, resistor_id_++, res + via_res, pin_node, grid_node);
    parasitics->incrCap(grid_node, cap / 2.0);
  } else {
    logger_->warn(EST,
                  26,
                  "Missing route to pin {} in net {}.",
                  pin_name,
                  net->getName());
  }
}

void MakeWireParasitics::makePartialParasiticsToPins(
    sta::Parasitics* parasitics,
    std::vector<grt::PinGridLocation>& pin_grid_locs,
    NodeRoutePtMap& node_map,
    sta::Scene* corner,
    sta::Parasitic* parasitic,
    odb::dbNet* net)
{
  for (grt::PinGridLocation& pin_loc : pin_grid_locs) {
    makePartialParasiticsToPin(
        parasitics, pin_loc, node_map, corner, parasitic, net);
  }
}

// Make parasitics for the wire from the pin to the grid location of the pin.
void MakeWireParasitics::makePartialParasiticsToPin(
    sta::Parasitics* parasitics,
    grt::PinGridLocation& pin_loc,
    NodeRoutePtMap& node_map,
    sta::Scene* corner,
    sta::Parasitic* parasitic,
    odb::dbNet* net)
{
  sta::Pin* sta_pin = pin_loc.bterm != nullptr ? staPin(pin_loc.bterm)
                                               : staPin(pin_loc.iterm);

  sta::ParasiticNode* pin_node
      = parasitics->ensureParasiticNode(parasitic, sta_pin, network_);

  odb::Point pt = pin_loc.pt;
  odb::Point grid_pt = pin_loc.grid_pt;

  // Use the route layer above the pin layer if there is a via
  // to the pin.
  int net_max_layer;
  int net_min_layer;
  global_router_->getNetLayerRange(net, net_min_layer, net_max_layer);
  int layer = net_min_layer + 1;
  grt::RoutePt grid_route(grid_pt.getX(), grid_pt.getY(), layer);
  sta::ParasiticNode* grid_node = node_map[grid_route];
  float via_res = 0;

  // Use the pin layer for the connection.
  if (grid_node == nullptr) {
    layer--;
    grid_route = grt::RoutePt(grid_pt.getX(), grid_pt.getY(), layer);
    grid_node = node_map[grid_route];
  } else {
    odb::dbTechLayer* cut_layer
        = tech_->findRoutingLayer(layer)->getLowerLayer();
    via_res = getCutLayerRes(cut_layer, corner);
  }

  const std::string pin_name = pin_loc.bterm != nullptr
                                   ? pin_loc.bterm->getName()
                                   : pin_loc.iterm->getName();
  if (grid_node) {
    // Make wire from pin to gcell center on pin layer.
    int wire_length_dbu
        = abs(pt.getX() - grid_pt.getX()) + abs(pt.getY() - grid_pt.getY());
    float res, cap;
    layerRC(wire_length_dbu, layer, corner, net, res, cap);
    sta::Units* units = sta_->units();
    debugPrint(
        logger_,
        EST,
        "est_rc",
        1,
        "{} -> {} ({:.2f}, {:.2f}) {:.2f}u layer={} r={} via_res={} c={}",
        parasitics->name(grid_node),
        parasitics->name(pin_node),
        block_->dbuToMicrons(pt.getX()),
        block_->dbuToMicrons(pt.getY()),
        block_->dbuToMicrons(wire_length_dbu),
        layer,
        units->resistanceUnit()->asString(res),
        units->resistanceUnit()->asString(via_res),
        units->capacitanceUnit()->asString(cap));

    debugPrint(logger_,
               EST,
               "est_rc",
               1,
               "pin {} -> to grid {}u layer={} r={} via_res={} c={}",
               pin_name,
               static_cast<int>(dbuToMeters(wire_length_dbu) * 1e+6),
               layer,
               units->resistanceUnit()->asString(res),
               units->resistanceUnit()->asString(via_res),
               units->capacitanceUnit()->asString(cap));

    // We could added the via resistor before the segment pi-model
    // but that would require an extra node and the accuracy of all
    // this is not that high.  Instead we just lump them together.
    parasitics->incrCap(pin_node, cap / 2.0);
    parasitics->makeResistor(
        parasitic, resistor_id_++, res + via_res, pin_node, grid_node);
    parasitics->incrCap(grid_node, cap / 2.0);
  } else {
    logger_->warn(EST, 350, "Missing route to pin {}.", pin_name);
  }
}

void MakeWireParasitics::layerRC(int wire_length_dbu,
                                 int layer_id,
                                 sta::Scene* corner,
                                 // Return values.
                                 float& res,
                                 float& cap) const
{
  odb::dbTechLayer* layer = tech_->findRoutingLayer(layer_id);
  double r_per_meter = 0.0;    // ohm/meter
  double cap_per_meter = 0.0;  // F/meter
  estimate_parasitics_->layerRC(layer, corner, r_per_meter, cap_per_meter);

  const float layer_width = block_->dbuToMicrons(layer->getWidth());
  if (r_per_meter == 0.0) {
    const float res_ohm_per_micron = layer->getResistance() / layer_width;
    r_per_meter = 1E+6 * res_ohm_per_micron;  // ohm/meter
  }

  if (cap_per_meter == 0.0) {
    const float cap_pf_per_micron = layer_width * layer->getCapacitance()
                                    + (2 * layer->getEdgeCapacitance());
    cap_per_meter = 1E+6 * 1E-12 * cap_pf_per_micron;  // F/meter
  }

  const float wire_length = dbuToMeters(wire_length_dbu);
  res = r_per_meter * wire_length;
  cap = cap_per_meter * wire_length;
}

void MakeWireParasitics::layerRC(int wire_length_dbu,
                                 int layer_id,
                                 sta::Scene* corner,
                                 odb::dbNet* net,
                                 // Return values.
                                 float& res,
                                 float& cap) const
{
  odb::dbTechLayer* layer = tech_->findRoutingLayer(layer_id);
  double r_per_meter = 0.0;    // ohm/meter
  double cap_per_meter = 0.0;  // F/meter

  estimate_parasitics_->layerRC(layer, corner, r_per_meter, cap_per_meter);

  const float layer_width = block_->dbuToMicrons(layer->getWidth());
  if (r_per_meter == 0.0) {
    const float res_ohm_per_micron = layer->getResistance() / layer_width;
    r_per_meter = 1E+6 * res_ohm_per_micron;  // ohm/meter
  }

  if (cap_per_meter == 0.0) {
    const float cap_pf_per_micron = (layer_width * layer->getCapacitance())
                                    + (2 * layer->getEdgeCapacitance());
    cap_per_meter = 1E+6 * 1E-12 * cap_pf_per_micron;  // F/meter
  }

  // Reduce resistance if the net has NDR with increased width
  if (net->getNonDefaultRule()) {
    odb::dbTechLayerRule* rule = net->getNonDefaultRule()->getLayerRule(layer);
    float ndr_ratio = (float) rule->getWidth() / layer->getWidth();
    r_per_meter /= ndr_ratio;
  }

  const float wire_length = dbuToMeters(wire_length_dbu);
  res = r_per_meter * wire_length;
  cap = cap_per_meter * wire_length;
}

double MakeWireParasitics::dbuToMeters(int dbu) const
{
  return (double) dbu / (tech_->getDbUnitsPerMicron() * 1E+6);
}

sta::ParasiticNode* MakeWireParasitics::ensureParasiticNode(
    int x,
    int y,
    int layer,
    NodeRoutePtMap& node_map,
    sta::Parasitics* parasitics,
    sta::Parasitic* parasitic,
    sta::Net* net) const
{
  grt::RoutePt pin_loc(x, y, layer);
  sta::ParasiticNode* node = node_map[pin_loc];
  if (node == nullptr) {
    node = parasitics->ensureParasiticNode(
        parasitic, net, node_map.size(), network_);
    node_map[pin_loc] = node;
  }
  return node;
}

float MakeWireParasitics::getNetSlack(odb::dbNet* net)
{
  return sta_->slack(net, sta::MinMax::max());
}
////////////////////////////////////////////////////////////////

float MakeWireParasitics::getCutLayerRes(odb::dbTechLayer* cut_layer,
                                         sta::Scene* corner,
                                         int num_cuts) const
{
  double res = 0.0;
  double cap = 0.0;
  estimate_parasitics_->layerRC(cut_layer, corner, res, cap);
  if (res == 0.0) {
    res = cut_layer->getResistance();  // assumes single cut
  }
  return res / num_cuts;
}

}  // namespace est
