/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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

#include "SteinerTree.hh"
#include "db_sta/dbNetwork.hh"
#include "grt/GlobalRouter.h"
#include "rsz/Resizer.hh"
#include "rsz/SpefWriter.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Parasitics.hh"
#include "sta/Report.hh"
#include "sta/Sdc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

using sta::NetConnectedPinIterator;
using sta::NetIterator;
using sta::PinSet;

using odb::dbInst;
using odb::dbMasterType;

////////////////////////////////////////////////////////////////

void Resizer::setLayerRC(dbTechLayer* layer,
                         const Corner* corner,
                         double res,
                         double cap)
{
  if (layer_res_.empty()) {
    int layer_count = db_->getTech()->getLayerCount();
    int corner_count = sta_->corners()->count();
    layer_res_.resize(layer_count);
    layer_cap_.resize(layer_count);
    for (int i = 0; i < layer_count; i++) {
      layer_res_[i].resize(corner_count);
      layer_cap_[i].resize(corner_count);
    }
  }

  layer_res_[layer->getNumber()][corner->index()] = res;
  layer_cap_[layer->getNumber()][corner->index()] = cap;
}

void Resizer::layerRC(dbTechLayer* layer,
                      const Corner* corner,
                      // Return values.
                      double& res,
                      double& cap) const
{
  if (layer_res_.empty()) {
    res = 0.0;
    cap = 0.0;
  } else {
    const int layer_level = layer->getNumber();
    res = layer_res_[layer_level][corner->index()];
    cap = layer_cap_[layer_level][corner->index()];
  }
}

////////////////////////////////////////////////////////////////

void Resizer::setHWireSignalRC(const Corner* corner, double res, double cap)
{
  wire_signal_res_.resize(sta_->corners()->count());
  wire_signal_cap_.resize(sta_->corners()->count());
  wire_signal_res_[corner->index()].h_res = res;
  wire_signal_cap_[corner->index()].h_cap = cap;
}
void Resizer::setVWireSignalRC(const Corner* corner, double res, double cap)
{
  wire_signal_res_.resize(sta_->corners()->count());
  wire_signal_cap_.resize(sta_->corners()->count());
  wire_signal_res_[corner->index()].v_res = res;
  wire_signal_cap_[corner->index()].v_cap = cap;
}

double Resizer::wireSignalResistance(const Corner* corner) const
{
  if (wire_signal_res_.empty()) {
    return 0.0;
  }

  return (wire_signal_res_[corner->index()].h_res
          + wire_signal_res_[corner->index()].v_res)
         / 2;
}

double Resizer::wireSignalHResistance(const Corner* corner) const
{
  if (wire_signal_res_.empty()) {
    return 0.0;
  }
  return wire_signal_res_[corner->index()].h_res;
}

double Resizer::wireSignalVResistance(const Corner* corner) const
{
  if (wire_signal_res_.empty()) {
    return 0.0;
  }
  return wire_signal_res_[corner->index()].v_res;
}

double Resizer::wireSignalCapacitance(const Corner* corner) const
{
  if (wire_signal_cap_.empty()) {
    return 0.0;
  }

  return (wire_signal_cap_[corner->index()].h_cap
          + wire_signal_cap_[corner->index()].v_cap)
         / 2;
}

double Resizer::wireSignalHCapacitance(const Corner* corner) const
{
  if (wire_signal_cap_.empty()) {
    return 0.0;
  }
  return wire_signal_cap_[corner->index()].h_cap;
}

double Resizer::wireSignalVCapacitance(const Corner* corner) const
{
  if (wire_signal_cap_.empty()) {
    return 0.0;
  }
  return wire_signal_cap_[corner->index()].v_cap;
}

