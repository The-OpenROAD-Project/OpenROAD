// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/ContainerHelpers.hh"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {
using std::max;
using std::string;
using std::vector;

using odb::dbMaster;

using odb::dbMaster;
using odb::Point;

using utl::RSZ;

using namespace sta;  // NOLINT

using InputSlews = std::array<Slew, RiseFall::index_count>;
using TgtSlews = std::array<Slew, RiseFall::index_count>;

BaseMove::BaseMove(Resizer* resizer)
{
  resizer_ = resizer;
  estimate_parasitics_ = resizer_->getEstimateParasitics();
  logger_ = resizer_->logger_;
  network_ = resizer_->network_;
  db_ = resizer_->db_;
  db_network_ = resizer_->db_network_;
  dbStaState::init(resizer_->sta_);
  sta_ = resizer_->sta_;
  opendp_ = resizer_->opendp_;

  accepted_count_ = 0;
  rejected_count_ = 0;
  all_inst_set_ = InstanceSet(db_network_);
  pending_count_ = 0;
  pending_inst_set_ = InstanceSet(db_network_);
}

void BaseMove::commitMoves()
{
  accepted_count_ += pending_count_;
  pending_count_ = 0;
  pending_inst_set_.clear();
}

void BaseMove::init()
{
  pending_count_ = 0;
  rejected_count_ = 0;
  accepted_count_ = 0;
  pending_inst_set_.clear();
  all_inst_set_.clear();
}

void BaseMove::undoMoves()
{
  rejected_count_ += pending_count_;
  pending_count_ = 0;
  pending_inst_set_.clear();
}

int BaseMove::hasMoves(Instance* inst) const
{
  return all_inst_set_.count(inst);
}

int BaseMove::hasPendingMoves(Instance* inst) const
{
  return pending_inst_set_.count(inst);
}

int BaseMove::numPendingMoves() const
{
  return pending_count_;
}

int BaseMove::numCommittedMoves() const
{
  return accepted_count_;
}

int BaseMove::numRejectedMoves() const
{
  return rejected_count_;
}

int BaseMove::numMoves() const
{
  return accepted_count_ + pending_count_;
}

void BaseMove::addMove(Instance* inst, int count)
{
  // Add it as a candidate move, not accepted yet
  // This count is for the cloned gates where we only count the clone
  // but we also add the main gate to the pending set.
  // This count is also used when we add more than 1 buffer during rebuffer.
  // Default is to add 1 to the pending count.
  pending_count_ += count;
  // Add it to all moves, even though it wasn't accepted.
  // This is the behavior to match the current resizer.
  all_inst_set_.insert(inst);
  // Also add it to the pending moves
  pending_inst_set_.insert(inst);
}

double BaseMove::area(Cell* cell)
{
  return area(db_network_->staToDb(cell));
}

double BaseMove::area(dbMaster* master)
{
  if (!master->isCoreAutoPlaceable()) {
    return 0;
  }
  return resizer_->dbuToMeters(master->getWidth())
         * resizer_->dbuToMeters(master->getHeight());
}

bool BaseMove::isPortEqiv(sta::FuncExpr* expr,
                          const LibertyCell* cell,
                          const LibertyPort* port_a,
                          const LibertyPort* port_b)
{
  if (port_a->libertyCell() != cell || port_b->libertyCell() != cell) {
    return false;
  }

  sta::LibertyCellPortIterator port_iter(cell);
  std::unordered_map<const LibertyPort*, std::vector<bool>> port_stimulus;
  size_t input_port_count = 0;
  while (port_iter.hasNext()) {
    LibertyPort* port = port_iter.next();
    if (port->direction()->isInput()) {
      ++input_port_count;
      port_stimulus[port] = {};
    }
  }

  if (input_port_count > 16) {
    // Not worth manually simulating all these values.
    // Probably need to do SAT solving or something else instead.
    return false;
  }

  // Generate stimulus for the ports
  size_t var_index = 0;
  for (auto& it : port_stimulus) {
    size_t truth_table_length = 0x1 << input_port_count;
    std::vector<bool>& variable_stimulus = it.second;
    variable_stimulus.resize(truth_table_length, false);
    for (int i = 0; i < truth_table_length; i++) {
      variable_stimulus[i] = static_cast<bool>((i >> var_index) & 0x1);
    }
    var_index++;
  }

  std::vector<bool> result_no_swap = simulateExpr(expr, port_stimulus);

  // Swap pins
  std::swap(port_stimulus.at(port_a), port_stimulus.at(port_b));

  std::vector<bool> result_with_swap = simulateExpr(expr, port_stimulus);

  // Check if truth tables are equivalent post swap. If they are then pins
  // are equivalent.
  for (size_t i = 0; i < result_no_swap.size(); i++) {
    if (result_no_swap[i] != result_with_swap[i]) {
      return false;
    }
  }

  return true;
}

