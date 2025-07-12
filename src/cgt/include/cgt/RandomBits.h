// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <random>

namespace cgt {

class RandomBits
{
 private:
  std::minstd_rand generator{std::random_device{}()};
  uint32_t bit_buffer = 0;
  uint32_t bits_remaining = 0;

 public:
  bool get()
  {
    if (bits_remaining == 0) {
      bit_buffer = generator();
      bits_remaining = sizeof(bit_buffer) * 8;
    }

    bool bit = bit_buffer & 1;
    bit_buffer >>= 1;
    bits_remaining--;

    return bit;
  }

  void seed(uint32_t new_seed)
  {
    generator.seed(new_seed);
    bits_remaining = 0;
  }
};

}  // namespace cgt
