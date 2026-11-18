// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "RegisterUtils.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Sequential.hh"

namespace ret {

const sta::Sequential* singleSequential(const sta::LibertyCell* cell)
{
  const sta::Sequential* seq = nullptr;
  int count = 0;
  for (const sta::Sequential& s : cell->sequentials()) {
    seq = &s;
    ++count;
  }
  return count == 1 ? seq : nullptr;
}

namespace {

constexpr size_t kMaxTruthTableInputs = 6;

std::optional<bool> constNetValue(sta::dbSta* sta, odb::dbNet* net)
{
  if (net == nullptr) {
    return std::nullopt;
  }
  if (net->getSigType() == odb::dbSigType::POWER) {
    return true;
  }
  if (net->getSigType() == odb::dbSigType::GROUND) {
    return false;
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
    if (func != nullptr && func->op() == sta::FuncExpr::Op::one) {
      return true;
    }
    if (func != nullptr && func->op() == sta::FuncExpr::Op::zero) {
      return false;
    }
    return std::nullopt;
  }
  return std::nullopt;
}

std::optional<bool> evalConstExpr(sta::dbSta* sta,
                                  odb::dbInst* inst,
                                  const sta::FuncExpr* e)
{
  if (e == nullptr) {
    return std::nullopt;
  }
  switch (e->op()) {
    case sta::FuncExpr::Op::port: {
      for (odb::dbITerm* iterm : inst->getITerms()) {
        if (strcmp(iterm->getMTerm()->getConstName(), e->port()->name().c_str())
            == 0) {
          return constNetValue(sta, iterm->getNet());
        }
      }
      return std::nullopt;
    }
    case sta::FuncExpr::Op::not_: {
      const std::optional<bool> v = evalConstExpr(sta, inst, e->left());
      return v.has_value() ? std::optional<bool>(!*v) : std::nullopt;
    }
    case sta::FuncExpr::Op::and_: {
      const std::optional<bool> a = evalConstExpr(sta, inst, e->left());
      const std::optional<bool> b = evalConstExpr(sta, inst, e->right());
      if (a == false || b == false) {
        return false;
      }
      return (a == true && b == true) ? std::optional<bool>(true)
                                      : std::nullopt;
    }
    case sta::FuncExpr::Op::or_: {
      const std::optional<bool> a = evalConstExpr(sta, inst, e->left());
      const std::optional<bool> b = evalConstExpr(sta, inst, e->right());
      if (a == true || b == true) {
        return true;
      }
      return (a == false && b == false) ? std::optional<bool>(false)
                                        : std::nullopt;
    }
    case sta::FuncExpr::Op::xor_: {
      const std::optional<bool> a = evalConstExpr(sta, inst, e->left());
      const std::optional<bool> b = evalConstExpr(sta, inst, e->right());
      if (!a.has_value() || !b.has_value()) {
        return std::nullopt;
      }
      return *a != *b;
    }
    case sta::FuncExpr::Op::one:
      return true;
    case sta::FuncExpr::Op::zero:
      return false;
  }
  return std::nullopt;
}

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

using EnableLevels = std::vector<std::pair<const sta::LibertyPort*, bool>>;

struct NextStateShape
{
  RegisterKind kind = RegisterKind::kOther;
  const sta::LibertyPort* data = nullptr;
  bool data_inverted = false;
  EnableLevels enables_inactive_at;
};

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

  bool holdsStateWhen(size_t i, bool v) const
  {
    for (uint32_t row = 0; row < rows_; ++row) {
      if (bit(row, i) == v && value(row) != stateBit(row)) {
        return false;
      }
    }
    return true;
  }

  bool passesWhenEnabled(size_t d, const EnableLevels& enables) const
  {
    for (uint32_t row = 0; row < rows_; ++row) {
      if (enablesActive(row, enables) && value(row) != bit(row, d)) {
        return false;
      }
    }
    return true;
  }

 private:
  static bool bit(uint32_t row, size_t i) { return ((row >> i) & 1u) != 0; }
  bool stateBit(uint32_t row) const { return bit(row, inputs_.size()); }

