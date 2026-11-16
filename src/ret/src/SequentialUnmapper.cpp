// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ret/SequentialUnmapper.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "RegisterUtils.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "spdlog/fmt/fmt.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Sequential.hh"
#include "utl/Logger.h"

namespace ret {

namespace {

bool isNegedge(const sta::Sequential* seq)
{
  return seq->clock() != nullptr
         && seq->clock()->op() == sta::FuncExpr::Op::not_;
}

bool usableCell(const sta::LibertyCell* cell)
{
  return !cell->dontUse() && !cell->isIsolationCell() && !cell->isLevelShifter()
         && !cell->alwaysOn() && !cell->isPad() && !cell->isMacro();
}

bool hasScanPin(odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::SCAN) {
      return true;
    }
  }
  return false;
}

sta::LibertyPort* statePort(sta::LibertyCell* cell,
                            const sta::Sequential* seq,
                            int& outputs)
{
  sta::LibertyPort* q_port = nullptr;
  outputs = 0;
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    sta::LibertyPort* port = port_iter.next();
    if (!port->direction()->isOutput()) {
      continue;
    }
    outputs++;
    const sta::FuncExpr* func = port->function();
    if (q_port == nullptr && func != nullptr
        && func->op() == sta::FuncExpr::Op::port
        && func->port() == seq->output()) {
      q_port = port;
    }
  }
  return q_port;
}

}  // namespace

SequentialUnmapper::SequentialUnmapper(odb::dbDatabase* db,
                                       sta::dbSta* sta,
                                       utl::Logger* logger)
    : db_(db), sta_(sta), logger_(logger)
{
}

bool SequentialUnmapper::prepare()
{
  bool ok = true;
  if (!findBaseSequentialCell(false, seq_master_, seq_q_mterm_)) {
    logger_->warn(utl::RET,
                  1,
                  "No usable register cell found: a single-bit positive-edge "
                  "D flip-flop without asynchronous controls is required.");
    ok = false;
  }
  if (!findInverterCell()) {
    logger_->warn(utl::RET, 2, "No inverter cell found.");
    ok = false;
  }
  if (!findTwoInputGate(true, and2_master_)) {
    logger_->warn(utl::RET, 3, "No 2-input AND cell found.");
    ok = false;
  }
  if (!findTwoInputGate(false, or2_master_)) {
    logger_->warn(utl::RET, 4, "No 2-input OR cell found.");
    ok = false;
  }
  if (!findTieCell(true)) {
    debugPrint(logger_, utl::RET, "cells", 1, "no tie-high cell");
  }
  if (!findTieCell(false)) {
    debugPrint(logger_, utl::RET, "cells", 1, "no tie-low cell");
  }
  if (!findBaseSequentialCell(
          true, negedge_seq_master_, negedge_seq_q_mterm_)) {
    debugPrint(logger_, utl::RET, "cells", 1, "no negative-edge register cell");
  }
  return ok;
}

bool SequentialUnmapper::findBaseSequentialCell(bool negedge,
                                                odb::dbMaster*& master,
                                                odb::dbMTerm*& q_mterm)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbMaster* best = nullptr;
  odb::dbMTerm* best_q = nullptr;
  float best_area = 0.0f;
  odb::dbMaster* fallback = nullptr;
  odb::dbMTerm* fallback_q = nullptr;

  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      network->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    sta::LibertyCellIterator cell_iter(lib);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      if (!usableCell(cell)) {
        continue;
      }
      const sta::Sequential* seq = singleSequential(cell);
      if (seq == nullptr || !seq->isRegister() || seq->clock() == nullptr
          || isNegedge(seq) != negedge || seq->clear() != nullptr
          || seq->preset() != nullptr || seq->output() == nullptr
          || seq->outputInv() == nullptr) {
        continue;
      }
      const sta::FuncExpr* data = seq->data();
      if (data == nullptr || data->op() != sta::FuncExpr::Op::port) {
        continue;
      }
      int outputs = 0;
      sta::LibertyPort* q_port = statePort(cell, seq, outputs);
      if (q_port == nullptr) {
        continue;
      }
      odb::dbMaster* candidate = db_->findMaster(cell->name().c_str());
      odb::dbMTerm* candidate_q
          = candidate != nullptr ? candidate->findMTerm(q_port->name().c_str())
                                 : nullptr;
      if (candidate == nullptr || candidate_q == nullptr) {
        continue;
      }
      if (outputs == 1) {
        if (best == nullptr || cell->area() < best_area) {
          best = candidate;
          best_q = candidate_q;
          best_area = cell->area();
        }
      } else if (fallback == nullptr) {
        fallback = candidate;
        fallback_q = candidate_q;
      }
    }
  }

  if (best == nullptr && fallback != nullptr) {
    debugPrint(logger_,
               utl::RET,
               "cells",
               1,
               "multi-output register cell {} is the {} base cell",
               fallback->getName(),
               negedge ? "negative-edge" : "positive-edge");
    best = fallback;
    best_q = fallback_q;
  }
  master = best;
  q_mterm = best_q;
  return master != nullptr;
}