bool BaseMove::simulateExpr(
    sta::FuncExpr* expr,
    std::unordered_map<const LibertyPort*, std::vector<bool>>& port_stimulus,
    size_t table_index)
{
  using Op = sta::FuncExpr::Op;
  const Op curr_op = expr->op();

  switch (curr_op) {
    case Op::not_:
      return !simulateExpr(expr->left(), port_stimulus, table_index);
    case Op::and_:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             && simulateExpr(expr->right(), port_stimulus, table_index);
    case Op::or_:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             || simulateExpr(expr->right(), port_stimulus, table_index);
    case Op::xor_:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             ^ simulateExpr(expr->right(), port_stimulus, table_index);
    case Op::one:
      return true;
    case Op::zero:
      return false;
    case Op::port:
      return port_stimulus[expr->port()][table_index];
  }

  logger_->error(RSZ, 91, "unrecognized expr op from OpenSTA");
  return false;
}

std::vector<bool> BaseMove::simulateExpr(
    sta::FuncExpr* expr,
    std::unordered_map<const LibertyPort*, std::vector<bool>>& port_stimulus)
{
  size_t table_length = 0x1 << port_stimulus.size();
  std::vector<bool> result;
  result.resize(table_length);
  for (size_t i = 0; i < table_length; i++) {
    result[i] = simulateExpr(expr, port_stimulus, i);
  }

  return result;
}

////////////////////////////////////////////////////////////////
Instance* BaseMove::makeBuffer(LibertyCell* cell,
                               const char* name,
                               Instance* parent,
                               const Point& loc)
{
  Instance* inst = resizer_->makeInstance(cell, name, parent, loc);
  return inst;
}

