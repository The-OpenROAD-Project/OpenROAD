// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "RegisterUtils.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "rsz/Resizer.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDelay.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
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

const sta::Sequential* singleSequential(sta::dbSta* sta, odb::dbInst* inst)
{
  const sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  return cell != nullptr ? singleSequential(cell) : nullptr;
}

std::vector<sta::LibertyCell*> libertyCells(sta::dbSta* sta)
{
  std::vector<sta::LibertyCell*> cells;
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      sta->getDbNetwork()->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyCellIterator cell_iter(lib_iter->next());
    while (cell_iter.hasNext()) {
      cells.push_back(cell_iter.next());
    }
  }
  return cells;
}

sta::LibertyCell* libertyCell(sta::dbSta* sta, odb::dbMaster* master)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Cell* cell = network->dbToSta(master);
  return cell != nullptr ? network->libertyCell(cell) : nullptr;
}

namespace {

// Not a cell of plain logic, whatever its liberty function.
bool isSpecialCell(const sta::LibertyCell* cell)
{
  return cell->isIsolationCell() || cell->isLevelShifter() || cell->isPad()
         || cell->isMacro();
}

}  // namespace

bool isUsableCell(const sta::LibertyCell* cell, rsz::Resizer* resizer)
{
  return !cell->dontUse() && (resizer == nullptr || !resizer->dontUse(cell))
         && !cell->alwaysOn() && !isSpecialCell(cell);
}

const sta::LibertyPort* libertyPort(sta::dbSta* sta, odb::dbITerm* pin)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  return network->libertyPort(network->dbToSta(pin));
}

bool isNegedge(const sta::Sequential* seq)
{
  return seq->clock() != nullptr
         && seq->clock()->op() == sta::FuncExpr::Op::not_;
}

bool hasSimpleClock(const sta::Sequential* seq)
{
  const sta::FuncExpr* clock = seq->clock();
  if (clock != nullptr && clock->op() == sta::FuncExpr::Op::not_) {
    clock = clock->left();
  }
  return clock != nullptr && clock->op() == sta::FuncExpr::Op::port;
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

bool isSignalInput(odb::dbITerm* pin)
{
  odb::dbNet* net = pin->getNet();
  return pin->getIoType() == odb::dbIoType::INPUT && net != nullptr
         && !net->getSigType().isSupply();
}

bool isRead(odb::dbNet* net)
{
  return net->getITerms().size() > 1 || !net->getBTerms().empty();
}

bool hasSecondDriver(odb::dbNet* net)
{
  int drivers = 0;
  for (odb::dbITerm* pin : net->getITerms()) {
    if (pin->getIoType() != odb::dbIoType::INPUT && ++drivers > 1) {
      return true;
    }
  }
  for (odb::dbBTerm* port : net->getBTerms()) {
    if (port->getIoType() != odb::dbIoType::OUTPUT && ++drivers > 1) {
      return true;
    }
  }
  return false;
}

void movePin(odb::dbITerm* pin, odb::dbNet* net, odb::dbModNet* mod_net)
{
  pin->disconnect();
  pin->connect(net);
  if (mod_net != nullptr) {
    pin->connect(mod_net);
  }
}

void sortUnique(std::vector<int>& values)
{
  std::ranges::sort(values);
  const auto repeats = std::ranges::unique(values);
  values.erase(repeats.begin(), repeats.end());
}

std::vector<odb::dbInst*> inIdOrder(
    const std::unordered_set<odb::dbInst*>& cells)
{
  std::vector<odb::dbInst*> ordered(cells.begin(), cells.end());
  std::ranges::sort(ordered, [](odb::dbInst* a, odb::dbInst* b) {
    return a->getId() < b->getId();
  });
  return ordered;
}

odb::dbInst* makeCell(sta::dbSta* sta,
                      sta::Instance* parent,
                      const char* base_name,
                      odb::dbMaster* master)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  const std::string name = network->block()->makeNewInstName(
      network->getModInst(parent),
      base_name,
      odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE);
  odb::dbInst* inst = network->staToDb(
      network->makeInstance(libertyCell(sta, master), name, parent));
  inst->setSourceType(odb::dbSourceType::TIMING);
  return inst;
}