  bool enablesActive(uint32_t row, const EnableLevels& enables) const
  {
    for (size_t i = 0; i < inputs_.size(); ++i) {
      for (const auto& [port, inactive] : enables) {
        if (port == inputs_[i] && bit(row, i) == inactive) {
          return false;
        }
      }
    }
    return true;
  }

  bool value(uint32_t row) const
  {
    Assignment values;
    for (size_t i = 0; i < inputs_.size(); ++i) {
      values[inputs_[i]] = bit(row, i);
    }
    values[state_] = stateBit(row);
    return evalExpr(next_, values, state_, state_inv_);
  }

  const sta::FuncExpr* next_;
  const sta::LibertyPort* state_;
  const sta::LibertyPort* state_inv_;
  std::vector<const sta::LibertyPort*> inputs_;
  uint32_t rows_;
};

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

NextStateShape nextStateShape(const sta::Sequential* seq)
{
  NextStateShape shape;
  const sta::FuncExpr* next = seq->data();
  if (next == nullptr) {
    return shape;
  }
  if (next->op() == sta::FuncExpr::Op::port) {
    shape.kind = RegisterKind::kPlain;
    shape.data = next->port();
    return shape;
  }
  if (next->op() == sta::FuncExpr::Op::not_ && next->left() != nullptr
      && next->left()->op() == sta::FuncExpr::Op::port) {
    shape.kind = RegisterKind::kPlain;
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
  for (size_t i = 0; i < inputs.size(); ++i) {
    for (bool v : {false, true}) {
      if (!is_enable[i] && table.holdsStateWhen(i, v)) {
        is_enable[i] = true;
        shape.enables_inactive_at.emplace_back(inputs[i], v);
      }
    }
  }
  if (shape.enables_inactive_at.empty()) {
    return shape;
  }
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (!is_enable[i]) {
      if (shape.data != nullptr
          || !table.passesWhenEnabled(i, shape.enables_inactive_at)) {
        return NextStateShape();
      }
      shape.data = inputs[i];
    }
  }
  if (shape.data == nullptr) {
    return NextStateShape();
  }
  shape.kind = RegisterKind::kEnable;
  return shape;
}

odb::dbITerm* findITerm(odb::dbInst* inst, const sta::LibertyPort* port)
{
  return port != nullptr ? inst->findITerm(port->name().c_str()) : nullptr;
}

std::optional<bool> outputInverted(const sta::FuncExpr* func,
                                   const sta::Sequential* seq)
{
  if (func == nullptr) {
    return std::nullopt;
  }
  bool inverted = false;
  if (func->op() == sta::FuncExpr::Op::not_) {
    inverted = true;
    func = func->left();
  }
  if (func == nullptr || func->op() != sta::FuncExpr::Op::port) {
    return std::nullopt;
  }
  if (func->port() == seq->output()) {
    return inverted;
  }
  if (seq->outputInv() != nullptr && func->port() == seq->outputInv()) {
    return !inverted;
  }
  return std::nullopt;
}

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
    const std::optional<bool> pin_inverted
        = port != nullptr ? outputInverted(port->function(), seq)
                          : std::nullopt;
    if (pin_inverted == false && state_pin == nullptr) {
      state_pin = iterm;
    }
    odb::dbNet* net = iterm->getNet();
    if (net == nullptr
        || (net->getITerms().size() <= 1 && net->getBTerms().empty())) {
      continue;
    }
    ++loaded;
    if (!pin_inverted.has_value()) {
      return false;
    }
    pin = iterm;
    inverted = *pin_inverted;
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

// The output pin of a tie cell that nothing reads any more.
odb::dbITerm* idleTieOutput(sta::dbSta* sta, odb::dbNet* net)
{
  if (net->getSigType().isSupply() || net->isDoNotTouch()
      || !net->getBTerms().empty() || net->getITerms().size() != 1
      || !constNetValue(sta, net).has_value()) {
    return nullptr;
  }
  odb::dbITerm* output = net->getFirstOutput();
  if (output == nullptr || output->getInst()->isDoNotTouch()) {
    return nullptr;
  }
  for (odb::dbITerm* iterm : output->getInst()->getITerms()) {
    if (iterm != output && !iterm->getSigType().isSupply()
        && iterm->getNet() != nullptr) {
      return nullptr;
    }
  }
  return output;
}

}  // namespace

