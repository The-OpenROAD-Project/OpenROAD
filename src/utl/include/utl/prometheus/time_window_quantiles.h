// MIT License

// Copyright (c) 2021 biaks (ianiskr@gmail.com)

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <chrono>
#include <cstddef>
#include <vector>

#include "ckms_quantiles.h"

namespace utl {
namespace detail {

class TimeWindowQuantiles
{
  using Clock = std::chrono::steady_clock;

 public:
  TimeWindowQuantiles(const std::vector<CKMSQuantiles::Quantile>& quantiles,
                      const Clock::duration max_age,
                      const int age_buckets)
      : quantiles_(quantiles),
        ckms_quantiles_(age_buckets, CKMSQuantiles(quantiles_)),
        last_rotation_(Clock::now()),
        rotation_interval_(max_age / age_buckets)
  {
  }

  double get(double q) const
  {
    CKMSQuantiles& current_bucket = rotate();
    return current_bucket.get(q);
  }

  void insert(double value)
  {
    rotate();
    for (auto& bucket : ckms_quantiles_) {
      bucket.insert(value);
    }
  }

 private:
  CKMSQuantiles& rotate() const
  {
    auto delta = Clock::now() - last_rotation_;
    while (delta > rotation_interval_) {
      ckms_quantiles_[current_bucket_].reset();

      if (++current_bucket_ >= ckms_quantiles_.size()) {
        current_bucket_ = 0;
      }

      delta -= rotation_interval_;
      last_rotation_ += rotation_interval_;
    }
    return ckms_quantiles_[current_bucket_];
  }

  const std::vector<CKMSQuantiles::Quantile>& quantiles_;
  mutable std::vector<CKMSQuantiles> ckms_quantiles_;
  mutable std::size_t current_bucket_{0};

  mutable Clock::time_point last_rotation_;
  const Clock::duration rotation_interval_;
};

}  // namespace detail
}  // namespace utl
