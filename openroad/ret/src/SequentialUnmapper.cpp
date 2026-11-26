// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "SequentialUnmapper.h"

#include <algorithm>
#include <cstddef>
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

bool hasScanPin(odb::dbMaster* master)
{
  for (odb::dbMTerm* mterm : master->getMTerms()) {
    if (mterm->getSigType() == odb::dbSigType::SCAN) {
      return true;
    }
  }
  return false;
}

int outputCount(const sta::LibertyCell* cell)
{
  int outputs = 0;
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    if (port_iter.next()->direction()->isOutput()) {
      ++outputs;
    }
  }
  return outputs;
}

const sta::LibertyPort* statePort(const sta::LibertyCell* cell,
                                  const sta::Sequential* seq)
{
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    const sta::LibertyPort* port = port_iter.next();
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
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    const sta::LibertyPort* port = port_iter.next();
    if (port->direction()->isPowerGround()) {
      continue;
    }
    if (port->direction()->isOutput()) {
      output = port;
      ++output_count;
    } else {
      ++input_count;
    }
  }
  return input_count == inputs && output_count == 1 ? output : nullptr;
}

// `op` of input pins: NOT of one, AND or OR of two.
bool isGateOf(const sta::FuncExpr* func, const sta::FuncExpr::Op op)
{
  const auto is_pin = [](const sta::FuncExpr* expr) {
    return expr != nullptr && expr->op() == sta::FuncExpr::Op::port;
  };
  return func != nullptr && func->op() == op && is_pin(func->left())
         && (op == sta::FuncExpr::Op::not_ || is_pin(func->right()));
}

}  // namespace

SequentialUnmapper::SequentialUnmapper(odb::dbDatabase* db,
                                       sta::dbSta* sta,
                                       utl::Logger* logger,
                                       rsz::Resizer* resizer)
    : db_(db), sta_(sta), logger_(logger), resizer_(resizer)
{
}

bool SequentialUnmapper::usableCell(const sta::LibertyCell* cell,
                                    const bool tie) const
{
  return isUsableCell(cell, tie ? nullptr : resizer_);
}

