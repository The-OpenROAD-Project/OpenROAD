// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "SequentialUnmapper.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "RegisterUtils.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "rsz/Resizer.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Sequential.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

int outputCount(const sta::LibertyCell* cell)
{
  return static_cast<int>(std::ranges::count_if(
      libertyPorts(cell), [](const sta::LibertyPort* port) {
        return port->direction()->isOutput();
      }));
}

const sta::LibertyPort* statePort(const sta::LibertyCell* cell,
                                  const sta::Sequential* seq)
{
  for (const sta::LibertyPort* port : libertyPorts(cell)) {
    const sta::FuncExpr* func = port->function();
    if (port->direction()->isOutput() && func != nullptr
        && func->op() == sta::FuncExpr::Op::port
        && func->port() == seq->output()) {
      return port;
    }
  }
  return nullptr;
}

// The output of a cell with this many inputs and one output, else nullptr.
const sta::LibertyPort* onlyOutput(const sta::LibertyCell* cell,
                                   const int inputs)
{
  const sta::LibertyPort* output = nullptr;
  int input_count = 0;
  int output_count = 0;
  for (const sta::LibertyPort* port : libertyPorts(cell)) {
    if (port->direction()->isPowerGround()) {
      continue;
    }
    if (port->direction()->isOutput()) {
      output = port;
      ++output_count;
    } else if (port->direction()->isInput()) {
      ++input_count;
    } else {
      return nullptr;
    }
  }
  return input_count == inputs && output_count == 1 ? output : nullptr;
}

// `op` of input pins: NOT of one, AND or OR of two different ones.
bool isGateOf(const sta::FuncExpr* func, const sta::FuncExpr::Op op)
{
  const auto is_input = [](const sta::FuncExpr* expr) {
    return expr != nullptr && expr->op() == sta::FuncExpr::Op::port
           && expr->port()->direction()->isInput();
  };
  return func != nullptr && func->op() == op && is_input(func->left())
         && (op == sta::FuncExpr::Op::not_
             || (is_input(func->right())
                 && func->right()->port() != func->left()->port()));
}

// A timing arc starts or ends at every signal pin of the cell.
bool isTimed(const sta::LibertyCell* cell)
{
  return std::ranges::all_of(
      libertyPorts(cell), [cell](const sta::LibertyPort* port) {
        const sta::PortDirection* direction = port->direction();
        return direction->isPowerGround() || direction->isInternal()
               || !cell->timingArcSets(port, nullptr).empty()
               || !cell->timingArcSets(nullptr, port).empty();
      });
}

// The first pin of the master that is an output always driving the constant
// (constantOutput); nullptr without one.
odb::dbMTerm* constantPin(const sta::LibertyCell* cell,
                          odb::dbMaster* master,
                          const bool one)
{
  for (const sta::LibertyPort* port : libertyPorts(cell)) {
    if (!port->direction()->isOutput() || constantOutput(port) != one) {
      continue;
    }
    odb::dbMTerm* pin = master->findMTerm(port->name().c_str());
    if (pin != nullptr) {
      return pin;
    }
  }
  return nullptr;
}

}  // namespace

SequentialUnmapper::SequentialUnmapper(odb::dbDatabase* db,
                                       sta::dbSta* sta,
                                       utl::Logger* logger,
                                       rsz::Resizer* resizer)
    : db_(db), sta_(sta), logger_(logger), resizer_(resizer)
{
}

odb::dbMaster* SequentialUnmapper::usableMaster(const sta::LibertyCell* cell,
                                                const bool tie) const
{
  odb::dbMaster* master = db_->findMaster(cell->name().c_str());
  const bool usable = isUsableCell(cell, tie ? nullptr : resizer_)
                      && (tie || isTimed(cell)) && master != nullptr
                      && hasLibertyPins(cell, master);
  return usable ? master : nullptr;
}

