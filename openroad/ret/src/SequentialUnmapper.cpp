// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "SequentialUnmapper.h"

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
  return !cell->dontUse()
         && (tie || resizer_ == nullptr || !resizer_->dontUse(cell))
         && !cell->isIsolationCell() && !cell->isLevelShifter()
         && !cell->alwaysOn() && !cell->isPad() && !cell->isMacro();
}

bool SequentialUnmapper::prepare()
{
  skip_reported_ = false;
  bool ok = true;
  findTieCell(true);
  findTieCell(false);
  if (!findBaseSequentialCell(false, seq_)
      && !findTiedBaseSequentialCell(false, seq_)) {
    logger_->warn(utl::RET,
                  1,
                  "No usable base register; a positive-edge D flip-flop "
                  "is needed.");
    ok = false;
  }
  reportTiedBase();
  if (!findInverterCell()) {
    logger_->warn(utl::RET, 2, "No inverter cell found.");
    ok = false;
  }
  and2_master_ = smallestTwoInputGate(true);
  if (and2_master_ == nullptr) {
    logger_->warn(utl::RET, 3, "No 2-input AND cell found.");
    ok = false;
  }
  or2_master_ = smallestTwoInputGate(false);
  if (or2_master_ == nullptr) {
    logger_->warn(utl::RET, 4, "No 2-input OR cell found.");
    ok = false;
  }
  if (!findBaseSequentialCell(true, negedge_seq_)
      && !findTiedBaseSequentialCell(true, negedge_seq_)) {
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

SequentialUnmapper::BaseRegister SequentialUnmapper::makeBaseRegister(
    const sta::LibertyCell* cell,
    const sta::LibertyPort* data,
    const sta::LibertyPort* output) const
{
  BaseRegister base;
  odb::dbMaster* master = db_->findMaster(cell->name().c_str());
  if (master != nullptr) {
    base.d = master->findMTerm(data->name().c_str());
    base.q = master->findMTerm(output->name().c_str());
  }
  if (base.d != nullptr && base.q != nullptr) {
    base.master = master;
  }
  return base;
}

bool SequentialUnmapper::findBaseSequentialCell(const bool negedge,
                                                BaseRegister& base) const
{
  BaseRegister best;
  float best_area = 0.0f;
  BaseRegister fallback;

  for (sta::LibertyCell* cell : libertyCells(sta_)) {
    if (!usableCell(cell) || cell->testCell() != nullptr) {
      continue;
    }
    const sta::Sequential* seq = singleSequential(cell);
    if (seq == nullptr || !seq->isRegister() || !hasSimpleClock(seq)
        || isNegedge(seq) != negedge || seq->clear() != nullptr
        || seq->preset() != nullptr || seq->output() == nullptr
        || seq->outputInv() == nullptr) {
      continue;
    }
    const sta::FuncExpr* data = seq->data();
    if (data == nullptr || data->op() != sta::FuncExpr::Op::port) {
      continue;
    }
    const sta::LibertyPort* q_port = statePort(cell, seq);
    if (q_port == nullptr) {
      continue;
    }
    const BaseRegister candidate
        = makeBaseRegister(cell, data->port(), q_port);
    if (candidate.master == nullptr || hasScanPin(candidate.master)) {
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
    best = std::move(fallback);
  }
  base = std::move(best);
  return base.master != nullptr;
}

bool SequentialUnmapper::tiedBaseRegister(const sta::LibertyCell* cell,
                                          const bool negedge,
                                          BaseRegister& base,
                                          float& area) const
{
  if (!usableCell(cell) || cell->testCell() != nullptr) {
    return false;
  }
  const sta::Sequential* seq = singleSequential(cell);
  if (seq == nullptr || !seq->isRegister() || !hasSimpleClock(seq)
      || isNegedge(seq) != negedge || seq->output() == nullptr
      || seq->outputInv() == nullptr) {
    return false;
  }
  const sta::FuncExpr* data = seq->data();
  const sta::LibertyPort* q_port = statePort(cell, seq);
  PinConstants constants;
  if (data == nullptr || data->op() != sta::FuncExpr::Op::port
      || q_port == nullptr || !inactiveAsyncControls(cell, seq, constants)) {
    return false;
  }
  base = makeBaseRegister(cell, data->port(), q_port);
  if (base.master == nullptr || hasScanPin(base.master)) {
    return false;
  }
  area = cell->area();
  for (const auto& [port, high] : constants) {
    odb::dbMTerm* mterm = base.master->findMTerm(port->name().c_str());
    if (mterm == nullptr || getTieMaster(high) == nullptr
        || getTieOutputMTerm(high) == nullptr) {
      return false;
    }
    base.ties.emplace_back(mterm, high);
    area += high ? tie1_area_ : tie0_area_;
  }
  return true;
}

bool SequentialUnmapper::findTiedBaseSequentialCell(const bool negedge,
                                                    BaseRegister& base) const
{
  base = BaseRegister();
  float best_area = 0.0f;
  float best_cap = 0.0f;
  for (sta::LibertyCell* cell : libertyCells(sta_)) {
    BaseRegister candidate;
    float area = 0.0f;
    if (!tiedBaseRegister(cell, negedge, candidate, area)) {
      continue;
    }
    const float cap = singleSequential(cell)->data()->port()->capacitance();
    if (base.master == nullptr || area < best_area
        || (area == best_area && cap < best_cap)) {
      base = std::move(candidate);
      best_area = area;
      best_cap = cap;
    }
  }
  return base.master != nullptr;
}

bool SequentialUnmapper::findInverterCell()
{
  odb::dbMaster* best = nullptr;
  float best_area = 0.0f;
  for (sta::LibertyCell* cell : libertyCells(sta_)) {
    if (!usableCell(cell) || cell->isSequential()) {
      continue;
    }
    int inputs = 0;
    int outputs = 0;
    bool inverting = false;
    sta::LibertyCellPortIterator port_iter(cell);
    while (port_iter.hasNext()) {
      sta::LibertyPort* port = port_iter.next();
      if (port->direction()->isPowerGround()) {
        continue;
      }
      if (!port->direction()->isOutput()) {
        ++inputs;
        continue;
      }
      ++outputs;
      const sta::FuncExpr* func = port->function();
      inverting = func != nullptr && func->op() == sta::FuncExpr::Op::not_
                  && func->left() != nullptr
                  && func->left()->op() == sta::FuncExpr::Op::port;
    }
    if (inputs != 1 || outputs != 1 || !inverting) {
      continue;
    }
    odb::dbMaster* candidate = db_->findMaster(cell->name().c_str());
    if (candidate != nullptr
        && (best == nullptr || cell->area() < best_area)) {
      best = candidate;
      best_area = cell->area();
    }
  }
  inv_master_ = best;
  return inv_master_ != nullptr;
}

odb::dbMaster* SequentialUnmapper::smallestTwoInputGate(const bool is_and) const
{
  const sta::FuncExpr::Op op
      = is_and ? sta::FuncExpr::Op::and_ : sta::FuncExpr::Op::or_;
  odb::dbMaster* best = nullptr;
  float best_area = 0.0f;
  for (sta::LibertyCell* cell : libertyCells(sta_)) {
    if (!usableCell(cell) || cell->isSequential()) {
      continue;
    }
    int inputs = 0;
    int outputs = 0;
    const sta::LibertyPort* out_port = nullptr;
    sta::LibertyCellPortIterator port_iter(cell);
    while (port_iter.hasNext()) {
      sta::LibertyPort* port = port_iter.next();
      if (port->direction()->isPowerGround()) {
        continue;
      }
      if (port->direction()->isOutput()) {
        out_port = port;
        ++outputs;
      } else {
        ++inputs;
      }
    }
    if (inputs != 2 || outputs != 1) {
      continue;
    }
    const sta::FuncExpr* func = out_port->function();
    if (func == nullptr || func->op() != op || func->left() == nullptr
        || func->right() == nullptr
        || func->left()->op() != sta::FuncExpr::Op::port
        || func->right()->op() != sta::FuncExpr::Op::port) {
      continue;
    }
    odb::dbMaster* candidate = db_->findMaster(cell->name().c_str());
    if (candidate != nullptr
        && (best == nullptr || cell->area() < best_area)) {
      best = candidate;
      best_area = cell->area();
    }
  }
  return best;
}

bool SequentialUnmapper::findTieCell(const bool one)
{
  const sta::FuncExpr::Op op
      = one ? sta::FuncExpr::Op::one : sta::FuncExpr::Op::zero;
  odb::dbMaster* best = nullptr;
  const sta::LibertyPort* best_port = nullptr;
  float best_area = 0.0f;
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
          && (best == nullptr || cell->area() < best_area)) {
        best = candidate;
        best_port = port;
        best_area = cell->area();
      }
    }
  }
  odb::dbMTerm* out_mterm
      = best != nullptr ? best->findMTerm(best_port->name().c_str()) : nullptr;
  if (one) {
    tie1_master_ = best;
    tie1_out_mterm_ = out_mterm;
    tie1_area_ = best_area;
  } else {
    tie0_master_ = best;
    tie0_out_mterm_ = out_mterm;
    tie0_area_ = best_area;
  }
  return best != nullptr && out_mterm != nullptr;
}

