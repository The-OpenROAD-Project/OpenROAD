// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/Checksum.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace syn::lir {

uint64_t checksumBytes(uint64_t seed, std::string_view bytes)
{
  uint64_t h = checksumCombine(seed, bytes.size());
  size_t i = 0;
  while (i + 8 <= bytes.size()) {
    uint64_t word = 0;
    for (size_t b = 0; b < 8; ++b) {
      word |= static_cast<uint64_t>(static_cast<uint8_t>(bytes[i + b]))
              << (8u * b);
    }
    h = checksumCombine(h, word);
    i += 8;
  }
  if (i < bytes.size()) {
    uint64_t tail = 0;
    for (size_t b = 0; i + b < bytes.size(); ++b) {
      tail |= static_cast<uint64_t>(static_cast<uint8_t>(bytes[i + b]))
              << (8u * b);
    }
    h = checksumCombine(h, tail);
  }
  return h;
}

}  // namespace syn::lir