bool SequentialUnmapper::prepare()
{
  skip_reported_ = false;
  bool ok = true;
  // Tie cells first: a base register with asynchronous controls is priced
  // with them.
  tie1_ = findTieCell(/*one=*/true);
  tie0_ = findTieCell(/*one=*/false);
  base_ = findBaseRegister(/*negedge=*/false);
  if (base_.master == nullptr) {
    logger_->warn(utl::RET,
                  1,
                  "No usable base register; a positive-edge D flip-flop "
                  "is needed.");
    ok = false;
  }
  reportTiedBase();
  inv_master_ = smallestGate(sta::FuncExpr::Op::not_, 1);
  if (inv_master_ == nullptr) {
    logger_->warn(utl::RET, 2, "No inverter cell found.");
    ok = false;
  }
  and2_master_ = smallestGate(sta::FuncExpr::Op::and_, 2);
  if (and2_master_ == nullptr) {
    logger_->warn(utl::RET, 3, "No 2-input AND cell found.");
    ok = false;
  }
  or2_master_ = smallestGate(sta::FuncExpr::Op::or_, 2);
  if (or2_master_ == nullptr) {
    logger_->warn(utl::RET, 4, "No 2-input OR cell found.");
    ok = false;
  }
  negedge_base_ = findBaseRegister(/*negedge=*/true);
  if (negedge_base_.master == nullptr) {
    debugPrint(logger_, utl::RET, "cells", 1, "no negative-edge register cell");
  }
  return ok;
}

std::vector<odb::dbMaster*> SequentialUnmapper::newCellMasters() const
{
  std::vector<odb::dbMaster*> masters;
  for (odb::dbMaster* master : {
           base_.master,
           inv_master_,
           and2_master_,
           or2_master_,
           tie1_.master,
           tie0_.master,
       }) {
    if (master != nullptr) {
      masters.push_back(master);
    }
  }
  return masters;
}

void SequentialUnmapper::reportTiedBase() const
{
  std::string ties;
  for (const auto& [mterm, high] : base_.ties) {
    ties += ties.empty() ? "" : ", ";
    ties += mterm->getName() + (high ? " tied high" : " tied low");
  }
  if (!ties.empty()) {
    logger_->info(utl::RET,
                  47,
                  "No plain D flip-flop; new registers are {} with {}.",
                  base_.master->getName(),
                  ties);
  }
}

SequentialUnmapper::BaseRegister SequentialUnmapper::baseRegister(
    const sta::LibertyCell* cell,
    const bool negedge) const
{
  odb::dbMaster* master = usableMaster(cell);
  if (master == nullptr || cell->testCell() != nullptr) {
    return {};
  }
  const sta::Sequential* seq = singleSequential(cell);
  if (seq == nullptr || !seq->isRegister() || isNegedge(seq) != negedge
      || seq->output() == nullptr || seq->outputInv() == nullptr
      || hasUnreadInput(cell, seq)) {
    return {};
  }
  const sta::FuncExpr* data = seq->data();
  const sta::LibertyPort* q_port = statePort(cell, seq);
  if (data == nullptr || data->op() != sta::FuncExpr::Op::port
      || q_port == nullptr || hasScanPin(master)
      || clockMTerm(seq, master) == nullptr) {
    return {};
  }
  odb::dbMTerm* data_pin = master->findMTerm(data->port()->name().c_str());
  odb::dbMTerm* output_pin = master->findMTerm(q_port->name().c_str());
  if (data_pin == nullptr || output_pin == nullptr) {
    return {};
  }
  return {.master = master, .data = data_pin, .output = output_pin, .ties = {}};
}

SequentialUnmapper::BaseRegister SequentialUnmapper::findBaseRegister(
    const bool negedge) const
{
  BaseRegister base = plainBaseRegister(negedge);
  return base.master != nullptr ? base : tiedBaseRegister(negedge);
}

