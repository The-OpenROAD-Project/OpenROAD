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
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "rsz/Resizer.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDelay.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Sequential.hh"

namespace ret {

const sta::Sequential* singleSequential(const sta::LibertyCell* cell)
{
  const sta::SequentialSeq& sequentials = cell->sequentials();
  return sequentials.size() == 1 ? &sequentials.front() : nullptr;
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

std::vector<const sta::LibertyPort*> libertyPorts(const sta::LibertyCell* cell)
{
  std::vector<const sta::LibertyPort*> ports;
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    ports.push_back(port_iter.next());
  }
  return ports;
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

// A pin read as it is or through one inverter.
struct Literal
{
  const sta::LibertyPort* port;
  bool inverted;
};

// nullopt for any other expression, nullptr included.
std::optional<Literal> literal(const sta::FuncExpr* expr)
{
  const bool inverted
      = expr != nullptr && expr->op() == sta::FuncExpr::Op::not_;
  const sta::FuncExpr* pin = inverted ? expr->left() : expr;
  if (pin == nullptr || pin->op() != sta::FuncExpr::Op::port) {
    return std::nullopt;
  }
  return Literal{.port = pin->port(), .inverted = inverted};
}

bool pinNameLess(odb::dbMTerm* a, odb::dbMTerm* b)
{
  return strcmp(a->getConstName(), b->getConstName()) < 0;
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

bool hasAsyncControl(const sta::Sequential* seq)
{
  return seq->clear() != nullptr || seq->preset() != nullptr;
}

odb::dbMTerm* clockMTerm(const sta::Sequential* seq, odb::dbMaster* master)
{
  const std::optional<Literal> clock = literal(seq->clock());
  if (!clock.has_value()) {
    return nullptr;
  }
  odb::dbMTerm* pin = master->findMTerm(clock->port->name().c_str());
  const auto marked = [](odb::dbMTerm* mterm) {
    return mterm->getSigType() == odb::dbSigType::CLOCK;
  };
  const bool only_mark
      = pin != nullptr && marked(pin)
        && std::ranges::count_if(master->getMTerms(), marked) == 1;
  return only_mark ? pin : nullptr;
}

std::optional<bool> outputInverted(const sta::FuncExpr* func,
                                   const sta::Sequential* seq)
{
  const std::optional<Literal> read = literal(func);
  if (!read.has_value()) {
    return std::nullopt;
  }
  if (read->port == seq->output()) {
    return read->inverted;
  }
  if (seq->outputInv() != nullptr && read->port == seq->outputInv()) {
    return !read->inverted;
  }
  return std::nullopt;
}

std::optional<bool> outputInverted(sta::dbSta* sta,
                                   odb::dbITerm* pin,
                                   const sta::Sequential* seq)
{
  const sta::LibertyPort* port = libertyPort(sta, pin);
  return port != nullptr ? outputInverted(port->function(), seq) : std::nullopt;
}

std::optional<bool> constantOutput(const sta::LibertyPort* port)
{
  const sta::FuncExpr* func = port->function();
  if (func == nullptr || port->tristateEnable() != nullptr) {
    return std::nullopt;
  }
  if (func->op() == sta::FuncExpr::Op::one) {
    return true;
  }
  if (func->op() == sta::FuncExpr::Op::zero) {
    return false;
  }
  return std::nullopt;
}

bool isSignalInput(odb::dbITerm* pin)
{
  odb::dbNet* net = pin->getNet();
  return pin->getIoType() == odb::dbIoType::INPUT && net != nullptr
         && !net->getSigType().isSupply();
}

bool drivesNet(odb::dbITerm* pin)
{
  return pin->getIoType() == odb::dbIoType::OUTPUT && pin->getNet() != nullptr;
}

bool isRead(odb::dbNet* net)
{
  return net->getITerms().hasMoreThan(1) || !net->getBTerms().empty();
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

odb::dbITerm* NetDrivers::driverOf(odb::dbNet* net)
{
  const auto [entry, is_new] = drivers_.try_emplace(net);
  if (is_new) {
    entry->second = net->getFirstOutput();
  }
  return entry->second;
}

void movePin(odb::dbITerm* pin, odb::dbNet* net, odb::dbModNet* mod_net)
{
  pin->disconnect();
  pin->connect(net);
  if (mod_net != nullptr) {
    pin->connect(mod_net);
  }
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

std::vector<odb::dbInst*> inIdOrder(
    const std::unordered_set<odb::dbInst*>& cells)
{
  std::vector<odb::dbInst*> ordered(cells.begin(), cells.end());
  std::ranges::sort(ordered, odb::compare_by_id);
  return ordered;
}

bool hasOneSupplyNetPerKind(odb::dbInst* inst)
{
  odb::dbNet* power = nullptr;
  odb::dbNet* ground = nullptr;
  for (odb::dbITerm* pin : inst->getITerms()) {
    odb::dbNet* net = pin->getNet();
    if (!pin->getSigType().isSupply() || net == nullptr) {
      continue;
    }
    odb::dbNet*& of_kind
        = pin->getSigType() == odb::dbSigType::POWER ? power : ground;
    if (of_kind != nullptr && of_kind != net) {
      return false;
    }
    of_kind = net;
  }
  return true;
}

SupplyPins supplyPins(odb::dbInst* inst)
{
  SupplyPins pins;
  for (odb::dbITerm* pin : inst->getITerms()) {
    if (pin->getSigType().isSupply()) {
      pins.push_back({.pin = pin->getMTerm(), .net = pin->getNet()});
    }
  }
  std::ranges::sort(pins, pinNameLess, &SupplyPin::pin);
  return pins;
}

bool sameSupply(const SupplyPins& a, const SupplyPins& b)
{
  return std::ranges::equal(a, b, [](const SupplyPin& x, const SupplyPin& y) {
    return x.net == y.net
           && strcmp(x.pin->getConstName(), y.pin->getConstName()) == 0;
  });
}

bool hasSupplyPinsOf(odb::dbMaster* master, odb::dbMaster* other)
{
  return std::ranges::all_of(other->getMTerms(), [master](odb::dbMTerm* pin) {
    odb::dbMTerm* same = master->findMTerm(pin->getConstName());
    return !pin->getSigType().isSupply()
           || (same != nullptr && same->getSigType() == pin->getSigType());
  });
}

namespace {

// The pin of `like` that a supply pin of a new cell is connected like: the
// pin of that name, else a connected pin of its kind when those are on one
// net.
odb::dbITerm* supplySource(odb::dbITerm* pin,
                           odb::dbInst* like,
                           const bool one_net_per_kind)
{
  const odb::dbSigType kind = pin->getSigType();
  odb::dbITerm* same = like->findITerm(pin->getMTerm()->getConstName());
  if (same != nullptr && same->getSigType() == kind) {
    return same;
  }
  if (one_net_per_kind) {
    for (odb::dbITerm* other : like->getITerms()) {
      if (other->getSigType() == kind && other->getNet() != nullptr) {
        return other;
      }
    }
  }
  return nullptr;
}

void connectSupplyLike(odb::dbInst* inst, odb::dbInst* like)
{
  const bool one_net_per_kind = hasOneSupplyNetPerKind(like);
  for (odb::dbITerm* pin : inst->getITerms()) {
    if (!pin->getSigType().isSupply()) {
      continue;
    }
    odb::dbITerm* source = supplySource(pin, like, one_net_per_kind);
    if (source == nullptr || source->getNet() == nullptr) {
      continue;
    }
    // With the module net: in a hierarchical netlist the cell reads the
    // supply under the name it has in its module.
    odb::dbModNet* mod_net = source->getModNet();
    const bool same_module
        = mod_net != nullptr && mod_net->getParent() == inst->getModule();
    movePin(pin, source->getNet(), same_module ? mod_net : nullptr);
    // As global_connect leaves it.
    if (source->isSpecial()) {
      pin->setSpecial();
    }
  }
}

}  // namespace

sta::Instance* parentInstance(sta::dbSta* sta, odb::dbInst* inst)
{
  const sta::dbNetwork* network = sta->getDbNetwork();
  return network->parent(network->dbToSta(inst));
}

odb::dbInst* makeCell(sta::dbSta* sta,
                      sta::Instance* parent,
                      const char* base_name,
                      odb::dbMaster* master,
                      odb::dbInst* like)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  const std::string name = network->block()->makeNewInstName(
      network->getModInst(parent),
      base_name,
      odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE);
  odb::dbInst* inst = network->staToDb(
      network->makeInstance(libertyCell(sta, master), name, parent));
  inst->setSourceType(odb::dbSourceType::TIMING);
  connectSupplyLike(inst, like);
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
  return port != nullptr ? constantOutput(port) : std::nullopt;
}

odb::dbITerm* findITerm(odb::dbInst* inst, const sta::LibertyPort* port)
{
  return port != nullptr ? inst->findITerm(port->name().c_str()) : nullptr;
}

// A pin's value; nullopt when it is not known.
using PinValue = std::function<std::optional<bool>(const sta::LibertyPort*)>;

// nullopt when pins without a value decide the expression.
std::optional<bool> evalExpr(const sta::FuncExpr* expr, const PinValue& value)
{
  if (expr == nullptr) {
    return std::nullopt;
  }
  switch (expr->op()) {
    case sta::FuncExpr::Op::port:
      return value(expr->port());
    case sta::FuncExpr::Op::not_: {
      const std::optional<bool> v = evalExpr(expr->left(), value);
      return v.has_value() ? std::optional<bool>(!*v) : std::nullopt;
    }
    case sta::FuncExpr::Op::and_: {
      const std::optional<bool> a = evalExpr(expr->left(), value);
      const std::optional<bool> b = evalExpr(expr->right(), value);
      if (a == false || b == false) {
        return false;
      }
      return (a == true && b == true) ? std::optional<bool>(true)
                                      : std::nullopt;
    }
    case sta::FuncExpr::Op::or_: {
      const std::optional<bool> a = evalExpr(expr->left(), value);
      const std::optional<bool> b = evalExpr(expr->right(), value);
      if (a == true || b == true) {
        return true;
      }
      return (a == false && b == false) ? std::optional<bool>(false)
                                        : std::nullopt;
    }
    case sta::FuncExpr::Op::xor_: {
      const std::optional<bool> a = evalExpr(expr->left(), value);
      const std::optional<bool> b = evalExpr(expr->right(), value);
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

void collectPorts(const sta::FuncExpr* expr,
                  std::vector<const sta::LibertyPort*>& ports)
{
  if (expr == nullptr) {
    return;
  }
  if (expr->op() == sta::FuncExpr::Op::port) {
    if (std::ranges::find(ports, expr->port()) == ports.end()) {
      ports.push_back(expr->port());
    }
    return;
  }
  collectPorts(expr->left(), ports);
  collectPorts(expr->right(), ports);
}

// In order of first appearance, which fixes the truth-table rows and the
// order tie cells are made in; sta::FuncExpr::ports() orders by address.
std::vector<const sta::LibertyPort*> exprPorts(
    std::initializer_list<const sta::FuncExpr*> exprs)
{
  std::vector<const sta::LibertyPort*> ports;
  for (const sta::FuncExpr* expr : exprs) {
    collectPorts(expr, ports);
  }
  return ports;
}

bool reads(const sta::FuncExpr* expr, const sta::LibertyPort* port)
{
  return expr != nullptr && expr->hasPort(port);
}

using Assignment = std::unordered_map<const sta::LibertyPort*, bool>;

bool rowBit(const uint32_t row, const size_t i)
{
  return ((row >> i) & 1u) != 0;
}

// The pins set from the bits of `row`, the first pin from the lowest bit.
Assignment assignment(const std::vector<const sta::LibertyPort*>& pins,
                      const uint32_t row)
{
  Assignment values;
  for (size_t i = 0; i < pins.size(); ++i) {
    values[pins[i]] = rowBit(row, i);
  }
  return values;
}

// `values` has every pin of the expression; state_inv reads the complement
// of state.
bool evalExpr(const sta::FuncExpr* expr,
              const Assignment& values,
              const sta::LibertyPort* state = nullptr,
              const sta::LibertyPort* state_inv = nullptr)
{
  const PinValue value = [&](const sta::LibertyPort* port) {
    const bool inverted = state_inv != nullptr && port == state_inv;
    return std::optional<bool>(inverted ? !values.at(state) : values.at(port));
  };
  return evalExpr(expr, value).value_or(false);
}

struct NextStateShape
{
  RegisterKind kind = RegisterKind::kOther;
  const sta::LibertyPort* data = nullptr;
  bool data_inverted = false;
  PinConstants enables_inactive_at;
};

// The next state for every value of the inputs and of the state.
class NextStateTable
{
 public:
  NextStateTable(const sta::FuncExpr* next,
                 const sta::LibertyPort* state,
                 const sta::LibertyPort* state_inv,
                 std::vector<const sta::LibertyPort*> inputs);

  // Whenever input i is v, the next state is the state.
  bool holdsStateWhen(size_t i, bool v) const;
  // Whenever no enable is at its inactive value, the next state is input d.
  bool passesWhenEnabled(size_t d, const PinConstants& enables) const;

 private:
  bool stateBit(const uint32_t row) const
  {
    return rowBit(row, inputs_.size());
  }
  bool enablesActive(uint32_t row, const PinConstants& enables) const;

  std::vector<const sta::LibertyPort*> inputs_;
  // By row: bit i of a row is input i, and the bit after the inputs is the
  // state.
  std::vector<bool> next_;
};

NextStateTable::NextStateTable(const sta::FuncExpr* next,
                               const sta::LibertyPort* state,
                               const sta::LibertyPort* state_inv,
                               std::vector<const sta::LibertyPort*> inputs)
    : inputs_(std::move(inputs))
{
  const uint32_t rows = 1u << (inputs_.size() + 1);
  next_.reserve(rows);
  for (uint32_t row = 0; row < rows; ++row) {
    Assignment values = assignment(inputs_, row);
    values[state] = stateBit(row);
    next_.push_back(evalExpr(next, values, state, state_inv));
  }
}

bool NextStateTable::holdsStateWhen(const size_t i, const bool v) const
{
  for (uint32_t row = 0; row < next_.size(); ++row) {
    if (rowBit(row, i) == v && next_[row] != stateBit(row)) {
      return false;
    }
  }
  return true;
}

bool NextStateTable::passesWhenEnabled(const size_t d,
                                       const PinConstants& enables) const
{
  for (uint32_t row = 0; row < next_.size(); ++row) {
    if (enablesActive(row, enables) && next_[row] != rowBit(row, d)) {
      return false;
    }
  }
  return true;
}

bool NextStateTable::enablesActive(const uint32_t row,
                                   const PinConstants& enables) const
{
  for (size_t i = 0; i < inputs_.size(); ++i) {
    for (const auto& [port, inactive] : enables) {
      if (port == inputs_[i] && rowBit(row, i) == inactive) {
        return false;
      }
    }
  }
  return true;
}

// The inputs the next state reads besides the state.  Empty when it reads a
// pin that is not an input, no input, or more inputs than a truth table
// takes.
std::vector<const sta::LibertyPort*> nextStateInputs(
    const sta::FuncExpr* next,
    const sta::LibertyPort* state,
    const sta::LibertyPort* state_inv)
{
  std::vector<const sta::LibertyPort*> inputs;
  for (const sta::LibertyPort* port : exprPorts({next})) {
    if (port != state && port != state_inv) {
      if (!port->direction()->isInput()) {
        return {};
      }
      inputs.push_back(port);
    }
  }
  if (inputs.size() > kMaxTruthTableInputs) {
    return {};
  }
  return inputs;
}

// Inputs that hold the state at one value are enables; the one other input
// is the data, passed while no enable is inactive.  kOther without an enable
// or without such a data input.
NextStateShape enableShape(const NextStateTable& table,
                           const std::vector<const sta::LibertyPort*>& inputs)
{
  NextStateShape shape;
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
    return {};
  }
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (!is_enable[i]) {
      if (shape.data != nullptr
          || !table.passesWhenEnabled(i, shape.enables_inactive_at)) {
        return {};
      }
      shape.data = inputs[i];
    }
  }
  if (shape.data == nullptr) {
    return {};
  }
  shape.kind = RegisterKind::kEnable;
  return shape;
}

NextStateShape nextStateShape(const sta::Sequential* seq)
{
  const sta::FuncExpr* next = seq->data();
  const std::optional<Literal> data = literal(next);
  if (data.has_value()) {
    NextStateShape shape;
    shape.kind = RegisterKind::kPlain;
    shape.data = data->port;
    shape.data_inverted = data->inverted;
    return shape;
  }
  const sta::LibertyPort* state = seq->output();
  const sta::LibertyPort* state_inv = seq->outputInv();
  if (next == nullptr || state == nullptr) {
    return {};
  }
  const std::vector<const sta::LibertyPort*> inputs
      = nextStateInputs(next, state, state_inv);
  if (inputs.empty()) {
    return {};
  }
  return enableShape(NextStateTable(next, state, state_inv, inputs), inputs);
}

struct LoadedOutput
{
  odb::dbITerm* pin = nullptr;
  bool inverted = false;  // reads the state inverted
};

// The one output with loads; without loads, the first output that reads the
// state as it is (pin: nullptr without one).  nullopt when two outputs have
// loads or the loaded one does not read the state.
std::optional<LoadedOutput> loadedOutput(sta::dbSta* sta,
                                         odb::dbInst* inst,
                                         const sta::Sequential* seq)
{
  LoadedOutput loaded;
  int loaded_count = 0;
  odb::dbITerm* state_pin = nullptr;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
      continue;
    }
    const std::optional<bool> inverted = outputInverted(sta, iterm, seq);
    if (inverted == false && state_pin == nullptr) {
      state_pin = iterm;
    }
    odb::dbNet* net = iterm->getNet();
    if (net == nullptr || !isRead(net)) {
      continue;
    }
    ++loaded_count;
    if (!inverted.has_value()) {
      return std::nullopt;
    }
    loaded = {.pin = iterm, .inverted = *inverted};
  }
  if (loaded_count > 1) {
    return std::nullopt;
  }
  if (loaded_count == 0) {
    return LoadedOutput{.pin = state_pin, .inverted = false};
  }
  return loaded;
}

// The output pin of a tie cell that nothing reads anymore.
odb::dbITerm* idleTieOutput(sta::dbSta* sta, odb::dbNet* net)
{
  if (net->getSigType().isSupply() || isRead(net)
      || !constNetValue(sta, net).has_value()) {
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

bool drivesKeptNet(odb::dbITerm* pin)
{
  odb::dbNet* net = pin->getNet();
  return drivesNet(pin)
         && (net->isDoNotTouch() || net->getNonDefaultRule() != nullptr);
}

bool isHeld(odb::dbInst* inst)
{
  if (inst->isDoNotTouch() || inst->isFixed() || isGrouped(inst)
      || inst->getScanInst() != nullptr) {
    return true;
  }
  return std::ranges::any_of(inst->getITerms(), [](odb::dbITerm* pin) {
    odb::dbNet* net = pin->getNet();
    return (net != nullptr && net->isDoNotTouch()) || drivesKeptNet(pin);
  });
}

bool hasPinConstraint(sta::dbSta* sta, odb::dbInst* inst)
{
  sta::dbNetwork* network = sta->getDbNetwork();
  for (const sta::Sdc* sdc : allSdcs(sta)) {
    for (odb::dbITerm* iterm : inst->getITerms()) {
      odb::dbNet* net = iterm->getNet();
      if (sdc->isConstrained(network->dbToSta(iterm))
          || (drivesNet(iterm) && sdc->isConstrained(network->dbToSta(net)))) {
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
  const auto any_disabled = [&](sta::VertexEdgeIterator& edges) {
    while (edges.hasNext()) {
      if (disabled(edges.next())) {
        return true;
      }
    }
    return false;
  };
  for (odb::dbITerm* iterm : inst->getITerms()) {
    sta::Vertex* vertex;
    sta::Vertex* bidirect_driver;
    graph->pinVertices(network->dbToSta(iterm), vertex, bidirect_driver);
    for (sta::Vertex* end : {vertex, bidirect_driver}) {
      if (end == nullptr) {
        continue;
      }
      sta::VertexInEdgeIterator in_edges(end, graph);
      sta::VertexOutEdgeIterator out_edges(end, graph);
      if (any_disabled(in_edges) || any_disabled(out_edges)) {
        return true;
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

bool hasScanPin(odb::dbMaster* master)
{
  return std::ranges::any_of(master->getMTerms(), [](odb::dbMTerm* mterm) {
    return mterm->getSigType() == odb::dbSigType::SCAN;
  });
}

bool isScanCell(sta::dbSta* sta, odb::dbInst* inst)
{
  return hasScanPin(inst->getMaster()) || sta->getDbNetwork()->isScanCell(inst);
}

namespace {

bool hasOpenInput(odb::dbInst* inst)
{
  return std::ranges::any_of(inst->getITerms(), [](odb::dbITerm* iterm) {
    return iterm->isInputSignal(/*io=*/false) && iterm->getNet() == nullptr;
  });
}

// An input pin that is neither a supply nor marked as a clock.
bool isLogicInput(odb::dbITerm* pin)
{
  return pin->isInputSignal(/*io=*/false)
         && pin->getSigType() != odb::dbSigType::CLOCK;
}

// An output of the cell does not always drive (three_state).
bool hasTristateOutput(const sta::LibertyCell* cell)
{
  return std::ranges::any_of(libertyPorts(cell),
                             [](const sta::LibertyPort* port) {
                               return port->direction()->isAnyOutput()
                                      && port->tristateEnable() != nullptr;
                             });
}

bool sameDirection(odb::dbMTerm* pin, const sta::LibertyPort* port)
{
  const sta::PortDirection* direction = port->direction();
  switch (pin->getIoType().getValue()) {
    case odb::dbIoType::INPUT:
      return direction->isInput();
    case odb::dbIoType::OUTPUT:
      return direction->isOutput() || direction->isTristate();
    case odb::dbIoType::INOUT:
      return direction->isBidirect();
    case odb::dbIoType::FEEDTHRU:
      return false;
  }
  return false;
}

}  // namespace

bool hasLibertyPins(const sta::LibertyCell* cell, odb::dbMaster* master)
{
  int pins = 0;
  for (odb::dbMTerm* pin : master->getMTerms()) {
    const sta::LibertyPort* port = cell->findLibertyPort(pin->getConstName());
    const bool supply
        = pin->getSigType().isSupply()
          || (port != nullptr && port->direction()->isPowerGround());
    if (supply) {
      continue;
    }
    if (port == nullptr || !sameDirection(pin, port)) {
      return false;
    }
    ++pins;
  }
  const std::ptrdiff_t ports = std::ranges::count_if(
      libertyPorts(cell), [](const sta::LibertyPort* port) {
        const sta::PortDirection* direction = port->direction();
        return !direction->isPowerGround() && !direction->isInternal();
      });
  return pins == ports;
}

bool isReplaceable(const sta::LibertyCell* cell,
                   const sta::Sequential* seq,
                   odb::dbMaster* master)
{
  return clockMTerm(seq, master) != nullptr && !hasTristateOutput(cell)
         && hasLibertyPins(cell, master);
}

UntouchedKind untouchedKind(sta::dbSta* sta, odb::dbInst* inst)
{
  const sta::Sequential* seq = singleSequential(sta, inst);
  if (seq == nullptr) {
    return UntouchedKind::kOther;
  }
  if (!seq->isRegister()) {
    return UntouchedKind::kLatch;
  }
  const sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  if (!isReplaceable(cell, seq, inst->getMaster())) {
    return UntouchedKind::kOther;
  }
  if (hasLiveAsyncControl(sta, inst, seq)) {
    return UntouchedKind::kAsync;
  }
  return isScanCell(sta, inst) ? UntouchedKind::kScan : UntouchedKind::kNone;
}

bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst)
{
  // The cheap tests first: untouchedKind reads the asynchronous control nets.
  return isScanCell(sta, inst) || hasOpenInput(inst)
         || untouchedKind(sta, inst) != UntouchedKind::kNone;
}

bool hasUnreadInput(const sta::LibertyCell* cell, const sta::Sequential* seq)
{
  std::vector<const sta::LibertyPort*> read;
  if (seq != nullptr) {
    read = exprPorts({seq->data(), seq->clock(), seq->clear(), seq->preset()});
  }
  return std::ranges::any_of(
      libertyPorts(cell), [&](const sta::LibertyPort* port) {
        return port->direction()->isInput()
               && std::ranges::find(read, port) == read.end();
      });
}

std::optional<PinConstants> inactiveAsyncControls(const sta::LibertyCell* cell,
                                                  const sta::Sequential* seq)
{
  const std::vector<const sta::LibertyPort*> controls
      = exprPorts({seq->clear(), seq->preset()});
  const bool ties_live_pin
      = std::ranges::any_of(controls, [&](const sta::LibertyPort* port) {
          return reads(seq->data(), port) || reads(seq->clock(), port);
        });
  if (controls.empty() || controls.size() > kMaxTruthTableInputs
      || ties_live_pin || hasUnreadInput(cell, seq)) {
    return std::nullopt;
  }

  const uint32_t rows = 1u << controls.size();
  for (uint32_t row = 0; row < rows; ++row) {
    const Assignment values = assignment(controls, row);
    bool active = false;
    for (const sta::FuncExpr* control : {seq->clear(), seq->preset()}) {
      active = active || (control != nullptr && evalExpr(control, values));
    }
    if (!active) {
      PinConstants constants;
      for (const sta::LibertyPort* port : controls) {
        constants.push_back({.port = port, .high = values.at(port)});
      }
      return constants;
    }
  }
  return std::nullopt;
}

namespace {

// Output nets that hold only the register's own pin.
std::vector<std::pair<odb::dbNet*, odb::dbModNet*>> unloadedOutputNets(
    odb::dbInst* inst)
{
  std::vector<std::pair<odb::dbNet*, odb::dbModNet*>> nets;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    odb::dbNet* net = iterm->getNet();
    if (drivesNet(iterm) && !isRead(net) && !net->isDoNotTouch()) {
      nets.emplace_back(net, iterm->getModNet());
    }
  }
  return nets;
}

// The nets on the register's asynchronous control pins, in control order.
// seq: nullptr gives none.
std::vector<odb::dbNet*> asyncControlNets(odb::dbInst* inst,
                                          const sta::Sequential* seq)
{
  std::vector<odb::dbNet*> nets;
  if (seq == nullptr) {
    return nets;
  }
  const std::vector<const sta::LibertyPort*> controls
      = exprPorts({seq->clear(), seq->preset()});
  for (const sta::LibertyPort* port : controls) {
    odb::dbITerm* iterm = findITerm(inst, port);
    odb::dbNet* net = iterm != nullptr ? iterm->getNet() : nullptr;
    if (net != nullptr && std::ranges::find(nets, net) == nets.end()) {
      nets.push_back(net);
    }
  }
  return nets;
}

// Discards the tie cells on these nets that nothing reads anymore, each with
// its net.  module: the one a tie cell must be in; nullptr: any.
void discardIdleTies(sta::dbSta* sta,
                     const std::vector<odb::dbNet*>& nets,
                     odb::dbModule* module,
                     Discards& discards)
{
  for (odb::dbNet* net : nets) {
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
}

}  // namespace

void discardRegister(sta::dbSta* sta, odb::dbInst* inst, Discards& discards)
{
  // Read before discards.add() disconnects the register's pins.
  const std::vector<odb::dbNet*> control_nets
      = asyncControlNets(inst, singleSequential(sta, inst));
  const std::vector<std::pair<odb::dbNet*, odb::dbModNet*>> unloaded
      = unloadedOutputNets(inst);
  odb::dbModule* module
      = sta->getDbNetwork()->hasHierarchy() ? inst->getModule() : nullptr;
  discards.add(inst);
  discardIdleTies(sta, control_nets, module, discards);
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
  const sta::LibertyPort* port = libertyPort(sta, pin);
  return port != nullptr && reads(seq->data(), port);
}

odb::dbITerm* registerDataPin(sta::dbSta* sta, odb::dbInst* inst)
{
  const sta::Sequential* seq = singleSequential(sta, inst);
  if (seq != nullptr && seq->isRegister()) {
    odb::dbITerm* data = findITerm(inst, nextStateShape(seq).data);
    if (data != nullptr) {
      return data;
    }
  }
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (isLogicInput(iterm) && isRegisterDataPin(sta, iterm)) {
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

odb::dbITerm* registerClockPin(sta::dbSta* sta, odb::dbInst* inst)
{
  const sta::Sequential* seq = singleSequential(sta, inst);
  odb::dbMTerm* clock
      = seq != nullptr ? clockMTerm(seq, inst->getMaster()) : nullptr;
  return clock != nullptr ? inst->getITerm(clock) : nullptr;
}

odb::dbNet* registerClockNet(sta::dbSta* sta, odb::dbInst* inst)
{
  odb::dbITerm* clock = registerClockPin(sta, inst);
  return clock != nullptr ? clock->getNet() : nullptr;
}

namespace {

// The enable pins with their nets, by pin name; nullopt when the cell lacks
// one of them.
std::optional<std::vector<EnablePin>> enablePins(
    odb::dbInst* inst,
    const PinConstants& enables_inactive_at)
{
  std::vector<EnablePin> enables;
  for (const auto& [port, inactive] : enables_inactive_at) {
    odb::dbITerm* iterm = findITerm(inst, port);
    if (iterm == nullptr) {
      return std::nullopt;
    }
    enables.push_back({.pin = iterm->getMTerm(), .net = iterm->getNet()});
  }
  std::ranges::sort(enables, pinNameLess, &EnablePin::pin);
  return enables;
}

// Every input besides the clock and the data pin is an enable.
bool hasOnlyEnableInputs(odb::dbInst* inst,
                         odb::dbMTerm* data,
                         const std::vector<EnablePin>& enables)
{
  return std::ranges::all_of(inst->getITerms(), [&](odb::dbITerm* iterm) {
    return !isLogicInput(iterm) || iterm->getMTerm() == data
           || std::ranges::find(enables, iterm->getMTerm(), &EnablePin::pin)
                  != enables.end();
  });
}

void classifyEnableRegister(odb::dbInst* inst,
                            const NextStateShape& shape,
                            RegisterClass& cls)
{
  std::optional<std::vector<EnablePin>> enables
      = enablePins(inst, shape.enables_inactive_at);
  if (!enables.has_value() || !hasOnlyEnableInputs(inst, cls.data, *enables)
      || cls.output == nullptr || cls.clock == nullptr) {
    return;
  }
  cls.kind = RegisterKind::kEnable;
  cls.master = inst->getMaster();
  cls.enables = std::move(*enables);
}

}  // namespace

RegisterClass registerClass(sta::dbSta* sta,
                            odb::dbInst* inst,
                            rsz::Resizer* resizer)
{
  RegisterClass cls;
  cls.clock = registerClockNet(sta, inst);
  cls.supply = supplyPins(inst);
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

  const std::optional<LoadedOutput> output = loadedOutput(sta, inst, seq);
  if (!output.has_value()) {
    return cls;
  }
  cls.output = output->pin != nullptr ? output->pin->getMTerm() : nullptr;
  // A cell with asynchronous controls, one that is not usable, or one with
  // an input its sequential does not read is never the cell of a new
  // register, so it does not keep its class.
  const sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  const bool keeps_cell = !hasAsyncControl(seq) && isUsableCell(cell, resizer)
                          && !hasUnreadInput(cell, seq);
  if (shape.kind == RegisterKind::kPlain) {
    const bool loads_see_complement = shape.data_inverted != output->inverted;
    if (!loads_see_complement) {
      cls.kind = RegisterKind::kPlain;
    } else if (keeps_cell && cls.output != nullptr && cls.clock != nullptr) {
      cls.kind = RegisterKind::kInverting;
      cls.master = inst->getMaster();
    }
    return cls;
  }
  if (!output->inverted && keeps_cell) {
    classifyEnableRegister(inst, shape, cls);
  }
  return cls;
}

bool keepsClass(const RegisterClass& cls)
{
  return cls.kind == RegisterKind::kEnable
         || cls.kind == RegisterKind::kInverting;
}

bool sameDomain(const RegisterClass& a, const RegisterClass& b)
{
  return a.clock == b.clock && sameSupply(a.supply, b.supply);
}

bool sameRegisterClass(const RegisterClass& a, const RegisterClass& b)
{
  if (a.kind != b.kind || !sameDomain(a, b)) {
    return false;
  }
  switch (a.kind) {
    case RegisterKind::kPlain:
      return true;
    case RegisterKind::kEnable:
      return a.master == b.master && a.output == b.output
             && a.enables == b.enables;
    case RegisterKind::kInverting:
      return a.master == b.master && a.output == b.output;
    case RegisterKind::kOther:
      return false;
  }
  return false;
}

bool isCombinationalGate(sta::dbSta* sta, odb::dbInst* inst)
{
  const sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  if (cell == nullptr || cell->isSequential() || isSpecialCell(cell)
      || hasTristateOutput(cell)) {
    return false;
  }
  return std::ranges::none_of(
      libertyPorts(cell), [](const sta::LibertyPort* port) {
        return port->direction()->isAnyOutput() && port->function() == nullptr;
      });
}

bool isSelfDualGate(sta::dbSta* sta, odb::dbInst* inst)
{
  const sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  if (cell == nullptr || cell->isSequential()) {
    return false;
  }
  int outputs = 0;
  for (const sta::LibertyPort* port : libertyPorts(cell)) {
    if (!port->direction()->isOutput()) {
      continue;
    }
    const sta::FuncExpr* func = port->function();
    if (func == nullptr) {
      return false;
    }
    const std::vector<const sta::LibertyPort*> inputs = exprPorts({func});
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