// Estimate slack impact from driver removal.
// Delay improvement from removed driver should be greater than
// delay degradation from prev driver for driver input pin path.
// Side input paths should absorb delay and slew degradation from prev driver.
// Delay degradation for side input paths comes from two sources:
// 1) delay degradation at prev driver due to increased load cap
// 2) delay degradation at side out pin due to degraded slew from prev driver
// Acceptance criteria are as follows:
// For direct fanout paths (fanout paths of drvr_pin), accept buffer removal
// if slack improves (may still be violating)
// For side fanout paths (fanout paths of side_out_pin*), accept buffer
// removal if slack doesn't become violating (no new violations)
//
//               input_net                             output_net
//  prev_drv_pin ------>  (drvr_input_pin   drvr_pin)  ------>  FO inst pin
//               |
//               ------>  (side_input_pin1  side_out_pin1) ----->
//               |
//               ------>  (side_input_pin2  side_out_pin2) ----->
//
bool BaseMove::estimatedSlackOK(const SlackEstimatorParams& params)
{
  const Scene* scene = params.corner;
  if (scene == nullptr) {
    // can't do any estimation without a corner
    return false;
  }

  GraphDelayCalc* dcalc = sta_->graphDelayCalc();

  ArcDelay old_delay[RiseFall::index_count];
  ArcDelay new_delay[RiseFall::index_count];
  Slew old_drvr_slew[RiseFall::index_count];
  Slew new_drvr_slew[RiseFall::index_count];
  float old_cap, new_cap;
  if (!resizer_->computeNewDelaysSlews(params.prev_driver_pin,
                                       params.driver,
                                       params.corner,
                                       old_delay,
                                       new_delay,
                                       old_drvr_slew,
                                       new_drvr_slew,
                                       old_cap,
                                       new_cap)) {
    return false;
  }

  // Check for max cap violation
  if (!checkMaxCapOK(params.prev_driver_pin, new_cap - old_cap)) {
    debugPrint(logger_,
               RSZ,
               "remove_buffer",
               1,
               "buffer {} is not removed "
               "because of max cap violation",
               db_network_->name(params.driver));
    return false;
  }

  const RiseFall* prev_driver_rf = params.prev_driver_path->transition(sta_);
  float delay_degrad
      = new_delay[prev_driver_rf->index()] - old_delay[prev_driver_rf->index()];
  float delay_imp = resizer_->bufferDelay(
      params.driver_cell,
      params.driver_path->transition(sta_),
      dcalc->loadCap(params.driver_pin, scene, MinMax::max()),
      scene,
      MinMax::max());

  // Check if degraded delay & slew can be absorbed by driver pin fanouts
  // Model slew degradation across wire from prev_drv_pin to the FO inst pin
  // based on Elmore delay that considers layers and vias for accurate
  // wire cap/res computation.
  // prev_driver_pin --->  (driver_input_pin   driver_pin) --->  pin
  //                 ^                                        ^
  //                 |                                        |
  //              old_driver_slew                         old_load_slew
  //
  // prev_driver_pin ----------------------------------------->  pin
  //                 ^                                        ^
  //                 |                                        |
  //              new_driver_slew                         new_load_slew
  //
  std::map<const Pin*, float> load_pin_slew;
  if (!resizer_->estimateSlewsAfterBufferRemoval(
          params.prev_driver_pin,
          params.driver,
          new_drvr_slew[prev_driver_rf->index()],
          params.corner,
          load_pin_slew)) {
    return false;
  }

  SceneSeq scenes1({const_cast<Scene*>(scene)});
  for (const auto& [load_pin, estimated_new_load_slew] : load_pin_slew) {
    Vertex* load_vertex = graph_->pinLoadVertex(load_pin);
    assert(load_vertex != nullptr);
    Slew old_load_slew[RiseFall::index_count];
    for (auto rf : RiseFall::range()) {
      old_load_slew[rf->index()] = sta_->slew(
          load_vertex, rf->asRiseFallBoth(), scenes1, MinMax::max());
    }
    Slew new_load_slew[RiseFall::index_count];
    for (auto rf : RiseFall::range()) {
      new_load_slew[rf->index()] = estimated_new_load_slew;
    }

    debugPrint(
        logger_,
        RSZ,
        "remove_buffer",
        1,
        "estimated in slew at fanout pin {} is {}, prev drvr out slew={}",
        db_network_->name(load_pin),
        estimated_new_load_slew,
        new_drvr_slew[prev_driver_rf->index()]);

    // Check if output pin of direct fanout instance can absorb delay and
    // slew
    // degradation
    if (!estimateInputSlewImpact(network_->instance(load_pin),
                                 scene,
                                 MinMax::max(),
                                 old_load_slew,
                                 new_load_slew,
                                 delay_degrad - delay_imp,
                                 params,
                                 /* accept if slack improves */ true)) {
      return false;
    }
  }

  // Check side fanout paths.  Side fanout paths get no delay benefit from
  // buffer removal.
  Net* input_net = network_->net(params.prev_driver_pin);
  auto pin_iter = std::unique_ptr<NetConnectedPinIterator>(
      network_->connectedPinIterator(input_net));
  while (pin_iter->hasNext()) {
    const Pin* side_input_pin = pin_iter->next();
    if (network_->isHierarchical(side_input_pin)
        || side_input_pin == params.prev_driver_pin
        || side_input_pin == params.driver_input_pin) {
      continue;
    }
    float old_slack
        = sta_->slack(graph_->pinLoadVertex(side_input_pin), resizer_->max_);
    float new_slack = old_slack - delay_degrad - params.setup_slack_margin;
    if (new_slack < 0) {
      float slack_degrad = old_slack - new_slack;
      const float kSlackDegradRatioLimit = 0.1;
      if (old_slack >= 0
          || (old_slack < 0
              && slack_degrad > kSlackDegradRatioLimit * abs(old_slack))) {
        // clang-format off
        debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed "
                   "because side input pin {} will have a violating slack of {}:"
                   " old slack={}, slack margin={}, delay_degrad={}",
                   db_network_->name(params.driver),
                   db_network_->name(side_input_pin), new_slack, old_slack,
                   params.setup_slack_margin, delay_degrad);
        // clang-format on
        return false;
      }
    }

    // Consider secondary degradation at side out pin from degraded input
    // slew.
    if (!estimateInputSlewImpact(network_->instance(side_input_pin),
                                 scene,
                                 MinMax::max(),
                                 old_drvr_slew,
                                 new_drvr_slew,
                                 delay_degrad,
                                 params,
                                 /* accept only if no new viol */ false)) {
      return false;
    }
  }  // for each pin of input_net

  // clang-format off
  debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} can be removed because"
             " direct fanouts and side fanouts can absorb delay/slew degradation",
             db_network_->name(params.driver));
  // clang-format on
  return true;
}