bool SequentialUnmapper::findInverterCell()
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbMaster* best = nullptr;
  float best_area = 0.0f;
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      network->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyCellIterator cell_iter(lib_iter->next());
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      if (!usableCell(cell)) {
        continue;
      }
      if (cell->isSequential()) {
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
          inputs++;
          continue;
        }
        outputs++;
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
  }
  inv_master_ = best;
  return inv_master_ != nullptr;
}

bool SequentialUnmapper::findTwoInputGate(bool is_and, odb::dbMaster*& master)
{
  const sta::FuncExpr::Op op
      = is_and ? sta::FuncExpr::Op::and_ : sta::FuncExpr::Op::or_;
  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbMaster* best = nullptr;
  float best_area = 0.0f;
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      network->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyCellIterator cell_iter(lib_iter->next());
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      if (!usableCell(cell)) {
        continue;
      }
      if (cell->isSequential()) {
        continue;
      }
      int inputs = 0;
      int outputs = 0;
      sta::LibertyPort* out_port = nullptr;
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        sta::LibertyPort* port = port_iter.next();
        if (port->direction()->isPowerGround()) {
          continue;
        }
        if (port->direction()->isOutput()) {
          out_port = port;
          outputs++;
        } else {
          inputs++;
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
  }
  master = best;
  return master != nullptr;
}

bool SequentialUnmapper::findTieCell(bool one)
{
  const sta::FuncExpr::Op op
      = one ? sta::FuncExpr::Op::one : sta::FuncExpr::Op::zero;
  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbMaster* best = nullptr;
  sta::LibertyPort* best_port = nullptr;
  float best_area = 0.0f;
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      network->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyCellIterator cell_iter(lib_iter->next());
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      if (!usableCell(cell)) {
        continue;
      }
      if (cell->isSequential()) {
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
  }
  odb::dbMTerm* out_mterm
      = best != nullptr ? best->findMTerm(best_port->name().c_str()) : nullptr;
  if (one) {
    tie1_master_ = best;
    tie1_out_mterm_ = out_mterm;
  } else {
    tie0_master_ = best;
    tie0_out_mterm_ = out_mterm;
  }
  return best != nullptr && out_mterm != nullptr;
}

void SequentialUnmapper::run(const std::unordered_set<odb::dbInst*>* candidates)
{
  block_ = db_->getChip()->getBlock();
  Untouched untouched;
  const std::vector<Register> registers
      = collectRegisters(candidates, untouched);

  unmapped_ = 0;
  skipped_ = 0;
  std::map<std::string, int> per_cell;
  for (const auto& [inst, seq] : registers) {
    const std::string master_name = inst->getMaster()->getName();
    const int before = unmapped_;
    decompose(inst, seq);
    if (unmapped_ > before) {
      per_cell[master_name]++;
    }
  }
  reportRun(per_cell, untouched);
}

