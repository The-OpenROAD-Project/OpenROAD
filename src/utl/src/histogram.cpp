// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "utl/histogram.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

#include "utl/Logger.h"

namespace utl {

template <typename DataType>
Histogram<DataType>::Histogram(Logger* logger) : logger_(logger)
{
  bins_.resize(1, 0);
}

template <typename DataType>
void Histogram<DataType>::generateBins(int bins,
                                       std::optional<DataType> bin_min,
                                       std::optional<DataType> bin_width)
{
  if (bins <= 0) {
    logger_->error(UTL, 70, "The number of bins must be positive.");
  }

  // Reset and populate each bin with count.
  std::ranges::fill(bins_, 0);
  bins_.resize(bins, 0);

  if (!hasData()) {
    logger_->warn(UTL, 71, "No histogram data available in the design.");
    return;
  }

  std::sort(data_.begin(), data_.end());

  min_val_ = bin_min.value_or(data_.front());
  bin_width_ = bin_width.value_or(computeBinWidth());

  if (bin_width_ == 0) {  // Special case for no variation in the data.
    bins_[0] = data_.size();
    return;
  }

  for (const auto& val : data_) {
    bins_[getBinIndex(val)]++;
  }
}

template <typename DataType>
void Histogram<DataType>::report(int precision) const
{
  if (!hasData()) {
    logger_->warn(UTL, 72, "The histogram is empty.");
    return;
  }

  const int num_bins = getBinsCount();
  const int largest_bin = *std::ranges::max_element(bins_);

  int bin_label_width = 0;
  for (int bin = 0; bin <= num_bins; bin++) {
    const DataType bin_val = min_val_ + bin * bin_width_;
    bin_label_width
        = std::max(static_cast<int>(formatBin(bin_val, 0, precision).size()),
                   bin_label_width);
  }

  // Print the histogram.
  for (int bin = 0; bin < num_bins; bin++) {
    const auto& [bin_start, bin_end] = getBinRange(bin);
    int bar_length  // Round the bar length to its closest value.
        = (max_bin_print_width_ * bins_[bin] + largest_bin / 2) / largest_bin;
    if (bar_length == 0 && bins_[bin] > 0) {
      bar_length = 1;  // Better readability when non-zero bins have a bar.
    }
    logger_->report("[{}, {}{}: {} ({})",
                    formatBin(bin_start, bin_label_width, precision),
                    formatBin(bin_end, bin_label_width, precision),
                    // The final bin is also closed from the right.
                    bin == num_bins - 1 ? "]" : ")",
                    std::string(bar_length, '*'),
                    bins_[bin]);
  }
}

template <typename DataType>
int Histogram<DataType>::getBinIndex(DataType val) const
{
  if (bin_width_ == 0) {
    return 0;
  }

  const int bins = getBinsCount();

  int bin = static_cast<int>((val - min_val_) / static_cast<float>(bin_width_));
  if (bin >= bins) {  // Special case for val with the maximum value.
    bin = bins - 1;
  }
  if (bin < 0) {
    return 0;
  }

  return bin;
}

template <typename DataType>
std::pair<DataType, DataType> Histogram<DataType>::getBinRange(int idx) const
{
  const DataType bin_start = min_val_ + idx * bin_width_;
  const DataType bin_end = min_val_ + (idx + 1) * bin_width_;

  return {bin_start, bin_end};
}

template <>
int Histogram<int>::computeBinWidth() const
{
  const float width
      = (data_.back() - getBinsMinimum()) / static_cast<float>(getBinsCount());
  return std::ceil(width);
}

template <>
std::string Histogram<int>::formatBin(int val, int width, int precision) const
{
  return fmt::format("{:>{}}", val, width);
}

template <>
float Histogram<float>::computeBinWidth() const
{
  return (data_.back() - getBinsMinimum()) / getBinsCount();
}

template <>
std::string Histogram<float>::formatBin(float val,
                                        int width,
                                        int precision) const
{
  return fmt::format("{:>{}.{}f}", val, width, precision);
}

template class Histogram<float>;
template class Histogram<int>;

}  // namespace utl
