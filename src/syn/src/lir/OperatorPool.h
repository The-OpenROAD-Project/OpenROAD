// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The design's operator pool: interned operator specs (OpSpecId), and a cache
// from OperatorKey (spec + architecture) to the shared module implementing it.
// The cache is only a hint: use Design::findOperatorModule(), which also
// checks that the module is unedited.
//
// Read-only by default, like the other pools: adding a spec or a cache entry
// needs write mode.

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/Ids.h"
#include "syn/lir/OperatorSpec.h"

namespace syn::lir {

class OperatorPool
{
 public:
  OperatorPool() = default;
  OperatorPool(const OperatorPool&) = delete;
  OperatorPool& operator=(const OperatorPool&) = delete;

  // Returns the id of `spec`, adding it if needed. In read-only mode it must
  // already exist. The spec must pass checkOpSpec.
  OpSpecId intern(const OpSpec& spec);
  // The spec of a valid id. The reference lives as long as the pool.
  const OpSpec& get(OpSpecId id) const;
  // Deterministic checksum of the spec of `id` (0 if `id` is invalid).
  uint64_t checksum(OpSpecId id, const ConstPool& consts) const
  {
    return id.isValid() ? lir::checksum(get(id), consts) : 0;
  }
  // Deterministic checksum of `key`; constant operands are read from
  // `consts`.
  uint64_t checksum(const OperatorKey& key, const ConstPool& consts) const;
  size_t size() const;

  // Returns the cached module for `key`, or an invalid id. Prefer
  // Design::findOperatorModule. Not allowed in parallel-write mode.
  ModId findModule(const OperatorKey& key) const;
  // Caches `module` for `key`, replacing any previous entry (write mode).
  void cacheModule(const OperatorKey& key, ModId module);
  // Drops every entry naming `module` (write mode).
  void uncacheModule(ModId module);
  size_t numCachedModules() const;

  size_t memoryBytes() const;

  TableMode mode() const { return mode_.load(std::memory_order_relaxed); }
  bool isReadOnly() const { return mode() == TableMode::kReadOnly; }
  bool isWritable() const { return mode() != TableMode::kReadOnly; }
  bool isParallelWrite() const { return mode() == TableMode::kParallelWrite; }
  // Call only while no other thread uses the pool.
  void setMode(TableMode mode) { mode_.store(mode, std::memory_order_relaxed); }

 private:
  absl::Mutex* lockIfParallel() const
  {
    return isParallelWrite() ? &mutex_ : nullptr;
  }
  // Unlocked: the caller holds `mutex_` or no other thread is writing.
  OpSpecId lookup(const OpSpec& spec) const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  const OpSpec& specAt(OpSpecId id) const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  ModId cachedAt(const OperatorKey& key) const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  size_t count() const ABSL_NO_THREAD_SAFETY_ANALYSIS { return specs_.size(); }
  size_t cachedCount() const ABSL_NO_THREAD_SAFETY_ANALYSIS
  {
    return cache_.size();
  }
  OpSpecId internUnlocked(const OpSpec& spec) ABSL_NO_THREAD_SAFETY_ANALYSIS;
  void setModuleUnlocked(const OperatorKey& key,
                         ModId module) ABSL_NO_THREAD_SAFETY_ANALYSIS;
  void eraseModuleUnlocked(ModId module) ABSL_NO_THREAD_SAFETY_ANALYSIS;

  std::atomic<TableMode> mode_{TableMode::kReadOnly};
  mutable absl::Mutex mutex_;
  // node_hash_map keeps the specs at stable addresses for specs_.
  absl::node_hash_map<OpSpec, uint32_t> index_ ABSL_GUARDED_BY(mutex_);
  std::vector<const OpSpec*> specs_ ABSL_GUARDED_BY(mutex_);
  absl::flat_hash_map<OperatorKey, ModId> cache_ ABSL_GUARDED_BY(mutex_);
};

}  // namespace syn::lir
