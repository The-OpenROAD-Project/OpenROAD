// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/Design.h"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>  // IWYU pragma: keep (used in asserts)
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/synchronization/mutex.h"
#include "syn/lir/Checksum.h"
#include "syn/lir/Ids.h"
#include "syn/lir/Module.h"
#include "syn/lir/Op.h"
#include "syn/lir/OperatorSpec.h"
#include "syn/lir/SourceLoc.h"
#include "syn/lir/WellFormed.h"
#include "utl/Logger.h"

namespace syn::lir {

Design::Design(const DesignOptions& options, utl::Logger* logger)
    : options_(options)
{
  setLogger(logger);
}

void Design::setMode(TableMode mode)
{
  setModulesMode(mode);
  setPoolsMode(mode);
}

void Design::setModulesMode(TableMode mode)
{
  modules_mode_.store(mode, std::memory_order_relaxed);
}

void Design::setPoolsMode(TableMode mode)
{
  names_.setMode(mode);
  consts_.setMode(mode);
  operators_.setMode(mode);
  cells_.setMode(mode);
}

absl::StatusOr<ModId> Design::addModuleUnlocked(Module&& module)
{
  assert(modules_.size()
         <= static_cast<size_t>(std::numeric_limits<int>::max()));
  const ModId id(static_cast<uint32_t>(modules_.size()));
  const NameId name = module.name();
  if (name.isValid() && !by_name_.emplace(name, id).second) {
    return absl::AlreadyExistsError(
        absl::StrCat("module '", names_.name(name), "' already exists"));
  }
  modules_.push_back(std::make_unique<Module>(std::move(module)));
  return id;
}

absl::StatusOr<ModId> Design::addModule(Module&& module)
{
  if (!isModulesWritable()) {
    return absl::FailedPreconditionError(
        "addModule() called while the design is read-only");
  }
  absl::MutexLockMaybe lock(lockModulesIfParallel());
  return addModuleUnlocked(std::move(module));
}

absl::Status Design::replaceModule(ModId id, Module&& module)
{
  if (id.value() >= static_cast<uint32_t>(numModules())) {
    return absl::NotFoundError(absl::StrCat("no module with id ", id.value()));
  }
  Module& target = this->module(id);
  if (target.isRemoved()) {
    return absl::FailedPreconditionError(
        absl::StrCat("replaceModule(): module ",
                     id.value(),
                     " is removed (its id stays reserved)"));
  }
  if (module.name() != target.name()) {
    return absl::InvalidArgumentError(
        "replaceModule() cannot change the module name");
  }
  // Caches key on the slot and its revision, so the revision must increase
  // whatever revision the incoming module carries.
  module.revision_ = target.revision() + 1;
  target = std::move(module);
  return absl::OkStatus();
}

absl::StatusOr<ModId> Design::copyModule(ModId src, NameId name)
{
  if (src.value() >= static_cast<uint32_t>(numModules())) {
    return absl::NotFoundError(absl::StrCat("no module with id ", src.value()));
  }
  if (!isModulesWritable()) {
    return absl::FailedPreconditionError(
        "copyModule() called while the design is read-only");
  }
  const Module& source = module(src);
  if (source.isRemoved()) {
    return absl::FailedPreconditionError(
        absl::StrCat("copyModule(): module ", src.value(), " is removed"));
  }
  Module copy = source.clone();
  copy.name_ = name;
  copy.revision_ = 0;
  if (copy.impl_.has_value()) {
    // The revision restarts, so the copy keeps the source's pristine state
    // through the modified bit.
    copy.impl_->modified = !source.isPristineOperator();
    copy.impl_->generated_revision = 0;
  }
  return addModule(std::move(copy));
}

absl::StatusOr<bool> Design::removeModuleUnlocked(ModId id)
{
  if (!id.isValid() || id.value() >= modules_.size()) {
    return absl::NotFoundError(absl::StrCat("no module with id ", id.value()));
  }
  Module& target = moduleAt(id);
  if (target.isRemoved()) {
    return absl::FailedPreconditionError(
        absl::StrCat("module ", id.value(), " is already removed"));
  }
  if (top_ == id) {
    return absl::FailedPreconditionError(
        absl::StrCat("removeModule(): module ",
                     id.value(),
                     " is the top; call clearTop() or setTop() first"));
  }
  const bool had_impl = target.impl_.has_value();
  // Dropping the cache entry writes the operator pool.
  if (had_impl && !operators_.isWritable()) {
    return absl::FailedPreconditionError(
        absl::StrCat("removeModule() of operator implementation ",
                     id.value(),
                     " called while the operator pool is read-only"));
  }
  if (target.name().isValid()) {
    by_name_.erase(target.name());
  }
  Module removed;
  removed.removed_ = true;
  removed.revision_ = target.revision() + 1;
  target = std::move(removed);
  return had_impl;
}

absl::Status Design::removeModule(ModId id)
{
  if (!isModulesWritable()) {
    return absl::FailedPreconditionError(
        "removeModule() called while the design is read-only");
  }
  absl::StatusOr<bool> had_impl;
  {
    absl::MutexLockMaybe lock(lockModulesIfParallel());
    had_impl = removeModuleUnlocked(id);
  }
  if (!had_impl.ok()) {
    return had_impl.status();
  }
  if (*had_impl) {
    operators_.uncacheModule(id);
  }
  return absl::OkStatus();
}

absl::Status Design::setModuleAttributes(ModId id,
                                         const ModuleAttributes& attributes)
{
  if (!isModulesWritable()) {
    return absl::FailedPreconditionError(
        "setModuleAttributes() called while the design is read-only");
  }
  if (!id.isValid() || id.value() >= static_cast<uint32_t>(numModules())) {
    return absl::NotFoundError(absl::StrCat("no module with id ", id.value()));
  }
  Module& target = module(id);
  if (target.isRemoved()) {
    return absl::FailedPreconditionError(
        absl::StrCat("module ", id.value(), " is removed"));
  }
  target.attributes_ = attributes;
  return absl::OkStatus();
}

std::vector<uint8_t> Design::computeFrozenModules() const
{
  const int n = numModules();
  const auto un = static_cast<uint32_t>(n);
  std::vector<uint8_t> frozen(n, 0);
  std::vector<std::vector<int>> children(n);
  std::vector<int> work;
  auto freeze = [&](int id) {
    if (frozen[id] == 0) {
      frozen[id] = 1;
      work.push_back(id);
    }
  };
  for (int i = 0; i < n; ++i) {
    const Module& m = module(ModId(static_cast<uint32_t>(i)));
    if (m.isRemoved()) {
      continue;
    }
    if (m.attributes().dont_touch) {
      freeze(i);
    }
    for (const ObjId obj : m.objects()) {
      if (m.isDead(obj) || m.op(obj) != Op::kInst
          || m.instModule(obj).value() >= un) {
        continue;
      }
      const int child = static_cast<int>(m.instModule(obj).value());
      children[i].push_back(child);
      if (m.objAttributes(obj).dontTouch()) {
        freeze(child);
      }
    }
  }
  while (!work.empty()) {
    const int id = work.back();
    work.pop_back();
    for (const int child : children[id]) {
      freeze(child);
    }
  }
  return frozen;
}

absl::Status Design::markFrozenModules()
{
  if (!isModulesWritable()) {
    return absl::FailedPreconditionError(
        "markFrozenModules() called while the design is read-only");
  }
  const std::vector<uint8_t> frozen = computeFrozenModules();
  const int n = static_cast<int>(frozen.size());
  for (int i = 0; i < n; ++i) {
    Module& m = module(ModId(static_cast<uint32_t>(i)));
    if (!m.isRemoved()) {
      m.attributes_.frozen_by_parent = frozen[i] != 0;
    }
  }
  return absl::OkStatus();
}

absl::Status Design::setTop(ModId id)
{
  if (!isModulesWritable()) {
    return absl::FailedPreconditionError(
        "setTop() called while the design is read-only");
  }
  absl::MutexLockMaybe lock(lockModulesIfParallel());
  if (!id.isValid() || id.value() >= static_cast<uint32_t>(moduleCount())) {
    return absl::InvalidArgumentError(
        absl::StrCat("setTop(): no module with id ", id.value()));
  }
  const Module& target = moduleAt(id);
  if (target.isRemoved()) {
    return absl::InvalidArgumentError(
        absl::StrCat("setTop(): module ", id.value(), " is removed"));
  }
  if (target.attributes().blackbox) {
    return absl::InvalidArgumentError(
        absl::StrCat("setTop(): module ", id.value(), " is a blackbox"));
  }
  // findOperatorModule() could hand the top out for instantiation.
  if (target.isOperatorImpl()) {
    return absl::InvalidArgumentError(
        absl::StrCat("setTop(): module ",
                     id.value(),
                     " is an operator implementation; call "
                     "clearOperatorImpl() first"));
  }
  storeTop(id);
  return absl::OkStatus();
}

void Design::clearTop()
{
  assert(isModulesWritable() && "Design::clearTop() needs write mode");
  absl::MutexLockMaybe lock(lockModulesIfParallel());
  storeTop(std::nullopt);
}

absl::StatusOr<ModId> Design::top() const
{
  // The computed root would depend on thread scheduling.
  assert(!isModulesParallelWrite()
         && "Design::top() cannot be called in parallel write mode");
  if (const std::optional<ModId> stored = storedTop(); stored.has_value()) {
    return *stored;
  }
  return computeTop();
}

absl::StatusOr<ModId> Design::computeTop() const
{
  const int n = numModules();
  const auto un = static_cast<uint32_t>(n);
  std::vector<uint8_t> instantiated(n, 0);
  for (int i = 0; i < n; ++i) {
    const Module& m = module(ModId(static_cast<uint32_t>(i)));
    if (m.isRemoved()) {
      continue;
    }
    for (const ObjId obj : m.objects()) {
      if (m.isDead(obj) || m.op(obj) != Op::kInst) {
        continue;
      }
      const uint32_t child = m.instModule(obj).value();
      if (child < un) {
        instantiated[child] = 1;
      }
    }
  }
  std::vector<ModId> candidates;
  for (int i = 0; i < n; ++i) {
    const auto ui = static_cast<uint32_t>(i);
    const Module& m = module(ModId(ui));
    if (!m.isRemoved() && instantiated[i] == 0 && !m.attributes().blackbox
        && !m.isOperatorImpl()) {
      candidates.emplace_back(ui);
    }
  }
  if (candidates.size() == 1) {
    return candidates[0];
  }
  if (candidates.empty()) {
    return absl::FailedPreconditionError(
        "the design has no top: every live module is instantiated, a "
        "blackbox or an operator implementation");
  }
  std::string list;
  for (const ModId id : candidates) {
    absl::StrAppend(&list, list.empty() ? "" : ", ");
    const NameId name = module(id).name();
    if (name.isValid()) {
      absl::StrAppend(&list, names_.name(name));
    } else {
      absl::StrAppend(&list, "module ", id.value());
    }
  }
  return absl::FailedPreconditionError(absl::StrCat(
      candidates.size(), " candidate tops: ", list, "; call setTop()"));
}

absl::Status Design::setModuleSourceLoc(ModId id, SourceLoc loc)
{
  if (!isModulesWritable()) {
    return absl::FailedPreconditionError(
        "setModuleSourceLoc() called while the design is read-only");
  }
  if (!id.isValid() || id.value() >= static_cast<uint32_t>(numModules())) {
    return absl::NotFoundError(absl::StrCat("no module with id ", id.value()));
  }
  Module& target = module(id);
  if (target.isRemoved()) {
    return absl::FailedPreconditionError(
        absl::StrCat("module ", id.value(), " is removed"));
  }
  target.source_loc_ = loc;
  return absl::OkStatus();
}

absl::Status Design::setOperatorImpl(ModId id, const OperatorKey& key)
{
  if (!isWritable()) {
    return absl::FailedPreconditionError(
        "setOperatorImpl() called while the design is read-only");
  }
  if (!id.isValid() || id.value() >= static_cast<uint32_t>(numModules())) {
    return absl::NotFoundError(absl::StrCat("no module with id ", id.value()));
  }
  Module& target = module(id);
  if (target.isRemoved()) {
    return absl::FailedPreconditionError(
        absl::StrCat("module ", id.value(), " is removed"));
  }
  if (!key.spec.isValid() || key.spec.rawIndex() >= operators_.size()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "operator spec ", key.spec.rawIndex(), " is not interned"));
  }
  if (absl::Status status
      = checkOperatorInterface(*this, target, operators_.get(key.spec));
      !status.ok()) {
    return status;
  }
  if (target.impl_.has_value()) {
    operators_.uncacheModule(id);
  }
  target.impl_ = OperatorImpl{
      .key = key,
      .generated_revision = target.revision(),
      .modified = false,
  };
  operators_.cacheModule(key, id);
  return absl::OkStatus();
}

