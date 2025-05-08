// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "infrastructure/Coordinates.h"
namespace dpl {
class Node;
class Architecture;
class DetailedMgr;
class Network;
class DetailedReorderer
{
 public:
  DetailedReorderer(Architecture* arch, Network* network);

  void run(DetailedMgr* mgrPtr, const std::string& command);
  void run(DetailedMgr* mgrPtr, const std::vector<std::string>& args);

 private:
  void reorder();
  void reorder(const std::vector<Node*>& nodes,
               int jstrt,
               int jstop,
               DbuX leftLimit,
               DbuX rightLimit,
               int segId,
               int rowId);
  double cost(const std::vector<Node*>& nodes, int istrt, int istop);

  // Standard stuff.
  Architecture* arch_;
  Network* network_;

  // For segments.
  DetailedMgr* mgrPtr_ = nullptr;

  // Other.
  int skipNetsLargerThanThis_ = 100;
  std::vector<int> edgeMask_;
  int traversal_ = 0;
  int windowSize_ = 3;
};

}  // namespace dpl
