/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, The Regents of the University of California
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
#include "GateCloner.hh"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

#include "db_sta/dbNetwork.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/Parasitics.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PathVertex.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"

namespace rsz {

using sta::Instance;
using sta::InstancePinIterator;
using sta::PathAnalysisPt;
using utl::Logger;
using utl::RSZ;
using sta::PathExpanded;

GateCloner::GateCloner(Resizer *resizer)
{
  resizer_ = resizer;
  sta_ = resizer->sta_;
  logger_ = resizer->logger();
  db_network_ = resizer->db_network_;
  network_ = resizer->network();
  min_max_ = sta::MinMax::max();
  sta_->checkCapacitanceLimitPreamble();
  capacitance_limits_initialized_ = true;
}

std::vector<Pin *>
GateCloner::levelDriverPins(bool reverse,
                            std::unordered_set<Pin *> filter_pins) const
{
  sta_->ensureGraph();
  sta_->ensureLevelized();

  std::vector<Pin *> terms;
  std::vector<Vertex*>       vertices;
  sta::VertexIterator        itr(network_->graph());
  while (itr.hasNext()) {
    Vertex* vtx = itr.next();
    if (vtx->isDriver(network_))
      vertices.push_back(vtx);
  }
  std::sort(
      vertices.begin(),
      vertices.end(),
      [=](const Vertex* v1, const Vertex* v2) -> bool {
        return (v1->level() < v2->level())
               || (v1->level() == v2->level()
                   && sta::stringLess(network_->pathName(v1->pin()),
                                      network_->pathName(v2->pin())));
      });
  for (auto& v : vertices) {
    auto pn = v->pin();
    if (!filter_pins.size() || filter_pins.count(pn)) {
      terms.push_back(v->pin());
    }
  }
  if (reverse) {
    std::reverse(terms.begin(), terms.end());
  }
  return terms;
}

bool
GateCloner::isSingleOutputCombinational(Instance* inst) const
{
  if (inst == network_->topInstance()) {
    return false;
  }
  return isSingleOutputCombinational(network_->libertyCell(inst));
}

bool GateCloner::isSingleOutputCombinational(LibertyCell* cell) const
{
  if (!cell) {
    return false;
  }
  auto output_pins = libraryOutputPins(cell);
  return (output_pins.size() == 1 && isCombinational(cell));
}

bool GateCloner::isCombinational(Instance* inst) const
{
  if (inst == network_->topInstance()) {
    return false;
  }
  return isCombinational(network_->libertyCell(inst));
}

bool GateCloner::isCombinational(LibertyCell* cell) const
{
  if (!cell) {
    return false;
  }
  return (!cell->isClockGate() && !cell->isPad() && !cell->isMacro()
          && !cell->hasSequentials());
}

std::vector<sta::LibertyPort*> GateCloner::libraryPins(Instance* inst) const
{
  return libraryPins(network_->libertyCell(inst));
}

std::vector<sta::LibertyPort*> GateCloner::libraryPins(LibertyCell* cell) const
{
  std::vector<sta::LibertyPort*> pins;
  sta::LibertyCellPortIterator itr(cell);
  while (itr.hasNext()) {
    auto port = itr.next();
    pins.push_back(port);
  }
  return pins;
}

std::vector<sta::LibertyPort*>
GateCloner::libraryInputPins(LibertyCell* cell) const
{
  auto pins = libraryPins(cell);
  for (auto it = pins.begin(); it != pins.end(); it++) {
    if (!((*it)->direction()->isAnyInput())) {
      it = pins.erase(it);
      it--;
    }
  }
  return pins;
}

std::vector<sta::LibertyPort*>
GateCloner::libraryOutputPins(LibertyCell* cell) const
{
  auto pins = libraryPins(cell);
  for (auto it = pins.begin(); it != pins.end(); it++) {
    if (!((*it)->direction()->isAnyOutput())) {
      it = pins.erase(it);
      it--;
    }
  }
  return pins;
}

std::vector<const Pin*> GateCloner::pins(Net* net) const
{
  std::vector<const Pin*> terms;
  auto pin_iter = network_->pinIterator(net);
  while (pin_iter->hasNext()) {
    auto pin = pin_iter->next();
    terms.push_back(pin);
  }
  return terms;
}

std::vector<const Pin*> GateCloner::pins(Instance* inst) const
{
  std::vector<const Pin*> terms;
  auto pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    auto pin = pin_iter->next();
    terms.push_back(pin);
  }
  return terms;
}