absl::Status Design::clearOperatorImpl(ModId id)
{
  if (!isWritable()) {
    return absl::FailedPreconditionError(
        "clearOperatorImpl() called while the design is read-only");
  }
  if (!id.isValid() || id.value() >= static_cast<uint32_t>(numModules())) {
    return absl::NotFoundError(absl::StrCat("no module with id ", id.value()));
  }
  Module& target = module(id);
  if (target.isRemoved()) {
    return absl::FailedPreconditionError(
        absl::StrCat("module ", id.value(), " is removed"));
  }
  target.impl_.reset();
  operators_.uncacheModule(id);
  return absl::OkStatus();
}

ModId Design::findOperatorModule(const OperatorKey& key) const
{
  const ModId id = operators_.findModule(key);
  if (!id.isValid() || id.value() >= static_cast<uint32_t>(numModules())) {
    return ModId();
  }
  const Module& found = module(id);
  const std::optional<OperatorImpl>& impl = found.operatorImpl();
  if (found.isRemoved() || !found.isPristineOperator() || !impl.has_value()
      || impl->key != key) {
    return ModId();
  }
  return id;
}

Module& Design::moduleAt(ModId id) const
{
  assert(id.value() < modules_.size());
  return *modules_[id.value()];
}

ModId Design::lookup(NameId name) const
{
  const auto it = by_name_.find(name);
  return it != by_name_.end() ? it->second : ModId();
}