int SequentialUnmapper::run(const std::unordered_set<odb::dbInst*>* candidates)
{
  block_ = db_->getChip()->getBlock();
  Untouched untouched;
  const std::vector<Register> registers
      = collectRegisters(candidates, untouched);

  unmapped_ = 0;
  skipped_ = 0;
  for (const auto& [inst, seq] : registers) {
    decompose(inst, seq);
  }
  reportRun(untouched);
  return unmapped_;
}

std::vector<SequentialUnmapper::Register> SequentialUnmapper::collectRegisters(
    const std::unordered_set<odb::dbInst*>* candidates,
    Untouched& untouched) const
{
  std::vector<Register> registers;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (candidates != nullptr && !candidates->contains(inst)) {
      continue;
    }
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
      registers.emplace_back(inst, seq);
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
      return tie1_master_ != nullptr;
    case sta::FuncExpr::Op::zero:
      return tie0_master_ != nullptr;
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

  std::vector<odb::dbITerm*> out_pins;
  odb::dbNet* clock_net = collectRegisterNets(inst, out_pins);
  const BaseRegister& base = isNegedge(seq) ? negedge_seq_ : seq_;
  if (base.master == nullptr || clock_net == nullptr || out_pins.empty()
      || !canSynth(seq->data())) {
    debugPrint(logger_,
               utl::RET,
               "unmap",
               1,
               "skip {}, cell {}: no gates for its next state",
               inst->getName(),
               inst->getMaster()->getName());
    ++skipped_;
    return;
  }
  if (!outputsReadState(seq, out_pins)) {
    debugPrint(logger_,
               utl::RET,
               "unmap",
               1,
               "skip {}, cell {}: an output does not read the state",
               inst->getName(),
               inst->getMaster()->getName());
    ++skipped_;
    return;
  }

  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbInst* flop = newInst("unmap_ff", base.master);
  const RegPins pins = getRegPins(flop, singleSequential(sta_, flop));
  if (pins.d == nullptr || pins.q == nullptr || pins.clk == nullptr) {
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

  state_net_ = newNet("unmap_state");
  pins.q->connect(state_net_);
  connectPin(pins.clk, clock_net, mod_net_);
  connectPin(pins.d, synth(seq->data()), mod_net_);
  for (const auto& [mterm, high] : base.ties) {
    flop->getITerm(mterm)->connect(makeTie(high));
  }

  const std::vector<OutputMove> moves
      = collectOutputMoves(inst, seq, out_pins);
  deleteRegister(sta_, inst);
  applyOutputMoves(moves);
  ++unmapped_;
}

odb::dbNet* SequentialUnmapper::collectRegisterNets(
    odb::dbInst* inst,
    std::vector<odb::dbITerm*>& out_pins)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbNet* clock_net = nullptr;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::INOUT
        || iterm->getSigType() == odb::dbSigType::POWER
        || iterm->getSigType() == odb::dbSigType::GROUND) {
      continue;
    }
    if (iterm->getSigType() == odb::dbSigType::CLOCK) {
      clock_net = iterm->getNet();
    }
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      const sta::LibertyPort* port
          = network->libertyPort(network->dbToSta(iterm));
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
  return clock_net;
}

