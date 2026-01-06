// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace utl {

class Logger;

// This class can be instantiated with DataType=<int|float> (as implemented in
// histogram.cpp)
template <typename DataType>
class Histogram
{
 public:
  Histogram(Logger* logger);
  virtual ~Histogram() = default;

  // Prints the histogram to the log. precision is used to control
  // the number of digits when displaying each bin's range.
  void report(int precision = 0) const;

  DataType getMinValue() const
  {
    return *std::min_element(data_.begin(), data_.end());
  }
  DataType getMaxValue() const
  {
    return *std::max_element(data_.begin(), data_.end());
  }
  int getMinBinCount() const { return *std::ranges::min_element(bins_); }
  int getMaxBinCount() const { return *std::ranges::max_element(bins_); }

  DataType getBinsWidth() const { return bin_width_; }
  DataType getBinsMinimum() const { return min_val_; }
  DataType getBinsRange() const { return getBinsCount() * getBinsWidth(); }
  DataType getBinsMaximum() const { return getBinsRange() + getBinsMinimum(); }
  int getBinsCount() const { return bins_.size(); }
  int getBinIndex(DataType val) const;
  std::pair<DataType, DataType> getBinRange(int idx) const;

  const std::vector<int>& getBins() const { return bins_; }
  void clearData() { data_.clear(); }
  bool hasData() const { return !data_.empty(); }
  const std::vector<DataType>& getData() const { return data_; }

  Logger* getLogger() const { return logger_; }

  // populate needs to call this to add data to the histogram
  void addData(DataType data) { data_.push_back(data); }
  // called after populate to build the bins data
  void generateBins(int bins,
                    std::optional<DataType> bin_min = {},
                    std::optional<DataType> bin_width = {});

 private:
  DataType computeBinWidth() const;
  std::string formatBin(DataType val, int width, int precision) const;

  Logger* logger_;

  std::vector<DataType> data_;

  // Bins are defined in bins_ as equally sized windows of width bin_width_
  // starting with smallest value min_val_ at the start of bin 0.
  std::vector<int> bins_;

  DataType min_val_ = 0;
  DataType bin_width_ = 0;

  // Max number of chars to print for a bin.
  static constexpr int max_bin_print_width_ = 50;
};

}  // namespace utl
