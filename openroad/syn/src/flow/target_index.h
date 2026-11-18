// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <map>
#include <utility>
#include <vector>

#include "flow/combinational_mapper_npn.h"

namespace sta {
class FuncExpr;
class LibertyCell;
class LibertyPort;
class Network;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace syn {

class Synthesis;

namespace cm {

static constexpr int kCutMaximum = 6;

// One candidate mapping target for an NPN-equivalence class
struct MapTarget
{
  sta::LibertyCell* cell;
  // The via NPN tells the mapper how to rewire the canonical inputs back to
  // this cell's actual ports.
  NPN via;
};

// Index of the available gates for fast lookup during combinational
// mapping or resynthesis
struct TargetIndex
{
  // The key is (num_inputs, canonical truth table)
  std::map<std::pair<int, Truth6>, std::vector<MapTarget>> classes;
  sta::LibertyCell* inverter = nullptr;
  // The int is the output port index (0 or 1)
  std::pair<sta::LibertyCell*, int> tie_low;
  std::pair<sta::LibertyCell*, int> tie_high;
};

// Build TargetIndex
void buildIndex(sta::Network* network,
                TargetIndex& index,
                utl::Logger* logger,
                const Synthesis& syn);

}  // namespace cm
}  // namespace syn
