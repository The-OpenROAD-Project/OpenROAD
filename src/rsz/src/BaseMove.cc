// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"

namespace rsz {

using std::max;
using std::string;
using std::vector;

using odb::dbMaster;

using odb::dbMaster;
using odb::Point;

using utl::RSZ;

using sta::ArcDcalcResult;
using sta::ArcDelay;
using sta::Cell;
using sta::DcalcAnalysisPt;
using sta::Edge;
using sta::fuzzyGreater;
using sta::GraphDelayCalc;
using sta::INF;
using sta::Instance;
using sta::InstancePinIterator;
using sta::InstanceSet;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::LoadPinIndexMap;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::Pin;
using sta::RiseFall;
using sta::Slack;
using sta::Slew;
using sta::TimingArc;
using sta::TimingArcSet;
using sta::Vertex;
using sta::VertexOutEdgeIterator;

using InputSlews = std::array<Slew, RiseFall::index_count>;
using TgtSlews = std::array<Slew, RiseFall::index_count>;

BaseMove::BaseMove(Resizer* resizer)
{
  resizer_ = resizer;
  logger_ = resizer_->logger_;
  network_ = resizer_->network_;
  db_ = resizer_->db_;
  db_network_ = resizer_->db_network_;
  dbStaState::init(resizer_->sta_);
  sta_ = resizer_->sta_;
  dbu_ = resizer_->dbu_;
  opendp_ = resizer_->opendp_;

  all_count_ = 0;
  all_inst_set_ = InstanceSet(db_network_);
  pending_count_ = 0;
  pending_inst_set_ = InstanceSet(db_network_);
}

void BaseMove::commitMoves()
{
  all_count_ += pending_count_;
  pending_count_ = 0;
  pending_inst_set_.clear();
}

void BaseMove::init()
{
  pending_count_ = 0;
  all_count_ = 0;
  pending_inst_set_.clear();
  all_inst_set_.clear();
}

void BaseMove::undoMoves()
{
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
  return all_count_;
}

int BaseMove::numMoves() const
{
  return all_count_ + pending_count_;
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
  return dbuToMeters(master->getWidth()) * dbuToMeters(master->getHeight());
}

double BaseMove::dbuToMeters(int dist) const
{
  return dist / (dbu_ * 1e+6);
}

// Rise/fall delays across all timing arcs into drvr_port.
// Uses target slew for input slew.
void BaseMove::gateDelays(const LibertyPort* drvr_port,
                          const float load_cap,
                          const DcalcAnalysisPt* dcalc_ap,
                          // Return values.
                          ArcDelay delays[RiseFall::index_count],
                          Slew slews[RiseFall::index_count])
{
  for (int rf_index : RiseFall::rangeIndex()) {
    delays[rf_index] = -INF;
    slews[rf_index] = -INF;
  }
  LibertyCell* cell = drvr_port->libertyCell();
  for (TimingArcSet* arc_set : cell->timingArcSets()) {
    if (arc_set->to() == drvr_port && !arc_set->role()->isTimingCheck()) {
      for (TimingArc* arc : arc_set->arcs()) {
        const RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        int out_rf_index = arc->toEdge()->asRiseFall()->index();
        // use annotated slews if available
        LibertyPort* port = arc->from();
        float in_slew = 0.0;
        auto it = input_slew_map_.find(port);
        if (it != input_slew_map_.end()) {
          const InputSlews& slew = it->second;
          in_slew = slew[in_rf->index()];
        } else {
          in_slew = tgt_slews_[in_rf->index()];
        }
        LoadPinIndexMap load_pin_index_map(network_);
        ArcDcalcResult dcalc_result
            = arc_delay_calc_->gateDelay(nullptr,
                                         arc,
                                         in_slew,
                                         load_cap,
                                         nullptr,
                                         load_pin_index_map,
                                         dcalc_ap);

        const ArcDelay& gate_delay = dcalc_result.gateDelay();
        const Slew& drvr_slew = dcalc_result.drvrSlew();
        delays[out_rf_index] = max(delays[out_rf_index], gate_delay);
        slews[out_rf_index] = max(slews[out_rf_index], drvr_slew);
      }
    }
  }
}

// Rise/fall delays across all timing arcs into drvr_port.
// Takes input slews and load cap
void BaseMove::gateDelays(const LibertyPort* drvr_port,
                          const float load_cap,
                          const Slew in_slews[RiseFall::index_count],
                          const DcalcAnalysisPt* dcalc_ap,
                          // Return values.
                          ArcDelay delays[RiseFall::index_count],
                          Slew out_slews[RiseFall::index_count])
{
  for (int rf_index : RiseFall::rangeIndex()) {
    delays[rf_index] = -INF;
    out_slews[rf_index] = -INF;
  }
  LibertyCell* cell = drvr_port->libertyCell();
  for (TimingArcSet* arc_set : cell->timingArcSets()) {
    if (arc_set->to() == drvr_port && !arc_set->role()->isTimingCheck()) {
      for (TimingArc* arc : arc_set->arcs()) {
        const RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        int out_rf_index = arc->toEdge()->asRiseFall()->index();
        LoadPinIndexMap load_pin_index_map(network_);
        ArcDcalcResult dcalc_result
            = arc_delay_calc_->gateDelay(nullptr,
                                         arc,
                                         in_slews[in_rf->index()],
                                         load_cap,
                                         nullptr,
                                         load_pin_index_map,
                                         dcalc_ap);

        const ArcDelay& gate_delay = dcalc_result.gateDelay();
        const Slew& drvr_slew = dcalc_result.drvrSlew();
        delays[out_rf_index] = max(delays[out_rf_index], gate_delay);
        out_slews[out_rf_index] = max(out_slews[out_rf_index], drvr_slew);
      }
    }
  }
}

ArcDelay BaseMove::gateDelay(const LibertyPort* drvr_port,
                             const RiseFall* rf,
                             const float load_cap,
                             const DcalcAnalysisPt* dcalc_ap)
{
  ArcDelay delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(drvr_port, load_cap, dcalc_ap, delays, slews);
  return delays[rf->index()];
}

ArcDelay BaseMove::gateDelay(const LibertyPort* drvr_port,
                             const float load_cap,
                             const DcalcAnalysisPt* dcalc_ap)
{
  ArcDelay delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(drvr_port, load_cap, dcalc_ap, delays, slews);
  return max(delays[RiseFall::riseIndex()], delays[RiseFall::fallIndex()]);
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
  sta::UnorderedMap<const LibertyPort*, std::vector<bool>> port_stimulus;
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
    sta::UnorderedMap<const LibertyPort*, std::vector<bool>>& port_stimulus,
    size_t table_index)
{
  using Operator = sta::FuncExpr::Operator;
  const Operator curr_op = expr->op();

  switch (curr_op) {
    case Operator::op_not:
      return !simulateExpr(expr->left(), port_stimulus, table_index);
    case Operator::op_and:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             && simulateExpr(expr->right(), port_stimulus, table_index);
    case Operator::op_or:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             || simulateExpr(expr->right(), port_stimulus, table_index);
    case Operator::op_xor:
      return simulateExpr(expr->left(), port_stimulus, table_index)
             ^ simulateExpr(expr->right(), port_stimulus, table_index);
    case Operator::op_one:
      return true;
    case Operator::op_zero:
      return false;
    case Operator::op_port:
      return port_stimulus[expr->port()][table_index];
  }

  logger_->error(RSZ, 91, "unrecognized expr op from OpenSTA");
  return false;
}

std::vector<bool> BaseMove::simulateExpr(
    sta::FuncExpr* expr,
    sta::UnorderedMap<const LibertyPort*, std::vector<bool>>& port_stimulus)
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
// For side fanout paths (fanout paths of side_out_pin*), accept buffer removal
// if slack doesn't become violating (no new violations)
//
//               input_net                             output_net
//  prev_drv_pin ------>  (drvr_input_pin   drvr_pin)  ------>
//               |
//               ------>  (side_input_pin1  side_out_pin1) ----->
//               |
//               ------>  (side_input_pin2  side_out_pin2) ----->
//
bool BaseMove::estimatedSlackOK(const SlackEstimatorParams& params)
{
  if (params.corner == nullptr) {
    // can't do any estimation without a corner
    return false;
  }

  // Prep for delay calc
  GraphDelayCalc* dcalc = sta_->graphDelayCalc();
  const DcalcAnalysisPt* dcalc_ap
      = params.corner->findDcalcAnalysisPt(resizer_->max_);
  LibertyPort* prev_drvr_port = network_->libertyPort(params.prev_driver_pin);
  if (prev_drvr_port == nullptr) {
    return false;
  }
  LibertyPort *buffer_input_port, *buffer_output_port;
  params.driver_cell->bufferPorts(buffer_input_port, buffer_output_port);
  const RiseFall* prev_driver_rf = params.prev_driver_path->transition(sta_);

  // Compute delay degradation at prev driver due to increased load cap
  resizer_->annotateInputSlews(network_->instance(params.prev_driver_pin),
                               dcalc_ap);
  ArcDelay old_delay[RiseFall::index_count], new_delay[RiseFall::index_count];
  Slew old_slew[RiseFall::index_count], new_slew[RiseFall::index_count];
  float old_cap = dcalc->loadCap(params.prev_driver_pin, dcalc_ap);
  resizer_->gateDelays(prev_drvr_port, old_cap, dcalc_ap, old_delay, old_slew);
  float new_cap = old_cap + dcalc->loadCap(params.driver_pin, dcalc_ap)
                  - resizer_->portCapacitance(buffer_input_port, params.corner);
  resizer_->gateDelays(prev_drvr_port, new_cap, dcalc_ap, new_delay, new_slew);
  float delay_degrad
      = new_delay[prev_driver_rf->index()] - old_delay[prev_driver_rf->index()];
  float delay_imp
      = resizer_->bufferDelay(params.driver_cell,
                              params.driver_path->transition(sta_),
                              dcalc->loadCap(params.driver_pin, dcalc_ap),
                              dcalc_ap);
  resizer_->resetInputSlews();

  // Check if degraded delay & slew can be absorbed by driver pin fanouts
  Net* output_net = network_->net(params.driver_pin);
  NetConnectedPinIterator* pin_iter
      = network_->connectedPinIterator(output_net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (pin == params.driver_pin) {
      continue;
    }
    float old_slack = sta_->pinSlack(pin, resizer_->max_);
    float new_slack = old_slack - delay_degrad + delay_imp;
    if (fuzzyGreater(old_slack, new_slack)) {
      // clang-format off
      debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed "
                 "because new output pin slack {} is worse than old slack {}",
                 db_network_->name(params.driver), db_network_->name(pin),
                 new_slack, old_slack);
      // clang-format on
      return false;
    }

    // Check if output pin of direct fanout instance can absorb delay and slew
    // degradation
    if (!estimateInputSlewImpact(network_->instance(pin),
                                 dcalc_ap,
                                 old_slew,
                                 new_slew,
                                 delay_degrad - delay_imp,
                                 params,
                                 /* accept if slack improves */ true)) {
      return false;
    }
  }

