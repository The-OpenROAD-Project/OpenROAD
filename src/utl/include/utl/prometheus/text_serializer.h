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

#include <array>
#include <cmath>
#include <iosfwd>
#include <limits>
#include <ostream>
#include <string>
#include <vector>

#include "metric_family.h"

#if __cpp_lib_to_chars >= 201611L
#include <charconv>
#endif

namespace utl {

class TextSerializer
{
  // Write a double as a string, with proper formatting for infinity and NaN
  static void WriteValue(std::ostream& out, double value)
  {
    if (std::isnan(value)) {
      out << "Nan";
    } else if (std::isinf(value)) {
      out << (value < 0 ? "-Inf" : "+Inf");
    } else {
      std::array<char, 128> buffer;

#if __cpp_lib_to_chars >= 201611L
      auto [ptr, ec]
          = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
      if (ec != std::errc()) {
        throw std::runtime_error("Could not convert double to string: "
                                 + std::make_error_code(ec).message());
      }
      out.write(buffer.data(), ptr - buffer.data());
#else
      int wouldHaveWritten
          = std::snprintf(buffer.data(),
                          buffer.size(),
                          "%.*g",
                          std::numeric_limits<double>::max_digits10 - 1,
                          value);
      if (wouldHaveWritten <= 0
          || static_cast<std::size_t>(wouldHaveWritten) >= buffer.size()) {
        throw std::runtime_error("Could not convert double to string");
      }
      out.write(buffer.data(), wouldHaveWritten);
#endif
    }
  }

  static void WriteValue(std::ostream& out, const std::string& value)
  {
    for (auto c : value) {
      switch (c) {
        case '\n':
          out << '\\' << 'n';
          break;
        case '\\':
          out << '\\' << c;
          break;
        case '"':
          out << '\\' << c;
          break;
        default:
          out << c;
          break;
      }
    }
  }

  // Write a line header: metric name and labels
  template <typename T = std::string>
  static void WriteHead(std::ostream& out,
                        const MetricFamily& family,
                        const ClientMetric& metric,
                        const std::string& suffix = "",
                        const std::string& extraLabelName = "",
                        const T& extraLabelValue = T())
  {
    out << family.name << suffix;

    if (!metric.label.empty() || !extraLabelName.empty()) {
      out << "{";
      const char* prefix = "";

      for (auto& lp : metric.label) {
        out << prefix << lp.name << "=\"";
        WriteValue(out, lp.value);
        out << "\"";
        prefix = ",";
      }
      if (!extraLabelName.empty()) {
        out << prefix << extraLabelName << "=\"";
        WriteValue(out, extraLabelValue);
        out << "\"";
      }
      out << "}";
    }
    out << " ";
  }

  // Write a line trailer: timestamp
  static void WriteTail(std::ostream& out, const ClientMetric& metric)
  {
    if (metric.timestamp_ms != 0) {
      out << " " << metric.timestamp_ms;
    }
    out << "\n";
  }

  static void SerializeCounter(std::ostream& out,
                               const MetricFamily& family,
                               const ClientMetric& metric)
  {
    WriteHead(out, family, metric);
    WriteValue(out, metric.counter.value);
    WriteTail(out, metric);
  }

  static void SerializeGauge(std::ostream& out,
                             const MetricFamily& family,
                             const ClientMetric& metric)
  {
    WriteHead(out, family, metric);
    WriteValue(out, metric.gauge.value);
    WriteTail(out, metric);
  }

  static void SerializeSummary(std::ostream& out,
                               const MetricFamily& family,
                               const ClientMetric& metric)
  {
    auto& sum = metric.summary;
    WriteHead(out, family, metric, "_count");
    out << sum.sample_count;
    WriteTail(out, metric);

    WriteHead(out, family, metric, "_sum");
    WriteValue(out, sum.sample_sum);
    WriteTail(out, metric);

    for (auto& q : sum.quantile) {
      WriteHead(out, family, metric, "", "quantile", q.quantile);
      WriteValue(out, q.value);
      WriteTail(out, metric);
    }
  }

  static void SerializeUntyped(std::ostream& out,
                               const MetricFamily& family,
                               const ClientMetric& metric)
  {
    WriteHead(out, family, metric);
    WriteValue(out, metric.untyped.value);
    WriteTail(out, metric);
  }

  static void SerializeHistogram(std::ostream& out,
                                 const MetricFamily& family,
                                 const ClientMetric& metric)
  {
    auto& hist = metric.histogram;
    WriteHead(out, family, metric, "_count");
    out << hist.sample_count;
    WriteTail(out, metric);

    WriteHead(out, family, metric, "_sum");
    WriteValue(out, hist.sample_sum);
    WriteTail(out, metric);

    double last = -std::numeric_limits<double>::infinity();
    for (auto& b : hist.bucket) {
      WriteHead(out, family, metric, "_bucket", "le", b.upper_bound);
      last = b.upper_bound;
      out << b.cumulative_count;
      WriteTail(out, metric);
    }

    if (last != std::numeric_limits<double>::infinity()) {
      WriteHead(out, family, metric, "_bucket", "le", "+Inf");
      out << hist.sample_count;
      WriteTail(out, metric);
    }
  }

  static void SerializeFamily(std::ostream& out, const MetricFamily& family)
  {
    if (!family.help.empty()) {
      out << "# HELP " << family.name << " " << family.help << "\n";
    }
    switch (family.type) {
      case PrometheusMetric::Type::Counter:
        out << "# TYPE " << family.name << " counter\n";
        for (auto& metric : family.metric) {
          SerializeCounter(out, family, metric);
        }
        break;
      case PrometheusMetric::Type::Gauge:
        out << "# TYPE " << family.name << " gauge\n";
        for (auto& metric : family.metric) {
          SerializeGauge(out, family, metric);
        }
        break;
      case PrometheusMetric::Type::Summary:
        out << "# TYPE " << family.name << " summary\n";
        for (auto& metric : family.metric) {
          SerializeSummary(out, family, metric);
        }
        break;
      case PrometheusMetric::Type::Untyped:
        out << "# TYPE " << family.name << " untyped\n";
        for (auto& metric : family.metric) {
          SerializeUntyped(out, family, metric);
        }
        break;
      case PrometheusMetric::Type::Histogram:
        out << "# TYPE " << family.name << " histogram\n";
        for (auto& metric : family.metric) {
          SerializeHistogram(out, family, metric);
        }
        break;
    }
  }

 public:
  static void Serialize(std::ostream& out,
                        const std::vector<MetricFamily>& metrics)
  {
    std::locale saved_locale = out.getloc();
    out.imbue(std::locale::classic());
    for (auto& family : metrics) {
      SerializeFamily(out, family);
    }
    out.imbue(saved_locale);
  }
};

}  // namespace utl