void Resizer::wireSignalRC(const Corner* corner,
                           // Return values.
                           double& res,
                           double& cap) const
{
  if (wire_signal_res_.empty()) {
    res = 0.0;
  } else {
    auto resistance = wire_signal_res_[corner->index()];
    res = (resistance.h_res + resistance.v_res) / 2;
  }
  if (wire_signal_cap_.empty()) {
    cap = 0.0;
  } else {
    auto capacitance = wire_signal_cap_[corner->index()];
    cap = (capacitance.h_cap + capacitance.v_cap) / 2;
  }
}

void Resizer::setHWireClkRC(const Corner* corner, double res, double cap)
{
  wire_clk_res_.resize(sta_->corners()->count());
  wire_clk_cap_.resize(sta_->corners()->count());
  wire_clk_res_[corner->index()].h_res = res;
  wire_clk_cap_[corner->index()].h_cap = cap;
}

void Resizer::setVWireClkRC(const Corner* corner, double res, double cap)
{
  wire_clk_res_.resize(sta_->corners()->count());
  wire_clk_cap_.resize(sta_->corners()->count());
  wire_clk_res_[corner->index()].v_res = res;
  wire_clk_cap_[corner->index()].v_cap = cap;
}

double Resizer::wireClkResistance(const Corner* corner) const
{
  if (wire_clk_res_.empty()) {
    return 0.0;
  }
  double h_clk_res = wire_clk_res_[corner->index()].h_res;
  if (h_clk_res > 0.0) {
    return h_clk_res;
  }
  return (wire_clk_res_[corner->index()].h_res
          + wire_clk_res_[corner->index()].v_res)
         / 2;
}

double Resizer::wireClkHResistance(const Corner* corner) const
{
  if (wire_clk_res_.empty()) {
    return 0.0;
  }

  return wire_clk_res_[corner->index()].h_res;
}

double Resizer::wireClkVResistance(const Corner* corner) const
{
  if (wire_clk_res_.empty()) {
    return 0.0;
  }

  return wire_clk_res_[corner->index()].v_res;
}

double Resizer::wireClkCapacitance(const Corner* corner) const
{
  if (wire_clk_cap_.empty()) {
    return 0.0;
  }

  return (wire_clk_cap_[corner->index()].h_cap
          + wire_clk_cap_[corner->index()].v_cap)
         / 2;
}

double Resizer::wireClkHCapacitance(const Corner* corner) const
{
  if (wire_clk_cap_.empty()) {
    return 0.0;
  }

  return wire_clk_cap_[corner->index()].h_cap;
}

double Resizer::wireClkVCapacitance(const Corner* corner) const
{
  if (wire_clk_cap_.empty()) {
    return 0.0;
  }

  return wire_clk_cap_[corner->index()].v_cap;
}

////////////////////////////////////////////////////////////////

void Resizer::ensureParasitics()
{
  estimateParasitics(global_router_->haveRoutes()
                         ? ParasiticsSrc::global_routing
                         : ParasiticsSrc::placement);
}

void Resizer::estimateParasitics(ParasiticsSrc src)
{
  std::map<Corner*, std::ostream*> spef_streams;
  estimateParasitics(src, spef_streams);
}

void Resizer::estimateParasitics(ParasiticsSrc src,
                                 std::map<Corner*, std::ostream*>& spef_streams)
{
  std::unique_ptr<SpefWriter> spef_writer;
  if (!spef_streams.empty()) {
    spef_writer = std::make_unique<SpefWriter>(logger_, sta_, spef_streams);
  }

  switch (src) {
    case ParasiticsSrc::placement:
      estimateWireParasitics(spef_writer.get());
      break;
    case ParasiticsSrc::global_routing:
      global_router_->estimateRC(spef_writer.get());
      parasitics_src_ = ParasiticsSrc::global_routing;
      break;
    case ParasiticsSrc::detailed_routing:
      // TODO: call rcx to extract parasitics and load them to STA
      parasitics_src_ = ParasiticsSrc::detailed_routing;
      break;
    case ParasiticsSrc::none:
      break;
  }
}

