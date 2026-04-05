// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

// Description:
// - An objective function to help with computation of change in wirelength
//   if doing some sort of moves (e.g., single, swap, sets, etc.).

#include <cstdint>
#include <vector>

#include "detailed_objective.h"
#include "infrastructure/network.h"

namespace dpl {

class DetailedOrient;
class DetailedMgr;

class DetailedHPWL : public DetailedObjective
{
  // For WL objective.
 public:
  explicit DetailedHPWL(Network* network);

  void init();
  double curr() override;
  double delta(const Journal& journal) override;
  void accept() override;
  // Other.
  void init(DetailedMgr* mgrPtr, DetailedOrient* orientPtr);

 private:
  Network* network_;

  DetailedMgr* mgrPtr_ = nullptr;
  DetailedOrient* orientPtr_ = nullptr;

  // Other.
  int skipNetsLargerThanThis_ = 100;
  std::vector<uint64_t> edge_hpwl_;
  std::vector<int> affected_edges_;
};

}  // namespace dpl