SequentialUnmapper::BaseRegister SequentialUnmapper::plainBaseRegister(
    const bool negedge) const
{
  BaseRegister best;
  float best_area = 0.0f;
  BaseRegister fallback;

  for (sta::LibertyCell* cell : libertyCells(sta_)) {
    const BaseRegister candidate = baseRegister(cell, negedge);
    if (candidate.master == nullptr
        || hasAsyncControl(singleSequential(cell))) {
      continue;
    }
    if (outputCount(cell) == 1) {
      if (best.master == nullptr || cell->area() < best_area) {
        best = candidate;
        best_area = cell->area();
      }
    } else if (fallback.master == nullptr) {
      fallback = candidate;
    }
  }

  if (best.master == nullptr && fallback.master != nullptr) {
    debugPrint(logger_,
               utl::RET,
               "cells",
               1,
               "multi-output register cell {} is the {} base cell",
               fallback.master->getName(),
               negedge ? "negative-edge" : "positive-edge");
    return fallback;
  }
  return best;
}

bool SequentialUnmapper::tieControls(const sta::LibertyCell* cell,
                                     BaseRegister& base,
                                     float& area) const
{
  const std::optional<PinConstants> constants
      = inactiveAsyncControls(cell, singleSequential(cell));
  if (!constants.has_value()) {
    return false;
  }
  area = cell->area();
  for (const auto& [port, high] : *constants) {
    odb::dbMTerm* mterm = base.master->findMTerm(port->name().c_str());
    const TieCell& tie = tieCell(high);
    if (mterm == nullptr || tie.master == nullptr) {
      return false;
    }
    base.ties.push_back({.pin = mterm, .high = high});
    area += tie.area;
  }
  return true;
}

SequentialUnmapper::BaseRegister SequentialUnmapper::tiedBaseRegister(
    const bool negedge) const
{
  BaseRegister best;
  float best_area = 0.0f;
  float best_cap = 0.0f;
  for (sta::LibertyCell* cell : libertyCells(sta_)) {
    BaseRegister candidate = baseRegister(cell, negedge);
    float area = 0.0f;
    if (candidate.master == nullptr || !tieControls(cell, candidate, area)) {
      continue;
    }
    const float cap = singleSequential(cell)->data()->port()->capacitance();
    if (best.master == nullptr || area < best_area
        || (area == best_area && cap < best_cap)) {
      best = std::move(candidate);
      best_area = area;
      best_cap = cap;
    }
  }
  return best;
}

odb::dbMaster* SequentialUnmapper::smallestGate(const sta::FuncExpr::Op op,
                                                const int inputs) const
{
  odb::dbMaster* best = nullptr;
  float best_area = 0.0f;
  for (sta::LibertyCell* cell : libertyCells(sta_)) {
    odb::dbMaster* candidate = usableMaster(cell);
    if (candidate == nullptr || cell->isSequential()) {
      continue;
    }
    const sta::LibertyPort* output = onlyOutput(cell, inputs);
    if (output == nullptr || !isGateOf(output->function(), op)) {
      continue;
    }
    if (best == nullptr || cell->area() < best_area) {
      best = candidate;
      best_area = cell->area();
    }
  }
  return best;
}

SequentialUnmapper::TieCell SequentialUnmapper::findTieCell(
    const bool one) const
{
  TieCell best;
  for (sta::LibertyCell* cell : libertyCells(sta_)) {
    odb::dbMaster* candidate = usableMaster(cell, /*tie=*/true);
    if (candidate == nullptr || cell->isSequential()
        || hasUnreadInput(cell, nullptr)) {
      continue;
    }
    odb::dbMTerm* output = constantPin(cell, candidate, one);
    if (output != nullptr
        && (best.master == nullptr || cell->area() < best.area)) {
      best = {.master = candidate, .output = output, .area = cell->area()};
    }
  }
  return best;
}

int SequentialUnmapper::run(const std::unordered_set<odb::dbInst*>* candidates,
                            Discards* discards)
{
  Discards own_discards;
  discards_ = discards != nullptr ? discards : &own_discards;
  Untouched untouched;
  const std::vector<Register> registers
      = collectRegisters(candidates, untouched);

  unmapped_ = 0;
  skipped_ = 0;
  for (const auto& [inst, seq] : registers) {
    decompose(inst, seq);
  }
  own_discards.destroy(sta_);
  discards_ = nullptr;
  reportRun(untouched);
  return unmapped_;
}

