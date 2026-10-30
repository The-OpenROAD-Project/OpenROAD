// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Design-wide string interner: every name is a dense 32-bit NameId, given in
// insertion order, so side tables can be vectors. With several writers the
// ids depend on thread order, so output must not depend on NameId values.
// Modes: kReadOnly (default; lock-free, no adds), kSingleWrite (one thread
// adds, no locks), kParallelWrite (intern(), internPrefixed() and name()
// lock; no find()). Call setMode() only while no other thread uses the
// interner. name() views stay valid for the interner's lifetime.

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "syn/lir/Ids.h"
#include "utl/Logger.h"

namespace syn::lir {

class NameInterner
{
 public:
  NameInterner();
  NameInterner(const NameInterner&) = delete;
  NameInterner& operator=(const NameInterner&) = delete;

  // Returns the id of `name`, adding it if needed. In read-only mode `name`
  // must already exist. A full table is a critical error.
  NameId intern(std::string_view name);
  // Returns the id of `name`, or an invalid id. Not allowed in parallel-write
  // mode.
  NameId find(std::string_view name) const;
  // Returns the id of `prefix` followed by the string of `leaf` (e.g.
  // "u1/u2/q"), adding it if needed.
  NameId internPrefixed(std::string_view prefix, NameId leaf);
  // The string of a valid id returned by this interner.
  std::string_view name(NameId id) const;
  // Deterministic checksum of the string of `id` (0 if `id` is invalid).
  uint64_t checksum(NameId id) const;
  // True if `id` was returned by this interner.
  bool contains(NameId id) const;

  size_t size() const;
  size_t memoryBytes() const;

  TableMode mode() const { return mode_.load(std::memory_order_relaxed); }
  bool isReadOnly() const { return mode() == TableMode::kReadOnly; }
  bool isWritable() const { return mode() != TableMode::kReadOnly; }
  bool isParallelWrite() const { return mode() == TableMode::kParallelWrite; }
  // Call only while no other thread uses the interner.
  void setMode(TableMode mode) { mode_.store(mode, std::memory_order_relaxed); }
  // Set by the owning Design, while no other thread uses the interner.
  void setLogger(utl::Logger* logger)
  {
    assert(logger != nullptr);
    logger_ = logger;
  }

 private:
  static constexpr size_t kFirstChunkBytes = size_t{4} * 1024;
  static constexpr size_t kMaxChunkBytes = size_t{1024} * 1024;

  // A string to look up, with its hash computed once.
  struct Key
  {
    std::string_view text;
    uint32_t hash = 0;
  };
  // The index stores NameIds, hashed and compared through the interner's
  // tables, and is looked up by Key.
  struct IndexHash
  {
    using is_transparent = void;
    const NameInterner* self = nullptr;
    // SwissTable reads H1 from the low bits and H2 from the top bits, so the
    // 32-bit hash goes in both halves.
    static size_t expand(uint32_t h)
    {
      // Shifted in 64 bits, so a 32-bit size_t just keeps the low half.
      return static_cast<size_t>((uint64_t{h} << 32u) | h);
    }
    size_t operator()(uint32_t id) const ABSL_NO_THREAD_SAFETY_ANALYSIS
    {
      return expand(self->hashes_[id]);
    }
    size_t operator()(const Key& key) const { return expand(key.hash); }
  };
  struct IndexEq
  {
    using is_transparent = void;
    const NameInterner* self = nullptr;
    bool operator()(uint32_t a, uint32_t b) const { return a == b; }
    bool operator()(uint32_t id, const Key& key) const
    {
      return self->text(id) == key.text;
    }
    bool operator()(const Key& key, uint32_t id) const
    {
      return self->text(id) == key.text;
    }
  };

  // Unlocked: the caller holds `mutex_` or no other thread is writing.
  NameId lookup(const Key& key) const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  std::string_view text(uint32_t id) const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  size_t count() const ABSL_NO_THREAD_SAFETY_ANALYSIS
  {
    return strings_.size();
  }
  // Copies `text` with its length prefix into stable storage.
  const char* store(std::string_view text) ABSL_NO_THREAD_SAFETY_ANALYSIS;
  // Returns the id of `key`, adding it if needed.
  NameId internUnlocked(const Key& key) ABSL_NO_THREAD_SAFETY_ANALYSIS;
  NameId internPrefixedUnlocked(std::string_view prefix,
                                NameId leaf) ABSL_NO_THREAD_SAFETY_ANALYSIS;

  // Guarded by `mutex_` only in kParallelWrite, which the thread-safety
  // analysis cannot express.
  //
  // Storage per name: strings_[id] points into a chunk at a LEB128 length
  // prefix followed by the bytes; hashes_[id] is its 32-bit hash; index_
  // holds only ids. Chunks never move, so name() views stay valid.
  mutable absl::Mutex mutex_;
  std::vector<const char*> strings_ ABSL_GUARDED_BY(mutex_);  // by NameId
  std::vector<uint32_t> hashes_ ABSL_GUARDED_BY(mutex_);      // by NameId
  absl::flat_hash_set<uint32_t, IndexHash, IndexEq> index_
      ABSL_GUARDED_BY(mutex_);
  std::vector<std::unique_ptr<char[]>> chunks_ ABSL_GUARDED_BY(mutex_);
  char* chunk_next_ ABSL_GUARDED_BY(mutex_) = nullptr;
  size_t chunk_left_ ABSL_GUARDED_BY(mutex_) = 0;
  size_t next_chunk_bytes_ ABSL_GUARDED_BY(mutex_) = kFirstChunkBytes;
  size_t chunk_bytes_ ABSL_GUARDED_BY(mutex_) = 0;  // allocated, all chunks
  std::string path_ ABSL_GUARDED_BY(mutex_);        // internPrefixed's buffer
  std::atomic<TableMode> mode_{TableMode::kReadOnly};
  utl::Logger* logger_ = utl::Logger::defaultLogger();
};

}  // namespace syn::lir