bool Resizer::haveEstimatedParasitics() const
{
  return parasitics_src_ != ParasiticsSrc::none;
}

void Resizer::incrementalParasiticsBegin()
{
  switch (parasitics_src_) {
    case ParasiticsSrc::placement:
      break;
    case ParasiticsSrc::global_routing:
    case ParasiticsSrc::detailed_routing:
      // TODO: add IncrementalDRoute
      incr_groute_ = new IncrementalGRoute(global_router_, block_);
      // Don't print verbose messages for incremental routing
      global_router_->setVerbose(false);
      break;
    case ParasiticsSrc::none:
      break;
  }
  parasitics_invalid_.clear();
}

void Resizer::incrementalParasiticsEnd()
{
  switch (parasitics_src_) {
    case ParasiticsSrc::placement:
      break;
    case ParasiticsSrc::global_routing:
    case ParasiticsSrc::detailed_routing:
      // TODO: add IncrementalDRoute
      delete incr_groute_;
      incr_groute_ = nullptr;
      break;
    case ParasiticsSrc::none:
      break;
  }
  parasitics_invalid_.clear();
}

void Resizer::updateParasitics(bool save_guides)
{
  switch (parasitics_src_) {
    case ParasiticsSrc::placement:
      for (const Net* net : parasitics_invalid_) {
        estimateWireParasitic(net);
      }
      parasitics_invalid_.clear();
      break;
    case ParasiticsSrc::global_routing:
    case ParasiticsSrc::detailed_routing: {
      // TODO: update detailed route for modified nets
      incr_groute_->updateRoutes(save_guides);
      for (const Net* net : parasitics_invalid_) {
        global_router_->estimateRC(db_network_->staToDb(net));
      }
      parasitics_invalid_.clear();
      break;
    }
    case ParasiticsSrc::none:
      break;
  }
}

bool Resizer::parasiticsValid() const
{
  return parasitics_invalid_.empty();
}

void Resizer::ensureWireParasitic(const Pin* drvr_pin)
{
  const Net* net = db_network_->dbToSta(db_network_->flatNet(drvr_pin));

  if (net) {
    ensureWireParasitic(drvr_pin, net);
  }
}

void Resizer::ensureWireParasitic(const Pin* drvr_pin, const Net* net)
{
  // Sufficient to check for parasitic for one corner because
  // they are all made at the same time.
  const Corner* corner = sta_->corners()->findCorner(0);
  const ParasiticAnalysisPt* parasitic_ap
      = corner->findParasiticAnalysisPt(max_);
  if (parasitics_invalid_.hasKey(net)
      || parasitics_->findPiElmore(drvr_pin, RiseFall::rise(), parasitic_ap)
             == nullptr) {
    switch (parasitics_src_) {
      case ParasiticsSrc::placement:
        estimateWireParasitic(drvr_pin, net);
        parasitics_invalid_.erase(net);
        break;
      case ParasiticsSrc::global_routing: {
        incr_groute_->updateRoutes();
        global_router_->estimateRC(db_network_->staToDb(net));
        parasitics_invalid_.erase(net);
        break;
      }
      case ParasiticsSrc::detailed_routing:
        // TODO: call incremental drt for the modified net
        break;
      case ParasiticsSrc::none:
        break;
    }
  }
}

////////////////////////////////////////////////////////////////

void Resizer::estimateWireParasitics(SpefWriter* spef_writer)
{
  initBlock();
  if (!wire_signal_cap_.empty()) {
    sta_->ensureClkNetwork();
    // Make separate parasitics for each corner, same for min/max.
    sta_->setParasiticAnalysisPts(true);

    NetIterator* net_iter = network_->netIterator(network_->topInstance());
    while (net_iter->hasNext()) {
      Net* net = net_iter->next();
      estimateWireParasitic(net, spef_writer);
    }
    delete net_iter;

    parasitics_src_ = ParasiticsSrc::placement;
    parasitics_invalid_.clear();
  }
}

