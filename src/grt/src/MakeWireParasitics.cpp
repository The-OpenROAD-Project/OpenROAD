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

MakeWireParasitics::MakeWireParasitics(ord::OpenRoad* openroad,
                                       GlobalRouter* grouter)
{
  grouter_ = grouter;
  logger_ = openroad->getLogger();
  sta_ = openroad->getSta();
  tech_ = openroad->getDb()->getTech();
  parasitics_ = sta_->parasitics();
  corner_ = sta_->cmdCorner();
  min_max_ = sta::MinMax::max();
  analysis_point_ = corner_->findParasiticAnalysisPt(min_max_);

  network_ = openroad->getDbNetwork();
  sta_net_ = nullptr;
  parasitic_ = nullptr;
  node_id_ = 0;
}

void MakeWireParasitics::estimateParasitcs(odb::dbNet* net,
                                           std::vector<Pin>& pins,
                                           std::vector<GSegment>& routes)
{
  debugPrint(logger_, GRT, "est_rc", 1, "net {}", net->getConstName());
  sta_net_ = network_->dbToSta(net);
  node_id_ = 0;
  node_map_.clear();

  parasitic_
      = parasitics_->makeParasiticNetwork(sta_net_, false, analysis_point_);
  makePinRoutePts(pins);
  makeRouteParasitics(net, routes);
  makeParasiticsToGrid(pins);
  reduceParasiticNetwork();
}

void MakeWireParasitics::makePinRoutePts(std::vector<Pin>& pins)
{
  for (Pin& pin : pins) {
    sta::Pin* sta_pin = staPin(pin);
    sta::ParasiticNode* pin_node
        = parasitics_->ensureParasiticNode(parasitic_, sta_pin);
    RoutePt route_pt = routePt(pin);
    node_map_[route_pt] = pin_node;
  }
}

RoutePt MakeWireParasitics::routePt(Pin& pin)
{
  const odb::Point& pt = pin.getPosition();
  int layer = pin.getTopLayer();
  return RoutePt(pt.getX(), pt.getY(), layer);
}

sta::Pin* MakeWireParasitics::staPin(Pin& pin)
{
  if (pin.isPort())
    return network_->dbToSta(pin.getBTerm());
  else
    return network_->dbToSta(pin.getITerm());
}

void MakeWireParasitics::makeRouteParasitics(odb::dbNet* net,
                                             std::vector<GSegment>& routes)
{
  for (GSegment& route : routes) {
    sta::ParasiticNode* n1
        = ensureParasiticNode(route.init_x, route.init_y, route.init_layer);
    sta::ParasiticNode* n2
        = ensureParasiticNode(route.final_x, route.final_y, route.final_layer);
    int wire_length_dbu
        = abs(route.init_x - route.final_x) + abs(route.init_y - route.final_y);
    sta::Units* units = sta_->units();
    float res = 0.0;
    float cap = 0.0;
    if (wire_length_dbu == 0) {
      // via
      int lower_layer = min(route.init_layer, route.final_layer);
      odb::dbTechLayer* cut_layer
          = tech_->findRoutingLayer(lower_layer)->getUpperLayer();
      res = cut_layer->getResistance();  // assumes single cut
      cap = 0.0;
      debugPrint(logger_,
                 GRT,
                 "est_rc",
                 1,
                 "{} -> {} via r={}",
                 parasitics_->name(n1),
                 parasitics_->name(n2),
                 units->resistanceUnit()->asString(res));
    } else if (route.init_layer == route.final_layer) {
      layerRC(wire_length_dbu, route.init_layer, res, cap);
      debugPrint(logger_,
                 GRT,
                 "est_rc",
                 1,
                 "{} -> {} {}u layer={} r={} c={}",
                 parasitics_->name(n1),
                 parasitics_->name(n2),
                 static_cast<int>(dbuToMeters(wire_length_dbu) * 1e+6),
                 route.init_layer,
                 units->resistanceUnit()->asString(res),
                 units->capacitanceUnit()->asString(cap));
    } else
      logger_->warn(GRT, 25, "non wire or via route found on net {}.",
                    net->getConstName());
    parasitics_->incrCap(n1, cap / 2.0, analysis_point_);
    parasitics_->makeResistor(nullptr, n1, n2, res, analysis_point_);
    parasitics_->incrCap(n2, cap / 2.0, analysis_point_);
  }
}