bool SequentialUnmapper::outputsReadState(
    const sta::Sequential* seq,
    const std::vector<odb::dbITerm*>& out_pins) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  for (odb::dbITerm* out_pin : out_pins) {
    const sta::LibertyPort* port
        = network->libertyPort(network->dbToSta(out_pin));
    if (port == nullptr
        || !outputInverted(port->function(), seq).has_value()) {
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
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::vector<OutputMove> moves;
  for (odb::dbITerm* out_pin : out_pins) {
    const sta::LibertyPort* port
        = network->libertyPort(network->dbToSta(out_pin));
    const bool inverted = outputInverted(port->function(), seq).value();
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
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::unordered_set<odb::dbNet*> destroyed;
  for (const OutputMove& move : moves) {
    for (odb::dbITerm* load : move.loads) {
      load->disconnect();
      load->connect(move.new_net);
    }
    for (odb::dbBTerm* bterm : move.ports) {
      bterm->disconnect();
      bterm->connect(move.new_net);
    }
    if (destroyed.insert(move.old_net).second) {
      network->deleteNet(network->dbToSta(move.old_net));
      if (move.old_mod_net != nullptr
          && move.old_mod_net->connectionCount() == 0) {
        odb::dbModNet::destroy(move.old_mod_net);
      }
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
      return makeTwoInputGate(true, synth(expr->left()), synth(expr->right()));
    case sta::FuncExpr::Op::or_:
      return makeTwoInputGate(false, synth(expr->left()), synth(expr->right()));
    case sta::FuncExpr::Op::xor_: {
      odb::dbNet* a = synth(expr->left());
      odb::dbNet* b = synth(expr->right());
      odb::dbNet* a_not_b = makeTwoInputGate(true, a, makeInverter(b));
      odb::dbNet* not_a_b = makeTwoInputGate(true, makeInverter(a), b);
      return makeTwoInputGate(false, a_not_b, not_a_b);
    }
    case sta::FuncExpr::Op::one:
      return makeTie(true);
    case sta::FuncExpr::Op::zero:
      return makeTie(false);
  }
  return nullptr;
}

SequentialUnmapper::RegPins SequentialUnmapper::getRegPins(
    odb::dbInst* inst,
    const sta::Sequential* seq) const
{
  RegPins pins;
  if (seq == nullptr) {
    return pins;
  }
  sta::dbNetwork* network = sta_->getDbNetwork();
  const sta::FuncExpr* data = seq->data();
  odb::dbMTerm* d_mterm
      = (data != nullptr && data->op() == sta::FuncExpr::Op::port)
            ? inst->getMaster()->findMTerm(data->port()->name().c_str())
            : nullptr;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::CLOCK) {
      pins.clk = iterm;
    } else if (iterm->getIoType() == odb::dbIoType::INPUT
               && iterm->getMTerm() == d_mterm) {
      pins.d = iterm;
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      const sta::LibertyPort* port
          = network->libertyPort(network->dbToSta(iterm));
      const sta::FuncExpr* func = port != nullptr ? port->function() : nullptr;
      if (func != nullptr && func->op() == sta::FuncExpr::Op::port
          && func->port() == seq->output()) {
        pins.q = iterm;
      }
    }
  }
  return pins;
}

void SequentialUnmapper::setParent(odb::dbInst* inst)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  parent_mod_inst_
      = network->hasHierarchy() ? inst->getModule()->getModInst() : nullptr;
  parent_ = parent_mod_inst_ != nullptr ? network->dbToSta(parent_mod_inst_)
                                        : network->topInstance();
}

