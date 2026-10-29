// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/NameInterner.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "absl/hash/hash.h"
#include "absl/synchronization/mutex.h"
#include "syn/lir/Checksum.h"
#include "syn/lir/Ids.h"
#include "utl/Logger.h"

namespace syn::lir {

namespace {

// LEB128 length prefix: 7 bits per byte, low bits first; a set top bit means
// another byte follows. 1 byte below 128 characters, at most 5 for 4 GB.
constexpr size_t kMaxPrefixBytes = 5;

size_t encodeLength(uint32_t length, char* out)
{
  size_t n = 0;
  while (length >= 0x80) {
    out[n++] = static_cast<char>((length & 0x7Fu) | 0x80u);
    length >>= 7u;
  }
  out[n++] = static_cast<char>(length);
  return n;
}

uint32_t hashOf(std::string_view text)
{
  return static_cast<uint32_t>(absl::HashOf(text));
}

}  // namespace

NameInterner::NameInterner() : index_(0, IndexHash{this}, IndexEq{this})
{
}

std::string_view NameInterner::text(uint32_t id) const
{
  // Decodes the length prefix written by store().
  assert(id < strings_.size());
  const auto* p = reinterpret_cast<const unsigned char*>(strings_[id]);
  uint32_t length = 0;
  uint32_t shift = 0;
  while ((*p & 0x80u) != 0) {
    length |= static_cast<uint32_t>(*p++ & 0x7Fu) << shift;
    shift += 7;
  }
  length |= static_cast<uint32_t>(*p++) << shift;
  return {reinterpret_cast<const char*>(p), length};
}

const char* NameInterner::store(std::string_view text)
{
  if (text.size() > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
    // L-IR uses message ids SYN 1100-1199.
    logger_->critical(utl::SYN,
                      1100,
                      "A name of {} bytes is too long for the name table "
                      "(limit 4 GB).",
                      text.size());
  }
  std::array<char, kMaxPrefixBytes> prefix;
  const size_t prefix_bytes
      = encodeLength(static_cast<uint32_t>(text.size()), prefix.data());
  const size_t bytes = prefix_bytes + text.size();
  char* destination = nullptr;
  if (bytes > next_chunk_bytes_ / 4) {
    // Large strings get a chunk of their own and leave the current one open.
    auto chunk = std::make_unique<char[]>(bytes);
    destination = chunk.get();
    chunks_.insert(chunks_.end() - (chunks_.empty() ? 0 : 1), std::move(chunk));
    chunk_bytes_ += bytes;
  } else {
    if (bytes > chunk_left_) {
      chunks_.push_back(std::make_unique<char[]>(next_chunk_bytes_));
      chunk_next_ = chunks_.back().get();
      chunk_left_ = next_chunk_bytes_;
      chunk_bytes_ += next_chunk_bytes_;
      next_chunk_bytes_ = std::min(next_chunk_bytes_ * 2, kMaxChunkBytes);
    }
    destination = chunk_next_;
    chunk_next_ += bytes;
    chunk_left_ -= bytes;
  }
  std::memcpy(destination, prefix.data(), prefix_bytes);
  if (!text.empty()) {
    std::memcpy(destination + prefix_bytes, text.data(), text.size());
  }
  return destination;
}

NameId NameInterner::lookup(const Key& key) const
{
  const auto it = index_.find(key);
  return it != index_.end() ? NameId(*it) : NameId();
}

NameId NameInterner::intern(std::string_view name)
{
  const Key key{.text = name, .hash = hashOf(name)};
  if (isReadOnly()) {
    // Without asserts, a new name yields the invalid id.
    const NameId id = lookup(key);
    assert(id.isValid() && "NameInterner is read-only: cannot add a name");
    return id;
  }
  if (!isParallelWrite()) {
    return internUnlocked(key);
  }
  absl::MutexLock lock(&mutex_);
  return internUnlocked(key);
}

NameId NameInterner::internUnlocked(const Key& key)
{
  if (const NameId id = lookup(key); id.isValid()) {
    return id;
  }
  const uint32_t id = static_cast<uint32_t>(strings_.size());
  if (id >= NameId::kInvalidValue) [[unlikely]] {
    logger_->critical(
        utl::SYN, 1101, "The name table is full ({} names).", strings_.size());
  }
  strings_.push_back(store(key.text));
  // Before the insert: the index hashes `id` through hashes_[id], so the
  // string is never hashed again, not even when the index grows.
  hashes_.push_back(key.hash);
  index_.insert(id);
  return NameId(id);
}

NameId NameInterner::internPrefixedUnlocked(std::string_view prefix,
                                            NameId leaf)
{
  path_.assign(prefix);
  path_.append(text(leaf.rawIndex()));
  return internUnlocked(Key{.text = path_, .hash = hashOf(path_)});
}

NameId NameInterner::internPrefixed(std::string_view prefix, NameId leaf)
{
  assert(leaf.isValid());
  if (isReadOnly()) {
    std::string path(prefix);
    path.append(text(leaf.rawIndex()));
    return intern(path);
  }
  if (!isParallelWrite()) {
    return internPrefixedUnlocked(prefix, leaf);
  }
  absl::MutexLock lock(&mutex_);
  return internPrefixedUnlocked(prefix, leaf);
}

NameId NameInterner::find(std::string_view name) const
{
  // Parallel-write mode would make the result depend on thread order.
  assert(!isParallelWrite()
         && "NameInterner::find() cannot be called in parallel write mode");
  if (!isParallelWrite()) {
    return lookup(Key{.text = name, .hash = hashOf(name)});
  }
  absl::MutexLock lock(&mutex_);
  return lookup(Key{.text = name, .hash = hashOf(name)});
}

std::string_view NameInterner::name(NameId id) const
{
  assert(id.isValid());
  if (!isParallelWrite()) {
    return text(id.rawIndex());
  }
  absl::MutexLock lock(&mutex_);
  return text(id.rawIndex());
}

uint64_t NameInterner::checksum(NameId id) const
{
  if (!id.isValid()) {
    return 0;
  }
  return checksumBytes(kChecksumSeed, name(id));
}

bool NameInterner::contains(NameId id) const
{
  return id.isValid() && id.rawIndex() < size();
}

size_t NameInterner::size() const
{
  if (!isParallelWrite()) {
    return count();
  }
  absl::MutexLock lock(&mutex_);
  return count();
}

size_t NameInterner::memoryBytes() const
{
  absl::MutexLock lock(&mutex_);
  return chunk_bytes_ + strings_.capacity() * sizeof(const char*)
         + hashes_.capacity() * sizeof(uint32_t)
         + chunks_.capacity() * sizeof(std::unique_ptr<char[]>)
         + index_.capacity() * (sizeof(uint32_t) + 1);
}

}  // namespace syn::lir