void MakeWireParasitics::makeParasiticsToGrid(std::vector<Pin>& pins)
{
  for (Pin& pin : pins) {
    RoutePt route_pt = routePt(pin);
    sta::ParasiticNode* pin_node = node_map_[route_pt];
    makeParasiticsToGrid(pin, pin_node);
  }
}

// Make parasitics for the wire from the pin to the grid location of the pin.
void MakeWireParasitics::makeParasiticsToGrid(Pin& pin,
                                              sta::ParasiticNode* pin_node)
{
  const odb::Point& grid_pt = pin.getOnGridPosition();
  int layer = pin.getTopLayer();
  RoutePt route_pt(grid_pt.getX(), grid_pt.getY(), layer);
  sta::ParasiticNode* grid_node = node_map_[route_pt];

  if (grid_node) {
    const odb::Point& pt = pin.getPosition();
    int wire_length_dbu
        = abs(pt.getX() - grid_pt.getX()) + abs(pt.getY() - grid_pt.getY());
    float res, cap;
    layerRC(wire_length_dbu, layer, res, cap);

    parasitics_->incrCap(pin_node, cap / 2.0, analysis_point_);
    parasitics_->makeResistor(
        nullptr, pin_node, grid_node, res, analysis_point_);
    parasitics_->incrCap(grid_node, cap / 2.0, analysis_point_);
  } else {
    logger_->warn(GRT, 26, "missing route to pin {}.", pin.getName());
  }
}

void MakeWireParasitics::layerRC(int wire_length_dbu,
                                 int layer_id,
                                 // Return values.
                                 float& res,
                                 float& cap)
{
  odb::dbTechLayer* layer = tech_->findRoutingLayer(layer_id);
  float layer_width = grouter_->dbuToMicrons(layer->getWidth());
  float res_ohm_per_micron = layer->getResistance() / layer_width;
  float cap_pf_per_micron
      = layer_width * layer->getCapacitance() + 2 * layer->getEdgeCapacitance();

  float r_per_meter = 1E+6 * res_ohm_per_micron;           // ohm/meter
  float cap_per_meter = 1E+6 * 1E-12 * cap_pf_per_micron;  // F/meter

  float wire_length = dbuToMeters(wire_length_dbu);
  res = r_per_meter * wire_length;
  cap = cap_per_meter * wire_length;
}

double MakeWireParasitics::dbuToMeters(int dbu)
{
  return (double) dbu / (tech_->getDbUnitsPerMicron() * 1E+6);
}

sta::ParasiticNode* MakeWireParasitics::ensureParasiticNode(int x,
                                                            int y,
                                                            int layer)
{
  RoutePt pin_loc(x, y, layer);
  sta::ParasiticNode* node = node_map_[pin_loc];
  if (node == nullptr) {
    node = parasitics_->ensureParasiticNode(parasitic_, sta_net_, node_id_++);
    node_map_[pin_loc] = node;
  }
  return node;
}

void MakeWireParasitics::reduceParasiticNetwork()
{
  sta::Sdc* sdc = sta_->sdc();
  sta::OperatingConditions* op_cond = sdc->operatingConditions(min_max_);
  sta::ReducedParasiticType reduce_to
      = sta_->arcDelayCalc()->reducedParasiticType();
  parasitics_->reduceTo(parasitic_,
                        sta_net_,
                        reduce_to,
                        op_cond,
                        corner_,
                        min_max_,
                        analysis_point_);
  parasitics_->deleteParasiticNetwork(sta_net_, analysis_point_);
}

}  // namespace grt