std::vector<const Pin*>
GateCloner::inputPins(Instance* inst, bool include_top_level) const
{
  vector<const Pin*> inst_pins = pins(inst);
  return filterPins(inst_pins, sta::PortDirection::input(), include_top_level);
}

std::vector<const Pin*>
GateCloner::outputPins(Instance* inst, bool include_top_level) const
{
  vector<const Pin*> inst_pins = pins(inst);
  return filterPins(inst_pins, sta::PortDirection::output(), include_top_level);

}

int GateCloner::gateClone(const Pin *drvr_pin, PathRef* drvr_path,
                          int drvr_index, PathExpanded* expanded,
                          float cap_factor, bool clone_largest_only)
{
  clone_count_ = 0;
  logger_ = resizer_->logger_;
  sta_ = resizer_->sta_;
  db_network_ = resizer_->db_network_;

  debugPrint(logger_, RSZ, "gate_cloner", 1,
             "Gate cloning Cap Factor:{} Largest Only:{}",
             cap_factor, clone_largest_only);

  Instance *drvr = network_->instance(drvr_pin);
  if (isSingleOutputCombinational(drvr)) {
    cloneTree(drvr, cap_factor, clone_largest_only);
  }

  return clone_count_;
}

float
GateCloner::maxLoad(LibertyCell* cell)
{
  sta::LibertyCellPortIterator itr(cell);
  while (itr.hasNext()) {
    LibertyPort *port = itr.next();
    if (port->direction()->isOutput()) {
      float limit;
      bool exists;
      port->capacitanceLimit(min_max_, limit, exists);
      if (exists) {
        return limit;
      }
    }
  }
  return 0;
}

LibertyCell*
GateCloner::largestLibraryCell(LibertyCell* cell)
{
  auto equiv_cells = sta_->equivCells(cell);
  auto largest = cell;
  float current_max = maxLoad(cell);
  if (equiv_cells) {
    for (auto e_cell : *equiv_cells) {
      auto cell_load = maxLoad(e_cell);
      if (cell_load > current_max) {
        current_max = cell_load;
        largest = e_cell;
      }
    }
  }
  return largest;
}

LibertyCell* GateCloner::halfDrivingPowerCell(Instance* inst)
{
  return halfDrivingPowerCell(network_->libertyCell(inst));
}
LibertyCell*
GateCloner::halfDrivingPowerCell(LibertyCell* cell)
{
  return  closestDriver(cell, sta_->equivCells(cell), 0.5);
}

LibertyCell* GateCloner::closestDriver(LibertyCell* cell,
                                       LibertyCellSeq *candidates, float scale)
{
  LibertyCell* closest = nullptr;
  if (candidates == nullptr || candidates->size() == 0  ||
      !isSingleOutputCombinational(cell)) {
    return nullptr;
  }
  auto output_pin = libraryOutputPins(cell)[0];

  auto current_limit = scale * maxLoad(output_pin->libertyCell());
  auto diff = sta::INF;
  for (auto& cand : *candidates) {
    auto limit = maxLoad(libraryOutputPins(cand)[0]->libertyCell());
    if (limit == current_limit) {
      return cand;
    }
    auto new_diff = std::fabs(limit - current_limit);
    if (new_diff < diff) {
      diff = new_diff;
      closest = cand;
    }
  }
  return closest;
}

std::vector<const Pin *>
GateCloner::fanoutPins(Net* pin_net, bool include_top_level) const
{
  std::vector<const Pin *> filtered_inst_pins;
  auto inst_pins = pins(pin_net);
  filtered_inst_pins = filterPins(inst_pins, sta::PortDirection::input(),
                                  include_top_level);

  if (include_top_level) {
    std::vector<const Pin *> filtered_top_pins;
    auto itr = network_->connectedPinIterator(pin_net);
    while (itr->hasNext()) {
      const Pin * term = itr->next();
      if (network_->isTopLevelPort(term) &&
          network_->direction(term)->isOutput()) {
        filtered_top_pins.push_back(term);
      }
    }
    delete itr;
    filtered_inst_pins.insert(filtered_inst_pins.end(),
                              filtered_top_pins.begin(),
                              filtered_top_pins.end());
  }
  return filtered_inst_pins;
}

