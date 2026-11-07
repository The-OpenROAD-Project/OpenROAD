// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RegisterUtils.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/Sequential.hh"

namespace ret {

namespace {

// Truth tables are enumerated over at most this many input ports (plus
// the state of a register), so a cell function is checked in at most
// 2^(kMaxTruthTableInputs + 1) rows.
constexpr size_t kMaxTruthTableInputs = 6;

const sta::Sequential* singleSequential(const sta::LibertyCell* cell)
{
  const sta::Sequential* seq = nullptr;
  int count = 0;
  for (const sta::Sequential& s : cell->sequentials()) {
    seq = &s;
    count++;
  }
  return count == 1 ? seq : nullptr;
}

// 0/1 when the net is a supply net or is driven by a tie cell, else -1.
int constNetValue(sta::dbSta* sta, odb::dbNet* net)
{
  if (net == nullptr) {
    return -1;
  }
  if (net->getSigType() == odb::dbSigType::POWER) {
    return 1;
  }
  if (net->getSigType() == odb::dbSigType::GROUND) {
    return 0;
  }
  sta::dbNetwork* network = sta->getDbNetwork();
  for (odb::dbITerm* iterm : net->getITerms()) {
    if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
      continue;
    }
    sta::Pin* pin = network->dbToSta(iterm);
    sta::LibertyPort* port
        = pin != nullptr ? network->libertyPort(pin) : nullptr;
    const sta::FuncExpr* func = port != nullptr ? port->function() : nullptr;
    if (func == nullptr) {
      return -1;
    }
    if (func->op() == sta::FuncExpr::Op::one) {
      return 1;
    }
    if (func->op() == sta::FuncExpr::Op::zero) {
      return 0;
    }
    return -1;
  }
  return -1;
}

// Ternary evaluation of a control expression under the constants on the
// register's input nets: 0, 1, or -1 (unknown).
int evalConstExpr(sta::dbSta* sta, odb::dbInst* inst, const sta::FuncExpr* e)
{
  if (e == nullptr) {
    return -1;
  }
  switch (e->op()) {
    case sta::FuncExpr::Op::port: {
      for (odb::dbITerm* iterm : inst->getITerms()) {
        if (strcmp(iterm->getMTerm()->getConstName(), e->port()->name().c_str())
            == 0) {
          return constNetValue(sta, iterm->getNet());
        }
      }
      return -1;
    }
    case sta::FuncExpr::Op::not_: {
      const int v = evalConstExpr(sta, inst, e->left());
      return v < 0 ? -1 : 1 - v;
    }
    case sta::FuncExpr::Op::and_: {
      const int a = evalConstExpr(sta, inst, e->left());
      const int b = evalConstExpr(sta, inst, e->right());
      if (a == 0 || b == 0) {
        return 0;
      }
      return (a == 1 && b == 1) ? 1 : -1;
    }
    case sta::FuncExpr::Op::or_: {
      const int a = evalConstExpr(sta, inst, e->left());
      const int b = evalConstExpr(sta, inst, e->right());
      if (a == 1 || b == 1) {
        return 1;
      }
      return (a == 0 && b == 0) ? 0 : -1;
    }
    case sta::FuncExpr::Op::xor_: {
      const int a = evalConstExpr(sta, inst, e->left());
      const int b = evalConstExpr(sta, inst, e->right());
      if (a < 0 || b < 0) {
        return -1;
      }
      return a != b ? 1 : 0;
    }
    case sta::FuncExpr::Op::one:
      return 1;
    case sta::FuncExpr::Op::zero:
      return 0;
  }
  return -1;
}

// Distinct ports of an expression, in first-appearance order.
void collectPorts(const sta::FuncExpr* e,
                  std::vector<const sta::LibertyPort*>& ports)
{
  if (e == nullptr) {
    return;
  }
  if (e->op() == sta::FuncExpr::Op::port) {
    if (std::ranges::find(ports, e->port()) == ports.end()) {
      ports.push_back(e->port());
    }
    return;
  }
  collectPorts(e->left(), ports);
  collectPorts(e->right(), ports);
}

// Boolean evaluation of next_state under one assignment of its support.
// The inverted state port reads as the complement of the state.
using Assignment = std::unordered_map<const sta::LibertyPort*, bool>;

bool evalExpr(const sta::FuncExpr* e,
              const Assignment& values,
              const sta::LibertyPort* state,
              const sta::LibertyPort* state_inv)
{
  switch (e->op()) {
    case sta::FuncExpr::Op::port:
      if (state_inv != nullptr && e->port() == state_inv) {
        return !values.at(state);
      }
      return values.at(e->port());
    case sta::FuncExpr::Op::not_:
      return !evalExpr(e->left(), values, state, state_inv);
    case sta::FuncExpr::Op::and_:
      return evalExpr(e->left(), values, state, state_inv)
             && evalExpr(e->right(), values, state, state_inv);
    case sta::FuncExpr::Op::or_:
      return evalExpr(e->left(), values, state, state_inv)
             || evalExpr(e->right(), values, state, state_inv);
    case sta::FuncExpr::Op::xor_:
      return evalExpr(e->left(), values, state, state_inv)
             != evalExpr(e->right(), values, state, state_inv);
    case sta::FuncExpr::Op::one:
      return true;
    case sta::FuncExpr::Op::zero:
      return false;
  }
  return false;
}

// What the liberty next_state function says about a cell, independent of any
// instance: the data port and the enable ports (each with the value at which
// it holds the state), or `other` when the function has no such shape.
struct NextStateShape
{
  RegisterKind kind = RegisterKind::other;
  const sta::LibertyPort* data = nullptr;
  bool data_inverted = false;  // next_state is the inverse of `data`
  std::vector<std::pair<const sta::LibertyPort*, bool>> enables;  // inactive
};

// Truth table of a register's next_state over its input ports and its
// state.  Bit i of a row is input i; the top bit is the state.
class NextStateTable
{
 public:
  NextStateTable(const sta::FuncExpr* next,
                 const sta::LibertyPort* state,
                 const sta::LibertyPort* state_inv,
                 std::vector<const sta::LibertyPort*> inputs)
      : next_(next),
        state_(state),
        state_inv_(state_inv),
        inputs_(std::move(inputs)),
        rows_(1u << (inputs_.size() + 1))
  {
  }

