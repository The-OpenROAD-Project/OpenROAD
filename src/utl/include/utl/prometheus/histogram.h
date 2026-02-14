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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "utl/prometheus/builder.h"
#include "utl/prometheus/client_metric.h"
#include "utl/prometheus/counter.h"
#include "utl/prometheus/family.h"
#include "utl/prometheus/gauge.h"
#include "utl/prometheus/prometheus_metric.h"

namespace utl {

/// \brief A histogram metric to represent aggregatable distributions of events.
///
/// This class represents the metric type histogram:
/// https://prometheus.io/docs/concepts/metric_types/#histogram
///
/// A histogram tracks the number of observations and the sum of the observed
/// values, allowing to calculate the average of the observed values.
///
/// At its core a histogram has a counter per bucket. The sum of observations
/// also behaves like a counter as long as there are no negative observations.
///
/// See https://prometheus.io/docs/practices/histograms/ for detailed
/// explanations of histogram usage and differences to summaries.
///
/// The class is thread-safe. No concurrent call to any API of this type causes
/// a data race.
template <typename Value_ = uint64_t>
class Histogram : public PrometheusMetric
{
 public:
  using Value = Value_;
  using BucketBoundaries = std::vector<Value>;
  using Family = CustomFamily<Histogram<Value>>;

  static const PrometheusMetric::Type static_type
      = PrometheusMetric::Type::Histogram;

  /// \brief Create a histogram with manually chosen buckets.
  ///
  /// The BucketBoundaries are a list of monotonically increasing values
  /// representing the bucket boundaries. Each consecutive pair of values is
  /// interpreted as a half-open interval [b_n, b_n+1) which defines one bucket.
  ///
  /// There is no limitation on how the buckets are divided, i.e, equal size,
  /// exponential etc..
  ///
  /// The bucket boundaries cannot be changed once the histogram is created.
  Histogram(const BucketBoundaries& buckets)
      : PrometheusMetric(static_type),
        bucket_boundaries_{buckets},
        bucket_counts_{buckets.size() + 1}
  {
    assert(std::is_sorted(std::begin(bucket_boundaries_),
                          std::end(bucket_boundaries_)));
  }

  /// \brief Observe the given amount.
  ///
  /// The given amount selects the 'observed' bucket. The observed bucket is
  /// chosen for which the given amount falls into the half-open interval [b_n,
  /// b_n+1). The counter of the observed bucket is incremented. Also the total
  /// sum of all observations is incremented.
  void Observe(const Value value)
  {
    // TODO: determine bucket list size at which binary search would be faster
    const auto bucket_index = static_cast<std::size_t>(std::distance(
        bucket_boundaries_.begin(),
        std::find_if(
            std::begin(bucket_boundaries_),
            std::end(bucket_boundaries_),
            [value](const Value boundary) { return boundary >= value; })));
    sum_.Increment(value);
    bucket_counts_[bucket_index].Increment();
  }

  /// \brief Observe multiple data points.
  ///
  /// Increments counters given a count for each bucket. (i.e. the caller of
  /// this function must have already sorted the values into buckets).
  /// Also increments the total sum of all observations by the given value.
  void ObserveMultiple(const std::vector<Value>& bucket_increments,
                       const Value sum_of_values)
  {
    if (bucket_increments.size() != bucket_counts_.size()) {
      throw std::length_error(
          "The size of bucket_increments was not equal to"
          "the number of buckets in the histogram.");
    }

    sum_.Increment(sum_of_values);

    for (std::size_t i{0}; i < bucket_counts_.size(); ++i) {
      bucket_counts_[i].Increment(bucket_increments[i]);
    }
  }

  /// \brief Get the current value of the counter.
  ///
  /// Collect is called by the PrometheusRegistry when collecting metrics.
  ClientMetric Collect() const override
  {
    auto metric = ClientMetric{};

    auto cumulative_count = 0ULL;
    metric.histogram.bucket.reserve(bucket_counts_.size());
    for (std::size_t i{0}; i < bucket_counts_.size(); ++i) {
      cumulative_count += static_cast<std::size_t>(bucket_counts_[i].Get());
      auto bucket = ClientMetric::Bucket{};
      bucket.cumulative_count = cumulative_count;
      bucket.upper_bound = i == bucket_boundaries_.size()
                               ? std::numeric_limits<double>::infinity()
                               : static_cast<double>(bucket_boundaries_[i]);
      metric.histogram.bucket.push_back(bucket);
    }
    metric.histogram.sample_count = cumulative_count;
    metric.histogram.sample_sum = static_cast<double>(sum_.Get());

    return metric;
  }

 private:
  const BucketBoundaries bucket_boundaries_;
  std::vector<Counter<Value_>> bucket_counts_;
  Gauge<Value_> sum_{};
};

/// \brief Return a builder to configure and register a Histogram metric.
///
/// @copydetails Family<>::Family()
///
/// Example usage:
///
/// \code
/// auto registry = std::make_shared<PrometheusRegistry>();
/// auto& histogram_family = utl::BuildHistogram()
///                              .Name("some_name")
///                              .Help("Additional description.")
///                              .Labels({{"key", "value"}})
///                              .Register(*registry);
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
/// To finish the configuration of the Histogram metric register it with
/// Register(PrometheusRegistry&).
using BuildHistogram = Builder<Histogram<double>>;

}  // namespace utl
