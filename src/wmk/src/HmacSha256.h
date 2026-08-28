// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Self-contained HMAC-SHA256 used by the routing watermark.  No external
// dependencies (avoids pulling OpenSSL into the wmk module).  Output is the
// raw 32-byte HMAC.  Tested against RFC 4231 test vectors in the cpp file.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wmk {

// SHA-256 of a byte string.  The routing watermark uses this to turn a public
// design identifier into the seed of its randomization null, so that the null
// is reproducible by anyone and depends on nothing secret.
std::array<std::uint8_t, 32> sha256(const std::string& msg);

// Compute HMAC-SHA256(key, msg) and write the 32-byte digest into out.
void hmac_sha256(const std::uint8_t* key,
                 std::size_t key_len,
                 const std::uint8_t* msg,
                 std::size_t msg_len,
                 std::uint8_t out[32]);

// Convenience overload: key is 32 bytes, msg is a std::string.
std::array<std::uint8_t, 32> hmac_sha256_key32(
    const std::array<std::uint8_t, 32>& key,
    const std::string& msg);

// Parse a 64-character hex string into a 32-byte key.  Returns false on
// invalid input (length, non-hex characters); on success the parsed bytes are
// written to key_out.
bool parse_hex_key32(const std::string& hex,
                     std::array<std::uint8_t, 32>& key_out);

// Length-prefixed HMAC-SHA256 over a sequence of parts.  Each part is preceded
// by its big-endian uint32 length, so distinct part tuples can never collide by
// concatenating to the same byte string -- ("ab","c") and ("a","bc") hash
// differently.  The placement and CTS watermarks derive their target values
// this way.
//
// The routing watermark deliberately does not: it hashes "net\0" || name
// directly, and changing either construction would invalidate every watermark
// already embedded with it.
std::array<std::uint8_t, 32> hmac_digest(
    const std::array<std::uint8_t, 32>& key,
    const std::vector<std::string>& parts);

// The tile a placement candidate sits in, as the PRF sees it: two big-endian
// int32.  Returned as bytes rather than a string because that is what it is --
// a fixed-width value, not text -- and the conversion to a hashed part happens
// at the call site.
std::string bytesPart(const std::array<std::uint8_t, 8>& value);

}  // namespace wmk
