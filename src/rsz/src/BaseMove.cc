// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"

#include "rsz/Resizer.hh"


namespace rsz {


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

   count_ = 0;
   all_count_ = 0;
   all_inst_set_ = InstanceSet(db_network_);

}

void
BaseMove::commitMoves()
{
    all_count_ += count_;
}

void
BaseMove::init()
{
    count_ = 0;
    all_count_ = 0;
    all_inst_set_.clear();
}

void
BaseMove::restoreMoves()
{
    count_ = 0;
} 

int
BaseMove::countMoves(Instance *inst) const 
{ 
    return all_inst_set_.count(inst);
}

int
BaseMove::pendingMoves() const 
{ 
    return count_;
} 

int
BaseMove::committedMoves() const 
{ 
    return all_count_;
} 

int
BaseMove::countMoves() const 
{ 
    return all_count_ + count_;
} 

void
BaseMove::addMove(Instance *inst)
{ 
    // Add it as a candidate move, not accepted yet
    count_++;
    // Add it to all moves, even though it wasn't accepted.
    // This is the behavior to match the current resizer.
    all_inst_set_.insert(inst); 
}

double 
BaseMove::area(Cell* cell) 
{ 
    return area(db_network_->staToDb(cell)); 
}

double 
BaseMove::area(dbMaster* master)
{
  if (!master->isCoreAutoPlaceable()) {
    return 0;
  }
  return dbuToMeters(master->getWidth()) * dbuToMeters(master->getHeight());
}

double 
BaseMove::dbuToMeters(int dist) const
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

bool 
BaseMove::isPortEqiv(sta::FuncExpr* expr,
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
// namespace rsz
}