std::vector<SequentialUnmapper::Register> SequentialUnmapper::collectRegisters(
    const std::unordered_set<odb::dbInst*>* candidates,
    Untouched& untouched) const
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::vector<Register> registers;
  for (odb::dbInst* inst : block_->getInsts()) {
    if (candidates != nullptr && !candidates->contains(inst)) {
      continue;
    }
    if (!inst->getMaster()->isSequential()) {
      continue;
    }
    sta::LibertyCell* cell = network->libertyCell(inst);
    const sta::Sequential* seq
        = cell != nullptr ? singleSequential(cell) : nullptr;
    if (seq == nullptr) {
      untouched.other++;
    } else if (!seq->isRegister()) {
      untouched.latches++;
    } else if (seq->clear() != nullptr || seq->preset() != nullptr) {
      untouched.async++;
    } else if (hasScanPin(inst)) {
      untouched.scan++;
    } else {
      registers.emplace_back(inst, seq);
    }
  }
  return registers;
}

void SequentialUnmapper::reportRun(const std::map<std::string, int>& per_cell,
                                   const Untouched& untouched) const
{
  logger_->info(utl::RET, 8, "Unmapped {} registers.", unmapped_);
  for (const auto& [name, count] : per_cell) {
    debugPrint(logger_, utl::RET, "unmap", 1, "{:>6} x {}", count, name);
  }
  if (skipped_ > 0) {
    logger_->warn(utl::RET,
                  9,
                  "{} registers were left as they are: their next_state "
                  "needs a cell the libraries do not have.",
                  skipped_);
  }
  const int total
      = untouched.async + untouched.latches + untouched.scan + untouched.other;
  if (total > 0) {
    logger_->warn(utl::RET,
                  10,
                  "Left {} sequential cells as they are: {} with asynchronous "
                  "controls, {} latches, {} scan cells, {} of another kind.",
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
      return inv_master_ != nullptr && canSynth(expr->left());
    case sta::FuncExpr::Op::and_:
      return and2_master_ != nullptr && canSynth(expr->left())
             && canSynth(expr->right());
    case sta::FuncExpr::Op::or_:
      return or2_master_ != nullptr && canSynth(expr->left())
             && canSynth(expr->right());
    case sta::FuncExpr::Op::xor_:
      return inv_master_ != nullptr && and2_master_ != nullptr
             && or2_master_ != nullptr && canSynth(expr->left())
             && canSynth(expr->right());
    case sta::FuncExpr::Op::one:
      return tie1_master_ != nullptr;
    case sta::FuncExpr::Op::zero:
      return tie0_master_ != nullptr;
  }
  return false;
}

bool SequentialUnmapper::isInverted(const sta::FuncExpr* func) const
{
  if (func == nullptr) {
    return false;
  }
  bool inverted = false;
  if (func->op() == sta::FuncExpr::Op::not_) {
    inverted = true;
    func = func->left();
  }
  if (func == nullptr || func->op() != sta::FuncExpr::Op::port) {
    return false;
  }
  if (func->port() == out_port_) {
    return inverted;
  }
  if (func->port() == out_inv_port_) {
    return !inverted;
  }
  return false;
}

void SequentialUnmapper::decompose(odb::dbInst* inst,
                                   const sta::Sequential* seq)
{
  in_port_net_.clear();
  created_.clear();
  out_port_ = seq->output();
  out_inv_port_ = seq->outputInv();
  const std::string inst_name = inst->getName();
  const std::string inst_master = inst->getMaster()->getName();

  std::vector<odb::dbITerm*> out_pins;
  odb::dbNet* clock_net = collectRegisterNets(inst, out_pins);
  odb::dbMaster* base = isNegedge(seq) ? negedge_seq_master_ : seq_master_;
  if (base == nullptr || clock_net == nullptr || out_pins.empty()
      || !canSynth(seq->data())) {
    debugPrint(logger_,
               utl::RET,
               "unmap",
               1,
               "skip {} ({}): next_state not expressible with the base cell "
               "and gates found",
               inst_name,
               inst_master);
    skipped_++;
    return;
  }

  odb::dbInst* flop = newInst("unmap_ff", base);
  const RegPins pins = getRegPins(
      flop, singleSequential(sta_->getDbNetwork()->libertyCell(flop)));
  if (pins.d == nullptr || pins.q == nullptr || pins.clk == nullptr) {
    logger_->warn(utl::RET,
                  13,
                  "Cannot rebuild {}: base cell {} has no usable D, Q or "
                  "clock pin.",
                  inst->getName(),
                  base->getName());
    odb::dbInst::destroy(flop);
    skipped_++;
    return;
  }

  state_net_ = newNet("unmap_state");
  pins.q->connect(state_net_);
  pins.clk->connect(clock_net);
  pins.d->connect(synth(seq->data()));

  const std::vector<OutputMove> moves = collectOutputMoves(inst, out_pins);
  odb::dbInst::destroy(inst);
  applyOutputMoves(moves);
  unmapped_++;
  traceDecompose(inst_name, inst_master, flop);
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
      sta::Pin* pin = network->dbToSta(iterm);
      sta::LibertyPort* port
          = pin != nullptr ? network->libertyPort(pin) : nullptr;
      if (port != nullptr) {
        in_port_net_[port] = iterm->getNet();
      }
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT
               && iterm->getNet() != nullptr) {
      out_pins.push_back(iterm);
    }
  }
  return clock_net;
}