std::vector<SequentialUnmapper::Register> SequentialUnmapper::collectRegisters(
    const std::unordered_set<odb::dbInst*>* candidates,
    Untouched& untouched) const
{
  std::vector<odb::dbInst*> insts;
  if (candidates != nullptr) {
    insts = inIdOrder(*candidates);
  } else {
    odb::dbBlock* block = db_->getChip()->getBlock();
    insts.assign(block->getInsts().begin(), block->getInsts().end());
  }
  std::vector<Register> registers;
  for (odb::dbInst* inst : insts) {
    if (!inst->getMaster()->isSequential()) {
      continue;
    }
    switch (untouchedKind(sta_, inst)) {
      case UntouchedKind::kNone:
        registers.push_back(
            {.inst = inst, .seq = singleSequential(sta_, inst)});
        break;
      case UntouchedKind::kOther:
        ++untouched.other;
        break;
      case UntouchedKind::kLatch:
        ++untouched.latches;
        break;
      case UntouchedKind::kAsync:
        ++untouched.async;
        break;
      case UntouchedKind::kScan:
        ++untouched.scan;
        break;
    }
  }
  return registers;
}

void SequentialUnmapper::reportRun(const Untouched& untouched)
{
  logger_->info(utl::RET, 8, "Unmapped {} registers.", unmapped_);
  if (skipped_ > 0 && !skip_reported_) {
    logger_->warn(utl::RET,
                  9,
                  "{} registers not unmapped; they cannot be rebuilt from "
                  "the library cells.",
                  skipped_);
    skip_reported_ = true;
  }
  const int total
      = untouched.async + untouched.latches + untouched.scan + untouched.other;
  if (total > 0) {
    logger_->warn(utl::RET,
                  10,
                  "{} sequential cells not retimed: {} asynchronous, {} "
                  "latches, {} scan, {} other.",
                  total,
                  untouched.async,
                  untouched.latches,
                  untouched.scan,
                  untouched.other);
  }
}

bool SequentialUnmapper::canSynthesize(const sta::FuncExpr* expr) const
{
  if (expr == nullptr) {
    return false;
  }
  switch (expr->op()) {
    case sta::FuncExpr::Op::port: {
      const sta::LibertyPort* port = expr->port();
      return port == out_port_ || port == out_inv_port_
             || input_nets_.contains(port);
    }
    case sta::FuncExpr::Op::not_:
      return canSynthesize(expr->left());
    case sta::FuncExpr::Op::and_:
    case sta::FuncExpr::Op::or_:
    case sta::FuncExpr::Op::xor_:
      return canSynthesize(expr->left()) && canSynthesize(expr->right());
    case sta::FuncExpr::Op::one:
    case sta::FuncExpr::Op::zero:
      return tieCell(expr->op() == sta::FuncExpr::Op::one).master != nullptr;
  }
  return false;
}

void SequentialUnmapper::decompose(odb::dbInst* inst,
                                   const sta::Sequential* seq)
{
  selectRegister(inst, seq);
  const std::vector<LoadedOutput> outputs = loadedOutputs(inst, seq);
  odb::dbNet* clock_net = registerClockNet(sta_, inst);
  const BaseRegister& base = isNegedge(seq) ? negedge_base_ : base_;
  if (base.master == nullptr || clock_net == nullptr || outputs.empty()
      || !canSynthesize(seq->data())) {
    skip(inst, "no gates for its next state");
    return;
  }
  const bool outputs_read_state = std::ranges::all_of(
      outputs,
      [](const LoadedOutput& output) { return output.inverted.has_value(); });
  if (!outputs_read_state) {
    skip(inst, "an output does not read the state");
    return;
  }
  if (!makeBaseRegister(base, clock_net, seq->data())) {
    return;
  }

  // The moves are collected before the discard disconnects the register and
  // applied after it: discardRegister takes an output net without loads as
  // the register's own.
  const std::vector<OutputMove> moves = collectOutputMoves(inst, outputs);
  discardRegister(sta_, inst, *discards_);
  applyOutputMoves(moves);
  ++unmapped_;
}

