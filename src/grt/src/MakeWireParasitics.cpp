/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
  _grouter = grouter;
  _logger = openroad->getLogger();
  _sta = openroad->getSta();
  _tech = openroad->getDb()->getTech();
  _parasitics = _sta->parasitics();
  _corner = _sta->cmdCorner();
  _min_max = sta::MinMax::max();
  _analysisPoint = _corner->findParasiticAnalysisPt(_min_max);

  _network = openroad->getDbNetwork();
  _sta_net = nullptr;
  _parasitic = nullptr;
  _node_id = 0;
}

void MakeWireParasitics::estimateParasitcs(odb::dbNet* net,
                                           std::vector<Pin>& pins,
                                           std::vector<GSegment>& routes)
{
  debugPrint(_logger, GRT, "est_rc", 1, "net {}", net->getConstName());
  _sta_net = _network->dbToSta(net);
  _node_id = 0;
  _node_map.clear();

  _parasitic
      = _parasitics->makeParasiticNetwork(_sta_net, false, _analysisPoint);
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
        = _parasitics->ensureParasiticNode(_parasitic, sta_pin);
    RoutePt route_pt = routePt(pin);
    _node_map[route_pt] = pin_node;
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
    return _network->dbToSta(pin.getBTerm());
  else
    return _network->dbToSta(pin.getITerm());
}

void MakeWireParasitics::makeRouteParasitics(odb::dbNet* net,
                                             std::vector<GSegment>& routes)
{
  for (GSegment& route : routes) {
    sta::ParasiticNode* n1
        = ensureParasiticNode(route.initX, route.initY, route.initLayer);
    sta::ParasiticNode* n2
        = ensureParasiticNode(route.finalX, route.finalY, route.finalLayer);
    int wire_length_dbu
        = abs(route.initX - route.finalX) + abs(route.initY - route.finalY);
    sta::Units* units = _sta->units();
    float res = 0.0;
    float cap = 0.0;
    if (wire_length_dbu == 0) {
      // via
      int lower_layer = min(route.initLayer, route.finalLayer);
      odb::dbTechLayer* cut_layer
          = _tech->findRoutingLayer(lower_layer)->getUpperLayer();
      res = cut_layer->getResistance();  // assumes single cut
      cap = 0.0;
      debugPrint(_logger,
                 GRT,
                 "est_rc",
                 1,
                 "{} -> {} via r={}",
                 _parasitics->name(n1),
                 _parasitics->name(n2),
                 units->resistanceUnit()->asString(res));
    } else if (route.initLayer == route.finalLayer) {
      layerRC(wire_length_dbu, route.initLayer, res, cap);
      debugPrint(_logger,
                 GRT,
                 "est_rc",
                 1,
                 "{} -> {} {}u layer={} r={} c={}",
                 _parasitics->name(n1),
                 _parasitics->name(n2),
                 static_cast<int>(dbuToMeters(wire_length_dbu) * 1e+6),
                 route.initLayer,
                 units->resistanceUnit()->asString(res),
                 units->capacitanceUnit()->asString(cap));
    } else
      _logger->warn(GRT, 25, "non wire or via route found on net {}.",
                    net->getConstName());
    _parasitics->incrCap(n1, cap / 2.0, _analysisPoint);
    _parasitics->makeResistor(nullptr, n1, n2, res, _analysisPoint);
    _parasitics->incrCap(n2, cap / 2.0, _analysisPoint);
  }
}

void MakeWireParasitics::makeParasiticsToGrid(std::vector<Pin>& pins)
{
  for (Pin& pin : pins) {
    RoutePt route_pt = routePt(pin);
    sta::ParasiticNode* pin_node = _node_map[route_pt];
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
  sta::ParasiticNode* grid_node = _node_map[route_pt];

  if (grid_node) {
    const odb::Point& pt = pin.getPosition();
    int wire_length_dbu
        = abs(pt.getX() - grid_pt.getX()) + abs(pt.getY() - grid_pt.getY());
    float res, cap;
    layerRC(wire_length_dbu, layer, res, cap);

    _parasitics->incrCap(pin_node, cap / 2.0, _analysisPoint);
    _parasitics->makeResistor(
        nullptr, pin_node, grid_node, res, _analysisPoint);
    _parasitics->incrCap(grid_node, cap / 2.0, _analysisPoint);
  } else {
    _logger->warn(GRT, 26, "missing route to pin {}.", pin.getName());
  }
}

void MakeWireParasitics::layerRC(int wire_length_dbu,
                                 int layer_id,
                                 // Return values.
                                 float& res,
                                 float& cap)
{
  odb::dbTechLayer* layer = _tech->findRoutingLayer(layer_id);
  float layerWidth = _grouter->dbuToMicrons(layer->getWidth());
  float resOhmPerMicron = layer->getResistance() / layerWidth;
  float capPfPerMicron
      = layerWidth * layer->getCapacitance() + 2 * layer->getEdgeCapacitance();

  float r_per_meter = 1E+6 * resOhmPerMicron;           // ohm/meter
  float cap_per_meter = 1E+6 * 1E-12 * capPfPerMicron;  // F/meter

  float wire_length = dbuToMeters(wire_length_dbu);
  res = r_per_meter * wire_length;
  cap = cap_per_meter * wire_length;
}

double MakeWireParasitics::dbuToMeters(int dbu)
{
  return (double) dbu / (_tech->getDbUnitsPerMicron() * 1E+6);
}

sta::ParasiticNode* MakeWireParasitics::ensureParasiticNode(int x,
                                                            int y,
                                                            int layer)
{
  RoutePt pin_loc(x, y, layer);
  sta::ParasiticNode* node = _node_map[pin_loc];
  if (node == nullptr) {
    node = _parasitics->ensureParasiticNode(_parasitic, _sta_net, _node_id++);
    _node_map[pin_loc] = node;
  }
  return node;
}

void MakeWireParasitics::reduceParasiticNetwork()
{
  sta::Sdc* sdc = _sta->sdc();
  sta::OperatingConditions* op_cond = sdc->operatingConditions(_min_max);
  sta::ReducedParasiticType reduce_to
      = _sta->arcDelayCalc()->reducedParasiticType();
  _parasitics->reduceTo(_parasitic,
                        _sta_net,
                        reduce_to,
                        op_cond,
                        _corner,
                        _min_max,
                        _analysisPoint);
  _parasitics->deleteParasiticNetwork(_sta_net, _analysisPoint);
}

}  // namespace grt