std::vector<SequentialUnmapper::OutputMove>
SequentialUnmapper::collectOutputMoves(
    odb::dbInst* inst,
    const std::vector<odb::dbITerm*>& out_pins)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  std::vector<OutputMove> moves;
  for (odb::dbITerm* out_pin : out_pins) {
    sta::LibertyPort* port = network->libertyPort(network->dbToSta(out_pin));
    const bool inverted = port != nullptr && isInverted(port->function());
    OutputMove move;
    move.old_net = out_pin->getNet();
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
      odb::dbNet::destroy(move.old_net);
    }
  }
}

void SequentialUnmapper::traceDecompose(const std::string& inst_name,
                                        const std::string& inst_master,
                                        odb::dbInst* flop)
{
  std::map<std::string, int> gates;
  for (odb::dbInst* created : created_) {
    if (created->getMaster() != flop->getMaster()) {
      gates[created->getMaster()->getName()]++;
    }
  }
  std::string gate_list;
  for (const auto& [name, count] : gates) {
    gate_list
        += fmt::format("{}{} x {}", gate_list.empty() ? "" : ", ", count, name);
  }
  debugPrint(logger_,
             utl::RET,
             "unmap",
             1,
             "{} ({}) -> {} ({}){}{}",
             inst_name,
             inst_master,
             flop->getName(),
             flop->getMaster()->getName(),
             gate_list.empty() ? "" : " + ",
             gate_list);
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
      sta::LibertyPort* port = network->libertyPort(network->dbToSta(iterm));
      const sta::FuncExpr* func = port != nullptr ? port->function() : nullptr;
      if (func != nullptr && func->op() == sta::FuncExpr::Op::port
          && func->port() == seq->output()) {
        pins.q = iterm;
      }
    }
  }
  return pins;
}

odb::dbNet* SequentialUnmapper::newNet(const char* base_name)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  return network->staToDb(
      network->makeNet(base_name,
                       network->topInstance(),
                       odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE));
}

odb::dbInst* SequentialUnmapper::newInst(const char* base_name,
                                         odb::dbMaster* master)
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  const std::string name = block_->makeNewInstName(
      nullptr, base_name, odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE);
  sta::LibertyCell* cell = network->libertyCell(network->dbToSta(master));
  odb::dbInst* inst = network->staToDb(
      network->makeInstance(cell, name, network->topInstance()));
  inst->setSourceType(odb::dbSourceType::TIMING);
  created_.push_back(inst);
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
      iterm->connect(input);
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      iterm->connect(output);
    }
  }
  return output;
}

odb::dbNet* SequentialUnmapper::makeTwoInputGate(bool is_and,
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
      iterm->connect(first_input ? a : b);
      first_input = false;
    } else if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
      iterm->connect(output);
    }
  }
  return output;
}

odb::dbNet* SequentialUnmapper::makeTie(bool one)
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
