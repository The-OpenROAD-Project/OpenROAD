// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "detailed_generator.h"
#include "rectangle.h"

namespace dpl {
class Edge;
}
namespace dpo {

class Architecture;
class DetailedMgr;
class Network;
class RoutingParams;
using dpl::Edge;

class DetailedVerticalSwap : public DetailedGenerator
{
 public:
  DetailedVerticalSwap(Architecture* arch, Network* network, RoutingParams* rt);
  DetailedVerticalSwap();

  // Intefaces for scripting.
  void run(DetailedMgr* mgrPtr, const std::string& command);
  void run(DetailedMgr* mgrPtr, const std::vector<std::string>& args);

  // Interface for move generation.
  bool generate(DetailedMgr* mgr, std::vector<Node*>& candidates) override;
  void stats() override;
  void init(DetailedMgr* mgr) override;

 private:
  void verticalSwap();  // tries to avoid overlap.
  bool calculateEdgeBB(const Edge* ed, const Node* nd, Rectangle& bbox);
  bool getRange(Node*, Rectangle&);
  double delta(Node* ndi, double new_x, double new_y);
  double delta(Node* ndi, Node* ndj);

  bool generate(Node* ndi);

  // Standard stuff.
  DetailedMgr* mgr_;
  Architecture* arch_;
  Network* network_;
  RoutingParams* rt_;

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
};

}  // namespace dpo