odb::dbNet* makeNet(sta::dbSta* sta,
                    sta::Instance* parent,
                    const char* base_name)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  return network->staToDb(network->makeNet(
      base_name, parent, odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE));
}

void Discards::add(odb::dbInst* cell)
{
  for (odb::dbITerm* pin : cell->getITerms()) {
    if (!pin->getSigType().isSupply()) {
      pin->disconnect();
    }
  }
  cells_.push_back(cell);
}

void Discards::add(odb::dbNet* net, odb::dbModNet* mod_net)
{
  nets_.push_back({.net = net, .mod_net = mod_net});
}

void Discards::keep()
{
  kept_cells_ = cells_.size();
  kept_nets_ = nets_.size();
}

void Discards::undo()
{
  cells_.resize(kept_cells_);
  nets_.resize(kept_nets_);
}

int Discards::registerCount() const
{
  return static_cast<int>(std::ranges::count_if(cells_, [](odb::dbInst* cell) {
    return cell->getMaster()->isSequential();
  }));
}

void Discards::destroy(sta::dbSta* sta)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  for (odb::dbInst* cell : cells_) {
    network->deleteInstance(network->dbToSta(cell));
  }
  for (const auto& [net, mod_net] : nets_) {
    network->deleteNet(network->dbToSta(net));
    if (mod_net != nullptr && mod_net->connectionCount() == 0) {
      odb::dbModNet::destroy(mod_net);
    }
  }
  clear();
}

void Discards::clear()
{
  cells_.clear();
  nets_.clear();
  keep();
}

void connectPin(odb::dbITerm* pin,
                odb::dbNet* net,
                const std::unordered_map<odb::dbNet*, odb::dbModNet*>& mod_nets)
{
  pin->connect(net);
  const auto it = mod_nets.find(net);
  if (it != mod_nets.end()) {
    pin->connect(it->second);
  }
}

namespace {

constexpr size_t kMaxTruthTableInputs = 6;

// The output pin that drives the net; nullptr when a port or a second pin
// can drive it too.
odb::dbITerm* onlyDriver(odb::dbNet* net)
{
  odb::dbITerm* driver = nullptr;
  for (odb::dbITerm* iterm : net->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::INPUT) {
      continue;
    }
    if (driver != nullptr || iterm->getIoType() != odb::dbIoType::OUTPUT) {
      return nullptr;
    }
    driver = iterm;
  }
  for (odb::dbBTerm* port : net->getBTerms()) {
    if (port->getIoType() != odb::dbIoType::OUTPUT) {
      return nullptr;
    }
  }
  return driver;
}

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
  // Tied only when one cell drives the net, and drives it always.
  odb::dbITerm* driver = onlyDriver(net);
  const sta::LibertyPort* port
      = driver != nullptr ? libertyPort(sta, driver) : nullptr;
  if (port == nullptr || port->tristateEnable() != nullptr
      || port->function() == nullptr) {
    return std::nullopt;
  }
  const sta::FuncExpr::Op op = port->function()->op();
  if (op == sta::FuncExpr::Op::one) {
    return true;
  }
  if (op == sta::FuncExpr::Op::zero) {
    return false;
  }
  return std::nullopt;
}

odb::dbITerm* findITerm(odb::dbInst* inst, const sta::LibertyPort* port)
{
  return port != nullptr ? inst->findITerm(port->name().c_str()) : nullptr;
}

// The value of an expression where `value` gives each pin's, nullopt for a
// pin that is not known; nullopt when such pins decide it.
using PinValue = std::function<std::optional<bool>(const sta::LibertyPort*)>;

