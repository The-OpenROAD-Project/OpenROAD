// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "utl/histogram.h"

#include "utl/Logger.h"

namespace utl {

template <typename DataType>
Histogram<DataType>::Histogram(Logger* logger, int bins) : logger_(logger)
{
  if (bins <= 0) {
    logger_->error(UTL, 70, "The number of bins must be positive.");
  }

  // Populate each bin with count.
  bins_.resize(bins, 0);
}

template <typename DataType>
void Histogram<DataType>::generateBins()
{
  if (!hasData()) {
    logger_->warn(UTL, 71, "No data for the histogram has been loaded.");
    return;
  }

  std::sort(data_.begin(), data_.end());

  min_val_ = data_.front();

  bin_width_ = computeBinWidth();
  const int bins = getBinCount();

  if (bin_width_ == 0) {  // Special case for no variation in the data.
    bins_[0] = data_.size();
    return;
  }

  const float bin_width = bin_width_;

  for (const auto& val : data_) {
    int bin = static_cast<int>((val - min_val_) / bin_width);
    if (bin >= bins) {  // Special case for val with the maximum value.
      bin = bins - 1;
    }
    bins_[bin]++;
  }
}

template <typename DataType>
void Histogram<DataType>::report(int precision) const
{
  if (!hasData()) {
    logger_->error(UTL, 72, "No data for the histogram has been loaded.");
    return;
  }

  const int num_bins = getBinCount();
  const int largest_bin = *std::max_element(bins_.begin(), bins_.end());

  int bin_label_width = 0;
  for (int bin = 0; bin <= num_bins; bin++) {
    const DataType bin_val = min_val_ + bin * bin_width_;
    bin_label_width
        = std::max(static_cast<int>(formatBin(bin_val, 0, precision).size()),
                   bin_label_width);
  }

  // Print the histogram.
  for (int bin = 0; bin < num_bins; ++bin) {
    const DataType bin_start = min_val_ + bin * bin_width_;
    const DataType bin_end = min_val_ + (bin + 1) * bin_width_;
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

template <>
int Histogram<int>::computeBinWidth() const
{
  const float width
      = (data_.back() - getBinMinimum()) / static_cast<float>(getBinCount());
  return std::ceil(width);
}

template <>
std::string Histogram<int>::formatBin(int pt, int width, int precision) const
{
  return fmt::format("{:>{}}", pt, width);
}

template <>
float Histogram<float>::computeBinWidth() const
{
  return (data_.back() - getBinMinimum()) / getBinCount();
}

template <>
std::string Histogram<float>::formatBin(float pt,
                                        int width,
                                        int precision) const
{
  return fmt::format("{:>{}.{}f}", pt, width, precision);
}

template class Histogram<float>;
template class Histogram<int>;

}  // namespace utl
