// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/LibCellTable.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>  // IWYU pragma: keep (used in asserts)
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/synchronization/mutex.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Sequential.hh"
#include "syn/lir/Checksum.h"
#include "syn/lir/Ids.h"

namespace syn::lir {
namespace {

// Appends the bit ports of top-level port `port`. A bus expands to its bits,
// LSB-first. A bundle adds nothing: OpenSTA lists its members as top-level
// ports too, in declaration order.
void appendPortBits(sta::LibertyPort* port, std::vector<sta::LibertyPort*>& out)
{
  if (port->isBundle()) {
    return;
  }
  if (!port->isBus()) {
    out.push_back(port);
    return;
  }
  const std::ptrdiff_t begin = std::ssize(out);
  sta::LibertyPortMemberIterator members(port);
  while (members.hasNext()) {
    out.push_back(members.next());
  }
  // Buses are expanded LSB-first.
  if (port->fromIndex() > port->toIndex()) {
    std::reverse(out.begin() + begin, out.end());
  }
}

int findPortByPtr(std::span<sta::LibertyPort* const> ports,
                  const sta::LibertyPort* target)
{
  const int n = static_cast<int>(ports.size());
  for (int i = 0; i < n; ++i) {
    if (ports[i] == target) {
      return i;
    }
  }
  return -1;
}

int findPortByName(std::span<sta::LibertyPort* const> ports,
                   std::string_view name)
{
  const int n = static_cast<int>(ports.size());
  for (int i = 0; i < n; ++i) {
    if (ports[i]->name() == name) {
      return i;
    }
  }
  return -1;
}

// True if `port` is a state variable of one of `cell`'s ff or latch groups
// (e.g. IQ or IQN of ff (IQ, IQN)).
bool isStatePort(const sta::LibertyCell* cell, const sta::LibertyPort* port)
{
  for (const sta::Sequential& seq : cell->sequentials()) {
    if (seq.output() == port || seq.outputInv() == port) {
      return true;
    }
  }
  return false;
}

// True if output `port` depends only on the state of `cell`'s ff or latch:
// it is a state variable, or its function reads state variables only.
// Tristate outputs (by direction or by three_state expression) and outputs
// without a known function are false.
bool dependsOnlyOnState(const sta::LibertyCell* cell,
                        const sta::LibertyPort* port)
{
  if (port->direction()->isTristate() || port->tristateEnable() != nullptr) {
    return false;
  }
  if (isStatePort(cell, port)) {
    return true;
  }
  const sta::FuncExpr* function = port->function();
  if (function == nullptr) {
    return false;
  }
  const sta::LibertyPortSet reads = function->ports();
  if (reads.empty()) {
    return false;  // a constant
  }
  for (const sta::LibertyPort* read : reads) {
    if (!isStatePort(cell, read)) {
      return false;
    }
  }
  return true;
}

bool isSupportedFuncExpr(const sta::FuncExpr* expr)
{
  switch (expr->op()) {
    case sta::FuncExpr::Op::port:
      return expr->port() != nullptr;
    case sta::FuncExpr::Op::not_:
      return expr->left() != nullptr && isSupportedFuncExpr(expr->left());
    case sta::FuncExpr::Op::and_:
    case sta::FuncExpr::Op::or_:
    case sta::FuncExpr::Op::xor_:
      return expr->left() != nullptr && expr->right() != nullptr
             && isSupportedFuncExpr(expr->left())
             && isSupportedFuncExpr(expr->right());
    case sta::FuncExpr::Op::one:
    case sta::FuncExpr::Op::zero:
      return true;
  }
  return false;
}

// True if every output has a known function that reads only the cell's
// inputs (a combinational cell can then be a kCell). An output reading
// another output or an internal pin is false.
bool allOutputsAreInputFunctions(std::span<sta::LibertyPort* const> inputs,
                                 std::span<sta::LibertyPort* const> outputs)
{
  for (const sta::LibertyPort* port : outputs) {
    const sta::FuncExpr* function = port->function();
    if (function == nullptr || !isSupportedFuncExpr(function)) {
      return false;
    }
    for (const sta::LibertyPort* read : function->ports()) {
      if (findPortByPtr(inputs, read) < 0) {
        return false;
      }
    }
  }
  return true;
}

// True for pads, hard macros, memories and interface-timing models: never
// logic to restructure, whatever their functions say (as in rsz and cut).
bool isOpaqueBlock(const sta::LibertyCell* cell)
{
  return cell->isPad() || cell->isMacro() || cell->isMemory()
         || cell->interfaceTiming();
}

// True if `cell`'s ff, latch or statetable is only in its test_cell, which
// OpenSTA keeps as a separate cell.
bool hasOnlyTestCellState(const sta::LibertyCell* cell)
{
  return !cell->isSequential() && cell->testCell() != nullptr
         && cell->testCell()->isSequential();
}

LibCellKind classify(const sta::LibertyCell* cell,
                     std::span<sta::LibertyPort* const> inputs,
                     std::span<sta::LibertyPort* const> outputs)
{
  if (outputs.empty() || isOpaqueBlock(cell)) {
    return LibCellKind::kBlackBox;
  }
  if (cell->isClockGate()) {
    return LibCellKind::kClockGate;
  }
  // The test cell's ports are separate objects, not tied to the cell's
  // outputs yet: a black box for now.
  if (hasOnlyTestCellState(cell)) {
    return LibCellKind::kBlackBox;
  }
  // Other statetable cells (synchronizers, retention flops, ...) have no ff
  // or latch and OpenSTA does not read their state_function: black boxes.
  if (cell->statetable() != nullptr) {
    return LibCellKind::kBlackBox;
  }
  if (!cell->isSequential()) {
    // A clock-to-output arc without an ff or latch is state OpenSTA did not
    // model.
    if (cell->hasInferedRegTimingArcs()) {
      return LibCellKind::kBlackBox;
    }
    return allOutputsAreInputFunctions(inputs, outputs)
               ? LibCellKind::kCombinational
               : LibCellKind::kBlackBox;
  }
  for (const sta::LibertyPort* port : outputs) {
    if (!dependsOnlyOnState(cell, port)) {
      return LibCellKind::kBlackBox;
    }
  }
  return LibCellKind::kRegister;
}

// Backup rails (retention, always-on) and internal rails (the switched
// supply of a power switch). Primary rails and wells do not count: most
// cells have well pins.
bool isBackupOrSwitchedRail(const sta::LibertyPort* port)
{
  const sta::PwrGndType pg = port->pwrGndType();
  return pg == sta::PwrGndType::backup_power
         || pg == sta::PwrGndType::backup_ground
         || pg == sta::PwrGndType::internal_power
         || pg == sta::PwrGndType::internal_ground;
}

bool hasPowerPortFlag(const sta::LibertyPort* port)
{
  return port->isolationCellData() || port->isolationCellEnable()
         || port->levelShifterData() || port->isSwitch();
}

bool hasPowerPinFlag(const sta::LibertyPort* port)
{
  if (hasPowerPortFlag(port)) {
    return true;
  }
  if (!port->isBus()) {
    return false;
  }
  sta::LibertyPortMemberIterator members(port);
  while (members.hasNext()) {
    if (hasPowerPortFlag(members.next())) {
      return true;
    }
  }
  return false;
}

bool isPowerSensitiveCell(const sta::LibertyCell* cell)
{
  // switchCellType() is fine_grain by default on every cell; only
  // coarse_grain comes from the library.
  if (cell->isIsolationCell() || cell->isLevelShifter() || cell->alwaysOn()
      || cell->switchCellType() == sta::SwitchCellType::coarse_grain) {
    return true;
  }
  sta::LibertyCellPortIterator ports(cell);
  while (ports.hasNext()) {
    const sta::LibertyPort* port = ports.next();
    if (port->isPwrGnd()) {
      if (isBackupOrSwitchedRail(port)) {
        return true;
      }
      continue;
    }
    // Internal pins included: a switch_pin may be internal.
    if (hasPowerPinFlag(port)) {
      return true;
    }
  }
  return false;
}

bool isModeledRegisterCell(const LibCellInfo& info)
{
  if (info.kind() != LibCellKind::kRegister || info.hasTristateOutputs()
      || !info.checkSupported().ok()) {
    return false;
  }
  const sta::LibertyCell* cell = info.cell();
  if (cell->sequentials().size() != 1) {
    return false;
  }
  const sta::Sequential& seq = cell->sequentials()[0];
  if (seq.output() == nullptr || seq.data() == nullptr
      || (seq.isLatch() && seq.clock() == nullptr)) {
    return false;
  }
  const std::array<const sta::FuncExpr*, 4> exprs
      = {seq.clock(), seq.data(), seq.clear(), seq.preset()};
  for (const sta::LibertyPort* input : info.inputs()) {
    // A pulse latch or pulse flop expects a pulse, not a level or an edge.
    if (input->pulseClkTrigger() != nullptr) {
      return false;
    }
    const bool used
        = std::ranges::any_of(exprs, [input](const sta::FuncExpr* expr) {
            return expr != nullptr && expr->hasPort(input);
          });
    if (!used) {
      return false;
    }
  }
  for (const sta::FuncExpr* expr : exprs) {
    if (expr == nullptr) {
      continue;
    }
    if (!isSupportedFuncExpr(expr)) {
      return false;
    }
    // Only next_state may read the state: a clock, clear or preset reading
    // it would let the register drive its own control.
    const bool may_read_state = expr == seq.data();
    for (const sta::LibertyPort* read : expr->ports()) {
      const bool is_state = read == seq.output() || read == seq.outputInv();
      if (is_state ? !may_read_state : info.findInput(read) < 0) {
        return false;
      }
    }
  }
  for (const sta::LibertyPort* out : info.outputs()) {
    if (out != seq.output() && out != seq.outputInv()
        && (out->function() == nullptr
            || !isSupportedFuncExpr(out->function()))) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::string_view libCellKindName(LibCellKind kind)
{
  switch (kind) {
    case LibCellKind::kCombinational:
      return "combinational";
    case LibCellKind::kClockGate:
      return "clock_gate";
    case LibCellKind::kRegister:
      return "register";
    case LibCellKind::kBlackBox:
      return "black_box";
  }
  return "?";
}

LibCellInfo::LibCellInfo(sta::LibertyCell* cell) : cell_(cell)
{
  assert(cell != nullptr);
  collectPins();
  kind_ = classify(cell_, inputs_, outputs_);
  is_power_sensitive_ = isPowerSensitiveCell(cell_);
  is_modeled_register_ = isModeledRegisterCell(*this);
}

void LibCellInfo::collectPins()
{
  sta::LibertyCellPortIterator ports(cell_);
  while (ports.hasNext()) {
    sta::LibertyPort* port = ports.next();
    if (port->isPwrGnd()) {
      continue;
    }
    const sta::PortDirection* direction = port->direction();
    if (direction->isInternal()) {
      continue;
    }
    if (direction->isBidirect()) {
      has_bidirectional_pins_ = true;
      continue;
    }
    if (direction->isUnknown()) {
      has_unknown_direction_pins_ = true;
      continue;
    }
    has_bus_pins_ |= port->hasMembers();  // bus or bundle
    if (direction->isInput()) {
      appendPortBits(port, inputs_);
    } else if (direction->isOutput() || direction->isTristate()) {
      has_tristate_outputs_
          |= direction->isTristate() || port->tristateEnable() != nullptr;
      appendPortBits(port, outputs_);
    }
  }
  assert(inputs_.size()
         <= static_cast<size_t>(std::numeric_limits<int>::max()));
  assert(outputs_.size()
         <= static_cast<size_t>(std::numeric_limits<int>::max()));
}

absl::Status LibCellInfo::checkSupported() const
{
  if (has_bus_pins_) {
    return absl::UnimplementedError(
        absl::StrCat("Liberty cell ",
                     name(),
                     " has bus or bundle pins, which are not supported yet"));
  }
  if (has_bidirectional_pins_) {
    return absl::UnimplementedError(
        absl::StrCat("Liberty cell ",
                     name(),
                     " has bidirectional pins, which are not supported yet"));
  }
  if (has_unknown_direction_pins_) {
    return absl::UnimplementedError(
        absl::StrCat("Liberty cell ",
                     name(),
                     " has pins without a direction, which are not supported "
                     "yet"));
  }
  if (has_tristate_outputs_
      && (kind_ == LibCellKind::kCombinational
          || kind_ == LibCellKind::kClockGate)) {
    return absl::UnimplementedError(
        absl::StrCat("Liberty cell ",
                     name(),
                     " has a tristate output, which is not supported yet"));
  }
  return absl::OkStatus();
}

std::string_view LibCellInfo::name() const
{
  return cell_->name();
}

bool LibCellInfo::isSequential() const
{
  return cell_->isSequential() || hasOnlyTestCellState(cell_);
}

int LibCellInfo::findInput(const sta::LibertyPort* port) const
{
  return findPortByPtr(inputs_, port);
}

int LibCellInfo::findInput(std::string_view name) const
{
  return findPortByName(inputs_, name);
}

int LibCellInfo::findOutput(const sta::LibertyPort* port) const
{
  return findPortByPtr(outputs_, port);
}

int LibCellInfo::findOutput(std::string_view name) const
{
  return findPortByName(outputs_, name);
}

uint64_t LibCellInfo::checksum() const
{
  uint64_t h = checksumBytes(kChecksumSeed, name());
  h = checksumCombine(h, static_cast<uint64_t>(kind_));
  h = checksumCombine(h, inputs_.size());
  for (const sta::LibertyPort* port : inputs_) {
    h = checksumBytes(h, port->name());
  }
  h = checksumCombine(h, outputs_.size());
  for (const sta::LibertyPort* port : outputs_) {
    h = checksumBytes(h, port->name());
  }
  return h;
}

LibCellId LibCellTable::lookup(const sta::LibertyCell* cell) const
{
  const auto it = ids_.find(cell);
  return it != ids_.end() ? it->second : LibCellId();
}

const LibCellInfo& LibCellTable::infoAt(LibCellId id) const
{
  assert(id.isValid() && id.rawIndex() < cells_.size());
  return *cells_[id.rawIndex()];
}

size_t LibCellTable::cellCount() const
{
  return cells_.size();
}

LibCellId LibCellTable::internUnlocked(sta::LibertyCell* cell)
{
  const LibCellId id(static_cast<uint32_t>(cells_.size()));
  const auto [it, inserted] = ids_.try_emplace(cell, id);
  if (!inserted) {
    return it->second;
  }
  cells_.push_back(std::make_unique<LibCellInfo>(cell));
  return id;
}

LibCellId LibCellTable::intern(sta::LibertyCell* cell)
{
  // Without asserts, a null cell yields the invalid id.
  assert(cell != nullptr);
  if (cell == nullptr) {
    return LibCellId();
  }
  if (isReadOnly()) {
    const LibCellId found = lookup(cell);
    assert(found.isValid()
           && "LibCellTable::intern() cannot add a new cell in read-only "
              "mode");
    return found;
  }
  absl::MutexLockMaybe lock(lockIfParallel());
  return internUnlocked(cell);
}

LibCellId LibCellTable::find(const sta::LibertyCell* cell) const
{
  // Parallel-write mode would make the result depend on thread order.
  assert(!isParallelWrite()
         && "LibCellTable::find() cannot be called in parallel write mode");
  absl::MutexLockMaybe lock(lockIfParallel());
  return lookup(cell);
}

const LibCellInfo& LibCellTable::info(LibCellId id) const
{
  absl::MutexLockMaybe lock(lockIfParallel());
  return infoAt(id);
}

size_t LibCellTable::size() const
{
  absl::MutexLockMaybe lock(lockIfParallel());
  return cellCount();
}

size_t LibCellTable::memoryBytes() const
{
  absl::MutexLock lock(mutex_);
  size_t total
      = cells_.capacity() * sizeof(std::unique_ptr<LibCellInfo>)
        + ids_.capacity()
              * (sizeof(std::pair<const sta::LibertyCell*, LibCellId>) + 1);
  for (const auto& info : cells_) {
    total += sizeof(LibCellInfo) + info->memoryBytes();
  }
  return total;
}

}  // namespace syn::lir