void Resizer::estimateWireParasitic(const Net* net, SpefWriter* spef_writer)
{
  PinSet* drivers = network_->drivers(net);
  if (drivers && !drivers->empty()) {
    PinSet::Iterator drvr_iter(drivers);
    const Pin* drvr_pin = drvr_iter.next();
    estimateWireParasitic(drvr_pin, net, spef_writer);
  }
}

void Resizer::estimateWireParasitic(const Pin* drvr_pin,
                                    const Net* net,
                                    SpefWriter* spef_writer)
{
  if (!network_->isPower(net) && !network_->isGround(net)
      && !sta_->isIdealClock(drvr_pin)
      && !db_network_->staToDb(net)->isSpecial()) {
    if (isPadNet(net)) {
      // When an input port drives a pad instance with huge input
      // cap the elmore delay is gigantic. Annotate with zero
      // wire capacitance to prevent wireload model parasitics from being used.
      makePadParasitic(net, spef_writer);
    } else {
      estimateWireParasiticSteiner(drvr_pin, net, spef_writer);
    }
  }
}

bool Resizer::isPadNet(const Net* net) const
{
  const Pin *pin1, *pin2;
  net2Pins(net, pin1, pin2);
  return pin1 && pin2
         && ((network_->isTopLevelPort(pin1) && isPadPin(pin2))
             || (network_->isTopLevelPort(pin2) && isPadPin(pin1)));
}

void Resizer::makePadParasitic(const Net* net, SpefWriter* spef_writer)
{
  const Pin *pin1, *pin2;
  net2Pins(net, pin1, pin2);
  for (Corner* corner : *sta_->corners()) {
    const ParasiticAnalysisPt* parasitics_ap
        = corner->findParasiticAnalysisPt(max_);
    Parasitic* parasitic
        = sta_->makeParasiticNetwork(net, false, parasitics_ap);
    ParasiticNode* n1
        = parasitics_->ensureParasiticNode(parasitic, pin1, network_);
    ParasiticNode* n2
        = parasitics_->ensureParasiticNode(parasitic, pin2, network_);

    // Use a small resistor to keep the connectivity intact.
    parasitics_->makeResistor(parasitic, 1, .001, n1, n2);
    if (spef_writer) {
      spef_writer->writeNet(corner, net, parasitic);
    }
    arc_delay_calc_->reduceParasitic(
        parasitic, net, corner, sta::MinMaxAll::all());
  }
  parasitics_->deleteParasiticNetworks(net);
}