bool hasLiveAsyncControl(sta::dbSta* sta,
                         odb::dbInst* inst,
                         const sta::Sequential* seq)
{
  for (const sta::FuncExpr* control : {seq->clear(), seq->preset()}) {
    if (control == nullptr) {
      continue;
    }
    const std::optional<bool> active = evalConstExpr(sta, inst, control);
    if (!active.has_value() || *active) {
      return true;
    }
  }
  return false;
}

bool isScanCell(sta::dbSta* sta, odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::SCAN) {
      return true;
    }
  }
  return sta->getDbNetwork()->isScanCell(inst);
}

bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst)
{
  if (isScanCell(sta, inst)) {
    return true;
  }
  sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  if (cell == nullptr) {
    return true;
  }
  const sta::Sequential* seq = singleSequential(cell);
  if (seq == nullptr || !seq->isRegister()) {
    return true;
  }
  return hasLiveAsyncControl(sta, inst, seq);
}

bool inactiveAsyncControls(const sta::LibertyCell* cell,
                           const sta::Sequential* seq,
                           PinConstants& constants)
{
  std::vector<const sta::LibertyPort*> controls;
  collectPorts(seq->clear(), controls);
  collectPorts(seq->preset(), controls);
  if (controls.empty() || controls.size() > kMaxTruthTableInputs) {
    return false;
  }
  std::vector<const sta::LibertyPort*> known = controls;
  collectPorts(seq->data(), known);
  collectPorts(seq->clock(), known);
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    const sta::LibertyPort* port = port_iter.next();
    if (port->direction()->isInput()
        && std::ranges::find(known, port) == known.end()) {
      return false;
    }
  }

  const uint32_t rows = 1u << controls.size();
  for (uint32_t row = 0; row < rows; ++row) {
    Assignment values;
    for (size_t i = 0; i < controls.size(); ++i) {
      values[controls[i]] = ((row >> i) & 1u) != 0;
    }
    bool active = false;
    for (const sta::FuncExpr* control : {seq->clear(), seq->preset()}) {
      active = active
               || (control != nullptr
                   && evalExpr(control, values, nullptr, nullptr));
    }
    if (!active) {
      constants.clear();
      for (const sta::LibertyPort* port : controls) {
        constants.emplace_back(port, values.at(port));
      }
      return true;
    }
  }
  return false;
}

void deleteRegister(sta::dbSta* sta, odb::dbInst* inst)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::LibertyCell* cell = network->libertyCell(inst);
  const sta::Sequential* seq
      = cell != nullptr ? singleSequential(cell) : nullptr;
  std::vector<const sta::LibertyPort*> controls;
  if (seq != nullptr) {
    collectPorts(seq->clear(), controls);
    collectPorts(seq->preset(), controls);
  }
  std::vector<odb::dbNet*> control_nets;
  for (const sta::LibertyPort* port : controls) {
    odb::dbITerm* iterm = findITerm(inst, port);
    odb::dbNet* net = iterm != nullptr ? iterm->getNet() : nullptr;
    if (net != nullptr
        && std::ranges::find(control_nets, net) == control_nets.end()) {
      control_nets.push_back(net);
    }
  }
  odb::dbModule* module = network->hasHierarchy() ? inst->getModule() : nullptr;
  network->deleteInstance(network->dbToSta(inst));
  for (odb::dbNet* net : control_nets) {
    odb::dbITerm* tie_output = idleTieOutput(sta, net);
    if (tie_output == nullptr
        || (module != nullptr
            && tie_output->getInst()->getModule() != module)) {
      continue;
    }
    odb::dbModNet* mod_net = tie_output->getModNet();
    network->deleteInstance(network->dbToSta(tie_output->getInst()));
    network->deleteNet(network->dbToSta(net));
    if (mod_net != nullptr && mod_net->connectionCount() == 0) {
      odb::dbModNet::destroy(mod_net);
    }
  }
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
    return false;
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