  // next_state == state on every row with input i at value v: input i is
  // an enable, inactive at v.
  bool holds(size_t i, bool v) const
  {
    for (unsigned row = 0; row < rows_; row++) {
      if (bit(row, i) == v && value(row) != stateBit(row)) {
        return false;
      }
    }
    return true;
  }

  // next_state == input d on every row where every enable is active.
  bool passes(size_t d,
              const std::vector<std::pair<const sta::LibertyPort*, bool>>&
                  enables) const
  {
    for (unsigned row = 0; row < rows_; row++) {
      if (enablesActive(row, enables) && value(row) != bit(row, d)) {
        return false;
      }
    }
    return true;
  }

 private:
  static bool bit(unsigned row, size_t i) { return ((row >> i) & 1u) != 0; }
  bool stateBit(unsigned row) const { return bit(row, inputs_.size()); }

  bool enablesActive(
      unsigned row,
      const std::vector<std::pair<const sta::LibertyPort*, bool>>& enables)
      const
  {
    for (size_t i = 0; i < inputs_.size(); i++) {
      for (const auto& [port, inactive] : enables) {
        if (port == inputs_[i] && bit(row, i) == inactive) {
          return false;
        }
      }
    }
    return true;
  }

  // next_state evaluated on a row.
  bool value(unsigned row) const
  {
    Assignment values;
    for (size_t i = 0; i < inputs_.size(); i++) {
      values[inputs_[i]] = bit(row, i);
    }
    values[state_] = stateBit(row);
    return evalExpr(next_, values, state_, state_inv_);
  }

