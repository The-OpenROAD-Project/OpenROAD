// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Public-domain SHA-256 + HMAC-SHA256.  Adapted from FIPS 180-4 reference
// pseudocode; kept short and dependency-free.

#include "HmacSha256.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace wmk {

namespace {

constexpr std::uint32_t K[64]
    = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
       0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
       0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
       0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
       0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
       0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
       0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
       0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
       0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
       0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
       0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

inline std::uint32_t rotr(std::uint32_t x, int n)
{
  return (x >> n) | (x << (32 - n));
}

struct Sha256Ctx
{
  std::uint32_t state[8];
  std::uint64_t bitlen;
  std::uint8_t buffer[64];
  std::size_t buffer_len;
};

void sha256_init(Sha256Ctx& c)
{
  c.state[0] = 0x6a09e667;
  c.state[1] = 0xbb67ae85;
  c.state[2] = 0x3c6ef372;
  c.state[3] = 0xa54ff53a;
  c.state[4] = 0x510e527f;
  c.state[5] = 0x9b05688c;
  c.state[6] = 0x1f83d9ab;
  c.state[7] = 0x5be0cd19;
  c.bitlen = 0;
  c.buffer_len = 0;
}

void sha256_transform(Sha256Ctx& c, const std::uint8_t block[64])
{
  std::uint32_t w[64];
  for (std::size_t i = 0; i < 16; ++i) {
    w[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24)
           | (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16)
           | (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8)
           | static_cast<std::uint32_t>(block[i * 4 + 3]);
  }
  for (int i = 16; i < 64; ++i) {
    std::uint32_t s0
        = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    std::uint32_t s1
        = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  std::uint32_t a = c.state[0], b = c.state[1], cc = c.state[2], d = c.state[3];
  std::uint32_t e = c.state[4], f = c.state[5], g = c.state[6], h = c.state[7];
  for (int i = 0; i < 64; ++i) {
    std::uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
    std::uint32_t ch = (e & f) ^ ((~e) & g);
    std::uint32_t t1 = h + S1 + ch + K[i] + w[i];
    std::uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
    std::uint32_t mj = (a & b) ^ (a & cc) ^ (b & cc);
    std::uint32_t t2 = S0 + mj;
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = cc;
    cc = b;
    b = a;
    a = t1 + t2;
  }
  c.state[0] += a;
  c.state[1] += b;
  c.state[2] += cc;
  c.state[3] += d;
  c.state[4] += e;
  c.state[5] += f;
  c.state[6] += g;
  c.state[7] += h;
}

void sha256_update(Sha256Ctx& c, const std::uint8_t* data, std::size_t len)
{
  while (len > 0) {
    std::size_t space = 64 - c.buffer_len;
    std::size_t take = (len < space) ? len : space;
    std::memcpy(c.buffer + c.buffer_len, data, take);
    c.buffer_len += take;
    data += take;
    len -= take;
    c.bitlen += static_cast<std::uint64_t>(take) * 8;
    if (c.buffer_len == 64) {
      sha256_transform(c, c.buffer);
      c.buffer_len = 0;
    }
  }
}

void sha256_final(Sha256Ctx& c, std::uint8_t out[32])
{
  std::uint64_t bitlen = c.bitlen;
  std::uint8_t pad = 0x80;
  sha256_update(c, &pad, 1);
  std::uint8_t zero = 0;
  while (c.buffer_len != 56) {
    sha256_update(c, &zero, 1);
  }
  std::uint8_t lenbuf[8];
  for (int i = 0; i < 8; ++i) {
    lenbuf[7 - i] = static_cast<std::uint8_t>(bitlen >> (i * 8));
  }
  sha256_update(c, lenbuf, 8);
  for (std::size_t i = 0; i < 8; ++i) {
    out[i * 4] = static_cast<std::uint8_t>(c.state[i] >> 24);
    out[i * 4 + 1] = static_cast<std::uint8_t>(c.state[i] >> 16);
    out[i * 4 + 2] = static_cast<std::uint8_t>(c.state[i] >> 8);
    out[i * 4 + 3] = static_cast<std::uint8_t>(c.state[i]);
  }
}

}  // namespace

std::array<std::uint8_t, 32> sha256(const std::string& msg)
{
  Sha256Ctx c;
  sha256_init(c);
  sha256_update(
      c, reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size());
  std::array<std::uint8_t, 32> out;
  sha256_final(c, out.data());
  return out;
}

void hmac_sha256(const std::uint8_t* key,
                 std::size_t key_len,
                 const std::uint8_t* msg,
                 std::size_t msg_len,
                 std::uint8_t out[32])
{
  std::uint8_t kpad[64];
  std::memset(kpad, 0, sizeof(kpad));
  if (key_len > 64) {
    Sha256Ctx c;
    sha256_init(c);
    sha256_update(c, key, key_len);
    sha256_final(c, kpad);
  } else {
    std::memcpy(kpad, key, key_len);
  }
  std::uint8_t ipad[64];
  std::uint8_t opad[64];
  for (int i = 0; i < 64; ++i) {
    ipad[i] = kpad[i] ^ 0x36;
    opad[i] = kpad[i] ^ 0x5c;
  }
  Sha256Ctx ci;
  sha256_init(ci);
  sha256_update(ci, ipad, 64);
  sha256_update(ci, msg, msg_len);
  std::uint8_t inner[32];
  sha256_final(ci, inner);

  Sha256Ctx co;
  sha256_init(co);
  sha256_update(co, opad, 64);
  sha256_update(co, inner, 32);
  sha256_final(co, out);
}

std::array<std::uint8_t, 32> hmac_sha256_key32(
    const std::array<std::uint8_t, 32>& key,
    const std::string& msg)
{
  std::array<std::uint8_t, 32> out;
  hmac_sha256(key.data(),
              32,
              reinterpret_cast<const std::uint8_t*>(msg.data()),
              msg.size(),
              out.data());
  return out;
}

bool parse_hex_key32(const std::string& hex,
                     std::array<std::uint8_t, 32>& key_out)
{
  if (hex.size() != 64) {
    return false;
  }
  auto nib = [](char c, int& v) {
    if (c >= '0' && c <= '9') {
      v = c - '0';
      return true;
    }
    if (c >= 'a' && c <= 'f') {
      v = 10 + (c - 'a');
      return true;
    }
    if (c >= 'A' && c <= 'F') {
      v = 10 + (c - 'A');
      return true;
    }
    return false;
  };
  for (std::size_t i = 0; i < 32; ++i) {
    int hi = 0, lo = 0;
    if (!nib(hex[i * 2], hi) || !nib(hex[i * 2 + 1], lo)) {
      return false;
    }
    key_out[i] = static_cast<std::uint8_t>((hi << 4) | lo);
  }
  return true;
}

std::string bytesPart(const std::array<std::uint8_t, 8>& value)
{
  return std::string(reinterpret_cast<const char*>(value.data()), value.size());
}

std::array<std::uint8_t, 32> hmac_digest(
    const std::array<std::uint8_t, 32>& key,
    const std::vector<std::string>& parts)
{
  // Build the length-prefixed message, then hash it in one call.
  std::vector<std::uint8_t> msg;
  for (const std::string& p : parts) {
    const std::uint32_t len = static_cast<std::uint32_t>(p.size());
    msg.push_back(static_cast<std::uint8_t>((len >> 24) & 0xff));
    msg.push_back(static_cast<std::uint8_t>((len >> 16) & 0xff));
    msg.push_back(static_cast<std::uint8_t>((len >> 8) & 0xff));
    msg.push_back(static_cast<std::uint8_t>(len & 0xff));
    msg.insert(msg.end(), p.begin(), p.end());
  }
  std::array<std::uint8_t, 32> out{};
  hmac_sha256(key.data(), key.size(), msg.data(), msg.size(), out.data());
  return out;
}

}  // namespace wmk
