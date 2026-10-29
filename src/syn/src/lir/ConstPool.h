// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Design-wide pool of 3-state constants wider than kMaxInlineConstWidth, and
// of LUT truth tables (encoding: ConstView.h). Each value is stored once, so
// equal constants have equal ConstIds, and stored words never move. Modes as
// in Design.h: read-only adds return an invalid id (internBinary():
// FailedPrecondition), which callers must check.

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Ids.h"
#include "utl/Logger.h"

namespace syn::lir {

class ConstPool
{
 public:
  ConstPool() = default;
  ConstPool(const ConstPool&) = delete;
  ConstPool& operator=(const ConstPool&) = delete;

  // Returns the id of the constant, adding it if needed (write mode only;
  // in read-only mode a new constant yields an invalid id). `aval` and `bval`
  // hold constNumWords(width) words each; bits above the width are ignored,
  // and `aval` is cleared wherever `bval` is set.
  ConstId intern(uint32_t width,
                 std::span<const uint64_t> aval,
                 std::span<const uint64_t> bval);
  // A two-state value, zero-extended or truncated to `width`.
  ConstId internUint(uint32_t width, uint64_t value);
  // Every bit set to `value`.
  ConstId internFill(uint32_t width, LogicValue value);
  // MSB-first digits 0, 1, x/X; the width is the number of digits.
  // FailedPrecondition for a new constant in read-only mode.
  absl::StatusOr<ConstId> internBinary(std::string_view msb_first);

  // The value of a valid id returned by this pool.
  ConstView get(ConstId id) const;
  // Deterministic checksum of the constant of `id` (0 if `id` is invalid).
  uint64_t checksum(ConstId id) const
  {
    return id.isValid() ? get(id).checksum() : 0;
  }

  size_t size() const;
  size_t memoryBytes() const;

  TableMode mode() const { return mode_.load(std::memory_order_relaxed); }
  bool isReadOnly() const { return mode() == TableMode::kReadOnly; }
  bool isWritable() const { return mode() != TableMode::kReadOnly; }
  bool isParallelWrite() const { return mode() == TableMode::kParallelWrite; }
  // Call only while no other thread uses the pool.
  void setMode(TableMode mode) { mode_.store(mode, std::memory_order_relaxed); }
  // Set by the owning Design. Call only while no other thread uses the pool.
  void setLogger(utl::Logger* logger)
  {
    assert(logger != nullptr);
    logger_ = logger;
  }

 private:
  static constexpr size_t kChunkWords = 4096;  // 32 KiB

  // Width and normalized words (aval then bval) of a constant, either stored
  // in the pool or, for lookups, in the caller's buffer.
  struct Key
  {
    uint32_t width = 0;
    const uint64_t* words = nullptr;

    bool operator==(const Key& other) const;
    template <typename H>
    friend H AbslHashValue(H h, const Key& key)
    {
      return H::combine_contiguous(H::combine(std::move(h), key.width),
                                   key.words,
                                   2 * size_t{constNumWords(key.width)});
    }
  };

  // `words` must be normalized (bits above the width cleared).
  ConstId internNormalized(uint32_t width, const uint64_t* words);
  // Unlocked: the caller holds `mutex_` (in kParallelWrite) or no other
  // thread is writing (kReadOnly / kSingleWrite).
  ConstId lookup(const Key& key) const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  ConstView viewAt(ConstId id) const ABSL_NO_THREAD_SAFETY_ANALYSIS;
  size_t count() const ABSL_NO_THREAD_SAFETY_ANALYSIS
  {
    return entries_.size();
  }
  // Copies `count` words into stable storage.
  const uint64_t* store(const uint64_t* words,
                        size_t count) ABSL_NO_THREAD_SAFETY_ANALYSIS;
  ConstId internUnlocked(const Key& key) ABSL_NO_THREAD_SAFETY_ANALYSIS;

  std::atomic<TableMode> mode_{TableMode::kReadOnly};
  utl::Logger* logger_ = utl::Logger::defaultLogger();
  // Storage per constant: entries_[id] is its width and a pointer into a
  // chunk at 2 * constNumWords(width) words, the aval plane then the bval
  // plane; index_ maps the same Key to the id. Chunks never move.
  mutable absl::Mutex mutex_;
  std::vector<Key> entries_ ABSL_GUARDED_BY(mutex_);  // by ConstId
  // Keys point into `chunks_`.
  absl::flat_hash_map<Key, uint32_t> index_ ABSL_GUARDED_BY(mutex_);
  std::vector<std::unique_ptr<uint64_t[]>> chunks_ ABSL_GUARDED_BY(mutex_);
  size_t chunk_used_ ABSL_GUARDED_BY(mutex_) = kChunkWords;
  size_t chunk_words_ ABSL_GUARDED_BY(mutex_) = 0;
};

}  // namespace syn::lir