std::optional<bool> evalExpr(const sta::FuncExpr* e, const PinValue& value)
{
  if (e == nullptr) {
    return std::nullopt;
  }
  switch (e->op()) {
    case sta::FuncExpr::Op::port:
      return value(e->port());
    case sta::FuncExpr::Op::not_: {
      const std::optional<bool> v = evalExpr(e->left(), value);
      return v.has_value() ? std::optional<bool>(!*v) : std::nullopt;
    }
    case sta::FuncExpr::Op::and_: {
      const std::optional<bool> a = evalExpr(e->left(), value);
      const std::optional<bool> b = evalExpr(e->right(), value);
      if (a == false || b == false) {
        return false;
      }
      return (a == true && b == true) ? std::optional<bool>(true)
                                      : std::nullopt;
    }
    case sta::FuncExpr::Op::or_: {
      const std::optional<bool> a = evalExpr(e->left(), value);
      const std::optional<bool> b = evalExpr(e->right(), value);
      if (a == true || b == true) {
        return true;
      }
      return (a == false && b == false) ? std::optional<bool>(false)
                                        : std::nullopt;
    }
    case sta::FuncExpr::Op::xor_: {
      const std::optional<bool> a = evalExpr(e->left(), value);
      const std::optional<bool> b = evalExpr(e->right(), value);
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

// The pins set from the bits of `row`, the first pin from the lowest bit.
Assignment assignment(const std::vector<const sta::LibertyPort*>& pins,
                      const uint32_t row)
{
  Assignment values;
  for (size_t i = 0; i < pins.size(); ++i) {
    values[pins[i]] = ((row >> i) & 1u) != 0;
  }
  return values;
}

// `values` has every pin of the expression; state_inv reads the complement
// of state.
bool evalExpr(const sta::FuncExpr* e,
              const Assignment& values,
              const sta::LibertyPort* state = nullptr,
              const sta::LibertyPort* state_inv = nullptr)
{
  const PinValue value = [&](const sta::LibertyPort* port) {
    const bool inverted = state_inv != nullptr && port == state_inv;
    return std::optional<bool>(inverted ? !values.at(state) : values.at(port));
  };
  return evalExpr(e, value).value_or(false);
}

struct NextStateShape
{
  RegisterKind kind = RegisterKind::kOther;
  const sta::LibertyPort* data = nullptr;
  bool data_inverted = false;
  PinConstants enables_inactive_at;
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

  bool holdsStateWhen(const size_t i, const bool v) const
  {
    for (uint32_t row = 0; row < rows_; ++row) {
      if (bit(row, i) == v && value(row) != stateBit(row)) {
        return false;
      }
    }
    return true;
  }

  bool passesWhenEnabled(const size_t d, const PinConstants& enables) const
  {
    for (uint32_t row = 0; row < rows_; ++row) {
      if (enablesActive(row, enables) && value(row) != bit(row, d)) {
        return false;
      }
    }
    return true;
  }

 private:
  static bool bit(const uint32_t row, const size_t i)
  {
    return ((row >> i) & 1u) != 0;
  }
  bool stateBit(const uint32_t row) const { return bit(row, inputs_.size()); }

  bool enablesActive(const uint32_t row, const PinConstants& enables) const
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

  bool value(const uint32_t row) const
  {
    Assignment values = assignment(inputs_, row);
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
    for (const bool v : {false, true}) {
      if (!is_enable[i] && table.holdsStateWhen(i, v)) {
        is_enable[i] = true;
        shape.enables_inactive_at.push_back({.port = inputs[i], .high = v});
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

bool loadedOutput(sta::dbSta* sta,
                  odb::dbInst* inst,
                  const sta::Sequential* seq,
                  odb::dbITerm*& pin,
                  bool& inverted)
{
  pin = nullptr;
  inverted = false;
  odb::dbITerm* state_pin = nullptr;
  int loaded = 0;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
      continue;
    }
    const sta::LibertyPort* port = libertyPort(sta, iterm);
    const std::optional<bool> pin_inverted
        = port != nullptr ? outputInverted(port->function(), seq)
                          : std::nullopt;
    if (pin_inverted == false && state_pin == nullptr) {
      state_pin = iterm;
    }
    odb::dbNet* net = iterm->getNet();
    if (net == nullptr || !isRead(net)) {
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

// The output pin of a tie cell that nothing reads anymore.
odb::dbITerm* idleTieOutput(sta::dbSta* sta, odb::dbNet* net)
{
  if (net->getSigType().isSupply() || !net->getBTerms().empty()
      || net->getITerms().size() != 1 || !constNetValue(sta, net).has_value()) {
    return nullptr;
  }
  // The net has a constant value, so a cell drives it.
  odb::dbITerm* output = onlyDriver(net);
  odb::dbInst* tie = output->getInst();
  if (isHeld(tie) || hasPinConstraint(sta, tie)
      || hasCellConstraint(sta, tie)) {
    return nullptr;
  }
  for (odb::dbITerm* iterm : tie->getITerms()) {
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
  const PinValue tied = [&](const sta::LibertyPort* port) {
    odb::dbITerm* iterm = findITerm(inst, port);
    return iterm != nullptr ? constNetValue(sta, iterm->getNet())
                            : std::nullopt;
  };
  for (const sta::FuncExpr* control : {seq->clear(), seq->preset()}) {
    if (control == nullptr) {
      continue;
    }
    const std::optional<bool> active = evalExpr(control, tied);
    if (!active.has_value() || *active) {
      return true;
    }
  }
  return false;
}

namespace {

std::vector<const sta::Sdc*> allSdcs(sta::dbSta* sta)
{
  std::vector<const sta::Sdc*> sdcs;
  for (const sta::Mode* mode : sta->modes()) {
    sdcs.push_back(mode->sdc());
    for (const auto& [corner, corner_sdc] : mode->cornerSdcs()) {
      sdcs.push_back(corner_sdc);
    }
  }
  return sdcs;
}

}  // namespace

bool isGrouped(odb::dbInst* inst)
{
  return inst->getGroup() != nullptr || inst->getRegion() != nullptr;
}

bool isHeld(odb::dbInst* inst)
{
  if (inst->isDoNotTouch() || inst->isFixed() || isGrouped(inst)
      || inst->getScanInst() != nullptr) {
    return true;
  }
  for (odb::dbITerm* pin : inst->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (net != nullptr && net->isDoNotTouch()) {
      return true;
    }
  }
  return false;
}

bool hasPinConstraint(sta::dbSta* sta, odb::dbInst* inst)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  for (const sta::Sdc* sdc : allSdcs(sta)) {
    for (odb::dbITerm* iterm : inst->getITerms()) {
      odb::dbNet* net = iterm->getNet();
      if (sdc->isConstrained(network->dbToSta(iterm))
          || (iterm->getIoType() == odb::dbIoType::OUTPUT && net != nullptr
              && sdc->isConstrained(network->dbToSta(net)))) {
        return true;
      }
    }
  }
  return false;
}

bool hasCellConstraint(sta::dbSta* sta, odb::dbInst* inst)
{
  const sta::Instance* cell = sta->getDbNetwork()->dbToSta(inst);
  return std::ranges::any_of(allSdcs(sta), [cell](const sta::Sdc* sdc) {
    return sdc->isConstrained(cell);
  });
}

bool hasDisabledEdge(sta::dbSta* sta, odb::dbInst* inst)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Graph* graph = sta->ensureGraph();
  const auto disabled = [sta](sta::Edge* edge) {
    return std::ranges::any_of(sta->modes(), [&](const sta::Mode* mode) {
      return sta->isDisabledConstraint(edge, mode->sdc());
    });
  };
  for (odb::dbITerm* iterm : inst->getITerms()) {
    sta::Vertex* vertex;
    sta::Vertex* bidirect_driver;
    graph->pinVertices(network->dbToSta(iterm), vertex, bidirect_driver);
    for (sta::Vertex* end : {vertex, bidirect_driver}) {
      if (end == nullptr) {
        continue;
      }
      sta::VertexInEdgeIterator in_iter(end, graph);
      while (in_iter.hasNext()) {
        if (disabled(in_iter.next())) {
          return true;
        }
      }
      sta::VertexOutEdgeIterator out_iter(end, graph);
      while (out_iter.hasNext()) {
        if (disabled(out_iter.next())) {
          return true;
        }
      }
    }
  }
  return false;
}

std::vector<odb::dbInst*> portDelayReferenceCells(sta::dbSta* sta)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  std::vector<odb::dbInst*> cells;
  const auto add = [&](const sta::PortDelay* delay) {
    if (delay->refPin() != nullptr) {
      odb::dbInst* inst = network->staToDb(network->instance(delay->refPin()));
      if (inst != nullptr) {
        cells.push_back(inst);
      }
    }
  };
  for (const sta::Sdc* sdc : allSdcs(sta)) {
    std::ranges::for_each(sdc->inputDelays(), add);
    std::ranges::for_each(sdc->outputDelays(), add);
  }
  return cells;
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

namespace {

bool hasOpenInput(odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() == odb::dbIoType::INPUT
        && !iterm->getSigType().isSupply() && iterm->getNet() == nullptr) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst)
{
  if (isScanCell(sta, inst) || hasOpenInput(inst)) {
    return true;
  }
  const sta::Sequential* seq = singleSequential(sta, inst);
  if (seq == nullptr || !seq->isRegister() || !hasSimpleClock(seq)) {
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
    const Assignment values = assignment(controls, row);
    bool active = false;
    for (const sta::FuncExpr* control : {seq->clear(), seq->preset()}) {
      active = active || (control != nullptr && evalExpr(control, values));
    }
    if (!active) {
      constants.clear();
      for (const sta::LibertyPort* port : controls) {
        constants.push_back({.port = port, .high = values.at(port)});
      }
      return true;
    }
  }
  return false;
}

namespace {

// Output nets that hold only the register's own pin.
std::vector<std::pair<odb::dbNet*, odb::dbModNet*>> unloadedOutputNets(
    odb::dbInst* inst)
{
  std::vector<std::pair<odb::dbNet*, odb::dbModNet*>> nets;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (iterm->getIoType() == odb::dbIoType::OUTPUT && net != nullptr
        && !isRead(net) && !net->isDoNotTouch()) {
      nets.emplace_back(net, iterm->getModNet());
    }
  }
  return nets;
}

}  // namespace

void discardRegister(sta::dbSta* sta, odb::dbInst* inst, Discards& discards)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  const sta::Sequential* seq = singleSequential(sta, inst);
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
  const std::vector<std::pair<odb::dbNet*, odb::dbModNet*>> unloaded
      = unloadedOutputNets(inst);
  discards.add(inst);
  for (odb::dbNet* net : control_nets) {
    odb::dbITerm* tie_output = idleTieOutput(sta, net);
    if (tie_output == nullptr
        || (module != nullptr
            && tie_output->getInst()->getModule() != module)) {
      continue;
    }
    odb::dbModNet* mod_net = tie_output->getModNet();
    discards.add(tie_output->getInst());
    discards.add(net, mod_net);
  }
  for (const auto& [net, mod_net] : unloaded) {
    discards.add(net, mod_net);
  }
}

bool isRegisterDataPin(sta::dbSta* sta, odb::dbITerm* pin)
{
  if (pin->getSigType() == odb::dbSigType::CLOCK) {
    return false;
  }
  const sta::Sequential* seq = singleSequential(sta, pin->getInst());
  if (seq == nullptr) {
    return true;
  }
  std::vector<const sta::LibertyPort*> ports;
  collectPorts(seq->data(), ports);
  return std::ranges::any_of(ports, [pin](const sta::LibertyPort* port) {
    return port->name() == pin->getMTerm()->getConstName();
  });
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
  const sta::Sequential* seq = singleSequential(sta, inst);
  return seq != nullptr && seq->isRegister() && isNegedge(seq);
}

odb::dbITerm* registerClockPin(odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::CLOCK) {
      return iterm;
    }
  }
  return nullptr;
}

odb::dbNet* registerClockNet(odb::dbInst* inst)
{
  odb::dbITerm* clock = registerClockPin(inst);
  return clock != nullptr ? clock->getNet() : nullptr;
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
    cls.enables.push_back({.pin = iterm->getMTerm(), .net = iterm->getNet()});
  }
  std::ranges::sort(cls.enables, [](const auto& a, const auto& b) {
    return strcmp(a.pin->getConstName(), b.pin->getConstName()) < 0;
  });
  for (odb::dbITerm* iterm : inst->getITerms()) {
    const odb::dbSigType sig = iterm->getSigType();
    if (sig == odb::dbSigType::CLOCK || sig == odb::dbSigType::POWER
        || sig == odb::dbSigType::GROUND
        || iterm->getIoType() != odb::dbIoType::INPUT
        || iterm->getMTerm() == cls.data) {
      continue;
    }
    const bool is_control
        = std::ranges::any_of(cls.enables, [&](const EnablePin& enable) {
            return enable.pin == iterm->getMTerm();
          });
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

RegisterClass registerClass(sta::dbSta* sta,
                            odb::dbInst* inst,
                            rsz::Resizer* resizer)
{
  RegisterClass cls;
  cls.clock = registerClockNet(inst);
  cls.scan = isScanCell(sta, inst) || inst->getScanInst() != nullptr;
  const sta::Sequential* seq = singleSequential(sta, inst);
  if (seq == nullptr || !seq->isRegister()) {
    return cls;
  }
  cls.negedge = isNegedge(seq);
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
  // A cell with asynchronous controls or one that is not usable is never the
  // cell of a new register, so it does not keep its class.
  const bool keeps_cell
      = seq->clear() == nullptr && seq->preset() == nullptr
        && isUsableCell(sta->getDbNetwork()->libertyCell(inst), resizer);
  if (shape.kind == RegisterKind::kPlain) {
    const bool loads_see_complement = shape.data_inverted != out_inverted;
    if (!loads_see_complement) {
      cls.kind = RegisterKind::kPlain;
    } else if (keeps_cell && cls.output != nullptr && cls.clock != nullptr) {
      cls.kind = RegisterKind::kInverting;
      cls.master = inst->getMaster();
    }
    return cls;
  }
  if (!out_inverted && keeps_cell) {
    classifyEnableRegister(inst, shape, cls);
  }
  return cls;
}

bool keepsClass(const RegisterClass& cls)
{
  return cls.kind == RegisterKind::kEnable
         || cls.kind == RegisterKind::kInverting;
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

bool isCombinationalGate(sta::dbSta* sta, odb::dbInst* inst)
{
  const sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  if (cell == nullptr || cell->isSequential() || isSpecialCell(cell)) {
    return false;
  }
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    const sta::LibertyPort* port = port_iter.next();
    if (port->direction()->isAnyOutput()
        && (port->function() == nullptr || port->tristateEnable() != nullptr)) {
      return false;
    }
  }
  return true;
}

bool isSelfDualGate(sta::dbSta* sta, odb::dbInst* inst)
{
  const sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  if (cell == nullptr || cell->isSequential()) {
    return false;
  }
  int outputs = 0;
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    const sta::LibertyPort* port = port_iter.next();
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
      if (evalExpr(func, assignment(inputs, row))
          == evalExpr(func, assignment(inputs, ~row))) {
        return false;
      }
    }
    ++outputs;
  }
  return outputs > 0;
}

}  // namespace ret