int Design::moduleCount() const
{
  return static_cast<int>(modules_.size());
}

std::optional<ModId> Design::storedTop() const
{
  absl::MutexLockMaybe lock(lockModulesIfParallel());
  return top_;
}

void Design::storeTop(std::optional<ModId> top)
{
  top_ = top;
}

Module& Design::module(ModId id)
{
  absl::MutexLockMaybe lock(lockModulesIfParallel());
  return moduleAt(id);
}

const Module& Design::module(ModId id) const
{
  absl::MutexLockMaybe lock(lockModulesIfParallel());
  return moduleAt(id);
}

ModId Design::findModule(NameId name) const
{
  // The result would depend on thread scheduling; locked so it cannot race.
  assert(!isModulesParallelWrite()
         && "Design::findModule() cannot be called in parallel write mode");
  absl::MutexLockMaybe lock(lockModulesIfParallel());
  return lookup(name);
}

ModId Design::findModule(std::string_view name) const
{
  assert(!isModulesParallelWrite()
         && "Design::findModule() cannot be called in parallel write mode");
  const NameId id = names_.find(name);
  return id.isValid() ? findModule(id) : ModId();
}

int Design::numModules() const
{
  absl::MutexLockMaybe lock(lockModulesIfParallel());
  return moduleCount();
}