  // Check side fanout paths.  Side fanout paths get no delay benefit from
  // buffer removal.
  Net* input_net = network_->net(params.prev_driver_pin);
  pin_iter = network_->connectedPinIterator(input_net);
  while (pin_iter->hasNext()) {
    const Pin* side_input_pin = pin_iter->next();
    if (side_input_pin == params.prev_driver_pin
        || side_input_pin == params.driver_input_pin) {
      continue;
    }
    float old_slack = sta_->pinSlack(side_input_pin, resizer_->max_);
    float new_slack = old_slack - delay_degrad - params.setup_slack_margin;
    if (new_slack < 0) {
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

    // Consider secondary degradation at side out pin from degraded input
    // slew.
    if (!estimateInputSlewImpact(network_->instance(side_input_pin),
                                 dcalc_ap,
                                 old_slew,
                                 new_slew,
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
                                       const DcalcAnalysisPt* dcalc_ap,
                                       Slew old_in_slew[RiseFall::index_count],
                                       Slew new_in_slew[RiseFall::index_count],
                                       // delay adjustment from prev stage
                                       float delay_adjust,
                                       SlackEstimatorParams params,
                                       bool accept_if_slack_improves)
{
  GraphDelayCalc* dcalc = sta_->graphDelayCalc();
  InstancePinIterator* pin_iter = network_->pinIterator(instance);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (!network_->direction(pin)->isOutput()) {
      continue;
    }
    LibertyPort* port = network_->libertyPort(pin);
    if (port == nullptr) {
      // reject the transform if we can't estimate
      // clang-format off
      debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed"
                 "because pin {} has no liberty port",
                 db_network_->name(params.driver), db_network_->name(pin));
      // clang-format on
      return false;
    }
    float load_cap = dcalc->loadCap(pin, dcalc_ap);
    ArcDelay old_delay[RiseFall::index_count], new_delay[RiseFall::index_count];
    Slew old_slew[RiseFall::index_count], new_slew[RiseFall::index_count];
    resizer_->gateDelays(
        port, load_cap, old_in_slew, dcalc_ap, old_delay, old_slew);
    resizer_->gateDelays(
        port, load_cap, new_in_slew, dcalc_ap, new_delay, new_slew);
    float delay_diff = max(
        new_delay[RiseFall::riseIndex()] - old_delay[RiseFall::riseIndex()],
        new_delay[RiseFall::fallIndex()] - old_delay[RiseFall::fallIndex()]);

    float old_slack
        = sta_->pinSlack(pin, resizer_->max_) - params.setup_slack_margin;
    float new_slack
        = old_slack - delay_diff - delay_adjust - params.setup_slack_margin;
    if ((accept_if_slack_improves && fuzzyGreater(old_slack, new_slack))
        || (!accept_if_slack_improves && new_slack < 0)) {
      // clang-format off
      debugPrint(logger_, RSZ, "remove_buffer", 1, "buffer {} is not removed"
                 "because pin {} will have a violating or worse slack of {}",
                 db_network_->name(params.driver), db_network_->name(pin),
                 new_slack);
      // clang-format on
      return false;
    }
  }

  return true;
}

bool BaseMove::hasPort(const Net* net)
{
  if (!net) {
    return false;
  }

  dbNet* db_net = db_network_->staToDb(net);
  return !db_net->getBTerms().empty();
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

////////////////////////////////////////////////////////////////
// namespace rsz
}  // namespace rsz