// Estimate impact from degraded input slew for this instance.
// Include all output pins for multi-outut gate (MOG) cells.
bool BaseMove::estimateInputSlewImpact(Instance* instance,
                                       const Scene* scene,
                                       const MinMax* min_max,
                                       Slew old_in_slew[RiseFall::index_count],
                                       Slew new_in_slew[RiseFall::index_count],
                                       // delay adjustment from prev stage
                                       float delay_adjust,
                                       SlackEstimatorParams params,
                                       bool accept_if_slack_improves)
{
  GraphDelayCalc* dcalc = sta_->graphDelayCalc();
  auto pin_iter
      = std::unique_ptr<InstancePinIterator>(network_->pinIterator(instance));
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (!network_->direction(pin)->isOutput()) {
      continue;
    }
    LibertyPort* port = network_->libertyPort(pin);
    if (port == nullptr) {
      // reject the transform if we can't estimate
      // clang-format off
      debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed "
                 "because pin {} has no liberty port",
                 db_network_->name(params.driver), db_network_->name(pin));
      // clang-format on
      return false;
    }
    float load_cap = dcalc->loadCap(pin, scene, min_max);
    ArcDelay old_delay[RiseFall::index_count], new_delay[RiseFall::index_count];
    Slew old_slew[RiseFall::index_count], new_slew[RiseFall::index_count];
    resizer_->gateDelays(
        port, load_cap, old_in_slew, scene, min_max, old_delay, old_slew);
    resizer_->gateDelays(
        port, load_cap, new_in_slew, scene, min_max, new_delay, new_slew);
    float delay_diff = max(
        new_delay[RiseFall::riseIndex()] - old_delay[RiseFall::riseIndex()],
        new_delay[RiseFall::fallIndex()] - old_delay[RiseFall::fallIndex()]);

    float old_slack = sta_->slack(graph_->pinDrvrVertex(pin), resizer_->max_)
                      - params.setup_slack_margin;
    float new_slack
        = old_slack - delay_diff - delay_adjust - params.setup_slack_margin;
    if ((accept_if_slack_improves && fuzzyGreater(old_slack, new_slack))
        || (!accept_if_slack_improves && new_slack < 0)) {
      // clang-format off
      debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed "
                 "because pin {} will have a violating or worse slack of {}",
                 db_network_->name(params.driver), db_network_->name(pin),
                 new_slack);
      // clang-format on
      return false;
    }
  }

  return true;
}

void BaseMove::getBufferPins(Instance* buffer, Pin*& ip, Pin*& op)
{
  ip = nullptr;
  op = nullptr;
  auto pin_iter
      = std::unique_ptr<InstancePinIterator>(network_->pinIterator(buffer));
  while (pin_iter->hasNext()) {
    Pin* pin = pin_iter->next();
    sta::PortDirection* dir = network_->direction(pin);
    if (dir->isAnyOutput()) {
      op = pin;
    }
    if (dir->isAnyInput()) {
      ip = pin;
    }
  }
}

int BaseMove::fanout(Vertex* vertex)
{
  int fanout = 0;
  VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge* edge = edge_iter.next();
    // Disregard output->output timing arcs
    if (edge->isWire()) {
      fanout++;
    }
  }
  return fanout;
}

