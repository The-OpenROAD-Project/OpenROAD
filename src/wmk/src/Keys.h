// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// The watermark's secret key: drawing it, and deriving the per-stage keys
// from it.
//
// One secret key is held by the owner.  Each stage gets its own key derived
// from it, so a key that leaks -- or is disclosed to prove one stage in court
// -- does not expose the others, and so the same secret key can protect
// several designs without the marks on one revealing the marks on another.
// The design identifier and the nonce are what separate them.
//
// Nothing here is written to the database or persisted by the module.  The
// secret key exists for as long as the command that uses it.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace wmk {

// Draw ``n`` bytes from the system's cryptographic random source.  Returns
// false if that source cannot be read, which is a hard failure: a key drawn
// from a predictable source is not secret, and quietly falling back to an
// ordinary generator would produce watermarks anyone could reproduce.
bool randomBytes(std::size_t n, std::vector<std::uint8_t>& out);

// The per-stage key of the PDMarks scheme:
//
//     K_s = HMAC-SHA256(K, ID(D_0), nu, "stage=" || s)
//
// The design identifier and nonce bind the derived key to one design version
// and one watermark instance, so re-running the flow on a revised design with
// the same secret key marks different objects.
std::array<std::uint8_t, 32> deriveStageKey(
    const std::array<std::uint8_t, 32>& master,
    const std::string& design_id,
    const std::vector<std::uint8_t>& nonce,
    const std::string& stage);

// Is this one of the three stages a key can be derived for?
bool isWatermarkStage(const std::string& stage);

// Lowercase hex.  The inverse is hexToBytes in HmacSha256.h.
std::string toHex(const std::uint8_t* data, std::size_t len);

}  // namespace wmk
