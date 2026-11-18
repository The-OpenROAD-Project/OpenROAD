// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "AliasTable.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace fsv {

AliasTable::AliasTable(const std::vector<double>& weights)
    : prob_(weights.size(), 1.0), alias_(weights.size())
{
  const int n = size();
  const double total = std::accumulate(weights.begin(), weights.end(), 0.0);
  if (n == 0 || !(total > 0)) {
    throw std::invalid_argument(
        "fsv: alias table needs weights with a positive sum");
  }
  std::vector<double> scaled(n);
  std::vector<int> small;
  std::vector<int> large;
  for (int i = 0; i < n; i++) {
    alias_[i] = i;
    scaled[i] = weights[i] * n / total;
    (scaled[i] < 1.0 ? small : large).push_back(i);
  }
  // Pair each under-full column with an over-full one.
  while (!small.empty() && !large.empty()) {
    const int s = small.back();
    small.pop_back();
    const int l = large.back();
    prob_[s] = scaled[s];
    alias_[s] = l;
    scaled[l] -= 1.0 - scaled[s];
    if (scaled[l] < 1.0) {
      large.pop_back();
      small.push_back(l);
    }
  }
  // Leftovers are full columns up to rounding.
  for (const int i : small) {
    prob_[i] = 1.0;
  }
  for (const int i : large) {
    prob_[i] = 1.0;
  }
}

int AliasTable::sample(const double u) const
{
  const double x = u * size();
  const int i = std::min(static_cast<int>(x), size() - 1);
  return (x - i) < prob_[i] ? i : alias_[i];
}

}  // namespace fsv