namespace {

void classifyEnableRegister(odb::dbInst* inst,
                            const NextStateShape& shape,
                            RegisterClass& cls)
{
  for (const auto& [port, inactive] : shape.enables_inactive_at) {
    odb::dbITerm* iterm = findITerm(inst, port);
    if (iterm == nullptr) {
      return;
    }
    cls.enables.emplace_back(iterm->getMTerm(), iterm->getNet());
  }
  std::ranges::sort(cls.enables, [](const auto& a, const auto& b) {
    return strcmp(a.first->getConstName(), b.first->getConstName()) < 0;
  });
  for (odb::dbITerm* iterm : inst->getITerms()) {
    const odb::dbSigType sig = iterm->getSigType();
    if (sig == odb::dbSigType::CLOCK || sig == odb::dbSigType::POWER
        || sig == odb::dbSigType::GROUND
        || iterm->getIoType() != odb::dbIoType::INPUT
        || iterm->getMTerm() == cls.data) {
      continue;
    }
    bool is_control = false;
    for (const auto& [mterm, net] : cls.enables) {
      is_control = is_control || mterm == iterm->getMTerm();
    }
    if (!is_control) {
      return;
    }
  }
  if (cls.output == nullptr || cls.clock == nullptr) {
    return;
  }
  cls.kind = RegisterKind::kEnable;
  cls.master = inst->getMaster();
}

}  // namespace

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
  if (shape.kind == RegisterKind::kOther || data == nullptr) {
    return cls;
  }
  cls.data = data->getMTerm();

  odb::dbITerm* out_pin = nullptr;
  bool out_inverted = false;
  if (!loadedOutput(sta, inst, seq, out_pin, out_inverted)) {
    return cls;
  }
  cls.output = out_pin != nullptr ? out_pin->getMTerm() : nullptr;
  // A cell with asynchronous controls is never the cell of a new register,
  // so it does not keep its class.
  const bool has_async = seq->clear() != nullptr || seq->preset() != nullptr;
  if (shape.kind == RegisterKind::kPlain) {
    const bool loads_see_complement = shape.data_inverted != out_inverted;
    if (!loads_see_complement) {
      cls.kind = RegisterKind::kPlain;
    } else if (!has_async && cls.output != nullptr && cls.clock != nullptr) {
      cls.kind = RegisterKind::kInverting;
      cls.master = inst->getMaster();
    }
    return cls;
  }
  if (!out_inverted && !has_async) {
    classifyEnableRegister(inst, shape, cls);
  }
  return cls;
}

bool sameRegisterClass(const RegisterClass& a, const RegisterClass& b)
{
  if (a.kind != b.kind) {
    return false;
  }
  switch (a.kind) {
    case RegisterKind::kPlain:
      return a.clock == b.clock;
    case RegisterKind::kEnable:
      return a.master == b.master && a.clock == b.clock && a.output == b.output
             && a.enables == b.enables;
    case RegisterKind::kInverting:
      return a.master == b.master && a.clock == b.clock && a.output == b.output;
    case RegisterKind::kOther:
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
    const uint32_t rows = 1u << inputs.size();
    for (uint32_t row = 0; row < rows; ++row) {
      Assignment values;
      Assignment complemented;
      for (size_t i = 0; i < inputs.size(); ++i) {
        const bool bit = ((row >> i) & 1u) != 0;
        values[inputs[i]] = bit;
        complemented[inputs[i]] = !bit;
      }
      if (evalExpr(func, values, nullptr, nullptr)
          == evalExpr(func, complemented, nullptr, nullptr)) {
        return false;
      }
    }
    ++outputs;
  }
  return outputs > 0;
}

}  // namespace ret
