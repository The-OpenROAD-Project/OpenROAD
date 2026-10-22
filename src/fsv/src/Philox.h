// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <cstdint>

namespace fsv {

// Counter-based Philox4x32-10 generator (Salmon et al., SC'11). Each walk
// gets its own (key, stream) pair, so its random numbers do not depend on
// which thread runs it.
class Philox
{
 public:
  Philox(const uint64_t key, const uint64_t stream)
      : key_{static_cast<uint32_t>(key), static_cast<uint32_t>(key >> 32)},
        ctr_{static_cast<uint32_t>(stream),
             static_cast<uint32_t>(stream >> 32),
             0,
             0}
  {
  }

  // Uniform in (0, 1).
  double uniform()
  {
    if (next_ == 4) {
      refill();
    }
    return (out_[next_++] + 0.5) * 0x1p-32;
  }

 private:
  void refill()
  {
    std::array<uint32_t, 4> c = ctr_;
    std::array<uint32_t, 2> k = key_;
    for (int round = 0; round < 10; round++) {
      const uint64_t p0 = uint64_t{0xD2511F53u} * c[0];
      const uint64_t p1 = uint64_t{0xCD9E8D57u} * c[2];
      c = {static_cast<uint32_t>(p1 >> 32) ^ c[1] ^ k[0],
           static_cast<uint32_t>(p1),
           static_cast<uint32_t>(p0 >> 32) ^ c[3] ^ k[1],
           static_cast<uint32_t>(p0)};
      k[0] += 0x9E3779B9u;
      k[1] += 0xBB67AE85u;
    }
    out_ = c;
    next_ = 0;
    if (++ctr_[2] == 0) {
      ++ctr_[3];
    }
  }

  std::array<uint32_t, 2> key_;
  std::array<uint32_t, 4> ctr_;
  std::array<uint32_t, 4> out_{};
  int next_ = 4;
};

}  // namespace fsv
