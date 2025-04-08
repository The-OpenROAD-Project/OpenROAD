// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "GreedyRefine.h"

#include <memory>
#include <set>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "Refiner.h"
#include "Utilities.h"

// ------------------------------------------------------------------------------
// K-way hyperedge greedy refinement
// ------------------------------------------------------------------------------

namespace par {

// Implement the greedy refinement pass
// Different from the FM refinement, greedy refinement
// only accepts possible gain
float GreedyRefine::Pass(
    const HGraphPtr& hgraph,
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance,
    Matrix<float>& block_balance,        // the current block balance
    Matrix<int>& net_degs,               // the current net degree
    std::vector<float>& cur_paths_cost,  // the current path cost
    Partitions& solution,
    std::vector<bool>& visited_vertices_flag)
{
  float total_gain = 0.0;  // total gain improvement
  int num_move = 0;
  for (int hyperedge_id = 0; hyperedge_id < hgraph->GetNumHyperedges();
       hyperedge_id++) {
    // check if the hyperedge is a straddled_hyperedge
    std::set<int> block_set;
    for (int block_id = 0; block_id < num_parts_; block_id++) {
      if (net_degs[hyperedge_id][block_id] > 0) {
        block_set.insert(block_id);
      }
    }
    // ignore the hyperedge if it's fully within one block
    if (block_set.size() <= 1) {
      continue;
    }
    // updated the iteration
    num_move++;
    if (num_move >= max_move_) {
      return total_gain;
    }
    // find the best candidate block
    // the initialization of best_gain is 0.0
    // define a lambda function to compare two HyperedgeGainPtr (>=)
    auto CompareHyperedgeGain
        = [&](const HyperedgeGainPtr& a, const HyperedgeGainPtr& b) {
            if (a->GetGain() > b->GetGain()) {
              return true;
            }
            // break ties based on vertex weight summation of
            // the hyperedge
            return a->GetGain() == b->GetGain()
                   && evaluator_->CalculateHyperedgeVertexWtSum(
                          a->GetHyperedge(), hgraph)
                          < evaluator_->CalculateHyperedgeVertexWtSum(
                              b->GetHyperedge(), hgraph);
          };

    std::shared_ptr<HyperedgeGain> best_gain_hyperedge;
    for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
      if (CheckHyperedgeMoveLegality(hyperedge_id,
                                     to_pid,
                                     hgraph,
                                     solution,
                                     block_balance,
                                     upper_block_balance,
                                     lower_block_balance)
          == true) {
        HyperedgeGainPtr gain_hyperedge = CalculateHyperedgeGain(
            hyperedge_id, to_pid, hgraph, solution, cur_paths_cost, net_degs);
        if (!best_gain_hyperedge
            || CompareHyperedgeGain(gain_hyperedge, best_gain_hyperedge)) {
          best_gain_hyperedge = gain_hyperedge;
        }
      }
    }

    // We only accept positive move
    if (best_gain_hyperedge && best_gain_hyperedge->GetDestinationPart() > -1
        && best_gain_hyperedge->GetGain() >= 0.0f) {
      AcceptHyperedgeGain(best_gain_hyperedge,
                          hgraph,
                          total_gain,
                          solution,
                          cur_paths_cost,
                          block_balance,
                          net_degs);
    }
  }

  // finish traversing all the hyperedges
  return total_gain;
}

}  // namespace par
