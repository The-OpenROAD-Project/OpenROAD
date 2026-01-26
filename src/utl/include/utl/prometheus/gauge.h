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
#include <ctime>

#include "utl/prometheus/atomic_floating.h"
#include "utl/prometheus/builder.h"
#include "utl/prometheus/client_metric.h"
#include "utl/prometheus/family.h"
#include "utl/prometheus/prometheus_metric.h"

namespace utl {

/// \brief A gauge metric to represent a value that can arbitrarily go up and
/// down.
///
/// The class represents the metric type gauge:
/// https://prometheus.io/docs/concepts/metric_types/#gauge
///
/// Gauges are typically used for measured values like temperatures or current
/// memory usage, but also "counts" that can go up and down, like the number of
/// running processes.
///
/// The class is thread-safe. No concurrent call to any API of this type causes
/// a data race.
template <typename Value_ = uint64_t>
class Gauge : public PrometheusMetric
{
  std::atomic<Value_> value{0};

 public:
  using Value = Value_;
  using Family = CustomFamily<Gauge<Value>>;

  static const PrometheusMetric::Type static_type
      = PrometheusMetric::Type::Gauge;

  Gauge()
      : PrometheusMetric(static_type)
  {}  ///< \brief Create a gauge that starts at 0.
  Gauge(const Value value_)
      : PrometheusMetric(static_type),
        value{value_}
  {}  ///< \brief Create a gauge that starts at the given amount.

  // original API

  void Increment() { ++value; }  ///< \brief Increment the gauge by 1.
  void Increment(const Value& val)
  {
    value += val;
  }  ///< \brief Increment the gauge by the given amount.

  void Decrement() { --value; }  ///< \brief Decrement the gauge by 1.
  void Decrement(const Value& val)
  {
    value -= val;
  }  ///< \brief Decrement the gauge by the given amount.

  void SetToCurrentTime()
  {  ///< \brief Set the gauge to the current unixtime in seconds.
    const time_t time = std::time(nullptr);
    value = static_cast<Value>(time);
  }
  void Set(const Value& val)
  {
    value = val;
  }  ///< \brief Set the gauge to the given value.
  Value Get() const
  {
    return value;
  }  ///< \brief Get the current value of the gauge.

  ClientMetric Collect() const override
  {  ///< \brief Get the current value of the gauge. Collect is called by the
     ///< PrometheusRegistry when collecting metrics.
    ClientMetric metric;
    metric.gauge.value = static_cast<double>(value);
    return metric;
  }

  // new API

  Gauge& operator++()
  {
    ++value;
    return *this;
  }

  Gauge& operator++(int)
  {
    ++value;
    return *this;
  }

  Gauge& operator--()
  {
    --value;
    return *this;
  }

  Gauge& operator--(int)
  {
    --value;
    return *this;
  }

  Gauge& operator+=(const Value& val)
  {
    value += val;
    return *this;
  }

  Gauge& operator-=(const Value& val)
  {
    value -= val;
    return *this;
  }
};

/// \brief Return a builder to configure and register a Gauge metric.
///
/// @copydetails Family<>::Family()
///
/// Example usage:
///
/// \code
/// auto registry = std::make_shared<PrometheusRegistry>();
/// auto& gauge_family = utl::BuildGauge()
///                          .Name("some_name")
///                          .Help("Additional description.")
///                          .Labels({{"key", "value"}})
///                          .Register(*registry);
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
/// To finish the configuration of the Gauge metric register it with
/// Register(PrometheusRegistry&).
using BuildGauge = Builder<Gauge<double>>;

}  // namespace utl
