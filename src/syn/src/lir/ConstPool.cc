// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/ConstPool.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "absl/container/inlined_vector.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/synchronization/mutex.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Ids.h"
#include "utl/Logger.h"

namespace syn::lir {
namespace {

using WordBuffer = absl::InlinedVector<uint64_t, 4>;

// Clears the bits above the width in both planes of `words` and clears aval
// wherever bval is set (an unknown bit is always (0, 1)).
void normalize(uint32_t width, uint64_t* words)
{
  const uint32_t n = constNumWords(width);
  if (n == 0) {
    return;
  }
  const uint64_t mask = constLastWordMask(width);
  words[n - 1] &= mask;
  words[2 * n - 1] &= mask;
  for (uint32_t i = 0; i < n; ++i) {
    words[i] &= ~words[n + i];
  }
}

}  // namespace

// ConstPool ------------------------------------------------------------------

bool ConstPool::Key::operator==(const Key& other) const
{
  const size_t count = 2 * size_t{constNumWords(width)};
  return width == other.width
         && (count == 0
             || std::memcmp(words, other.words, count * sizeof(uint64_t)) == 0);
}

ConstId ConstPool::intern(uint32_t width,
                          std::span<const uint64_t> aval,
                          std::span<const uint64_t> bval)
{
  const uint32_t n = constNumWords(width);
  assert(aval.size() == n && bval.size() == n);
  WordBuffer words(2 * size_t{n});
  std::copy(aval.begin(), aval.begin() + n, words.begin());
  std::copy(bval.begin(), bval.begin() + n, words.begin() + n);
  normalize(width, words.data());
  return internNormalized(width, words.data());
}

ConstId ConstPool::internUint(uint32_t width, uint64_t value)
{
  const uint32_t n = constNumWords(width);
  WordBuffer words(2 * size_t{n}, 0);
  if (n > 0) {
    words[0] = value;
  }
  normalize(width, words.data());
  return internNormalized(width, words.data());
}

ConstId ConstPool::internFill(uint32_t width, LogicValue value)
{
  const uint32_t n = constNumWords(width);
  const uint8_t v = static_cast<uint8_t>(value);
  WordBuffer words(2 * size_t{n});
  std::fill(words.begin(), words.begin() + n, (v & 1u) ? ~uint64_t{0} : 0);
  std::fill(words.begin() + n, words.end(), (v & 2u) ? ~uint64_t{0} : 0);
  normalize(width, words.data());
  return internNormalized(width, words.data());
}

absl::StatusOr<ConstId> ConstPool::internBinary(std::string_view msb_first)
{
  const uint32_t width = static_cast<uint32_t>(msb_first.size());
  const uint32_t n = constNumWords(width);
  WordBuffer words(2 * size_t{n}, 0);
  for (uint32_t i = 0; i < width; ++i) {
    uint64_t a = 0;
    uint64_t b = 0;
    switch (msb_first[width - 1 - i]) {
      case '0':
        break;
      case '1':
        a = 1;
        break;
      case 'x':
      case 'X':
        b = 1;
        break;
      default:
        return absl::InvalidArgumentError(
            absl::StrCat("invalid binary digit in '", msb_first, "'"));
    }
    words[i / 64] |= a << (i % 64);
    words[n + i / 64] |= b << (i % 64);
  }
  const ConstId id = internNormalized(width, words.data());
  if (!id.isValid()) {
    return absl::FailedPreconditionError(
        "constant pool is read-only: cannot add a new constant");
  }
  return id;
}

ConstId ConstPool::lookup(const Key& key) const
{
  const auto it = index_.find(key);
  return it != index_.end() ? ConstId(it->second) : ConstId();
}

ConstView ConstPool::viewAt(ConstId id) const
{
  assert(id.rawIndex() < entries_.size());
  const Key& key = entries_[id.rawIndex()];
  return ConstView(key.width, key.words);
}

const uint64_t* ConstPool::store(const uint64_t* words, size_t count)
{
  if (count == 0) {
    return nullptr;
  }
  if (count > kChunkWords / 4) {
    // Large constants get a chunk of their own and leave the current one open.
    auto chunk = std::make_unique<uint64_t[]>(count);
    std::memcpy(chunk.get(), words, count * sizeof(uint64_t));
    const uint64_t* stored = chunk.get();
    chunks_.insert(chunks_.end() - (chunks_.empty() ? 0 : 1), std::move(chunk));
    chunk_words_ += count;
    return stored;
  }
  if (chunks_.empty() || chunk_used_ + count > kChunkWords) {
    chunks_.push_back(std::make_unique<uint64_t[]>(kChunkWords));
    chunk_used_ = 0;
    chunk_words_ += kChunkWords;
  }
  uint64_t* destination = chunks_.back().get() + chunk_used_;
  std::memcpy(destination, words, count * sizeof(uint64_t));
  chunk_used_ += count;
  return destination;
}

ConstId ConstPool::internUnlocked(const Key& key)
{
  if (const ConstId id = lookup(key); id.isValid()) {
    return id;
  }
  const uint32_t id = static_cast<uint32_t>(entries_.size());
  if (id == ConstId::kInvalidValue) [[unlikely]] {
    logger_->critical(
        utl::SYN, 1102, "The constant pool is full ({} constants).", id);
  }
  // `key` may point into the caller's buffer; the index keeps a copy that
  // points into the pool.
  const Key stored{
      .width = key.width,
      .words = store(key.words, 2 * size_t{constNumWords(key.width)}),
  };
  entries_.push_back(stored);
  index_.emplace(stored, id);
  return ConstId(id);
}

ConstId ConstPool::internNormalized(uint32_t width, const uint64_t* words)
{
  const Key key{.width = width, .words = words};
  if (isReadOnly()) {
    return lookup(key);  // invalid if new: callers check
  }
  if (!isParallelWrite()) {
    return internUnlocked(key);
  }
  absl::MutexLock lock(&mutex_);
  return internUnlocked(key);
}

ConstView ConstPool::get(ConstId id) const
{
  assert(id.isValid());
  if (!isParallelWrite()) {
    return viewAt(id);
  }
  absl::MutexLock lock(&mutex_);
  return viewAt(id);
}

size_t ConstPool::size() const
{
  if (!isParallelWrite()) {
    return count();
  }
  absl::MutexLock lock(&mutex_);
  return count();
}

size_t ConstPool::memoryBytes() const
{
  absl::MutexLock lock(&mutex_);
  return chunk_words_ * sizeof(uint64_t) + entries_.capacity() * sizeof(Key)
         + chunks_.capacity() * sizeof(std::unique_ptr<uint64_t[]>)
         + index_.capacity() * (sizeof(std::pair<Key, uint32_t>) + 1);
}

}  // namespace syn::lir
