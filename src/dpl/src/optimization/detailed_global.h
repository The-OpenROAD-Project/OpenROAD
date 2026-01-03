// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "detailed_generator.h"
#include "infrastructure/Objects.h"

namespace odb {
class Rect;
}
namespace dpl {
class Edge;
class Architecture;
class DetailedMgr;
class Network;

class DetailedGlobalSwap : public DetailedGenerator
{
 public:
  DetailedGlobalSwap(Architecture* arch, Network* network);
  DetailedGlobalSwap();

  // Interfaces for scripting.
  void run(DetailedMgr* mgrPtr, const std::string& command);
  void run(DetailedMgr* mgrPtr, std::vector<std::string>& args);

  // Interface for move generation.
  bool generate(DetailedMgr* mgr, std::vector<Node*>& candidates) override;
  void stats() override;
  void init(DetailedMgr* mgr) override;

 private:
  void globalSwap();  // tries to avoid overlap.
  bool calculateEdgeBB(Edge* ed, Node* nd, odb::Rect& bbox);
  bool getRange(Node*, odb::Rect&);
  bool generate(Node* ndi);
  bool generateWirelengthOptimalMove(Node* ndi);  // Original wirelength-based generator
  bool generateRandomMove(Node* ndi);             // New random exploration generator
  double calculateAdaptivePowerWeight();  // calculates adaptive power weight

  // Standard stuff.
  DetailedMgr* mgr_;
  Architecture* arch_;
  Network* network_;

  // Other.
  int skipNetsLargerThanThis_;
  std::vector<int> edgeMask_;
  int traversal_;

  std::vector<double> xpts_;
  std::vector<double> ypts_;

  // For use as a move generator.
  int attempts_;
  int moves_;
  int swaps_;

  // Power-density-aware placement support
  std::vector<double> power_contribution_;
  double power_weight_;
  
  // Two-pass budget-constrained optimization support
  double budget_hpwl_;
  bool is_profiling_pass_;
  double tradeoff_;  // 0.0 = pure wirelength opt, 1.0 = full power/wirelength tradeoff
};

}  // namespace dpl