void SequentialUnmapper::skip(odb::dbInst* inst, const char* reason)
{
  debugPrint(logger_,
             utl::RET,
             "unmap",
             1,
             "skip {}, cell {}: {}",
             inst->getName(),
             inst->getMaster()->getName(),
             reason);
  ++skipped_;
}

void SequentialUnmapper::selectRegister(odb::dbInst* inst,
                                        const sta::Sequential* seq)
{
  register_ = inst;
  parent_ = parentInstance(sta_, inst);
  out_port_ = seq->output();
  out_inv_port_ = seq->outputInv();
  input_nets_.clear();
  mod_nets_.clear();
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (!iterm->isInputSignal(/*io=*/false)) {
      continue;
    }
    const sta::LibertyPort* port = libertyPort(sta_, iterm);
    odb::dbNet* net = iterm->getNet();
    if (port != nullptr && net != nullptr) {
      input_nets_[port] = net;
      if (iterm->getModNet() != nullptr) {
        mod_nets_[net] = iterm->getModNet();
      }
    }
  }
}

std::vector<SequentialUnmapper::LoadedOutput> SequentialUnmapper::loadedOutputs(
    odb::dbInst* inst,
    const sta::Sequential* seq) const
{
  std::vector<LoadedOutput> outputs;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType().isSupply() || !drivesNet(iterm)
        || !isRead(iterm->getNet())) {
      continue;
    }
    outputs.push_back(
        {.pin = iterm, .inverted = outputInverted(sta_, iterm, seq)});
  }
  return outputs;
}

bool SequentialUnmapper::makeBaseRegister(const BaseRegister& base,
                                          odb::dbNet* clock_net,
                                          const sta::FuncExpr* next_state)
{
  odb::dbInst* flop
      = makeCell(sta_, parent_, "unmap_ff", base.master, register_);
  odb::dbITerm* clock_pin = registerClockPin(sta_, flop);
  if (clock_pin == nullptr) {
    logger_->warn(utl::RET,
                  13,
                  "Cannot rebuild {}: base cell {} has no usable D, Q or "
                  "clock pin.",
                  register_->getName(),
                  base.master->getName());
    sta::dbNetwork* network = sta_->getDbNetwork();
    network->deleteInstance(network->dbToSta(flop));
    ++skipped_;
    return false;
  }

  state_net_ = makeNet(sta_, parent_, "unmap_state");
  flop->getITerm(base.output)->connect(state_net_);
  connectPin(clock_pin, clock_net, mod_nets_);
  connectPin(flop->getITerm(base.data), synthesize(next_state), mod_nets_);
  for (const auto& [mterm, high] : base.ties) {
    flop->getITerm(mterm)->connect(makeTie(high));
  }
  return true;
}

std::vector<SequentialUnmapper::OutputMove>
SequentialUnmapper::collectOutputMoves(odb::dbInst* inst,
                                       const std::vector<LoadedOutput>& outputs)
{
  std::vector<OutputMove> moves;
  for (const LoadedOutput& output : outputs) {
    OutputMove move;
    move.old_net = output.pin->getNet();
    move.old_mod_net = output.pin->getModNet();
    move.new_net = output.inverted.value_or(false) ? makeInverter(state_net_)
                                                   : state_net_;
    for (odb::dbITerm* load : move.old_net->getITerms()) {
      if (load->getInst() != inst) {
        move.loads.push_back(load);
      }
    }
    for (odb::dbBTerm* bterm : move.old_net->getBTerms()) {
      move.ports.push_back(bterm);
    }
    moves.push_back(std::move(move));
  }
  return moves;
}

