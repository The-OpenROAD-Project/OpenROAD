// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// A design: the modules and the design-wide pools they share (names, 3-state
// constants, operator specs, Liberty cells). The pools are kReadOnly (default;
// no locks, no growth), kSingleWrite (one thread grows them) or
// kParallelWrite (every access locks); switch with the *Scope classes while no
// other thread uses the design. Modules are not synchronized: one thread at a
// time owns a module, and only it may touch the module while it changes.

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/Ids.h"
#include "syn/lir/LibCellTable.h"
#include "syn/lir/Module.h"
#include "syn/lir/NameInterner.h"
#include "syn/lir/OperatorPool.h"
#include "syn/lir/OperatorSpec.h"
#include "syn/lir/SourceLoc.h"
#include "utl/Logger.h"

namespace syn::lir {

// Design-wide settings that passes and editing primitives read from the
// design they work on.
struct DesignOptions
{
  // ModuleBuilder::install and Editor::commit fail if the result is not
  // well formed (WellFormed.h). Off by default.
  bool check_well_formed = false;
};

class Design
{
 public:
  // Construction and options.
  explicit Design(const DesignOptions& options = DesignOptions(),
                  utl::Logger* logger = utl::Logger::defaultLogger());
  Design(const Design&) = delete;
  Design& operator=(const Design&) = delete;
  const DesignOptions& options() const { return options_; }
  // Changes the options between passes: call only while no other thread
  // uses the design.
  void setOptions(const DesignOptions& options) { options_ = options; }

  // Modules.
  // Adds a module. Named modules must have unique names; anonymous modules
  // (invalid name) are allowed and are not found by findModule(). Fails with
  // FailedPrecondition in read-only mode.
  absl::StatusOr<ModId> addModule(Module&& module);
  // Replaces the contents of a module (e.g. after a ModuleBuilder rebuild),
  // keeping its ModId, address and name. The revision becomes the old one
  // plus one. Fails with FailedPrecondition if the module is removed. The
  // caller must own the module; allowed in every mode.
  absl::Status replaceModule(ModId id, Module&& module);
  // Adds a copy of module `src` named `name`, with revision 0. Fails like
  // addModule(), and with FailedPrecondition if `src` is removed.
  absl::StatusOr<ModId> copyModule(ModId src, NameId name);
  // Removes a module: frees its contents and releases its name. Its ModId
  // stays reserved (Module::isRemoved()); no instance may refer to it, and
  // it may not be the stored top (see setTop()). Write mode only; an
  // operator implementation also needs a writable operator pool.
  absl::Status removeModule(ModId id);
  // The returned reference stays valid for the lifetime of the design.
  Module& module(ModId id);
  const Module& module(ModId id) const;
  // Not allowed in kParallelWrite mode.
  ModId findModule(NameId name) const;
  ModId findModule(std::string_view name) const;
  int numModules() const;

  // Module attributes, source locations and frozen modules.
  // Sets the attributes of module `id` in place. The revision stays. Write
  // mode only.
  absl::Status setModuleAttributes(ModId id,
                                   const ModuleAttributes& attributes);
  // Sets the source location of module `id` in place (write mode only).
  absl::Status setModuleSourceLoc(ModId id, SourceLoc loc);
  // Frozen flag of every module, by ModId: dont_touch modules and every
  // module a dont_touch instance or a frozen module instantiates.
  std::vector<uint8_t> computeFrozenModules() const;
  // Sets ModuleAttributes::frozen_by_parent of every live module from
  // computeFrozenModules(). Run it after setting dont_touch on a module or
  // an instance. Write mode only.
  absl::Status markFrozenModules();

  // Top module.
  // Sets the top: a live module that is not a blackbox or an operator
  // implementation. Write mode only.
  absl::Status setTop(ModId id);
  // Write mode only.
  void clearTop();
  // The top set by setTop(). Else the only candidate root (a live module
  // that is not a blackbox or an operator implementation and that no
  // live module instantiates), found by an O(objects) scan that is not
  // cached; else FailedPrecondition listing the candidates. Not allowed in
  // kParallelWrite mode.
  absl::StatusOr<ModId> top() const;

  // Operator implementations.
  // Marks module `id` as the pristine implementation of operator `key`
  // (OperatorSpec.h) and caches it. The spec must be interned and the ports
  // must match it. Write mode only.
  absl::Status setOperatorImpl(ModId id, const OperatorKey& key);
  // Drops module `id`'s implementation record and cache entries. Write mode
  // only; fails with FailedPrecondition if the module is removed.
  absl::Status clearOperatorImpl(ModId id);
  // The cached module implementing `key`, if it still carries that record
  // and is pristine; else an invalid id.
  ModId findOperatorModule(const OperatorKey& key) const;