LibertyCell* BaseMove::upsizeCell(LibertyPort* in_port,
                                  LibertyPort* drvr_port,
                                  const float load_cap,
                                  const float prev_drive,
                                  const Scene* scene,
                                  const MinMax* min_max)
{
  const int lib_ap = scene->libertyIndex(min_max);
  LibertyCell* cell = drvr_port->libertyCell();
  sta::LibertyCellSeq swappable_cells = resizer_->getSwappableCells(cell);
  if (!swappable_cells.empty()) {
    const char* in_port_name = in_port->name();
    const char* drvr_port_name = drvr_port->name();
    sort(
        swappable_cells,
        [=, this](const LibertyCell* cell1, const LibertyCell* cell2) {
          const LibertyPort* port1 = static_cast<const LibertyPort*>(
                                         cell1->findLibertyPort(drvr_port_name))
                                         ->scenePort(lib_ap);
          const LibertyPort* port2 = static_cast<const LibertyPort*>(
                                         cell2->findLibertyPort(drvr_port_name))
                                         ->scenePort(lib_ap);
          const float drive1 = port1->driveResistance();
          const float drive2 = port2->driveResistance();
          const ArcDelay intrinsic1 = port1->intrinsicDelay(this);
          const ArcDelay intrinsic2 = port2->intrinsicDelay(this);
          const float capacitance1 = port1->capacitance();
          const float capacitance2 = port2->capacitance();
          return std::tie(drive2, intrinsic1, capacitance1)
                 < std::tie(drive1, intrinsic2, capacitance2);
        });
    const float drive = static_cast<const LibertyPort*>(drvr_port)
                            ->scenePort(lib_ap)
                            ->driveResistance();
    const float delay = resizer_->gateDelay(drvr_port, load_cap, scene, min_max)
                        + prev_drive
                              * static_cast<const LibertyPort*>(in_port)
                                    ->scenePort(lib_ap)
                                    ->capacitance();

    for (LibertyCell* swappable : swappable_cells) {
      LibertyCell* swappable_corner = swappable->sceneCell(lib_ap);
      LibertyPort* swappable_drvr
          = swappable_corner->findLibertyPort(drvr_port_name);
      LibertyPort* swappable_input
          = swappable_corner->findLibertyPort(in_port_name);
      const float swappable_drive = swappable_drvr->driveResistance();
      // Include delay of previous driver into swappable gate.
      const float swappable_delay
          = resizer_->gateDelay(swappable_drvr, load_cap, scene, min_max)
            + prev_drive * swappable_input->capacitance();
      if (swappable_drive < drive && swappable_delay < delay) {
        return swappable;
      }
    }
  }
  return nullptr;
};

// Replace LEF with LEF so ports stay aligned in instance.
bool BaseMove::replaceCell(Instance* inst, const LibertyCell* replacement)
{
  const char* replacement_name = replacement->name();
  dbMaster* replacement_master = db_->findMaster(replacement_name);

  if (!replacement_master) {
    return false;
  }

  // Check if replacement would cause max_cap violations on input nets
  if (!checkMaxCapViolation(inst, replacement)) {
    return false;
  }

  odb::dbInst* dinst = db_network_->staToDb(inst);
  dbMaster* master = dinst->getMaster();
  resizer_->designAreaIncr(-area(master));
  Cell* replacement_cell1 = db_network_->dbToSta(replacement_master);
  sta_->replaceCell(inst, replacement_cell1);
  resizer_->designAreaIncr(area(replacement_master));

  // Legalize the position of the instance in case it leaves the die
  if (estimate_parasitics_->getParasiticsSrc()
          == est::ParasiticsSrc::global_routing
      || estimate_parasitics_->getParasiticsSrc()
             == est::ParasiticsSrc::detailed_routing) {
    opendp_->legalCellPos(db_network_->staToDb(inst));
  }

  return true;
}

