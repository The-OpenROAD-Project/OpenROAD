// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace utl {

// Draw `n` bytes from the OS entropy source; false when it cannot be read.
// A caller drawing a secret must fail then, never fall back.
bool randomBytes(std::size_t n, std::vector<std::uint8_t>& out);

// Lowercase hex, two digits per byte.
std::string toHex(const std::uint8_t* data, std::size_t len);

}  // namespace utl