bool SequentialUnmapper::prepare()
{
  skip_reported_ = false;
  bool ok = true;
  tie1_ = findTieCell(/*one=*/true);
  tie0_ = findTieCell(/*one=*/false);
  seq_ = findBaseRegister(/*negedge=*/false);
  if (seq_.master == nullptr) {
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
  negedge_seq_ = findBaseRegister(/*negedge=*/true);
  if (negedge_seq_.master == nullptr) {
    debugPrint(logger_, utl::RET, "cells", 1, "no negative-edge register cell");
  }
  return ok;
}

void SequentialUnmapper::reportTiedBase() const
{
  std::string ties;
  for (const auto& [mterm, high] : seq_.ties) {
    ties += ties.empty() ? "" : ", ";
    ties += mterm->getName() + (high ? " tied high" : " tied low");
  }
  if (!ties.empty()) {
    logger_->info(utl::RET,
                  47,
                  "No plain D flip-flop; new registers are {} with {}.",
                  seq_.master->getName(),
                  ties);
  }
}

SequentialUnmapper::BaseRegister SequentialUnmapper::baseRegister(
    const sta::LibertyCell* cell,
    const bool negedge) const
{
  if (!usableCell(cell) || cell->testCell() != nullptr) {
    return {};
  }
  const sta::Sequential* seq = singleSequential(cell);
  if (seq == nullptr || !seq->isRegister() || !hasSimpleClock(seq)
      || isNegedge(seq) != negedge || seq->output() == nullptr
      || seq->outputInv() == nullptr) {
    return {};
  }
  const sta::FuncExpr* data = seq->data();
  const sta::LibertyPort* q_port = statePort(cell, seq);
  odb::dbMaster* master = db_->findMaster(cell->name().c_str());
  if (data == nullptr || data->op() != sta::FuncExpr::Op::port
      || q_port == nullptr || master == nullptr || hasScanPin(master)) {
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
    const sta::Sequential* seq = singleSequential(cell);
    if (candidate.master == nullptr || seq->clear() != nullptr
        || seq->preset() != nullptr) {
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
  PinConstants constants;
  if (!inactiveAsyncControls(cell, singleSequential(cell), constants)) {
    return false;
  }
  area = cell->area();
  for (const auto& [port, high] : constants) {
    odb::dbMTerm* mterm = base.master->findMTerm(port->name().c_str());
    const TieCell& tie = tieCell(high);
    if (mterm == nullptr || tie.master == nullptr || tie.output == nullptr) {
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
    if (!usableCell(cell) || cell->isSequential()) {
      continue;
    }
    const sta::LibertyPort* output = onlyOutput(cell, inputs);
    if (output == nullptr || !isGateOf(output->function(), op)) {
      continue;
    }
    odb::dbMaster* candidate = db_->findMaster(cell->name().c_str());
    if (candidate != nullptr && (best == nullptr || cell->area() < best_area)) {
      best = candidate;
      best_area = cell->area();
    }
  }
  return best;
}

SequentialUnmapper::TieCell SequentialUnmapper::findTieCell(
    const bool one) const
{
  const sta::FuncExpr::Op op
      = one ? sta::FuncExpr::Op::one : sta::FuncExpr::Op::zero;
  TieCell best;
  for (sta::LibertyCell* cell : libertyCells(sta_)) {
    if (!usableCell(cell, /*tie=*/true) || cell->isSequential()) {
      continue;
    }
    sta::LibertyCellPortIterator port_iter(cell);
    while (port_iter.hasNext()) {
      sta::LibertyPort* port = port_iter.next();
      const sta::FuncExpr* func = port->function();
      if (!port->direction()->isOutput() || func == nullptr
          || func->op() != op) {
        continue;
      }
      odb::dbMaster* candidate = db_->findMaster(cell->name().c_str());
      if (candidate != nullptr
          && (best.master == nullptr || cell->area() < best.area)) {
        best = {
            .master = candidate,
            .output = candidate->findMTerm(port->name().c_str()),
            .area = cell->area(),
        };
      }
    }
  }
  return best;
}

int SequentialUnmapper::run(const std::unordered_set<odb::dbInst*>* candidates,
                            Discards* discards)
{
  block_ = db_->getChip()->getBlock();
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
    insts.assign(block_->getInsts().begin(), block_->getInsts().end());
  }
  std::vector<Register> registers;
  for (odb::dbInst* inst : insts) {
    if (!inst->getMaster()->isSequential()) {
      continue;
    }
    const sta::Sequential* seq = singleSequential(sta_, inst);
    if (seq == nullptr || (seq->isRegister() && !hasSimpleClock(seq))) {
      ++untouched.other;
    } else if (!seq->isRegister()) {
      ++untouched.latches;
    } else if (hasLiveAsyncControl(sta_, inst, seq)) {
      ++untouched.async;
    } else if (isScanCell(sta_, inst)) {
      ++untouched.scan;
    } else {
      registers.push_back({.inst = inst, .seq = seq});
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

bool SequentialUnmapper::canSynth(const sta::FuncExpr* expr) const
{
  if (expr == nullptr) {
    return false;
  }
  switch (expr->op()) {
    case sta::FuncExpr::Op::port: {
      const sta::LibertyPort* port = expr->port();
      return port == out_port_ || port == out_inv_port_
             || in_port_net_.contains(port);
    }
    case sta::FuncExpr::Op::not_:
      return canSynth(expr->left());
    case sta::FuncExpr::Op::and_:
    case sta::FuncExpr::Op::or_:
    case sta::FuncExpr::Op::xor_:
      return canSynth(expr->left()) && canSynth(expr->right());
    case sta::FuncExpr::Op::one:
    case sta::FuncExpr::Op::zero:
      return tieCell(expr->op() == sta::FuncExpr::Op::one).master != nullptr;
  }
  return false;
}

void SequentialUnmapper::decompose(odb::dbInst* inst,
                                   const sta::Sequential* seq)
{
  in_port_net_.clear();
  mod_net_.clear();
  setParent(inst);
  out_port_ = seq->output();
  out_inv_port_ = seq->outputInv();

  const std::vector<odb::dbITerm*> out_pins = collectRegisterNets(inst);
  odb::dbNet* clock_net = registerClockNet(inst);
  const BaseRegister& base = isNegedge(seq) ? negedge_seq_ : seq_;
  if (base.master == nullptr || clock_net == nullptr || out_pins.empty()
      || !canSynth(seq->data())) {
    skip(inst, "no gates for its next state");
    return;
  }
  if (!outputsReadState(seq, out_pins)) {
    skip(inst, "an output does not read the state");
    return;
  }

  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbInst* flop = makeCell(sta_, parent_, "unmap_ff", base.master);
  odb::dbITerm* clock_pin = registerClockPin(flop);
  if (clock_pin == nullptr) {
    logger_->warn(utl::RET,
                  13,
                  "Cannot rebuild {}: base cell {} has no usable D, Q or "
                  "clock pin.",
                  inst->getName(),
                  base.master->getName());
    network->deleteInstance(network->dbToSta(flop));
    ++skipped_;
    return;
  }

  state_net_ = makeNet(sta_, parent_, "unmap_state");
  flop->getITerm(base.output)->connect(state_net_);
  connectPin(clock_pin, clock_net, mod_net_);
  connectPin(flop->getITerm(base.data), synth(seq->data()), mod_net_);
  for (const auto& [mterm, high] : base.ties) {
    flop->getITerm(mterm)->connect(makeTie(high));
  }

  const std::vector<OutputMove> moves = collectOutputMoves(inst, seq, out_pins);
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

std::vector<odb::dbITerm*> SequentialUnmapper::collectRegisterNets(
    odb::dbInst* inst)
{
  std::vector<odb::dbITerm*> out_pins;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::INOUT
        || iterm->getSigType() == odb::dbSigType::POWER
        || iterm->getSigType() == odb::dbSigType::GROUND) {
      continue;
    }
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      const sta::LibertyPort* port = libertyPort(sta_, iterm);
      if (port != nullptr && iterm->getNet() != nullptr) {
        in_port_net_[port] = iterm->getNet();
        if (iterm->getModNet() != nullptr) {
          mod_net_[iterm->getNet()] = iterm->getModNet();
        }
      }
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT
               && iterm->getNet() != nullptr
               && (iterm->getNet()->getITerms().size() > 1
                   || !iterm->getNet()->getBTerms().empty())) {
      out_pins.push_back(iterm);
    }
  }
  return out_pins;
}

bool SequentialUnmapper::outputsReadState(
    const sta::Sequential* seq,
    const std::vector<odb::dbITerm*>& out_pins) const
{
  for (odb::dbITerm* out_pin : out_pins) {
    const sta::LibertyPort* port = libertyPort(sta_, out_pin);
    if (port == nullptr || !outputInverted(port->function(), seq).has_value()) {
      return false;
    }
  }
  return true;
}

std::vector<SequentialUnmapper::OutputMove>
SequentialUnmapper::collectOutputMoves(
    odb::dbInst* inst,
    const sta::Sequential* seq,
    const std::vector<odb::dbITerm*>& out_pins)
{
  std::vector<OutputMove> moves;
  for (odb::dbITerm* out_pin : out_pins) {
    const sta::LibertyPort* port = libertyPort(sta_, out_pin);
    // outputsReadState has checked every output.
    const bool inverted = outputInverted(port->function(), seq).value_or(false);
    OutputMove move;
    move.old_net = out_pin->getNet();
    move.old_mod_net = out_pin->getModNet();
    move.new_net = inverted ? makeInverter(state_net_) : state_net_;
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
      load->disconnect();
      load->connect(move.new_net);
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

odb::dbNet* SequentialUnmapper::synth(const sta::FuncExpr* expr)
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
      return in_port_net_.at(port);
    }
    case sta::FuncExpr::Op::not_:
      return makeInverter(synth(expr->left()));
    case sta::FuncExpr::Op::and_:
      return makeAnd(synth(expr->left()), synth(expr->right()));
    case sta::FuncExpr::Op::or_:
      return makeOr(synth(expr->left()), synth(expr->right()));
    case sta::FuncExpr::Op::xor_: {
      odb::dbNet* a = synth(expr->left());
      odb::dbNet* b = synth(expr->right());
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

void SequentialUnmapper::setParent(odb::dbInst* inst)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbModInst* mod_inst
      = network->hasHierarchy() ? inst->getModule()->getModInst() : nullptr;
  parent_ = mod_inst != nullptr ? network->dbToSta(mod_inst)
                                : network->topInstance();
}

odb::dbNet* SequentialUnmapper::makeGate(const char* base_name,
                                         odb::dbMaster* master,
                                         const std::vector<odb::dbNet*>& inputs)
{
  odb::dbInst* gate = makeCell(sta_, parent_, base_name, master);
  odb::dbNet* output = makeNet(sta_, parent_, "unmap_net");
  size_t next = 0;
  for (odb::dbITerm* iterm : gate->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      connectPin(iterm, inputs[std::min(next, inputs.size() - 1)], mod_net_);
      ++next;
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
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
  const TieCell& cell = tieCell(one);
  odb::dbInst* tie
      = makeCell(sta_, parent_, one ? "unmap_tie1" : "unmap_tie0", cell.master);
  odb::dbNet* output = makeNet(sta_, parent_, "unmap_net");
  if (cell.output != nullptr) {
    tie->getITerm(cell.output)->connect(output);
  }
  return output;
}

}  // namespace ret
