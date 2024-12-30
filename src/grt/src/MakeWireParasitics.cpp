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

#include "MakeWireParasitics.h"

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "rsz/SpefWriter.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Corner.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Sdc.hh"
#include "sta/StaState.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace grt {

using utl::GRT;

using std::abs;
using std::min;

MakeWireParasitics::MakeWireParasitics(utl::Logger* logger,
                                       rsz::Resizer* resizer,
                                       sta::dbSta* sta,
                                       odb::dbTech* tech,
                                       odb::dbBlock* block,
                                       GlobalRouter* grouter)
    : grouter_(grouter),
      tech_(tech),
      block_(block),
      logger_(logger),
      resizer_(resizer),
      sta_(sta),
      network_(sta_->getDbNetwork()),
      parasitics_(sta_->parasitics()),
      arc_delay_calc_(sta_->arcDelayCalc()),
      min_max_(sta::MinMax::max()),
      resistor_id_(1)
{
}

void MakeWireParasitics::estimateParasitcs(odb::dbNet* net,
                                           std::vector<Pin>& pins,
                                           GRoute& route,
                                           rsz::SpefWriter* spef_writer)
{
  debugPrint(logger_, GRT, "est_rc", 1, "net {}", net->getConstName());
  if (logger_->debugCheck(GRT, "est_rc", 2)) {
    for (GSegment& segment : route) {
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

  for (sta::Corner* corner : *sta_->corners()) {
    NodeRoutePtMap node_map;

    sta::ParasiticAnalysisPt* analysis_point
        = corner->findParasiticAnalysisPt(min_max_);
    sta::Parasitic* parasitic
        = parasitics_->makeParasiticNetwork(sta_net, false, analysis_point);
    makeRouteParasitics(
        net, route, sta_net, corner, analysis_point, parasitic, node_map);
    makeParasiticsToPins(
        pins, net, node_map, corner, analysis_point, parasitic);

    if (spef_writer) {
      spef_writer->writeNet(corner, sta_net, parasitic);
    }

    arc_delay_calc_->reduceParasitic(
        parasitic, sta_net, corner, sta::MinMaxAll::all());
  }
  parasitics_->deleteParasiticNetworks(sta_net);
}

void MakeWireParasitics::estimateParasitcs(odb::dbNet* net, GRoute& route)
{
  debugPrint(logger_, GRT, "est_rc", 1, "net {}", net->getConstName());
  if (logger_->debugCheck(GRT, "est_rc", 2)) {
    for (GSegment& segment : route) {
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
  grt::Net* grt_net = grouter_->getNet(net);

  for (sta::Corner* corner : *sta_->corners()) {
    NodeRoutePtMap node_map;

    sta::ParasiticAnalysisPt* analysis_point
        = corner->findParasiticAnalysisPt(min_max_);
    sta::Parasitic* parasitic
        = parasitics_->makeParasiticNetwork(sta_net, false, analysis_point);
    makeRouteParasitics(
        net, route, sta_net, corner, analysis_point, parasitic, node_map);
    makePartialParasiticsToPins(
        grt_net->getPins(), node_map, corner, analysis_point, parasitic, net);
    arc_delay_calc_->reduceParasitic(
        parasitic, sta_net, corner, sta::MinMaxAll::all());
  }

  parasitics_->deleteParasiticNetworks(sta_net);
}

void MakeWireParasitics::clearParasitics()
{
  // Remove any existing parasitics.
  sta_->deleteParasitics();

  // Make separate parasitics for each corner.
  sta_->setParasiticAnalysisPts(true);
}

sta::Pin* MakeWireParasitics::staPin(Pin& pin) const
{
  if (pin.isPort())
    return network_->dbToSta(pin.getBTerm());
  else
    return network_->dbToSta(pin.getITerm());
}

void MakeWireParasitics::makeRouteParasitics(
    odb::dbNet* net,
    GRoute& route,
    sta::Net* sta_net,
    sta::Corner* corner,
    sta::ParasiticAnalysisPt* analysis_point,
    sta::Parasitic* parasitic,
    NodeRoutePtMap& node_map)
{
  const int min_routing_layer = grouter_->getMinRoutingLayer();

  size_t resistor_id_ = 1;
  for (GSegment& segment : route) {
    const int wire_length_dbu = segment.length();

    const int init_layer = segment.init_layer;
    bool is_valid_layer = init_layer >= min_routing_layer;
    sta::ParasiticNode* n1 = is_valid_layer
                                 ? ensureParasiticNode(segment.init_x,
                                                       segment.init_y,
                                                       init_layer,
                                                       node_map,
                                                       parasitic,
                                                       sta_net)
                                 : nullptr;

    const int final_layer = segment.final_layer;
    is_valid_layer = final_layer >= min_routing_layer;
    sta::ParasiticNode* n2 = is_valid_layer
                                 ? ensureParasiticNode(segment.final_x,
                                                       segment.final_y,
                                                       final_layer,
                                                       node_map,
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
                 GRT,
                 "est_rc",
                 1,
                 "{} -> {} via {}-{} r={}",
                 parasitics_->name(n1),
                 parasitics_->name(n2),
                 segment.init_layer,
                 segment.final_layer,
                 units->resistanceUnit()->asString(res));
    } else if (segment.init_layer == segment.final_layer) {
      layerRC(wire_length_dbu, segment.init_layer, corner, res, cap);
      debugPrint(logger_,
                 GRT,
                 "est_rc",
                 1,
                 "{} -> {} {:.2f}u layer={} r={} c={}",
                 parasitics_->name(n1),
                 parasitics_->name(n2),
                 dbuToMeters(wire_length_dbu) * 1e+6,
                 segment.init_layer,
                 units->resistanceUnit()->asString(res),
                 units->capacitanceUnit()->asString(cap));
    } else
      logger_->warn(GRT,
                    25,
                    "Non wire or via route found on net {}.",
                    net->getConstName());
    parasitics_->incrCap(n1, cap / 2.0);
    parasitics_->makeResistor(parasitic, resistor_id_++, res, n1, n2);
    parasitics_->incrCap(n2, cap / 2.0);
  }
}

void MakeWireParasitics::makeParasiticsToPins(
    std::vector<Pin>& pins,
    odb::dbNet* net,
    NodeRoutePtMap& node_map,
    sta::Corner* corner,
    sta::ParasiticAnalysisPt* analysis_point,
    sta::Parasitic* parasitic)
{
  for (Pin& pin : pins) {
    makeParasiticsToPin(pin, net, node_map, corner, analysis_point, parasitic);
  }
}

// Make parasitics for the wire from the pin to the grid location of the pin.
void MakeWireParasitics::makeParasiticsToPin(
    Pin& pin,
    odb::dbNet* net,
    NodeRoutePtMap& node_map,
    sta::Corner* corner,
    sta::ParasiticAnalysisPt* analysis_point,
    sta::Parasitic* parasitic)
{
  sta::Pin* sta_pin = staPin(pin);
  sta::ParasiticNode* pin_node
      = parasitics_->ensureParasiticNode(parasitic, sta_pin, network_);

  odb::Point pt = pin.getPosition();
  odb::Point grid_pt = pin.getOnGridPosition();

  // Use the route layer above the pin layer if there is a via
  // to the pin.
  int layer = pin.getConnectionLayer() + 1;
  RoutePt grid_route(grid_pt.getX(), grid_pt.getY(), layer);
  sta::ParasiticNode* grid_node = node_map[grid_route];
  float via_res = 0;

  // Use the pin layer for the connection.
  if (grid_node == nullptr) {
    layer--;
    grid_route = RoutePt(grid_pt.getX(), grid_pt.getY(), layer);
    grid_node = node_map[grid_route];
  } else {
    odb::dbTechLayer* cut_layer
        = tech_->findRoutingLayer(layer)->getLowerLayer();
    via_res = getCutLayerRes(cut_layer, corner);
  }

  if (grid_node) {
    // Make wire from pin to gcell center on pin layer.
    int wire_length_dbu
        = abs(pt.getX() - grid_pt.getX()) + abs(pt.getY() - grid_pt.getY());
    float res, cap;
    layerRC(wire_length_dbu, layer, corner, res, cap);
    sta::Units* units = sta_->units();
    debugPrint(
        logger_,
        GRT,
        "est_rc",
        1,
        "{} -> {} ({:.2f}, {:.2f}) {:.2f}u layer={} r={} via_res={} c={}",
        parasitics_->name(grid_node),
        parasitics_->name(pin_node),
        block_->dbuToMicrons(pt.getX()),
        block_->dbuToMicrons(pt.getY()),
        block_->dbuToMicrons(wire_length_dbu),
        layer,
        units->resistanceUnit()->asString(res),
        units->resistanceUnit()->asString(via_res),
        units->capacitanceUnit()->asString(cap));

    debugPrint(logger_,
               GRT,
               "est_rc",
               1,
               "pin {} -> to grid {}u layer={} r={} via_res={} c={}",
               pin.getName(),
               static_cast<int>(dbuToMeters(wire_length_dbu) * 1e+6),
               layer,
               units->resistanceUnit()->asString(res),
               units->resistanceUnit()->asString(via_res),
               units->capacitanceUnit()->asString(cap));

    // We could added the via resistor before the segment pi-model
    // but that would require an extra node and the accuracy of all
    // this is not that high.  Instead we just lump them together.
    parasitics_->incrCap(pin_node, cap / 2.0);
    parasitics_->makeResistor(
        parasitic, resistor_id_++, res + via_res, pin_node, grid_node);
    parasitics_->incrCap(grid_node, cap / 2.0);
  } else {
    logger_->warn(GRT,
                  26,
                  "Missing route to pin {} in net {}.",
                  pin.getName(),
                  net->getName());
  }
}

void MakeWireParasitics::makePartialParasiticsToPins(
    std::vector<Pin>& pins,
    NodeRoutePtMap& node_map,
    sta::Corner* corner,
    sta::ParasiticAnalysisPt* analysis_point,
    sta::Parasitic* parasitic,
    odb::dbNet* net)
{
  for (Pin& pin : pins) {
    makePartialParasiticsToPin(
        pin, node_map, corner, analysis_point, parasitic, net);
  }
}

// Make parasitics for the wire from the pin to the grid location of the pin.
void MakeWireParasitics::makePartialParasiticsToPin(
    Pin& pin,
    NodeRoutePtMap& node_map,
    sta::Corner* corner,
    sta::ParasiticAnalysisPt* analysis_point,
    sta::Parasitic* parasitic,
    odb::dbNet* net)
{
  sta::Pin* sta_pin = staPin(pin);
  sta::ParasiticNode* pin_node
      = parasitics_->ensureParasiticNode(parasitic, sta_pin, network_);

  odb::Point pt = pin.getPosition();
  odb::Point grid_pt = pin.getOnGridPosition();

  // Use the route layer above the pin layer if there is a via
  // to the pin.
  int net_max_layer;
  int net_min_layer;
  grouter_->getNetLayerRange(net, net_min_layer, net_max_layer);
  int layer = net_min_layer + 1;
  RoutePt grid_route(grid_pt.getX(), grid_pt.getY(), layer);
  sta::ParasiticNode* grid_node = node_map[grid_route];
  float via_res = 0;

  // Use the pin layer for the connection.
  if (grid_node == nullptr) {
    layer--;
    grid_route = RoutePt(grid_pt.getX(), grid_pt.getY(), layer);
    grid_node = node_map[grid_route];
  } else {
    odb::dbTechLayer* cut_layer
        = tech_->findRoutingLayer(layer)->getLowerLayer();
    via_res = getCutLayerRes(cut_layer, corner);
  }

  if (grid_node) {
    // Make wire from pin to gcell center on pin layer.
    int wire_length_dbu
        = abs(pt.getX() - grid_pt.getX()) + abs(pt.getY() - grid_pt.getY());
    float res, cap;
    layerRC(wire_length_dbu, layer, corner, res, cap);
    sta::Units* units = sta_->units();
    debugPrint(
        logger_,
        GRT,
        "est_rc",
        1,
        "{} -> {} ({:.2f}, {:.2f}) {:.2f}u layer={} r={} via_res={} c={}",
        parasitics_->name(grid_node),
        parasitics_->name(pin_node),
        block_->dbuToMicrons(pt.getX()),
        block_->dbuToMicrons(pt.getY()),
        block_->dbuToMicrons(wire_length_dbu),
        layer,
        units->resistanceUnit()->asString(res),
        units->resistanceUnit()->asString(via_res),
        units->capacitanceUnit()->asString(cap));

    debugPrint(logger_,
               GRT,
               "est_rc",
               1,
               "pin {} -> to grid {}u layer={} r={} via_res={} c={}",
               pin.getName(),
               static_cast<int>(dbuToMeters(wire_length_dbu) * 1e+6),
               layer,
               units->resistanceUnit()->asString(res),
               units->resistanceUnit()->asString(via_res),
               units->capacitanceUnit()->asString(cap));

    // We could added the via resistor before the segment pi-model
    // but that would require an extra node and the accuracy of all
    // this is not that high.  Instead we just lump them together.
    parasitics_->incrCap(pin_node, cap / 2.0);
    parasitics_->makeResistor(
        parasitic, resistor_id_++, res + via_res, pin_node, grid_node);
    parasitics_->incrCap(grid_node, cap / 2.0);
  } else {
    logger_->warn(GRT, 350, "Missing route to pin {}.", pin.getName());
  }
}

void MakeWireParasitics::layerRC(int wire_length_dbu,
                                 int layer_id,
                                 sta::Corner* corner,
                                 // Return values.
                                 float& res,
                                 float& cap) const
{
  odb::dbTechLayer* layer = tech_->findRoutingLayer(layer_id);
  double r_per_meter = 0.0;    // ohm/meter
  double cap_per_meter = 0.0;  // F/meter
  resizer_->layerRC(layer, corner, r_per_meter, cap_per_meter);

  const float layer_width = block_->dbuToMicrons(layer->getWidth());
  if (r_per_meter == 0.0) {
    const float res_ohm_per_micron = layer->getResistance() / layer_width;
    r_per_meter = 1E+6 * res_ohm_per_micron;  // ohm/meter
  }

  if (cap_per_meter == 0.0) {
    const float cap_pf_per_micron = layer_width * layer->getCapacitance()
                                    + 2 * layer->getEdgeCapacitance();
    cap_per_meter = 1E+6 * 1E-12 * cap_pf_per_micron;  // F/meter
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
    sta::Parasitic* parasitic,
    sta::Net* net) const
{
  RoutePt pin_loc(x, y, layer);
  sta::ParasiticNode* node = node_map[pin_loc];
  if (node == nullptr) {
    node = parasitics_->ensureParasiticNode(
        parasitic, net, node_map.size(), network_);
    node_map[pin_loc] = node;
  }
  return node;
}

float MakeWireParasitics::getNetSlack(odb::dbNet* net)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Net* sta_net = network->dbToSta(net);
  float slack = sta_->netSlack(sta_net, sta::MinMax::max());
  return slack;
}
////////////////////////////////////////////////////////////////

std::vector<int> MakeWireParasitics::routeLayerLengths(odb::dbNet* db_net) const
{
  NetRouteMap& routes = grouter_->getRoutes();
  std::vector<int> layer_lengths(grouter_->getMaxRoutingLayer() + 1);
  if (!db_net->getSigType().isSupply()) {
    GRoute& route = routes[db_net];
    std::set<RoutePt> route_pts;
    for (GSegment& segment : route) {
      if (segment.isVia()) {
        route_pts.insert(
            RoutePt(segment.init_x, segment.init_y, segment.init_layer));
        route_pts.insert(
            RoutePt(segment.final_x, segment.final_y, segment.final_layer));
      } else {
        int layer = segment.init_layer;
        layer_lengths[layer] += segment.length();
        route_pts.insert(RoutePt(segment.init_x, segment.init_y, layer));
        route_pts.insert(RoutePt(segment.final_x, segment.final_y, layer));
      }
    }
    // Mimic MakeWireParasitics::makeParasiticsToPin functionality.
    Net* net = grouter_->getNet(db_net);
    for (Pin& pin : net->getPins()) {
      int layer = pin.getConnectionLayer() + 1;
      odb::Point grid_pt = pin.getOnGridPosition();
      odb::Point pt = pin.getPosition();

      std::vector<std::pair<odb::Point, odb::Point>> ap_positions;
      bool has_access_points
          = grouter_->findPinAccessPointPositions(pin, ap_positions);
      if (has_access_points) {
        auto ap_position = ap_positions.front();
        pt = ap_position.first;
        grid_pt = ap_position.second;
      }

      RoutePt grid_route(grid_pt.getX(), grid_pt.getY(), layer);
      auto pt_itr = route_pts.find(grid_route);
      if (pt_itr == route_pts.end())
        layer--;
      int wire_length_dbu
          = abs(pt.getX() - grid_pt.getX()) + abs(pt.getY() - grid_pt.getY());
      layer_lengths[layer] += wire_length_dbu;
    }
  }
  return layer_lengths;
}

float MakeWireParasitics::getCutLayerRes(odb::dbTechLayer* cut_layer,
                                         sta::Corner* corner,
                                         int num_cuts) const
{
  double res = 0.0;
  double cap = 0.0;
  resizer_->layerRC(cut_layer, corner, res, cap);
  if (res == 0.0) {
    res = cut_layer->getResistance();  // assumes single cut
  }
  return res / num_cuts;
}

}  // namespace grt
