// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The check of extraction against the field solver on a design's nets:
// each net's window solved by fsv against the values extraction left in
// ODB.

#pragma once

#include <string>
#include <vector>

#include "fsv/Stack.h"

namespace odb {
class dbBlock;
class dbNet;
}  // namespace odb

namespace pex {

struct CompareOptions
{
  double halo = 10;  // µm around a net that its window takes in
  // The field solver's tolerance on the total; couplings get the same
  // fraction of the total.
  double tol = 0.005;
  int threads = 1;
};

// A net in fF: its total, and its couplings to the other signal nets that
// carry at least 5% of the field solver's total (the accuracy target's
// couplings), from both sides.
struct NetComparison
{
  struct Coupling
  {
    std::string net;
    double fsv = 0;
    double pex = 0;
  };
  std::string net;
  double fsv_total = 0;
  double fsv_sigma = 0;
  double pex_total = 0;
  std::vector<Coupling> couplings;
};

// `count` of the N nets with parasitics: every (N/count)-th in the
// block's net order, starting with the first; all N when count >= N.
std::vector<odb::dbNet*> sampleNets(odb::dbBlock* block, int count);

// Solves each net's window on the stack, design layers matched to the
// stack's by name. Throws std::invalid_argument for a net without
// parasitics, and what the field solver throws.
std::vector<NetComparison> compareWithFsv(odb::dbBlock* block,
                                          const fsv::Stack& stack,
                                          const std::vector<odb::dbNet*>& nets,
                                          const CompareOptions& options);

}  // namespace pex
