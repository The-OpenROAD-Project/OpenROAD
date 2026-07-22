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

#include <atomic>
#include <cstdint>

#include "utl/prometheus/atomic_floating.h"
#include "utl/prometheus/builder.h"
#include "utl/prometheus/client_metric.h"
#include "utl/prometheus/family.h"
#include "utl/prometheus/prometheus_metric.h"

namespace utl {

/// \brief A counter metric to represent a monotonically increasing value.
///
/// This class represents the metric type counter:
/// https://prometheus.io/docs/concepts/metric_types/#counter
///
/// The value of the counter can only increase. Example of counters are:
/// - the number of requests served
/// - tasks completed
/// - errors
///
/// Do not use a counter to expose a value that can decrease - instead use a
/// Gauge.
///
/// The class is thread-safe. No concurrent call to any API of this type causes
/// a data race.
template <typename Value_ = uint64_t>
class Counter : public PrometheusMetric
{
  std::atomic<Value_> value{};

 public:
  using Value = Value_;
  using Family = CustomFamily<Counter<Value>>;

  static const PrometheusMetric::Type static_type
      = PrometheusMetric::Type::Counter;

  Counter()
      : PrometheusMetric(PrometheusMetric::Type::Counter)
  {}  ///< \brief Create a counter that starts at 0.

  // original API

  void Increment()
  {  ///< \brief Increment the counter by 1.
    ++value;
  }

  void Increment(const Value& val)
  {  ///< \brief Increment the counter by a given amount. The counter will not
     ///< change if the given amount is negative.
    if (val > 0) {
      value += val;
    }
  }

  Value Get() const
  {  ///< \brief Get the current value of the counter.
    return value;
  }

  ClientMetric Collect() const override
  {  ///< /// \brief Get the current value of the counter. Collect is called by
     ///< the PrometheusRegistry when collecting metrics.
    ClientMetric metric;
    metric.counter.value = static_cast<double>(value);
    return metric;
  }

  // new API

  Counter& operator++()
  {
    ++value;
    return *this;
  }

  Counter& operator++(int)
  {
    ++value;
    return *this;
  }

  Counter& operator+=(const Value& val)
  {
    value += val;
    return *this;
  }
};

/// \brief Return a builder to configure and register a Counter metric.
///
/// @copydetails Family<>::Family()
///
/// Example usage:
///
/// \code
/// auto registry = std::make_shared<PrometheusRegistry>();
/// auto& counter_family = utl::BuildCounter()
///                            .Name("some_name")
///                            .Help("Additional description.")
///                            .Labels({{"key", "value"}})
///                            .Register(*registry);
///
/// ...
/// \endcode
///
/// \return An object of unspecified type T, i.e., an implementation detail
/// except that it has the following members:
///
/// - Name(const std::string&) to set the metric name,
/// - Help(const std::string&) to set an additional description.
/// - Label(const std::map<std::string, std::string>&) to assign a set of
///   key-value pairs (= labels) to the metric.
///
/// To finish the configuration of the Counter metric, register it with
/// Register(PrometheusRegistry&).
using BuildCounter = Builder<Counter<double>>;

}  // namespace utl