// Check if replacing inst with replacement cell would cause max_cap violation
// on any of the fanin nets.
bool BaseMove::checkMaxCapViolation(Instance* inst,
                                    const LibertyCell* replacement)
{
  LibertyCell* current_cell = network_->libertyCell(inst);
  if (!current_cell) {
    return true;  // Nothing to check, just allow it
  }

  // Iterate through all input pins of the instance
  InstancePinIterator* pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    Pin* pin = pin_iter->next();

    // Only check input pins
    if (!network_->direction(pin)->isAnyInput()) {
      continue;
    }
    sta::PinSet* drivers = network_->drivers(pin);
    if (drivers) {
      // Calculate capacitance delta (new - old)
      float old_cap = getInputPinCapacitance(pin, current_cell);
      float new_cap = getInputPinCapacitance(pin, replacement);
      float cap_delta = new_cap - old_cap;
      if (cap_delta <= 0.0) {
        continue;
      }
      for (const Pin* drvr_pin : *drivers) {
        if (!checkMaxCapOK(drvr_pin, cap_delta)) {
          delete pin_iter;
          return false;
        }
      }
    }
  }

  delete pin_iter;
  return true;
}

// Get input capacitance for a specific port in a liberty cell
float BaseMove::getInputPinCapacitance(Pin* pin, const LibertyCell* cell)
{
  LibertyPort* port = network_->libertyPort(pin);
  if (!port) {
    return 0.0;
  }

  // Find corresponding port in the new cell
  LibertyPort* cell_port = cell->findLibertyPort(port->name());
  if (!cell_port) {
    return 0.0;
  }

  // Get worst capacitance
  float cap = 0.0;
  for (auto rf : RiseFall::range()) {
    float port_cap = cell_port->capacitance(rf, resizer_->max_);
    cap = max(cap, port_cap);
  }

  return cap;
}

// Check for possible max cap violation with new cap_delta
// If max cap is already violating, accept a solution only if
// it does not worsen the violation
bool BaseMove::checkMaxCapOK(const Pin* drvr_pin, float cap_delta)
{
  float cap, max_cap, cap_slack;
  const Scene* corner;
  const RiseFall* tr;
  sta_->checkCapacitance(drvr_pin,
                         sta_->scenes(),
                         resizer_->max_,
                         // return values
                         cap,
                         max_cap,
                         cap_slack,
                         tr,
                         corner);

  if (max_cap > 0.0 && corner) {
    float new_cap = cap + cap_delta;
    // If it is already violating, accept only if violation is no worse
    if (cap_slack < 0.0) {
      return new_cap <= cap;
    }
    return new_cap <= max_cap;
  }
  return true;
}

Slack BaseMove::getWorstInputSlack(Instance* inst)
{
  Slack worst_slack = INF;
  auto pin_iter
      = std::unique_ptr<InstancePinIterator>(network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (network_->direction(pin)->isInput()) {
      Vertex* vertex = graph_->pinDrvrVertex(pin);
      if (vertex) {
        worst_slack
            = std::min(worst_slack, sta_->slack(vertex, resizer_->max_));
      }
    }
  }
  return worst_slack;
}

Slack BaseMove::getWorstOutputSlack(Instance* inst)
{
  Slack worst_slack = INF;

  // Iterate through all pins of the instance to find output pins
  auto pin_iter
      = std::unique_ptr<InstancePinIterator>(network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    const Pin* inst_pin = pin_iter->next();
    if (network_->direction(inst_pin)->isOutput()) {
      Vertex* vertex = graph_->pinLoadVertex(inst_pin);
      if (vertex) {
        worst_slack
            = std::min(worst_slack, sta_->slack(vertex, resizer_->max_));
      }
    }
  }
  return worst_slack;
}

ArcDelay BaseMove::getWorstIntrinsicDelay(const LibertyPort* input_port)
{
  const LibertyCell* cell = input_port->libertyCell();
  vector<const LibertyPort*> output_ports = getOutputPorts(cell);

  // Just return the worst of all the outputs, if there's more than one
  ArcDelay worst_intrinsic_delay = -INF;
  for (const LibertyPort* output_port : output_ports) {
    if (output_port->direction()->isOutput()) {
      worst_intrinsic_delay
          = max(worst_intrinsic_delay, output_port->intrinsicDelay(nullptr));
    }
  }
  return worst_intrinsic_delay;
}

