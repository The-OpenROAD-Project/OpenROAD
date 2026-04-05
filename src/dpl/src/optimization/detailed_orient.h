// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

// Description:
// Various routines related to cell orientation and cell flipping.

#pragma once

#include <string>
#include <vector>

#include "odb/db.h"
namespace dpl {
class Node;
class Architecture;
class DetailedMgr;
class Network;

class DetailedOrient
{
 public:
  DetailedOrient(Architecture* arch, Network* network);

  void run(DetailedMgr* mgrPtr, const std::string& command);
  void run(DetailedMgr* mgrPtr, std::vector<std::string>& args);

  // Useful to have some of these routines accessible/public.
  int orientCells(int& changed);
  bool orientSingleHeightCellForRow(Node* ndi, int row);
  bool orientMultiHeightCellForRow(Node* ndi, int row);
  int flipCells();

  // Other.
  unsigned orientFind(Node* ndi, int row);
  static bool isLegalSym(unsigned siteSym, unsigned cellOri);
  static unsigned getMasterSymmetry(odb::dbMaster* master);

 private:
  Architecture* arch_;
  Network* network_;

  DetailedMgr* mgrPtr_ = nullptr;

  int skipNetsLargerThanThis_ = 100;
  int traversal_ = 0;
  std::vector<int> edgeMask_;
};

}  // namespace dpl