bool
GateCloner::violatesMaximumCapacitance(Pin* term, float limit_scale_factor)
{
  const sta::Corner*   corner;
  const sta::RiseFall* rf;
  float                cap, limit, ignore;

  if (!capacitance_limits_initialized_) {
    sta_->checkCapacitanceLimitPreamble();
    capacitance_limits_initialized_ = true;
  }

  // Needed?:  float load_cap = network_->graphDelayCalc()->loadCap(term, resizer_->tgt_slew_dcalc_ap_);
  sta_->checkCapacitance(term, nullptr, sta::MinMax::max(), corner, rf,
                         cap, limit, ignore);
  float diff = (limit_scale_factor * limit) - cap;
  return diff < 0.0 && limit > 0.0;
}

bool
GateCloner::violatesMaximumTransition(Pin* term, float limit_scale_factor)
{
  const sta::Corner*   corner;
  const sta::RiseFall* rf;
  float                slew, limit, ignore;
  sta_->checkSlew(term, nullptr, sta::MinMax::max(), false, corner, rf,
                  slew, limit, ignore);
  float diff = (limit_scale_factor * limit) - slew;
  return diff > 0.0 && limit > 0.0;
}

Vertex* GateCloner::vertex(Pin* term) const
{
  Vertex *vertex, *bidirect_drvr_vertex;
  sta_->graph()->pinVertices(term, vertex, bidirect_drvr_vertex);
  return vertex;
}

float GateCloner::pin_slew(Pin* term) const
{
  return std::max(slew(term, true), slew(term, false));
}

float GateCloner::slew(Pin* term, bool is_rise) const
{
  if (network_->direction(term)->isInput()) {
    return sta_->vertexSlew(vertex(term),
        is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(),
        sta::MinMax::max());
  }

  auto pin_net = network_->net(term);
  // TODO: Fix with usual pin iterator fun
  /*
  auto net_pins = network_->pinspins(pin_net);
  for (auto connected_pin : net_pins) {
    if (connected_pin != term) {
      return sta_->vertexSlew(vertex(connected_pin),
          is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(),
          sta::MinMax::max());
    }
  }
   */
  return sta_->vertexSlew(vertex(term),
      is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(),
      sta::MinMax::max());
}

void GateCloner::cloneTree(Instance* inst, float cap_factor,
                           bool clone_largest_only)
{
  std::unique_ptr<InstancePinIterator> inst_pin_iter{network_->pinIterator(inst)};
  if (!inst_pin_iter->hasNext()) {
    return;
  }

  Pin *output_pin = inst_pin_iter->next();
  Net* net = network_->net(output_pin);
  if (net == nullptr) {
    return;
  }

  SteinerTree *tree = resizer_->makeSteinerTree(output_pin);
  if (tree == nullptr) {
    return;
  }
  tree->populateSides(); // needed to build up the data structures
  LibertyCell *drvr_cell = network_->libertyCell(inst);
  float total_net_load = resizer_->totalLoad(tree);
  float output_target_load = resizer_->findTargetLoad(drvr_cell);

  // This means the net is a bad candidate for cloning
  if (total_net_load < output_target_load) {
    return;
  }

  if (!violatesMaximumTransition(output_pin) &&
      !violatesMaximumCapacitance(output_pin)) {
    return;
  }

  // TODO: Why do we have a cap_limit and c_limit. Seems a bit redundant. Double check
  float c_limit = cap_factor * output_target_load;
  auto slew = pin_slew(output_pin);
  float cap = sta_->graphDelayCalc()->loadCap(output_pin,
                                              resizer_->tgt_slew_dcalc_ap_);
  LibertyPort *output_port = network_->libertyPort(output_pin);
  bool exists;
  float slew_limit;
  float cap_limit;
  output_port->capacitanceLimit(MinMax::max(), cap_limit, exists);
  sta_->findSlewLimit(output_port, sta_->cmdCorner(), MinMax::max(),
                      slew_limit, exists);
  float slew_ratio = slew / slew_limit;
  float cap_ratio = -1;
  if (cap_limit > 0) {
    cap_ratio = cap / cap_limit;
  }

  const char *cellName = drvr_cell->name();
  const char *instName = network_->name(inst);
  debugPrint(logger_, RSZ, "gate_cloner", 1, "{} {} output_target_load: {}",
             instName, cellName, output_target_load);
  debugPrint(logger_, RSZ, "gate_cloner", 1, "{} {} c_limit: {}",
             instName, cellName, c_limit);
  debugPrint(logger_, RSZ, "gate_cloner", 1, "{} {} total_net_load: {}",
             instName, cellName, total_net_load);

  // TODO: This predicate is very questionable
  //if (slew_ratio < 1.4 && cap_ratio < 1.4 ) {
  //  return;
  // }

  clone_largest_only = false;

  if (clone_largest_only && drvr_cell != largestLibraryCell(drvr_cell)) {
    debugPrint(logger_, RSZ, "gate_cloner", 1,  "{} {} is not the largest cell",
               instName, cellName);
    return;
  }
  int fanout_count = fanoutPins(net).size();
  // Don't bother cloning for < 10 fanout count
  // TODO: Once we insert a cloned gate ... does the fanout go down ?
  if (fanout_count <= 10) {
    return;
  }

  auto half_drvr = halfDrivingPowerCell(drvr_cell);
  // TODO: Another questionable predicate. We can clone without half driver cells
  //if (half_drvr == drvr_cell) {
  //  return;
  //}

  debugPrint(logger_, RSZ, "gate_cloner", 1,  "Cloning {} {}", instName,
             cellName);
  output_target_load = (*(resizer_->target_load_map_))[half_drvr];
  c_limit = cap_factor * output_target_load;
  topDownClone(tree, tree->top(), tree->drvrPt(), c_limit, half_drvr);
}

