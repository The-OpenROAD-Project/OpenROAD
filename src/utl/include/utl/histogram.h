// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

namespace utl {

class Logger;

template <typename DataType>
class Histogram
{
 public:
  Histogram(Logger* logger, int bins);
  virtual ~Histogram() = default;

  // Prints the histogram to the log. precision is used to control
  // the number of digits when displaying each bin's range.
  void report(int precision = 0) const;

  DataType getBinWidth() const { return bin_width_; }
  DataType getBinMinimum() const { return min_val_; }
  DataType getBinRange() const { return bins_.size() * getBinWidth(); }
  DataType getBinMaximum() const { return getBinRange() + getBinMinimum(); }
  int getBinCount() const { return bins_.size(); }

  const std::vector<int>& getBins() const { return bins_; }
  void clearData() { data_.clear(); }
  bool hasData() const { return !data_.empty(); }

  Logger* getLogger() const { return logger_; }

  // populate needs to call this to add data to the histogram
  void addData(DataType data) { data_.push_back(data); }
  // called after populate to build the bins data
  void generateBins();

 private:
  DataType computeBinWidth() const;
  std::string formatBin(DataType pt, int width, int precision) const;

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
