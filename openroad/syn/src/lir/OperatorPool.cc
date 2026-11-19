// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/OperatorPool.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>  // IWYU pragma: keep (used in asserts)
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "syn/lir/Checksum.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/Ids.h"
#include "syn/lir/OperatorSpec.h"

namespace syn::lir {

OpSpecId OperatorPool::lookup(const OpSpec& spec) const
{
  const auto it = index_.find(spec);
  return it != index_.end() ? it->second : OpSpecId();
}

const OpSpec& OperatorPool::specAt(OpSpecId id) const
{
  assert(id.rawIndex() < specs_.size());
  return *specs_[id.rawIndex()];
}

ModId OperatorPool::cachedAt(const OperatorKey& key) const
{
  const auto it = cache_.find(key);
  return it != cache_.end() ? it->second : ModId();
}

int OperatorPool::cachedCount() const
{
  assert(cache_.size() <= static_cast<size_t>(std::numeric_limits<int>::max()));
  return static_cast<int>(cache_.size());
}

OpSpecId OperatorPool::internUnlocked(const OpSpec& spec)
{
  const OpSpecId id(static_cast<uint32_t>(specs_.size()));
  const auto [it, inserted] = index_.try_emplace(spec, id);
  if (!inserted) {
    return it->second;
  }
  specs_.push_back(&it->first);
  return id;
}

OpSpecId OperatorPool::intern(const OpSpec& spec)
{
  if (isReadOnly()) {
    // Without asserts, a new spec yields the invalid id.
    const OpSpecId id = lookup(spec);
    assert(id.isValid()
           && "OperatorPool is read-only: cannot add an operator spec");
    return id;
  }
  // Entry points that take user data validate the spec first.
  assert(checkOpSpec(spec).ok());
  absl::MutexLockMaybe lock(lockIfParallel());
  return internUnlocked(spec);
}

const OpSpec& OperatorPool::get(OpSpecId id) const
{
  assert(id.isValid());
  absl::MutexLockMaybe lock(lockIfParallel());
  return specAt(id);
}

uint64_t OperatorPool::checksum(const OperatorKey& key,
                                const ConstPool& consts) const
{
  assert(key.generator_version >= 0);
  uint64_t hash = checksumCombine(kChecksumSeed, checksum(key.spec, consts));
  hash = checksumCombine(hash, key.arch);
  hash = checksumCombine(hash, static_cast<uint16_t>(key.generator_version));
  hash = checksumCombine(hash, key.arch_params);
  return hash;
}

size_t OperatorPool::size() const
{
  absl::MutexLockMaybe lock(lockIfParallel());
  return count();
}

ModId OperatorPool::findModule(const OperatorKey& key) const
{
  assert(key.generator_version >= 0);
  // Parallel-write mode would make the result depend on thread order.
  assert(!isParallelWrite()
         && "OperatorPool::findModule() cannot be called in parallel write "
            "mode");
  absl::MutexLockMaybe lock(lockIfParallel());
  return cachedAt(key);
}

void OperatorPool::setModuleUnlocked(const OperatorKey& key, ModId module)
{
  cache_.insert_or_assign(key, module);
}

void OperatorPool::cacheModule(const OperatorKey& key, ModId module)
{
  assert(key.generator_version >= 0);
  // Without asserts, skipped: the cache is optional and lock-free readers
  // must not see it change.
  assert(!isReadOnly()
         && "OperatorPool is read-only: cannot cache an implementation");
  if (isReadOnly()) {
    return;
  }
  absl::MutexLockMaybe lock(lockIfParallel());
  setModuleUnlocked(key, module);
}

void OperatorPool::eraseModuleUnlocked(ModId module)
{
  absl::erase_if(
      cache_, [module](const auto& entry) { return entry.second == module; });
}

void OperatorPool::uncacheModule(ModId module)
{
  // Without asserts, skipped like cacheModule(): lock-free readers must not see
  // the cache change, and findOperatorModule() rejects stale entries.
  assert(!isReadOnly()
         && "OperatorPool is read-only: cannot drop an implementation");
  if (isReadOnly()) {
    return;
  }
  absl::MutexLockMaybe lock(lockIfParallel());
  eraseModuleUnlocked(module);
}

int OperatorPool::numCachedModules() const
{
  absl::MutexLockMaybe lock(lockIfParallel());
  return cachedCount();
}

size_t OperatorPool::memoryBytes() const
{
  absl::MutexLock lock(mutex_);
  size_t total
      = specs_.capacity() * sizeof(const OpSpec*)
        + index_.capacity() * (sizeof(void*) + 1)
        + cache_.capacity() * (sizeof(std::pair<OperatorKey, ModId>) + 1);
  for (const auto& [spec, id] : index_) {
    total += sizeof(std::pair<const OpSpec, OpSpecId>)
             + spec.operands.capacity() * sizeof(OpOperand)
             + spec.terms.capacity() * sizeof(DatapathTerm);
  }
  return total;
}

}  // namespace syn::lir
