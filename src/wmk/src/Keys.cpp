// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Keys.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "HmacSha256.h"

namespace wmk {

namespace {

const char* const kStages[] = {"placement", "cts", "routing"};

}  // namespace

bool randomBytes(std::size_t n, std::vector<std::uint8_t>& out)
{
  out.assign(n, 0);
  if (n == 0) {
    return true;
  }
  // Read the operating system's entropy source directly.  The C++ standard
  // library's random_device is not required to be non-deterministic, and on
  // some toolchains it is not, so it must not be used to draw a secret key.
  std::FILE* f = std::fopen("/dev/urandom", "rb");
  if (f == nullptr) {
    out.clear();
    return false;
  }
  const std::size_t got = std::fread(out.data(), 1, n, f);
  std::fclose(f);
  if (got != n) {
    out.clear();
    return false;
  }
  return true;
}

std::array<std::uint8_t, 32> deriveStageKey(
    const std::array<std::uint8_t, 32>& master,
    const std::string& design_id,
    const std::vector<std::uint8_t>& nonce,
    const std::string& stage)
{
  const std::string nonce_str(nonce.begin(), nonce.end());
  return hmac_digest(master, {design_id, nonce_str, "stage=" + stage});
}

bool isWatermarkStage(const std::string& stage)
{
  for (const char* s : kStages) {
    if (stage == s) {
      return true;
    }
  }
  return false;
}

std::string toHex(const std::uint8_t* data, std::size_t len)
{
  static const char* kDigits = "0123456789abcdef";
  std::string out;
  out.reserve(len * 2);
  for (std::size_t i = 0; i < len; ++i) {
    out.push_back(kDigits[data[i] >> 4]);
    out.push_back(kDigits[data[i] & 0x0f]);
  }
  return out;
}

}  // namespace wmk