void Resizer::estimateWireParasiticSteiner(const Pin* drvr_pin,
                                           const Net* net,
                                           SpefWriter* spef_writer)
{
  SteinerTree* tree = makeSteinerTree(drvr_pin);
  if (tree) {
    debugPrint(logger_,
               RSZ,
               "resizer_parasitics",
               1,
               "estimate wire {}",
               sdc_network_->pathName(net));
    for (Corner* corner : *sta_->corners()) {
      const ParasiticAnalysisPt* parasitics_ap
          = corner->findParasiticAnalysisPt(max_);
      Parasitic* parasitic
          = sta_->makeParasiticNetwork(net, false, parasitics_ap);
      bool is_clk = global_router_->isNonLeafClock(db_network_->staToDb(net));
      double wire_cap = 0.0;
      double wire_res = 0.0;
      int branch_count = tree->branchCount();
      size_t resistor_id = 1;
      for (int i = 0; i < branch_count; i++) {
        Point pt1, pt2;
        SteinerPt steiner_pt1, steiner_pt2;
        int wire_length_dbu;
        tree->branch(i, pt1, steiner_pt1, pt2, steiner_pt2, wire_length_dbu);
        if (wire_length_dbu) {
          double dx = dbuToMeters(abs(pt1.x() - pt2.x()))
                      / dbuToMeters(wire_length_dbu);
          double dy = dbuToMeters(abs(pt1.y() - pt2.y()))
                      / dbuToMeters(wire_length_dbu);

          if (is_clk) {
            wire_cap = dx * wireClkHCapacitance(corner)
                       + dy * wireClkVCapacitance(corner);
            wire_res = dx * wireClkHResistance(corner)
                       + dy * wireClkVResistance(corner);
          } else {
            wire_cap = dx * wireSignalHCapacitance(corner)
                       + dy * wireSignalVCapacitance(corner);
            wire_res = dx * wireSignalHResistance(corner)
                       + dy * wireSignalVResistance(corner);
          }
        } else {
          wire_cap = is_clk ? wireClkCapacitance(corner)
                            : wireSignalCapacitance(corner);
          wire_res = is_clk ? wireClkResistance(corner)
                            : wireSignalResistance(corner);
        }
        ParasiticNode* n1 = parasitics_->ensureParasiticNode(
            parasitic, net, steiner_pt1, network_);
        ParasiticNode* n2 = parasitics_->ensureParasiticNode(
            parasitic, net, steiner_pt2, network_);
        if (wire_length_dbu == 0) {
          // Use a small resistor to keep the connectivity intact.
          parasitics_->makeResistor(parasitic, resistor_id++, 1.0e-3, n1, n2);
        } else {
          double length = dbuToMeters(wire_length_dbu);
          double cap = length * wire_cap;
          double res = length * wire_res;
          // Make pi model for the wire.
          debugPrint(logger_,
                     RSZ,
                     "resizer_parasitics",
                     2,
                     " pi {} l={} c2={} rpi={} c1={} {}",
                     parasitics_->name(n1),
                     units_->distanceUnit()->asString(length),
                     units_->capacitanceUnit()->asString(cap / 2.0),
                     units_->resistanceUnit()->asString(res),
                     units_->capacitanceUnit()->asString(cap / 2.0),
                     parasitics_->name(n2));
          parasitics_->incrCap(n1, cap / 2.0);
          parasitics_->makeResistor(parasitic, resistor_id++, res, n1, n2);
          parasitics_->incrCap(n2, cap / 2.0);
        }
        parasiticNodeConnectPins(parasitic, n1, tree, steiner_pt1, resistor_id);
        parasiticNodeConnectPins(parasitic, n2, tree, steiner_pt2, resistor_id);
      }
      if (spef_writer) {
        spef_writer->writeNet(corner, net, parasitic);
      }
      arc_delay_calc_->reduceParasitic(
          parasitic, net, corner, sta::MinMaxAll::all());
    }
    parasitics_->deleteParasiticNetworks(net);
    delete tree;
  }
}

float Resizer::pinCapacitance(const Pin* pin,
                              const DcalcAnalysisPt* dcalc_ap) const
{
  LibertyPort* port = network_->libertyPort(pin);
  if (port) {
    int lib_ap = dcalc_ap->libertyIndex();
    LibertyPort* corner_port = port->cornerPort(lib_ap);
    return corner_port->capacitance();
  }
  return 0.0;
}

float Resizer::totalLoad(SteinerTree* tree) const
{
  if (!tree) {
    return 0;
  }

  SteinerPt top_pt = tree->top();
  SteinerPt drvr_pt = tree->drvrPt();

  if (top_pt == SteinerNull) {
    return 0;
  }

  auto top_loc = tree->location(top_pt);
  auto drvr_loc = tree->location(drvr_pt);
  int length = tree->distance(drvr_pt, top_pt);
  double dx
      = dbuToMeters(std::abs(top_loc.x() - drvr_loc.x())) / dbuToMeters(length);
  double dy
      = dbuToMeters(std::abs(top_loc.y() - drvr_loc.y())) / dbuToMeters(length);

  float load = 0.0, max_load = 0.0;

  debugPrint(logger_, RSZ, "resizer_parasitics", 1, "Steiner totalLoad ");
  // For now we will just look at the worst corner for totalLoad
  for (Corner* corner : *sta_->corners()) {
    double wire_cap = dx * wireSignalHCapacitance(corner)
                      + dy * wireSignalVCapacitance(corner);
    float top_length = dbuToMeters(tree->distance(drvr_pt, top_pt));
    float subtree_load = subtreeLoad(tree, wire_cap, top_pt);
    load = top_length * wire_cap + subtree_load;
    max_load = std::max(max_load, load);
  }
  return max_load;
}

