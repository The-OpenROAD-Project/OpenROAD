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

class DetailedDisplacement : public DetailedObjective
{
  // For WL objective.
 public:
  explicit DetailedDisplacement(Architecture* arch);

  void init();
  double curr() override;
  double delta(const Journal& journal) override;
  void getCandidates(std::vector<Node*>& candidates);

  // Other.
  void init(DetailedMgr* mgrPtr, DetailedOrient* orientPtr);
  uint64_t delta(Node* ndi, DbuX new_x, DbuY new_y);
  uint64_t delta(Node* ndi, Node* ndj);
  uint64_t delta(Node* ndi,
                 DbuX target_xi,
                 DbuY target_yi,
                 Node* ndj,
                 DbuX target_xj,
                 DbuY target_yj);

 private:
  Architecture* arch_;

  DetailedMgr* mgrPtr_ = nullptr;
  DetailedOrient* orientPtr_ = nullptr;

  // Other.
  DbuY singleRowHeight_;
  std::vector<uint64_t> tot_;
  std::vector<uint64_t> del_;
  std::vector<int> count_;
  int nSets_ = 0;
};

}  // namespace dpo