void SequentialUnmapper::applyOutputMoves(const std::vector<OutputMove>& moves)
{
  std::unordered_set<odb::dbNet*> discarded;
  for (const OutputMove& move : moves) {
    for (odb::dbITerm* load : move.loads) {
      movePin(load, move.new_net);
    }
    for (odb::dbBTerm* bterm : move.ports) {
      bterm->disconnect();
      bterm->connect(move.new_net);
    }
    if (discarded.insert(move.old_net).second) {
      discards_->add(move.old_net, move.old_mod_net);
    }
  }
}

odb::dbNet* SequentialUnmapper::synthesize(const sta::FuncExpr* expr)
{
  switch (expr->op()) {
    case sta::FuncExpr::Op::port: {
      const sta::LibertyPort* port = expr->port();
      if (port == out_port_) {
        return state_net_;
      }
      if (port == out_inv_port_) {
        return makeInverter(state_net_);
      }
      return input_nets_.at(port);
    }
    case sta::FuncExpr::Op::not_:
      return makeInverter(synthesize(expr->left()));
    // Operands go into locals so that the left one's gates are made, and
    // named, first.
    case sta::FuncExpr::Op::and_: {
      odb::dbNet* a = synthesize(expr->left());
      odb::dbNet* b = synthesize(expr->right());
      return makeAnd(a, b);
    }
    case sta::FuncExpr::Op::or_: {
      odb::dbNet* a = synthesize(expr->left());
      odb::dbNet* b = synthesize(expr->right());
      return makeOr(a, b);
    }
    case sta::FuncExpr::Op::xor_: {
      odb::dbNet* a = synthesize(expr->left());
      odb::dbNet* b = synthesize(expr->right());
      odb::dbNet* a_not_b = makeAnd(a, makeInverter(b));
      odb::dbNet* not_a_b = makeAnd(makeInverter(a), b);
      return makeOr(a_not_b, not_a_b);
    }
    case sta::FuncExpr::Op::one:
      return makeTie(/*one=*/true);
    case sta::FuncExpr::Op::zero:
      return makeTie(/*one=*/false);
  }
  return nullptr;
}

odb::dbNet* SequentialUnmapper::makeGate(const char* base_name,
                                         odb::dbMaster* master,
                                         const std::vector<odb::dbNet*>& inputs)
{
  odb::dbInst* gate = makeCell(sta_, parent_, base_name, master, register_);
  odb::dbNet* output = makeNet(sta_, parent_, "unmap_net");
  size_t next = 0;
  for (odb::dbITerm* iterm : gate->getITerms()) {
    if (iterm->isInputSignal(/*io=*/false)) {
      connectPin(iterm, inputs[next++], mod_nets_);
    } else if (iterm->isOutputSignal(/*io=*/false)) {
      iterm->connect(output);
    }
  }
  return output;
}

odb::dbNet* SequentialUnmapper::makeInverter(odb::dbNet* input)
{
  return makeGate("unmap_inv", inv_master_, {input});
}

odb::dbNet* SequentialUnmapper::makeAnd(odb::dbNet* a, odb::dbNet* b)
{
  return makeGate("unmap_and", and2_master_, {a, b});
}

odb::dbNet* SequentialUnmapper::makeOr(odb::dbNet* a, odb::dbNet* b)
{
  return makeGate("unmap_or", or2_master_, {a, b});
}

odb::dbNet* SequentialUnmapper::makeTie(const bool one)
{
  return makeTieNet(
      parent_, one ? "unmap_tie1" : "unmap_tie0", "unmap_net", one, register_);
}

odb::dbNet* SequentialUnmapper::makeTieNet(sta::Instance* parent,
                                           const char* cell_name,
                                           const char* net_name,
                                           const bool one,
                                           odb::dbInst* like) const
{
  const TieCell& cell = tieCell(one);
  odb::dbInst* tie = makeCell(sta_, parent, cell_name, cell.master, like);
  odb::dbNet* output = makeNet(sta_, parent, net_name);
  tie->getITerm(cell.output)->connect(output);
  return output;
}

}  // namespace ret