float Resizer::subtreeLoad(SteinerTree* tree,
                           float cap_per_micron,
                           SteinerPt pt) const
{
  if (pt == SteinerNull) {
    return 0;
  }
  SteinerPt left_pt = tree->left(pt);
  SteinerPt right_pt = tree->right(pt);

  if ((left_pt == SteinerNull) && (right_pt == SteinerNull)) {
    return (this->pinCapacitance(tree->pin(pt), tgt_slew_dcalc_ap_));
  }

  float left_cap = 0;
  float right_cap = 0;

  if (left_pt != SteinerNull) {
    const float left_length = dbuToMeters(tree->distance(pt, left_pt));
    left_cap = subtreeLoad(tree, cap_per_micron, left_pt)
               + (left_length * cap_per_micron);
  }
  if (right_pt != SteinerNull) {
    const float right_length = dbuToMeters(tree->distance(pt, right_pt));
    right_cap = subtreeLoad(tree, cap_per_micron, right_pt)
                + (right_length * cap_per_micron);
  }
  return left_cap + right_cap;
}

void Resizer::parasiticNodeConnectPins(Parasitic* parasitic,
                                       ParasiticNode* node,
                                       SteinerTree* tree,
                                       SteinerPt pt,
                                       size_t& resistor_id)
{
  const PinSeq* pins = tree->pins(pt);
  if (pins) {
    for (const Pin* pin : *pins) {
      ParasiticNode* pin_node
          = parasitics_->ensureParasiticNode(parasitic, pin, network_);
      // Use a small resistor to keep the connectivity intact.
      parasitics_->makeResistor(
          parasitic, resistor_id++, 1.0e-3, node, pin_node);
    }
  }
}

void Resizer::net2Pins(const Net* net, const Pin*& pin1, const Pin*& pin2) const
{
  pin1 = nullptr;
  pin2 = nullptr;
  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
  if (pin_iter->hasNext()) {
    pin1 = pin_iter->next();
  }
  if (pin_iter->hasNext()) {
    pin2 = pin_iter->next();
  }
  delete pin_iter;
}

bool Resizer::isPadPin(const Pin* pin) const
{
  Instance* inst = network_->instance(pin);
  return inst && !network_->isTopInstance(inst) && isPad(inst);
}

bool Resizer::isPad(const Instance* inst) const
{
  dbInst* db_inst = db_network_->staToDb(inst);
  const auto type = db_inst->getMaster()->getType().getValue();
  // Use switch so if new types are added we get a compiler warning.
  switch (type) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::CORE_SPACER:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
    case dbMasterType::COVER:
    case dbMasterType::RING:
      return false;
    case dbMasterType::COVER_BUMP:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
      return true;
  }
  // gcc warniing
  return false;
}

void Resizer::parasiticsInvalid(const Net* net)
{
  odb::dbNet* db_net = db_network_->flatNet(net);
  if (haveEstimatedParasitics()) {
    debugPrint(logger_,
               RSZ,
               "resizer_parasitics",
               2,
               "parasitics invalid {}",
               network_->pathName(net));
    parasitics_invalid_.insert(db_network_->dbToSta(db_net));
  }
}

void Resizer::parasiticsInvalid(const dbNet* net)
{
  parasiticsInvalid(db_network_->dbToSta(net));
}

}  // namespace rsz