  // Design-wide pools: names, constants, operator specs and Liberty cells.
  NameInterner& names() { return names_; }
  const NameInterner& names() const { return names_; }
  ConstPool& consts() { return consts_; }
  const ConstPool& consts() const { return consts_; }
  OperatorPool& operators() { return operators_; }
  const OperatorPool& operators() const { return operators_; }
  LibCellTable& cells() { return cells_; }
  const LibCellTable& cells() const { return cells_; }

  // Table modes and mode scopes.
  TableMode modulesMode() const;
  // True when both the module table and every pool are read-only.
  bool isReadOnly() const { return isModulesReadOnly() && isPoolsReadOnly(); }
  // True when the module table is read-only.
  bool isModulesReadOnly() const;
  // True when every design-wide pool is read-only.
  bool isPoolsReadOnly() const;
  // True when both the module table and every pool are in a write mode.
  bool isWritable() const { return isModulesWritable() && isPoolsWritable(); }
  bool isModulesWritable() const;
  bool isPoolsWritable() const;
  // True when both the module table and every pool are in kParallelWrite.
  bool isParallelWrite() const;
  bool isModulesParallelWrite() const;
  bool isPoolsParallelWrite() const;
  // RAII scopes that set a mode and restore the previous one on exit
  // (defined below Design). Scopes nest. Call only while no other thread
  // uses the design.
  class SingleWriteScope;
  class ParallelWriteScope;
  class ReadOnlyScope;
  class ModulesSingleWriteScope;
  class ModulesReadOnlyScope;
  class PoolsSingleWriteScope;
  class PoolsParallelWriteScope;
  class PoolsReadOnlyScope;

  // Logger.
  // Never null: utl::Logger::defaultLogger() until replaced (OpenROAD sets
  // its own). Call setLogger() only while no other thread uses the design.
  utl::Logger* logger() const { return logger_; }
  void setLogger(utl::Logger* logger);

  // Memory and checksum.
  // Heap bytes of the modules and pools. No module may be edited meanwhile.
  size_t memoryBytes() const;
  // Deterministic checksum of the design.
  uint64_t checksum() const;

 private:
  class ModeScope;

  void setMode(TableMode mode);
  void setModulesMode(TableMode mode);
  void setPoolsMode(TableMode mode);

  absl::Mutex* lockModulesIfParallel() const
  {
    return isModulesParallelWrite() ? &mutex_ : nullptr;
  }
  // Unlocked access: the caller holds `mutex_` (in kParallelWrite) or no
  // other thread is writing (kReadOnly / kSingleWrite).
  Module& moduleAt(ModId id) const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  ModId lookup(NameId name) const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  int moduleCount() const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  absl::StatusOr<ModId> addModuleUnlocked(Module&& module)
      ABSL_NO_THREAD_SAFETY_ANALYSIS;
  // Returns whether the removed module had an operator implementation.
  absl::StatusOr<bool> removeModuleUnlocked(ModId id)
      ABSL_NO_THREAD_SAFETY_ANALYSIS;
  // Access to top_: storedTop() locks in kParallelWrite; storeTop() needs
  // the caller to hold the lock, like moduleAt().
  std::optional<ModId> storedTop() const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  void storeTop(std::optional<ModId> top) ABSL_NO_THREAD_SAFETY_ANALYSIS;
  // top() when none is stored: one scan of the instances.
  absl::StatusOr<ModId> computeTop() const;

  DesignOptions options_;
  utl::Logger* logger_ = utl::Logger::defaultLogger();
  // Read-only by default, like the design.
  NameInterner names_;
  ConstPool consts_;
  OperatorPool operators_;
  LibCellTable cells_;
  std::atomic<TableMode> modules_mode_{TableMode::kReadOnly};
  // Guards the module table in kParallelWrite mode.
  mutable absl::Mutex mutex_;
  // unique_ptr keeps module addresses stable while the table grows.
  std::vector<std::unique_ptr<Module>> modules_ ABSL_GUARDED_BY(mutex_);
  absl::flat_hash_map<NameId, ModId> by_name_ ABSL_GUARDED_BY(mutex_);
  std::optional<ModId> top_ ABSL_GUARDED_BY(mutex_);
};

// Sets the mode of the module table and/or the pools for the scope's
// lifetime, then restores the previous modes. Scopes nest.
class [[nodiscard]] Design::ModeScope
{
 public:
  ModeScope(Design& design,
            std::optional<TableMode> modules_mode,
            std::optional<TableMode> pools_mode)
      : design_(design),
        prev_modules_(design.modulesMode()),
        prev_names_(design.names_.mode()),
        prev_consts_(design.consts_.mode()),
        prev_operators_(design.operators_.mode()),
        prev_cells_(design.cells_.mode())
  {
    if (modules_mode.has_value()) {
      design_.setModulesMode(*modules_mode);
    }
    if (pools_mode.has_value()) {
      design_.setPoolsMode(*pools_mode);
    }
  }
  ~ModeScope()
  {
    design_.setModulesMode(prev_modules_);
    design_.names_.setMode(prev_names_);
    design_.consts_.setMode(prev_consts_);
    design_.operators_.setMode(prev_operators_);
    design_.cells_.setMode(prev_cells_);
  }
  ModeScope(const ModeScope&) = delete;
  ModeScope& operator=(const ModeScope&) = delete;

