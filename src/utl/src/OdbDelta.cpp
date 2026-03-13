// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/OdbDelta.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include "utl/ScopedTemporaryFile.h"

namespace utl {

namespace {

// Write a value in little-endian format to a byte vector.
template <typename T>
void writeLE(std::vector<uint8_t>& out, T value)
{
  for (size_t i = 0; i < sizeof(T); ++i) {
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    value >>= 8;
  }
}

// Read a little-endian value from a byte pointer and advance it.
template <typename T>
T readLE(const uint8_t*& ptr, const uint8_t* end)
{
  if (ptr + sizeof(T) > end) {
    throw std::runtime_error("OdbDelta: unexpected end of delta file");
  }
  T value = 0;
  for (size_t i = 0; i < sizeof(T); ++i) {
    value |= static_cast<T>(ptr[i]) << (8 * i);
  }
  ptr += sizeof(T);
  return value;
}

}  // namespace

std::vector<uint8_t> computeOdbDelta(const std::vector<uint8_t>& base,
                                     const std::vector<uint8_t>& current,
                                     uint32_t block_size)
{
  std::vector<uint8_t> delta;

  // Reserve a reasonable estimate.
  delta.reserve(28 + current.size() / 4);

  // Header: magic, version, block_size, base_size, current_size.
  writeLE<uint32_t>(delta, OdbDelta::MAGIC);
  writeLE<uint32_t>(delta, OdbDelta::VERSION);
  writeLE<uint32_t>(delta, block_size);
  writeLE<uint64_t>(delta, base.size());
  writeLE<uint64_t>(delta, current.size());

  // Compare blocks that overlap with base.
  const size_t common_len = std::min(base.size(), current.size());
  const size_t num_common_blocks = (common_len + block_size - 1) / block_size;

  for (size_t i = 0; i < num_common_blocks; ++i) {
    const size_t offset = i * block_size;
    const size_t len
        = std::min(static_cast<size_t>(block_size), common_len - offset);

    if (std::memcmp(base.data() + offset, current.data() + offset, len) != 0) {
      // Block differs — write block index + data.
      writeLE<uint32_t>(delta, static_cast<uint32_t>(i));
      delta.insert(delta.end(),
                   current.data() + offset,
                   current.data() + offset + len);
      // Pad to block_size if this is a short final block.
      if (len < block_size) {
        delta.resize(delta.size() + (block_size - len), 0);
      }
    }
  }

  // Append any new data beyond the base size.
  if (current.size() > base.size()) {
    const size_t tail_start = num_common_blocks * block_size;
    const size_t num_tail_blocks
        = (current.size() - tail_start + block_size - 1) / block_size;
    for (size_t i = 0; i < num_tail_blocks; ++i) {
      const size_t block_idx = num_common_blocks + i;
      const size_t offset = tail_start + i * block_size;
      const size_t len = std::min(static_cast<size_t>(block_size),
                                  current.size() - offset);

      writeLE<uint32_t>(delta, static_cast<uint32_t>(block_idx));
      delta.insert(delta.end(),
                   current.data() + offset,
                   current.data() + offset + len);
      if (len < block_size) {
        delta.resize(delta.size() + (block_size - len), 0);
      }
    }
  }

  return delta;
}

std::vector<uint8_t> applyOdbDelta(const std::vector<uint8_t>& base,
                                   const std::vector<uint8_t>& delta)
{
  const uint8_t* ptr = delta.data();
  const uint8_t* end = delta.data() + delta.size();

  // Read header.
  const uint32_t magic = readLE<uint32_t>(ptr, end);
  if (magic != OdbDelta::MAGIC) {
    throw std::runtime_error(
        "OdbDelta: invalid magic number (not an .odb.delta file)");
  }

  const uint32_t version = readLE<uint32_t>(ptr, end);
  if (version != OdbDelta::VERSION) {
    throw std::runtime_error("OdbDelta: unsupported version "
                             + std::to_string(version));
  }

  const uint32_t block_size = readLE<uint32_t>(ptr, end);
  const uint64_t base_size = readLE<uint64_t>(ptr, end);
  const uint64_t current_size = readLE<uint64_t>(ptr, end);

  if (base.size() != base_size) {
    throw std::runtime_error(
        "OdbDelta: base size mismatch (expected "
        + std::to_string(base_size) + ", got " + std::to_string(base.size())
        + "). Wrong base .odb file?");
  }

  // Start with a copy of the base, resized to current_size.
  std::vector<uint8_t> result(current_size, 0);
  std::memcpy(result.data(), base.data(), std::min(base.size(), result.size()));

  // Apply changed blocks.
  while (ptr < end) {
    const uint32_t block_idx = readLE<uint32_t>(ptr, end);
    const size_t offset = static_cast<size_t>(block_idx) * block_size;

    if (offset >= current_size) {
      throw std::runtime_error("OdbDelta: block index out of range");
    }

    const size_t len
        = std::min(static_cast<size_t>(block_size), current_size - offset);
    const size_t read_len
        = std::min(static_cast<size_t>(block_size),
                   static_cast<size_t>(end - ptr));

    if (read_len < len) {
      throw std::runtime_error("OdbDelta: truncated block data");
    }

    std::memcpy(result.data() + offset, ptr, len);
    ptr += block_size;  // Always advance by full block_size (may include pad).
  }

  return result;
}

std::vector<uint8_t> readFileBytes(const char* filename)
{
  InStreamHandler handler(filename, true);
  std::istream& stream = handler.getStream();

  // InStreamHandler may set exception flags on the stream.
  // Clear them so that EOF doesn't throw.
  stream.exceptions(std::ios::goodbit);

  // Read entire stream into vector.
  std::vector<uint8_t> bytes;
  constexpr size_t chunk = 1024 * 1024;  // 1 MB chunks.
  while (stream) {
    const size_t pos = bytes.size();
    bytes.resize(pos + chunk);
    stream.read(reinterpret_cast<char*>(bytes.data() + pos), chunk);
    bytes.resize(pos + stream.gcount());
  }

  if (bytes.empty()) {
    throw std::runtime_error(std::string("OdbDelta: failed to read file: ")
                             + filename);
  }

  return bytes;
}

}  // namespace utl