  const sta::FuncExpr* next_;
  const sta::LibertyPort* state_;
  const sta::LibertyPort* state_inv_;
  std::vector<const sta::LibertyPort*> inputs_;
  unsigned rows_;
};

// The input ports next_state reads, excluding the state; false when it
// reads something that is not an input, or too many inputs to enumerate.
bool nextStateInputs(const sta::FuncExpr* next,
                     const sta::LibertyPort* state,
                     const sta::LibertyPort* state_inv,
                     std::vector<const sta::LibertyPort*>& inputs)
{
  std::vector<const sta::LibertyPort*> support;
  collectPorts(next, support);
  for (const sta::LibertyPort* port : support) {
    if (port != state && port != state_inv) {
      if (!port->direction()->isInput()) {
        return false;
      }
      inputs.push_back(port);
    }
  }
  return !inputs.empty() && inputs.size() <= kMaxTruthTableInputs;
}

// Truth-table check over the support of next_state.  A port is an enable at
// value v when next_state equals the state for every assignment with that
// port at v.  With every enable active, next_state must equal exactly one
// remaining input port, the data port, and no other input may remain.
NextStateShape nextStateShape(const sta::Sequential* seq)
{
  NextStateShape shape;
  const sta::FuncExpr* next = seq->data();
  if (next == nullptr) {
    return shape;
  }
  if (next->op() == sta::FuncExpr::Op::port) {
    shape.kind = RegisterKind::plain;
    shape.data = next->port();
    return shape;
  }
  if (next->op() == sta::FuncExpr::Op::not_ && next->left() != nullptr
      && next->left()->op() == sta::FuncExpr::Op::port) {
    shape.kind = RegisterKind::plain;
    shape.data = next->left()->port();
    shape.data_inverted = true;
    return shape;
  }
  const sta::LibertyPort* state = seq->output();
  const sta::LibertyPort* state_inv = seq->outputInv();
  std::vector<const sta::LibertyPort*> inputs;
  if (state == nullptr || !nextStateInputs(next, state, state_inv, inputs)) {
    return shape;
  }

  const NextStateTable table(next, state, state_inv, inputs);
  std::vector<bool> is_enable(inputs.size(), false);
  for (size_t i = 0; i < inputs.size(); i++) {
    for (bool v : {false, true}) {
      if (!is_enable[i] && table.holds(i, v)) {
        is_enable[i] = true;
        shape.enables.emplace_back(inputs[i], v);
      }
    }
  }
  if (shape.enables.empty()) {
    return shape;
  }
  for (size_t i = 0; i < inputs.size(); i++) {
    if (!is_enable[i]) {
      // A second data candidate, or an input that is not passed through.
      if (shape.data != nullptr || !table.passes(i, shape.enables)) {
        return NextStateShape();
      }
      shape.data = inputs[i];
    }
  }
  if (shape.data == nullptr) {
    return NextStateShape();
  }
  shape.kind = RegisterKind::enable;
  return shape;
}

odb::dbITerm* findITerm(odb::dbInst* inst, const sta::LibertyPort* port)
{
  return port != nullptr ? inst->findITerm(port->name().c_str()) : nullptr;
}

// 0 when an output pin function reads the register state, 1 when it reads
// its complement, -1 for anything else.
int outputPolarity(const sta::FuncExpr* func, const sta::Sequential* seq)
{
  if (func == nullptr) {
    return -1;
  }
  bool inverted = false;
  if (func->op() == sta::FuncExpr::Op::not_) {
    inverted = true;
    func = func->left();
  }
  if (func == nullptr || func->op() != sta::FuncExpr::Op::port) {
    return -1;
  }
  if (func->port() == seq->output()) {
    return inverted ? 1 : 0;
  }
  if (seq->outputInv() != nullptr && func->port() == seq->outputInv()) {
    return inverted ? 0 : 1;
  }
  return -1;
}

// The one output pin the register's loads read, and whether it presents the
// complement of the state.  With no load at all the pin that reads the state
// is returned.  False when two output pins carry loads or the loaded pin has
// no recognizable function: such a register cannot be rebuilt pin for pin.
bool loadedOutput(sta::dbSta* sta,
                  odb::dbInst* inst,
                  const sta::Sequential* seq,
                  odb::dbITerm*& pin,
                  bool& inverted)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  pin = nullptr;
  inverted = false;
  odb::dbITerm* state_pin = nullptr;
  int loaded = 0;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
      continue;
    }
    sta::Pin* sta_pin = network->dbToSta(iterm);
    sta::LibertyPort* port
        = sta_pin != nullptr ? network->libertyPort(sta_pin) : nullptr;
    const int polarity
        = port != nullptr ? outputPolarity(port->function(), seq) : -1;
    if (polarity == 0 && state_pin == nullptr) {
      state_pin = iterm;
    }
    odb::dbNet* net = iterm->getNet();
    if (net == nullptr
        || (net->getITerms().size() <= 1 && net->getBTerms().empty())) {
      continue;
    }
    loaded++;
    if (polarity < 0) {
      return false;
    }
    pin = iterm;
    inverted = polarity == 1;
  }
  if (loaded > 1) {
    return false;
  }
  if (loaded == 0) {
    pin = state_pin;
    inverted = false;
  }
  return true;
}

}  // namespace

bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::SCAN) {
      return true;
    }
  }
  sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  if (cell == nullptr) {
    return true;
  }
  const sta::Sequential* seq = singleSequential(cell);
  if (seq == nullptr || !seq->isRegister()) {
    return true;
  }
  // Asynchronous controls tied inactive make the register plain in practice.
  for (const sta::FuncExpr* control : {seq->clear(), seq->preset()}) {
    if (control != nullptr && evalConstExpr(sta, inst, control) != 0) {
      return true;
    }
  }
  return false;
}

bool isRegisterDataPin(sta::dbSta* sta, odb::dbITerm* pin)
{
  if (pin->getSigType() == odb::dbSigType::CLOCK) {
    return false;
  }
  sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(pin->getInst());
  if (cell == nullptr) {
    return true;
  }
  const sta::Sequential* seq = singleSequential(cell);
  if (seq == nullptr) {
    return true;
  }
  std::vector<const sta::LibertyPort*> ports;
  collectPorts(seq->data(), ports);
  for (const sta::LibertyPort* port : ports) {
    if (strcmp(port->name().c_str(), pin->getMTerm()->getConstName()) == 0) {
      return true;
    }
  }
  return false;
}

odb::dbITerm* registerDataPin(sta::dbSta* sta, odb::dbInst* inst)
{
  const RegisterClass cls = registerClass(sta, inst);
  if (cls.data != nullptr) {
    return inst->getITerm(cls.data);
  }
  // No recognized shape: the first input in the next_state support.
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() != odb::dbIoType::INPUT) {
      continue;
    }
    const odb::dbSigType sig = iterm->getSigType();
    if (sig == odb::dbSigType::CLOCK || sig == odb::dbSigType::POWER
        || sig == odb::dbSigType::GROUND) {
      continue;
    }
    if (isRegisterDataPin(sta, iterm)) {
      return iterm;
    }
  }
  return nullptr;
}

bool isNegedgeRegister(sta::dbSta* sta, odb::dbInst* inst)
{
  sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  const sta::Sequential* seq
      = cell != nullptr ? singleSequential(cell) : nullptr;
  if (seq == nullptr || !seq->isRegister()) {
    return false;  // latches are don't-touch, not negative-edge registers
  }
  const sta::FuncExpr* clock = seq->clock();
  return clock != nullptr && clock->op() == sta::FuncExpr::Op::not_;
}

odb::dbNet* registerClockNet(odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::CLOCK) {
      return iterm->getNet();
    }
  }
  return nullptr;
}

