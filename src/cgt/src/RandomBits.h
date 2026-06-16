// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <random>

namespace cgt {

class RandomBits
{
 public:
  bool get()
  {
    if (bits_remaining_ == 0) {
      bit_buffer_ = generator_();
      bits_remaining_ = sizeof(bit_buffer_) * 8;
    }

    bool bit = bit_buffer_ & 1;
    bit_buffer_ >>= 1;
    bits_remaining_--;

    return bit;
  }

 private:
  std::mt19937 generator_;
  uint32_t bit_buffer_ = 0;
  uint32_t bits_remaining_ = 0;
};

}  // namespace cgt
