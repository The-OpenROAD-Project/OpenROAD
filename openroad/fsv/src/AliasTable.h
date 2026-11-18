// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

namespace fsv {

// Walker/Vose alias table: draws index i with probability w[i] / sum(w) in
// constant time from one uniform number.
class AliasTable
{
 public:
  // Throws std::invalid_argument if the weights are empty or sum to zero.
  explicit AliasTable(const std::vector<double>& weights);

  int sample(double u) const;
  int size() const { return static_cast<int>(prob_.size()); }

 private:
  std::vector<double> prob_;
  std::vector<int> alias_;
};

}  // namespace fsv
