// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/LibCellTable.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
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
#include "sta/PortDirection.hh"
#include "sta/Sequential.hh"
#include "syn/lir/Checksum.h"
#include "syn/lir/Ids.h"

namespace syn::lir {
namespace {

void appendPortBits(sta::LibertyPort* port, std::vector<sta::LibertyPort*>& out)
{
  if (!port->isBus()) {
    out.push_back(port);
    return;
  }
  const size_t begin = out.size();
  sta::LibertyPortMemberIterator members(port);
  while (members.hasNext()) {
    out.push_back(members.next());
  }
  // Buses are expanded LSB-first.
  if (port->fromIndex() > port->toIndex()) {
    std::reverse(out.begin() + static_cast<std::ptrdiff_t>(begin), out.end());
  }
}

int findPortByPtr(std::span<sta::LibertyPort* const> ports,
                  const sta::LibertyPort* target)
{
  for (size_t i = 0; i < ports.size(); ++i) {
    if (ports[i] == target) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int findPortByName(std::span<sta::LibertyPort* const> ports,
                   std::string_view name)
{
  for (size_t i = 0; i < ports.size(); ++i) {
    if (ports[i]->name() == name) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

// True if output `port` depends only on the cell's state. Tristate outputs
// and outputs without a known function are false.
bool dependsOnlyOnState(const sta::LibertyCell* cell,
                        const sta::LibertyPort* port)
{
  if (port->tristateEnable() != nullptr) {
    return false;
  }
  for (const sta::Sequential& seq : cell->sequentials()) {
    if (seq.output() == port || seq.outputInv() == port) {
      return true;
    }
  }
  const sta::FuncExpr* function = port->function();
  if (function == nullptr) {
    return false;
  }
  for (const sta::LibertyPort* read : function->ports()) {
    if (!read->direction()->isInternal()) {
      return false;
    }
  }
  return true;
}

LibCellKind classify(const sta::LibertyCell* cell,
                     std::span<sta::LibertyPort* const> outputs)
{
  if (cell->isClockGate()) {
    return LibCellKind::kClockGate;
  }
  if (!cell->isSequential()) {
    return LibCellKind::kCombinational;
  }
  if (outputs.empty()) {
    return LibCellKind::kBlackBox;
  }
  for (const sta::LibertyPort* port : outputs) {
    if (!dependsOnlyOnState(cell, port)) {
      return LibCellKind::kBlackBox;
    }
  }
  return LibCellKind::kRegister;
}

}  // namespace

const char* libCellKindName(LibCellKind kind)
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
  sta::LibertyCellPortIterator ports(cell);
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
    // Bundles too, though appendPortBits expands only buses.
    has_bus_pins_ |= port->hasMembers();
    if (direction->isInput()) {
      appendPortBits(port, inputs_);
    } else if (direction->isOutput() || direction->isTristate()) {
      has_tristate_outputs_
          |= direction->isTristate() || port->tristateEnable() != nullptr;
      appendPortBits(port, outputs_);
    }
  }
  kind_ = classify(cell_, outputs_);
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
  return cell_->isSequential();
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
  absl::MutexLock lock(&mutex_);
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