/* bool
RepairHold::checkMaxSlewCap(const Pin *drvr_pin)
{
  float cap, limit, slack;
  const Corner *corner;
  const RiseFall *tr;
  sta_->checkCapacitance(drvr_pin, nullptr, max_,
                         corner, tr, cap, limit, slack);
  float slack_limit_ratio = slack / limit;
  if (slack_limit_ratio < hold_slack_limit_ratio_max_)
    return false;
Slew slew;
sta_->checkSlew(drvr_pin, nullptr, max_, false,
                corner, tr, slew, limit, slack);
slack_limit_ratio = slack / limit;
if (slack_limit_ratio < hold_slack_limit_ratio_max_)
  return false;

resizer_->checkLoadSlews(drvr_pin, 0.0, slew, limit, slack, corner);
slack_limit_ratio = slack / limit;
if (slack_limit_ratio < hold_slack_limit_ratio_max_)
  return false;
return true;
} */

float
GateCloner::worstSlack(Pin* term) const
{
  // sta_->findRequireds();
  auto vert = vertex(term);
  sta_->vertexRequired(vert, sta::MinMax::min());
  sta::PathRef ref = sta_->vertexWorstSlackPath(vert, sta::MinMax::max());

  if (!ref.tag(sta_)) {
    return sta::INF;
  }
  return ref.slack(sta_);
}

float
GateCloner::worstSlack() const
{
  // TODO: Why do we care about the worst slack for the design. Seems a bit
  // idiotic to me.
  float   ws;
  Vertex* vert;
  sta_->findRequireds();
  sta_->worstSlack(min_max_, ws, vert);
  return ws;
}

int
GateCloner::disconnectAll(Net* net) const
{
  int count = 0;
  for (auto& pin : pins(net)) {
    sta_->disconnectPin(const_cast<Pin*>(pin));
    count++;
  }
  return count;
}

std::vector<const Pin*>
GateCloner::filterPins(std::vector<const Pin*>& terms,
                       sta::PortDirection *direction,
                       bool include_top_level) const
{
  std::vector<const Pin*> inst_terms;

  for (auto& term : terms) {
    Instance* inst = network_->instance(term);
    if (inst) {
      if (term && network_->direction(term) == direction) {
        inst_terms.push_back(term);
      }
    }
    else if (include_top_level) {
    }
  }
  return inst_terms;
}

float
GateCloner::required(Pin* term, bool is_rise, PathAnalysisPt* path_ap) const
{
  auto vert = network_->graph()->pinLoadVertex(term);
  auto req  = sta_->vertexRequired(
      vert, is_rise ? sta::RiseFall::rise() : sta::RiseFall::fall(), path_ap);
  if (sta::fuzzyInf(req))
  {
    return 0;
  }
  return req;
}