odb::dbNet* SequentialUnmapper::newNet(const char* base_name)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  return network->staToDb(network->makeNet(
      base_name, parent_, odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE));
}

odb::dbInst* SequentialUnmapper::newInst(const char* base_name,
                                         odb::dbMaster* master)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  const std::string name = block_->makeNewInstName(
      parent_mod_inst_,
      base_name,
      odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE);
  sta::LibertyCell* cell = network->libertyCell(network->dbToSta(master));
  odb::dbInst* inst
      = network->staToDb(network->makeInstance(cell, name, parent_));
  inst->setSourceType(odb::dbSourceType::TIMING);
  return inst;
}

odb::dbNet* SequentialUnmapper::makeInverter(odb::dbNet* input)
{
  odb::dbInst* inv = newInst("unmap_inv", inv_master_);
  odb::dbNet* output = newNet("unmap_net");
  for (odb::dbITerm* iterm : inv->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      connectPin(iterm, input, mod_net_);
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      iterm->connect(output);
    }
  }
  return output;
}

odb::dbNet* SequentialUnmapper::makeTwoInputGate(const bool is_and,
                                                 odb::dbNet* a,
                                                 odb::dbNet* b)
{
  odb::dbInst* gate = newInst(is_and ? "unmap_and" : "unmap_or",
                              is_and ? and2_master_ : or2_master_);
  odb::dbNet* output = newNet("unmap_net");
  bool first_input = true;
  for (odb::dbITerm* iterm : gate->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      connectPin(iterm, first_input ? a : b, mod_net_);
      first_input = false;
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      iterm->connect(output);
    }
  }
  return output;
}

odb::dbNet* SequentialUnmapper::makeTie(const bool one)
{
  odb::dbInst* tie = newInst(one ? "unmap_tie1" : "unmap_tie0",
                             one ? tie1_master_ : tie0_master_);
  odb::dbMTerm* out_mterm = one ? tie1_out_mterm_ : tie0_out_mterm_;
  odb::dbNet* output = newNet("unmap_net");
  for (odb::dbITerm* iterm : tie->getITerms()) {
    if (iterm->getMTerm() == out_mterm) {
      iterm->connect(output);
    }
  }
  return output;
}

}  // namespace ret
