// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "detailed_generator.h"

namespace odb {
class Rect;
}
namespace dpl {
class Edge;
class Architecture;
class DetailedMgr;
class Network;

class DetailedVerticalSwap : public DetailedGenerator
{
 public:
  DetailedVerticalSwap(Architecture* arch, Network* network);
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
  bool calculateEdgeBB(const Edge* ed, const Node* nd, odb::Rect& bbox);
  bool getRange(Node*, odb::Rect&);

  bool generate(Node* ndi);

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
};

}  // namespace dpl