std::vector<PathPoint>
GateCloner::expandPath(sta::Path* path, bool enumed) const
{
  std::vector<PathPoint> points;
  sta::PathExpanded      expanded(path, sta_);
  for (size_t i = 1; i < expanded.size(); i++) {
    auto ref = expanded.path(i);
    auto pin = ref->vertex(sta_)->pin();
    auto is_rising = ref->transition(sta_) == sta::RiseFall::rise();
    auto arrival = ref->arrival(sta_);
    auto path_ap = ref->pathAnalysisPt(sta_);
    auto path_required = enumed ? 0 : ref->required(sta_);
    if (!path_required || sta::fuzzyInf(path_required)) {
      path_required = required(pin, is_rising, path_ap);
    }
    auto slack = enumed ? path_required - arrival : ref->slack(sta_);
    points.push_back(
        PathPoint(pin, is_rising, arrival, path_required, slack, path_ap));
  }
  return points;
}


std::vector<PathPoint>
GateCloner::worstSlackPath(const Pin* term, bool trim) const
{
  sta::PathRef path;
  std::vector<PathPoint> expanded;
  // sta_->search()->endpointsInvalid();
  path = sta_->vertexWorstSlackPath(vertex(const_cast<Pin *>(term)),
                                    sta::MinMax::max());
  expanded = expandPath(&path, sta::MinMax::max());
  if (trim && !path.isNull()) {
    for (int i = expanded.size() - 1; i >= 0; i--) {
      if (expanded[i].pin() == term) {
        expanded.resize(i + 1);
        break;
      }
    }
  }
  return expanded;
}

void GateCloner::topDownClone(SteinerTree *tree, SteinerPt current, SteinerPt prev,
                              float c_limit, LibertyCell* driver_cell)
{
  double cap_per_micron=0.0;
  SteinerPt drvr = tree->drvrPt();

  for (Corner* corner : *sta_->corners()) {
    double wire_cap = resizer_->wireSignalCapacitance(corner);
    cap_per_micron = std::max(cap_per_micron, wire_cap);
  }

  float src_wire_len =   resizer_->dbuToMeters(tree->distance(drvr, current));
  float src_wire_cap = src_wire_len * cap_per_micron;
  if (src_wire_cap > c_limit) {
    return;
  }

  SteinerPt left = tree->left(current);
  SteinerPt right = tree->right(current);
  if (left != SteinerNull) {
    float cap_left = resizer_->subtreeLoad(tree, cap_per_micron, left) + src_wire_cap;
    bool is_leaf
        = tree->left(left) == SteinerNull && tree->right(left) == SteinerNull;
    if (cap_left < c_limit || is_leaf) {
      cloneInstance(tree, left, current, driver_cell);
    } else {
      topDownClone(tree, left, current, c_limit, driver_cell);
    }
  }

  if (right != SteinerNull) {
    float cap_right = resizer_->subtreeLoad(tree, cap_per_micron, right) + src_wire_cap;
    bool is_leaf
        = tree->left(right) == SteinerNull && tree->right(right) == SteinerNull;
    if (cap_right < c_limit || is_leaf) {
      cloneInstance(tree, right, current, driver_cell);
    } else {
      topDownClone(tree, right, current, c_limit, driver_cell);
    }
  }
}

