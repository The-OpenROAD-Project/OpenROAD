// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

// Description:
// - An objective function to help with computation of change in wirelength
//   if doing some sort of moves (e.g., single, swap, sets, etc.).

#include <vector>

#include "detailed_objective.h"

namespace dpo {

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
  void getCandidates(std::vector<Node*>& candidates);

  // Other.
  void init(DetailedMgr* mgrPtr, DetailedOrient* orientPtr);
  double delta(Node* ndi, double new_x, double new_y);
  double delta(Node* ndi, Node* ndj);
  double delta(Node* ndi,
               double target_xi,
               double target_yi,
               Node* ndj,
               double target_xj,
               double target_yj);

 private:
  Network* network_;

  DetailedMgr* mgrPtr_ = nullptr;
  DetailedOrient* orientPtr_ = nullptr;

  // Other.
  int skipNetsLargerThanThis_ = 100;
  int traversal_ = 0;
  std::vector<int> edgeMask_;
};

}  // namespace dpo
