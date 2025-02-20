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
#include <stdexcept>

#include "client_metric.h"
#include "family.h"
#include "prometheus_metric.h"

namespace utl {

class Benchmark : public PrometheusMetric
{
#ifndef NDEBUG
  bool already_started = false;
#endif

  std::chrono::time_point<std::chrono::high_resolution_clock> start_;
  std::chrono::time_point<std::chrono::high_resolution_clock>::duration elapsed
      = std::chrono::time_point<std::chrono::high_resolution_clock>::duration::
          zero();  // elapsed time

 public:
  using Value = double;
  using Family = CustomFamily<Benchmark>;

  static const PrometheusMetric::Type static_type
      = PrometheusMetric::Type::Counter;

  Benchmark() : PrometheusMetric(PrometheusMetric::Type::Counter) {}

  void start()
  {
#ifndef NDEBUG
    if (already_started) {
      throw std::runtime_error("try to start already started counter");
    }
    already_started = true;
#endif

    start_ = std::chrono::high_resolution_clock::now();
  }

  void stop()
  {
#ifndef NDEBUG
    if (already_started == false) {
      throw std::runtime_error("try to stop already stoped counter");
    }
#endif

    std::chrono::time_point<std::chrono::high_resolution_clock> stop;
    stop = std::chrono::high_resolution_clock::now();
    elapsed += stop - start_;

#ifndef NDEBUG
    already_started = false;
#endif
  }

  double Get() const
  {
    return std::chrono::duration_cast<std::chrono::duration<double>>(elapsed)
        .count();
  }

  ClientMetric Collect() const override
  {
    ClientMetric metric;
    metric.counter.value = Get();
    return metric;
  }
};

}  // namespace utl
