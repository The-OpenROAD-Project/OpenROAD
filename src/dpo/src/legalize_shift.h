// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

///////////////////////////////////////////////////////////////////////////////
// Description:
//
// Header file for simple legalizer used to populate data structures
// prior to detailed improvement.

#pragma once

#include <vector>
namespace dpl {
class Node;
}
namespace dpo {

class Architecture;
class DetailedMgr;
class Network;
using dpl::Node;

class ShiftLegalizer
{
 public:
  ShiftLegalizer();
  ~ShiftLegalizer();

  bool legalize(DetailedMgr& mgr);

 private:
  struct Clump;

  double shift(std::vector<Node*>& cells);
  double clump(std::vector<Node*>& order);
  void merge(Clump* r);
  bool violated(Clump* r, Clump*& l, int& dist);

  DetailedMgr* mgr_ = nullptr;
  Architecture* arch_ = nullptr;
  Network* network_ = nullptr;

  // For clumping.
  std::vector<Clump> clumps_;
  std::vector<int> offset_;
  std::vector<Clump*> ptr_;
  std::vector<std::vector<int>> outgoing_;
  std::vector<std::vector<int>> incoming_;
  std::vector<Node*> dummiesRight_;
  std::vector<Node*> dummiesLeft_;
};

}  // namespace dpo
