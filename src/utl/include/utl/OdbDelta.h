// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <vector>

namespace utl {

struct OdbDelta
{
  static constexpr uint32_t MAGIC = 0x4F444244;  // "ODBD"
  static constexpr uint32_t VERSION = 1;
  static constexpr uint32_t DEFAULT_BLOCK_SIZE = 4096;
};

// Compute delta between base and current serialized .odb bytes.
// Returns self-describing delta file content.
std::vector<uint8_t> computeOdbDelta(
    const std::vector<uint8_t>& base,
    const std::vector<uint8_t>& current,
    uint32_t block_size = OdbDelta::DEFAULT_BLOCK_SIZE);

// Apply delta to base bytes. Returns reconstructed full .odb bytes.
std::vector<uint8_t> applyOdbDelta(const std::vector<uint8_t>& base,
                                   const std::vector<uint8_t>& delta);

// Read entire file into byte vector (handles .gz via InStreamHandler).
std::vector<uint8_t> readFileBytes(const char* filename);

}  // namespace utl
