// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/random_bytes.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace utl {

bool randomBytes(std::size_t n, std::vector<std::uint8_t>& out)
{
  out.assign(n, 0);
  if (n == 0) {
    return true;
  }
  // std::random_device may be deterministic on some toolchains, so read the
  // operating system's source directly.
  std::FILE* f = std::fopen("/dev/urandom", "rb");
  if (f == nullptr) {
    out.clear();
    return false;
  }
  std::setvbuf(f, nullptr, _IONBF, 0);  // read only the n bytes asked for
  const std::size_t got = std::fread(out.data(), 1, n, f);
  std::fclose(f);
  if (got != n) {
    out.clear();
    return false;
  }
  return true;
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

}  // namespace utl