 private:
  Design& design_;
  const TableMode prev_modules_;
  const TableMode prev_names_;
  const TableMode prev_consts_;
  const TableMode prev_operators_;
  const TableMode prev_cells_;
};

// Allows adding modules and pool entries on a single thread (no locks):
//
//   {
//     Design::SingleWriteScope write(design);
//     elaborate(design);      // adds modules and names without mutex locks
//   }                         // read-only again
class [[nodiscard]] Design::SingleWriteScope : public Design::ModeScope
{
 public:
  explicit SingleWriteScope(Design& design)
      : ModeScope(design, TableMode::kSingleWrite, TableMode::kSingleWrite)
  {
  }
};

// Allows adding modules and pool entries from multiple threads at once
// (reads and writes take each table's mutex):
class [[nodiscard]] Design::ParallelWriteScope : public Design::ModeScope
{
 public:
  explicit ParallelWriteScope(Design& design)
      : ModeScope(design, TableMode::kParallelWrite, TableMode::kParallelWrite)
  {
  }
};

// Makes the whole design read-only, e.g. to run workers from inside a
// SingleWriteScope:
//
//   {
//     Design::ReadOnlyScope read_only(design);
//     runInParallel(design);  // workers read the tables without locks
//   }                         // workers joined; previous mode restored
class [[nodiscard]] Design::ReadOnlyScope : public Design::ModeScope
{
 public:
  explicit ReadOnlyScope(Design& design)
      : ModeScope(design, TableMode::kReadOnly, TableMode::kReadOnly)
  {
  }
};

class [[nodiscard]] Design::ModulesSingleWriteScope : public Design::ModeScope
{
 public:
  explicit ModulesSingleWriteScope(Design& design)
      : ModeScope(design, TableMode::kSingleWrite, std::nullopt)
  {
  }
};

class [[nodiscard]] Design::ModulesReadOnlyScope : public Design::ModeScope
{
 public:
  explicit ModulesReadOnlyScope(Design& design)
      : ModeScope(design, TableMode::kReadOnly, std::nullopt)
  {
  }
};

// Allows interning new names and constants on a single thread (no locks)
// while leaving the module table's mode unchanged.
class [[nodiscard]] Design::PoolsSingleWriteScope : public Design::ModeScope
{
 public:
  explicit PoolsSingleWriteScope(Design& design)
      : ModeScope(design, std::nullopt, TableMode::kSingleWrite)
  {
  }
};

// Allows interning new names and constants from several threads (pools take
// their mutexes), leaving the module table's mode unchanged.
class [[nodiscard]] Design::PoolsParallelWriteScope : public Design::ModeScope
{
 public:
  explicit PoolsParallelWriteScope(Design& design)
      : ModeScope(design, std::nullopt, TableMode::kParallelWrite)
  {
  }
};

class [[nodiscard]] Design::PoolsReadOnlyScope : public Design::ModeScope
{
 public:
  explicit PoolsReadOnlyScope(Design& design)
      : ModeScope(design, std::nullopt, TableMode::kReadOnly)
  {
  }
};

// Inline definitions.

inline TableMode Design::modulesMode() const
{
  return modules_mode_.load(std::memory_order_relaxed);
}

inline bool Design::isModulesReadOnly() const
{
  return modulesMode() == TableMode::kReadOnly;
}

inline bool Design::isPoolsReadOnly() const
{
  return names_.isReadOnly() && consts_.isReadOnly() && operators_.isReadOnly()
         && cells_.isReadOnly();
}

inline bool Design::isModulesWritable() const
{
  return modulesMode() != TableMode::kReadOnly;
}

inline bool Design::isPoolsWritable() const
{
  return names_.isWritable() && consts_.isWritable() && operators_.isWritable()
         && cells_.isWritable();
}

inline bool Design::isParallelWrite() const
{
  return isModulesParallelWrite() && isPoolsParallelWrite();
}

inline bool Design::isModulesParallelWrite() const
{
  return modulesMode() == TableMode::kParallelWrite;
}

inline bool Design::isPoolsParallelWrite() const
{
  return names_.isParallelWrite() && consts_.isParallelWrite()
         && operators_.isParallelWrite() && cells_.isParallelWrite();
}

}  // namespace syn::lir