RegisterClass registerClass(sta::dbSta* sta, odb::dbInst* inst)
{
  RegisterClass cls;
  cls.clock = registerClockNet(inst);
  sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  const sta::Sequential* seq
      = cell != nullptr ? singleSequential(cell) : nullptr;
  if (seq == nullptr || !seq->isRegister()) {
    return cls;
  }
  const NextStateShape shape = nextStateShape(seq);
  odb::dbITerm* data = findITerm(inst, shape.data);
  if (shape.kind == RegisterKind::other || data == nullptr) {
    return cls;
  }
  cls.data = data->getMTerm();

  // The loads decide which output pin a rebuilt register drives.  Loads on
  // two output pins cannot be reproduced by a bypass or by a chain register.
  odb::dbITerm* out_pin = nullptr;
  bool out_inverted = false;
  if (!loadedOutput(sta, inst, seq, out_pin, out_inverted)) {
    return cls;
  }
  cls.output = out_pin != nullptr ? out_pin->getMTerm() : nullptr;
  if (shape.kind == RegisterKind::plain) {
    // One cycle later the loads see the data pin, or its complement when
    // exactly one of the stored value and the output pin is inverted.
    if (shape.data_inverted == out_inverted) {
      cls.kind = RegisterKind::plain;
    } else if (cls.output != nullptr && cls.clock != nullptr) {
      cls.kind = RegisterKind::inverting;
      cls.master = inst->getMaster();
    }
    return cls;
  }
  if (out_inverted) {
    return cls;  // an enable register read through its inverted output
  }

  // Enable register: every signal input must be the data pin, an enable pin
  // or the clock.
  for (const auto& [port, inactive] : shape.enables) {
    odb::dbITerm* iterm = findITerm(inst, port);
    if (iterm == nullptr) {
      return cls;
    }
    cls.controls.emplace_back(iterm->getMTerm(), iterm->getNet());
  }
  std::ranges::sort(cls.controls, [](const auto& a, const auto& b) {
    return strcmp(a.first->getConstName(), b.first->getConstName()) < 0;
  });
  for (odb::dbITerm* iterm : inst->getITerms()) {
    const odb::dbSigType sig = iterm->getSigType();
    if (sig == odb::dbSigType::CLOCK || sig == odb::dbSigType::POWER
        || sig == odb::dbSigType::GROUND) {
      continue;
    }
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      if (iterm->getMTerm() == cls.data) {
        continue;
      }
      bool is_control = false;
      for (const auto& [mterm, net] : cls.controls) {
        is_control = is_control || mterm == iterm->getMTerm();
      }
      if (!is_control) {
        return cls;  // an input the next_state shape does not account for
      }
    }
  }
  if (cls.output == nullptr || cls.clock == nullptr) {
    return cls;
  }
  cls.kind = RegisterKind::enable;
  cls.master = inst->getMaster();
  return cls;
}

bool sameRegisterClass(const RegisterClass& a, const RegisterClass& b)
{
  if (a.kind != b.kind) {
    return false;
  }
  switch (a.kind) {
    case RegisterKind::plain:
      return a.clock == b.clock;
    case RegisterKind::enable:
      return a.master == b.master && a.clock == b.clock && a.output == b.output
             && a.controls == b.controls;
    case RegisterKind::inverting:
      return a.master == b.master && a.clock == b.clock && a.output == b.output;
    case RegisterKind::other:
      return false;
  }
  return false;
}

bool isSelfDualGate(sta::dbSta* sta, odb::dbInst* inst)
{
  sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  if (cell == nullptr || cell->isSequential()) {
    return false;
  }
  int outputs = 0;
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    sta::LibertyPort* port = port_iter.next();
    if (!port->direction()->isOutput()) {
      continue;
    }
    const sta::FuncExpr* func = port->function();
    if (func == nullptr) {
      return false;
    }
    std::vector<const sta::LibertyPort*> inputs;
    collectPorts(func, inputs);
    if (inputs.empty() || inputs.size() > kMaxTruthTableInputs) {
      return false;
    }
    const unsigned rows = 1u << inputs.size();
    for (unsigned row = 0; row < rows; row++) {
      Assignment values;
      Assignment complemented;
      for (size_t i = 0; i < inputs.size(); i++) {
        const bool bit = ((row >> i) & 1u) != 0;
        values[inputs[i]] = bit;
        complemented[inputs[i]] = !bit;
      }
      if (evalExpr(func, values, nullptr, nullptr)
          == evalExpr(func, complemented, nullptr, nullptr)) {
        return false;
      }
    }
    outputs++;
  }
  return outputs > 0;
}

}  // namespace ret