vector<const LibertyPort*> BaseMove::getOutputPorts(const LibertyCell* cell)
{
  vector<const LibertyPort*> fanouts;

  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    const LibertyPort* port = port_iter.next();
    if (!port->isPwrGnd() && port->direction()->isOutput()) {
      fanouts.push_back(port);
    }
  }

  return fanouts;
}

vector<const Pin*> BaseMove::getOutputPins(const Instance* inst)
{
  vector<const Pin*> outputs;

  auto pin_iter
      = std::unique_ptr<InstancePinIterator>(network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (network_->direction(pin)->isOutput()) {
      outputs.push_back(pin);
    }
  }

  return outputs;
}

bool BaseMove::checkMaxCapViolation(const Pin* output_pin,
                                    LibertyPort* output_port,
                                    float output_cap)
{
  float max_cap;
  bool cap_limit_exists;
  // FIXME: Can we update to consider multiple corners?
  output_port->capacitanceLimit(resizer_->max_, max_cap, cap_limit_exists);

  debugPrint(logger_,
             RSZ,
             "opt_moves",
             3,
             " fanout pin {} cap {} output_cap {} ",
             output_port->name(),
             max_cap,
             output_cap);

  if (cap_limit_exists && max_cap > 0.0 && output_cap > max_cap) {
    debugPrint(logger_,
               RSZ,
               "opt_moves",
               2,
               "  skip based on max cap {} gate={} cap={} max_cap={}",
               network_->pathName(output_pin),
               output_port->libertyCell()->name(),
               output_cap,
               max_cap);
    return true;
  }

  return false;
}

bool BaseMove::checkMaxSlewViolation(const Pin* output_pin,
                                     LibertyPort* output_port,
                                     float output_slew_factor,
                                     float output_cap,
                                     const Scene* scene)
{
  float output_res = output_port->driveResistance();
  float output_slew = output_slew_factor * output_res * output_cap;
  float max_slew;
  bool slew_limit_exists;

  sta_->findSlewLimit(
      output_port, scene, resizer_->max_, max_slew, slew_limit_exists);

  if (output_slew > max_slew) {
    debugPrint(logger_,
               RSZ,
               "opt_moves",
               2,
               "  skip based on max slew {} gate={} slew={} max_slew={}",
               network_->pathName(output_pin),
               output_port->libertyCell()->name(),
               output_slew,
               max_slew);
    return true;
  }

  return false;
}

float BaseMove::computeElmoreSlewFactor(const Pin* output_pin,
                                        LibertyPort* output_port,
                                        float output_load_cap)
{
  float elmore_slew_factor = 0.0;

  // Get the vertex for the output pin
  Vertex* output_vertex = graph_->pinDrvrVertex(output_pin);

  // Get the output slew
  const Slew output_slew = sta_->slew(output_vertex,
                                      sta::RiseFallBoth::riseFall(),
                                      sta_->scenes(),
                                      resizer_->max_);

  // Get the output resistance
  float output_res = output_port->driveResistance();

  // Can have gates without fanout (e.g. QN of flop) which have no load
  if (output_res > 0.0 && output_load_cap > 0.0) {
    elmore_slew_factor = output_slew / (output_res * output_load_cap);
  }

  return elmore_slew_factor;
}

////////////////////////////////////////////////////////////////

sta::LibertyCellSeq BaseMove::getSwappableCells(LibertyCell* base)
{
  if (base->isBuffer()) {
    if (!resizer_->buffer_fast_sizes_.contains(base)) {
      return sta::LibertyCellSeq();
    }

    sta::LibertyCellSeq buffer_sizes;
    buffer_sizes.reserve(resizer_->buffer_fast_sizes_.size());
    for (LibertyCell* buffer : resizer_->buffer_fast_sizes_) {
      buffer_sizes.push_back(buffer);
    }
    // Sort output to ensure deterministic order
    std::ranges::sort(buffer_sizes,
                      [](LibertyCell const* c1, LibertyCell const* c2) {
                        auto area1 = c1->area();
                        auto area2 = c2->area();
                        if (area1 != area2) {
                          return area1 < area2;
                        }
                        return c1->id() < c2->id();
                      });
    return buffer_sizes;
  }
  return resizer_->getSwappableCells(base);
}

////////////////////////////////////////////////////////////////
// namespace rsz
}  // namespace rsz