void Design::setLogger(utl::Logger* logger)
{
  assert(logger != nullptr);
  logger_ = logger;
  names_.setLogger(logger);
  consts_.setLogger(logger);
}

size_t Design::memoryBytes() const
{
  size_t total = names_.memoryBytes() + consts_.memoryBytes()
                 + operators_.memoryBytes() + cells_.memoryBytes();
  absl::MutexLock lock(mutex_);
  total += modules_.capacity() * sizeof(std::unique_ptr<Module>)
           + by_name_.capacity() * (sizeof(std::pair<NameId, ModId>) + 1);
  for (const auto& module : modules_) {
    total += sizeof(Module) + module->memoryBytes();
  }
  return total;
}

uint64_t Design::checksum() const
{
  const int num_modules = numModules();
  uint64_t hash
      = checksumCombine(kChecksumSeed, static_cast<uint32_t>(num_modules));
  for (int i = 0; i < num_modules; ++i) {
    const ModId id(static_cast<uint32_t>(i));
    hash = checksumCombine(hash, id.value());
    hash = checksumCombine(hash, module(id).checksum(*this));
  }
  const std::optional<ModId> top = storedTop();
  hash = checksumCombine(hash, top.has_value() ? 1 : 0);
  if (top.has_value()) {
    hash = checksumCombine(hash, top->value());
  }
  return hash;
}

}  // namespace syn::lir
