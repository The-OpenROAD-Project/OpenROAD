// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Deterministic 64-bit checksums for L-IR objects, modules and designs.
// Unlike absl::Hash they use no random seed, so results are reproducible.
//
// checksumCombine() is order-dependent. Do not fold the numeric value of an
// unordered pool id (NameId, ConstId, ...); fold the checksum of the entry
// it refers to instead.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace syn::lir {

inline constexpr uint64_t kChecksumSeed = 0x9e3779b97f4a7c15ULL;

// Bijective 64-bit finalizer (splitmix64).
constexpr uint64_t checksumMix(uint64_t x)
{
  x ^= x >> 30u;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27u;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31u;
  return x;
}

// Order-dependent (non-commutative) combination of `value` into `seed`.
constexpr uint64_t checksumCombine(uint64_t seed, uint64_t value)
{
  return checksumMix(
      seed
      ^ (checksumMix(value) + kChecksumSeed + (seed << 6u) + (seed >> 2u)));
}

// Order-dependent combination of `bytes` (length followed by little-endian
// 8-byte chunks) into `seed`.
constexpr uint64_t checksumBytes(uint64_t seed, std::string_view bytes)
{
  uint64_t h = checksumCombine(seed, bytes.size());
  size_t i = 0;
  while (i + 8 <= bytes.size()) {
    uint64_t word = 0;
    for (size_t b = 0; b < 8; ++b) {
      word |= static_cast<uint64_t>(static_cast<uint8_t>(bytes[i + b]))
              << (8 * b);
    }
    h = checksumCombine(h, word);
    i += 8;
  }
  if (i < bytes.size()) {
    uint64_t tail = 0;
    for (size_t b = 0; i + b < bytes.size(); ++b) {
      tail |= static_cast<uint64_t>(static_cast<uint8_t>(bytes[i + b]))
              << (8 * b);
    }
    h = checksumCombine(h, tail);
  }
  return h;
}

}  // namespace syn::lir
