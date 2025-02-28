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

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace utl {

struct ClientMetric
{
  // Label

  struct Label
  {
    std::string name;
    std::string value;

    Label(std::string name_, std::string value_)
        : name(std::move(name_)), value(std::move(value_))
    {
    }

    bool operator<(const Label& rhs) const
    {
      return std::tie(name, value) < std::tie(rhs.name, rhs.value);
    }

    bool operator==(const Label& rhs) const
    {
      return std::tie(name, value) == std::tie(rhs.name, rhs.value);
    }
  };

  std::vector<Label> label;

  // Counter

  struct Counter
  {
    double value = 0.0;
  };

  Counter counter;

  // Gauge

  struct Gauge
  {
    double value = 0.0;
  };

  Gauge gauge;

  // Summary

  struct Quantile
  {
    double quantile = 0.0;
    double value = 0.0;
  };

  struct Summary
  {
    std::uint64_t sample_count = 0;
    double sample_sum = 0.0;
    std::vector<Quantile> quantile;
  };

  Summary summary;

  // Histogram

  struct Bucket
  {
    std::uint64_t cumulative_count = 0;
    double upper_bound = 0.0;
  };

  struct Histogram
  {
    std::uint64_t sample_count = 0;
    double sample_sum = 0.0;
    std::vector<Bucket> bucket;
  };

  Histogram histogram;

  // Untyped

  struct Untyped
  {
    double value = 0;
  };

  Untyped untyped;

  // Timestamp

  std::int64_t timestamp_ms = 0;
};

}  // namespace utl