void GateCloner::topDownConnect(SteinerTree *tree, SteinerPt k, Net* net)
{
  if (k == SteinerNull) {
    return;
  }
  if (tree->left(k) == SteinerNull && tree->right(k) == SteinerNull) {
    Instance *inst      = network_->instance(tree->pin(k));
    auto term_port = network_->port(tree->pin(k));
    sta_->connectPin(inst, term_port, net);
  }
  else {
    topDownConnect(tree, tree->left(k), net);
    topDownConnect(tree, tree->right(k), net);
  }
}
void GateCloner::cloneInstance(SteinerTree *tree, SteinerPt current, SteinerPt prev,
                               LibertyCell* driver_cell)
{
  SteinerPt drvr = tree->drvrPt();
  const Pin *output_pin = tree->pin(drvr);
  auto inst = network_->instance(output_pin);
  Net* output_net = network_->net(output_pin);

  // TODO remove this if condition later
  if  (output_net == NULL) {
    return;
  }
  resizer_->estimateWireParasitic(output_net);
  sta_->ensureLevelized();
  sta_->findRequireds();
  sta_->findDelays();

  auto wp = worstSlackPath(output_pin);
  if (!wp.size()) {
    return;
  }

  auto preslack = worstSlack(wp[wp.size() - 1].pin());
  auto ws       = worstSlack();
  Net* clone_net = resizer_->Resizer::makeUniqueNet();
  LibertyPort *output_port = network_->libertyPort(output_pin);
  // Double check what this topDownConnect fun does.
  topDownConnect(tree, current, clone_net);

  std::unordered_set<Net*> para_nets;
  para_nets.insert(clone_net);
  para_nets.insert(output_net);
  resizer_->estimateWireParasitic(clone_net);

  int fanout_count = fanoutPins(network_->net(output_pin)).size();
  if (fanout_count == 0) {
    disconnectAll(clone_net);
    topDownConnect(tree, current, output_net);
    sta_->deleteNet(clone_net);
    para_nets.erase(clone_net);
  }
  else {
    if (wp.size()) {
      std::string instance_name =  resizer_->makeUniqueInstName("clone_");
      // Make a cloned instance and connect it to the design
      Instance *cloned_inst =
          resizer_->makeInstance(driver_cell, instance_name.c_str(),
                                 network_->topInstance(), tree->location(prev));
      resizer_->cloned_gate_count_++;
      debugPrint(logger_, RSZ, "gate_cloner", 1,
                 "Gate cloning {}({}) {} {}", instance_name.c_str(),
                 driver_cell->name(), output_port->name(),
                 network_->name(clone_net));
      sta_->connectPin(cloned_inst, output_port, clone_net);

      // TODO: Need to double check on this. But what we need to do here is to
      // get the original instance and connect the input ports in parallel to
      // the cloned instance.
      // This is why we are iterating through the "inst" pins and connecting the
      // ports of the cloned_inst the same way.
      sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
      while (pin_iter->hasNext()) {
        const Pin* pin = pin_iter->next();
        sta_->delaysInvalidFrom(pin);
        sta_->delaysInvalidFrom(cloned_inst);
        sta_->delaysInvalidFromFanin(pin);
        if (network_->direction(pin)->isInput()) {
          Net* target_net = network_->net(pin);
          LibertyPort* target_port = network_->libertyPort(pin);
          sta_->connectPin(cloned_inst, target_port, target_net);
          para_nets.insert(target_net);
        }
      }
      clone_count_++;
    }
    else {
      disconnectAll(clone_net);
      topDownConnect(tree, current, output_net);
      sta_->deleteNet(clone_net);
      para_nets.erase(clone_net);
    }
  }

  for (auto& net : para_nets) {
    resizer_->estimateWireParasitic(net);
  }
}

int GateCloner::run(const Pin *drvr_pin, PathRef* drvr_path, int drvr_index,
                    PathExpanded* expanded)
{
  bool clone_largest_only = false;
  float cap_factor = 1.4;
  sta_->search()->endpointsInvalid();
  sta_->ensureLevelized();
  sta_->findRequireds();
  sta_->findDelays();
  int rc = gateClone(drvr_pin, drvr_path, drvr_index, expanded, cap_factor,
                     clone_largest_only);
  return rc;
}

PathPoint::PathPoint(Pin *path_pin, bool is_rise, float path_arrival,
                     float path_required, float path_slack,
                     PathAnalysisPt *pt)
    : pin_(path_pin), is_rise_(is_rise), arrival_(path_arrival),
      required_(path_required), slack_(path_slack), path_ap_(pt)
{
}
Pin* PathPoint::pin() const      { return pin_; }
bool PathPoint::isRise() const   { return is_rise_; }
float PathPoint::arrival() const { return arrival_; }
float PathPoint::required() const{ return required_;}
float PathPoint::slack() const   {  return slack_;  }
PathAnalysisPt* PathPoint::analysisPoint() const { return path_ap_; }

int PathPoint::analysisPointIndex() const
{
  if (!path_ap_) {
    return -1;
  }
  return path_ap_->index();
}

}  // namespace rsz
