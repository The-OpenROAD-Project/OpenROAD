///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Precision Innovations Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "mbff.h"

#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>
#include <omp.h>
#include <ortools/linear_solver/linear_solver.h>
#include <ortools/sat/cp_model.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <vector>

/*
height and width of a flip-flop
ratios are used for:
    (1) finding all slot locations in a tray
    (2) finding power consumption per slot
*/

namespace gpl {

constexpr int NUM_SIZES = 7;
constexpr float WIDTH = 2.448, HEIGHT = 1.2;
constexpr float RATIOS[NUM_SIZES]
    = {1, 0.95, 0.875, 0.854, 0.854, 0.844, 0.844};

int MBFF::GetBitCnt(int bit_idx)
{
  if (bit_idx == 0) {
    return 1;
  }
  return (1 << (bit_idx + 1));
}

int MBFF::GetRows(int slot_cnt)
{
  if (slot_cnt == 1 || slot_cnt == 2 || slot_cnt == 4) {
    return 1;
  }
  if (slot_cnt == 8) {
    return 2;
  }
  return 4;
}

float MBFF::GetDist(const odb::Point& a, const odb::Point& b)
{
  return (abs(a.x() - b.x()) + abs(a.y() - b.y()));
}

/*
This LP finds new tray centers such that the sum of displacements from the new
trays' slots to their matching flops is minimized
*/

float MBFF::RunLP(const std::vector<Flop>& flops,
                  std::vector<Tray>& trays,
                  const std::vector<std::pair<int, int>>& clusters)
{
  int num_flops = static_cast<int>(flops.size());
  int num_trays = static_cast<int>(trays.size());

  std::unique_ptr<operations_research::MPSolver> solver(
      operations_research::MPSolver::CreateSolver("GLOP"));
  double inf = solver->infinity();

  std::vector<operations_research::MPVariable*> tray_x(num_trays);
  std::vector<operations_research::MPVariable*> tray_y(num_trays);

  for (int i = 0; i < num_trays; i++) {
    tray_x[i] = solver->MakeNumVar(0, inf, "");
    tray_y[i] = solver->MakeNumVar(0, inf, "");
  }

  // displacement from flop to tray slot
  std::vector<operations_research::MPVariable*> disp_x(num_flops);
  std::vector<operations_research::MPVariable*> disp_y(num_flops);

  for (int i = 0; i < num_flops; i++) {
    disp_x[i] = solver->MakeNumVar(0, inf, "");
    disp_y[i] = solver->MakeNumVar(0, inf, "");

    int tray_idx = clusters[i].first;
    int slot_idx = clusters[i].second;

    const odb::Point& flop = flops[i].pt;
    const odb::Point& tray = trays[tray_idx].pt;
    const odb::Point& slot = trays[tray_idx].slots[slot_idx];

    float shift_x = slot.x() - tray.x();
    float shift_y = slot.y() - tray.y();

    operations_research::MPConstraint* c1
        = solver->MakeRowConstraint(shift_x - flop.x(), inf, "");
    c1->SetCoefficient(disp_x[i], 1);
    c1->SetCoefficient(tray_x[tray_idx], -1);

    operations_research::MPConstraint* c2
        = solver->MakeRowConstraint(flop.x() - shift_x, inf, "");
    c2->SetCoefficient(disp_x[i], 1);
    c2->SetCoefficient(tray_x[tray_idx], 1);

    operations_research::MPConstraint* c3
        = solver->MakeRowConstraint(shift_y - flop.y(), inf, "");
    c3->SetCoefficient(disp_y[i], 1);
    c3->SetCoefficient(tray_y[tray_idx], -1);

    operations_research::MPConstraint* c4
        = solver->MakeRowConstraint(flop.y() - shift_y, inf, "");
    c4->SetCoefficient(disp_y[i], 1);
    c4->SetCoefficient(tray_y[tray_idx], 1);
  }

  operations_research::MPObjective* objective = solver->MutableObjective();
  for (int i = 0; i < num_flops; i++) {
    objective->SetCoefficient(disp_x[i], 1);
    objective->SetCoefficient(disp_y[i], 1);
  }
  objective->SetMinimization();
  solver->Solve();

  float tot_disp = 0;
  for (int i = 0; i < num_trays; i++) {
    float new_x = tray_x[i]->solution_value();
    float new_y = tray_y[i]->solution_value();

    Tray new_tray;
    new_tray.pt = odb::Point(new_x, new_y);

    tot_disp += GetDist(trays[i].pt, new_tray.pt);
    trays[i].pt = new_tray.pt;
  }

  return tot_disp;
}

/*

this ILP finds a set of trays (given all of the tray candidates from capacitated
k-means) such that
(1) each flop gets mapped to exactly one slot
(2) [(a) + (b)] is minimized, where
        (a) = sum of displacements from a flop to its slot
        (b) = alpha * (sum of the chosen tray costs)

we ignore timing-critical path constraints / objectives so that the algorithm is
scalable
instead, we add a coefficient of 2 to (a) for each flop that is in a
timing-critical path's start/end point
*/

float MBFF::RunILP(const std::vector<Flop>& flops,
                   const std::vector<std::vector<Tray>>& all_trays,
                   float alpha)
{
  std::vector<Tray> trays;
  for (int i = 0; i < NUM_SIZES; i++) {
    trays.insert(trays.end(), all_trays[i].begin(), all_trays[i].end());
  }

  int num_flops = static_cast<int>(flops.size());
  int num_trays = static_cast<int>(trays.size());

  /*
  NOTE: CP-SAT constraints only work with INTEGERS
  so, all coefficients (shift_x and shift_y) are multiplied by 100
  */
  operations_research::sat::CpModelBuilder cp_model;
  int inf = std::numeric_limits<int>::max();

  // cand_tray[i] = tray indices that have a slot which contains flop i
  std::vector<int> cand_tray[num_flops];

  // cand_slot[i] = slot indices that are mapped to flop i
  std::vector<int> cand_slot[num_flops];

  for (int i = 0; i < num_trays; i++) {
    for (int j = 0; j < static_cast<int>(trays[i].slots.size()); j++) {
      if (trays[i].cand[j] >= 0) {
        cand_tray[trays[i].cand[j]].push_back(i);
        cand_slot[trays[i].cand[j]].push_back(j);
      }
    }
  }

  // has a flop been mapped to a slot?
  std::vector<operations_research::sat::BoolVar> mapped[num_flops];

  // displacement from flop to slot
  std::vector<operations_research::sat::IntVar> disp_x[num_flops];
  std::vector<operations_research::sat::IntVar> disp_y[num_flops];

  for (int i = 0; i < num_flops; i++) {
    for (int j = 0; j < static_cast<int>(cand_tray[i].size()); j++) {
      disp_x[i].push_back(
          cp_model.NewIntVar(operations_research::Domain(0, inf)));
      disp_y[i].push_back(
          cp_model.NewIntVar(operations_research::Domain(0, inf)));
      mapped[i].push_back(cp_model.NewBoolVar());
    }
  }

  // add constraints for displacements
  float max_dist = 0;
  for (int i = 0; i < num_flops; i++) {
    for (int j = 0; j < static_cast<int>(cand_tray[i].size()); j++) {
      const int tray_idx = cand_tray[i][j];
      const int slot_idx = cand_slot[i][j];

      const float shift_x
          = trays[tray_idx].slots[slot_idx].x() - flops[i].pt.x();
      const float shift_y
          = trays[tray_idx].slots[slot_idx].y() - flops[i].pt.y();

      max_dist = std::max(
          max_dist, std::max(shift_x, -shift_x) + std::max(shift_y, -shift_y));

      // absolute value constraints for x
      cp_model.AddLessOrEqual(
          0, disp_x[i][j] - int(1000 * shift_x) * mapped[i][j]);
      cp_model.AddLessOrEqual(
          0, disp_x[i][j] + int(1000 * shift_x) * mapped[i][j]);

      // absolute value constraints for y
      cp_model.AddLessOrEqual(
          0, disp_y[i][j] - int(1000 * shift_y) * mapped[i][j]);
      cp_model.AddLessOrEqual(
          0, disp_y[i][j] + int(1000 * shift_y) * mapped[i][j]);
    }
  }

  for (int i = 0; i < num_flops; i++) {
    for (int j = 0; j < static_cast<int>(cand_tray[i].size()); j++) {
      cp_model.AddLessOrEqual(disp_x[i][j] + disp_y[i][j],
                              int(1000 * max_dist));
    }
  }

  /*
  remove timing-critical path constraints / objective.
  instead, replace it with a higher coefficient value when it comes to
  minimizing the displacement from flop to slot
  */

  std::vector<int> coeff(num_flops, 1);
  for (int i = 0; i < num_flops; i++) {
    if (flops_in_path_.count(i)) {
      coeff[i] = 1;
    }
  }

  // check that each flop is matched to a single slot
  for (int i = 0; i < num_flops; i++) {
    operations_research::sat::LinearExpr mapped_flop;
    for (int j = 0; j < static_cast<int>(cand_tray[i].size()); j++) {
      mapped_flop += mapped[i][j];
    }
    cp_model.AddLessOrEqual(1, mapped_flop);
    cp_model.AddLessOrEqual(mapped_flop, 1);
  }

  std::vector<operations_research::sat::BoolVar> tray_used(num_trays);
  for (int i = 0; i < num_trays; i++) {
    tray_used[i] = cp_model.NewBoolVar();
  }

  for (int i = 0; i < num_trays; i++) {
    operations_research::sat::LinearExpr slots_used;

    std::vector<int> flop_ind;
    for (int j = 0; j < static_cast<int>(trays[i].slots.size()); j++) {
      if (trays[i].cand[j] >= 0) {
        flop_ind.push_back(trays[i].cand[j]);
      }
    }

    for (int j = 0; j < static_cast<int>(flop_ind.size()); j++) {
      // TODO: SWITCH TO BINARY SEARCH
      int tray_idx = 0;
      for (int k = 0; k < static_cast<int>(cand_tray[flop_ind[j]].size());
           k++) {
        if (cand_tray[flop_ind[j]][k] == i) {
          tray_idx = k;
        }
      }

      cp_model.AddLessOrEqual(mapped[flop_ind[j]][tray_idx], tray_used[i]);
      slots_used += mapped[flop_ind[j]][tray_idx];
    }

    // check that tray_used <= slots_used
    cp_model.AddLessOrEqual(tray_used[i], slots_used);
  }

  // calculate the cost of each tray
  std::vector<float> tray_cost(num_trays);
  for (int i = 0; i < num_trays; i++) {
    int bit_idx = 0;
    for (int j = 0; j < NUM_SIZES; j++) {
      if (GetBitCnt(j) == static_cast<int>(trays[i].slots.size())) {
        bit_idx = j;
      }
    }
    if (GetBitCnt(bit_idx) == 1) {
      tray_cost[i] = 1.00;
    } else {
      tray_cost[i] = ((float) GetBitCnt(bit_idx))
                     * (RATIOS[bit_idx] - GetBitCnt(bit_idx) * 0.0015);
    }
  }

  /*
      - DoubleLinearExpr support double coefficients
      - can only be used for the objective function
  */

  operations_research::sat::DoubleLinearExpr obj;

  // add the sum of all distances
  for (int i = 0; i < num_flops; i++) {
    for (int j = 0; j < static_cast<int>(cand_tray[i].size()); j++) {
      obj.AddTerm(disp_x[i][j], coeff[i] * 0.001);
      obj.AddTerm(disp_y[i][j], coeff[i] * 0.001);
    }
  }

  // add the tray usage constraints
  for (int i = 0; i < num_trays; i++) {
    obj.AddTerm(tray_used[i], alpha * tray_cost[i]);
  }

  cp_model.Minimize(obj);

  operations_research::sat::Model model;
  operations_research::sat::SatParameters parameters;

  operations_research::sat::CpSolverResponse response
      = operations_research::sat::SolveCpModel(cp_model.Build(), &model);

  if (response.status() == operations_research::sat::CpSolverStatus::FEASIBLE
      || response.status()
             == operations_research::sat::CpSolverStatus::OPTIMAL) {
    // update slot_disp_ vectors
    for (int i = 0; i < num_flops; i++) {
      for (int j = 0; j < static_cast<int>(cand_tray[i].size()); j++) {
        if (operations_research::sat::SolutionIntegerValue(response,
                                                           mapped[i][j])
            == 1) {
          slot_disp_x_[flops[i].idx]
              = trays[cand_tray[i][j]].slots[cand_slot[i][j]].x()
                - flops[i].pt.x();
          slot_disp_y_[flops[i].idx]
              = trays[cand_tray[i][j]].slots[cand_slot[i][j]].y()
                - flops[i].pt.y();
        }
      }
    }

    return static_cast<float>(response.objective_value());
  }

  return 0;
}

void MBFF::GetSlots(const odb::Point& tray,
                    int rows,
                    int cols,
                    std::vector<odb::Point>& slots)
{
  int bit_idx = 0;
  for (int i = 1; i < NUM_SIZES; i++) {
    if (rows * cols == GetBitCnt(i)) {
      bit_idx = i;
    }
  }

  float center_x = tray.x();
  float center_y = tray.y();

  slots.clear();
  for (int i = 0; i < rows * cols; i++) {
    int new_col = i % cols;
    int new_row = i / cols;

    float new_x
        = center_x + WIDTH * RATIOS[bit_idx] * ((new_col + 0.5) - (cols / 2.0));
    float new_y = center_y + HEIGHT * ((new_row + 0.5) - (rows / 2.0));

    odb::Point new_slot(new_x, new_y);

    slots.push_back(new_slot);
  }
}

Flop MBFF::GetNewFlop(const std::vector<Flop>& prob_dist, float tot_dist)
{
  float rand_num = (float) (rand() % 101), cum_sum = 0;
  Flop new_flop;
  for (int i = 0; i < static_cast<int>(prob_dist.size()); i++) {
    cum_sum += prob_dist[i].prob;
    new_flop = prob_dist[i];
    if (cum_sum * 100.0 >= rand_num * tot_dist) {
      break;
    }
  }
  return new_flop;
}

void MBFF::GetStartTrays(std::vector<Flop> flops,
                         int num_trays,
                         float AR,
                         std::vector<Tray>& trays)
{
  int num_flops = static_cast<int>(flops.size());

  /* pick a random flop */
  int rand_idx = rand() % (num_flops);
  Tray tray_zero;
  tray_zero.pt = flops[rand_idx].pt;

  std::set<int> used_flops;
  used_flops.insert(rand_idx);
  trays.push_back(tray_zero);

  float tot_dist = 0;
  for (int i = 0; i < num_flops; i++) {
    float contr = GetDist(flops[i].pt, tray_zero.pt) / AR;
    flops[i].prob = contr, tot_dist += contr;
  }

  while (static_cast<int>(trays.size()) < num_trays) {
    std::vector<Flop> prob_dist;
    for (int i = 0; i < num_flops; i++) {
      if (!used_flops.count(flops[i].idx)) {
        prob_dist.push_back(flops[i]);
      }
    }

    std::sort(prob_dist.begin(), prob_dist.end());

    Flop new_flop = GetNewFlop(prob_dist, tot_dist);
    used_flops.insert(new_flop.idx);

    Tray new_tray;
    new_tray.pt = new_flop.pt;
    trays.push_back(new_tray);

    for (int i = 0; i < num_flops; i++) {
      float new_contr = GetDist(flops[i].pt, new_tray.pt) / AR;
      flops[i].prob += new_contr, tot_dist += new_contr;
    }
  }
}

Tray MBFF::GetOneBit(const odb::Point& pt)
{
  float new_x = pt.x() - WIDTH / 2.0;
  float new_y = pt.y() - HEIGHT / 2.0;

  Tray tray;
  tray.pt = odb::Point(new_x, new_y);
  tray.slots.push_back(pt);

  return tray;
}

void MBFF::MinCostFlow(const std::vector<Flop>& flops,
                       std::vector<Tray>& trays,
                       int sz,
                       std::vector<std::pair<int, int>>& clusters)
{
  int num_flops = static_cast<int>(flops.size());
  int num_trays = static_cast<int>(trays.size());

  lemon::ListDigraph graph;
  std::vector<lemon::ListDigraph::Node> nodes;
  std::vector<lemon::ListDigraph::Arc> edges;
  std::vector<int> cur_costs, cur_caps;

  // add edges from source to flop
  lemon::ListDigraph::Node src = graph.addNode(), sink = graph.addNode();
  for (int i = 0; i < num_flops; i++) {
    lemon::ListDigraph::Node flop_node = graph.addNode();
    nodes.push_back(flop_node);

    lemon::ListDigraph::Arc src_to_flop = graph.addArc(src, flop_node);
    edges.push_back(src_to_flop);
    cur_costs.push_back(0), cur_caps.push_back(1);
  }

  std::vector<std::pair<int, int>> slot_to_tray;

  for (int i = 0; i < num_trays; i++) {
    std::vector<odb::Point> tray_slots = trays[i].slots;

    for (int j = 0; j < static_cast<int>(tray_slots.size()); j++) {
      lemon::ListDigraph::Node slot_node = graph.addNode();
      nodes.push_back(slot_node);

      // add edges from flop to slot
      for (int k = 0; k < num_flops; k++) {
        lemon::ListDigraph::Arc flop_to_slot
            = graph.addArc(nodes[k], slot_node);
        edges.push_back(flop_to_slot);

        int edge_cost = (100 * GetDist(flops[k].pt, tray_slots[j]));
        cur_costs.push_back(edge_cost), cur_caps.push_back(1);
      }

      // add edges from slot to sink
      lemon::ListDigraph::Arc slot_to_sink = graph.addArc(slot_node, sink);
      edges.push_back(slot_to_sink);
      cur_costs.push_back(0), cur_caps.push_back(1);

      slot_to_tray.push_back(std::make_pair(i, j));
    }
  }

  // run min-cost flow
  lemon::ListDigraph::ArcMap<int> costs(graph), caps(graph), flow(graph);
  lemon::ListDigraph::NodeMap<int> labels(graph);
  lemon::NetworkSimplex<lemon::ListDigraph, int, int> new_graph(graph);

  for (int i = 0; i < static_cast<int>(edges.size()); i++) {
    costs[edges[i]] = cur_costs[i], caps[edges[i]] = cur_caps[i];
  }

  labels[src] = -1;
  for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
    labels[nodes[i]] = i;
  }
  labels[sink] = static_cast<int>(nodes.size());

  new_graph.costMap(costs);
  new_graph.upperMap(caps);
  new_graph.stSupply(src, sink, num_flops);
  new_graph.run();
  new_graph.flowMap(flow);

  // get, and save, the clustering solution
  clusters.clear();
  clusters.resize(num_flops);
  for (lemon::ListDigraph::ArcIt itr(graph); itr != lemon::INVALID; ++itr) {
    int u = labels[graph.source(itr)], v = labels[graph.target(itr)];
    if (flow[itr] != 0 && u < num_flops && v >= num_flops) {
      v -= num_flops;
      int tray_idx = slot_to_tray[v].first;
      int slot_idx = slot_to_tray[v].second;
      clusters[u] = std::make_pair(tray_idx, slot_idx),
      trays[tray_idx].cand[slot_idx] = u;
    }
  }
}

float MBFF::GetSilh(const std::vector<Flop>& flops,
                    const std::vector<Tray>& trays,
                    const std::vector<std::pair<int, int>>& clusters)
{
  int num_flops = static_cast<int>(flops.size());
  int num_trays = static_cast<int>(trays.size());

  float tot = 0;
  for (int i = 0; i < num_flops; i++) {
    float min_num = std::numeric_limits<float>::max();
    float max_den = GetDist(flops[i].pt,
                            trays[clusters[i].first].slots[clusters[i].second]);
    for (int j = 0; j < num_trays; j++) {
      if (j != clusters[i].first) {
        max_den = std::max(
            max_den, GetDist(flops[i].pt, trays[j].slots[clusters[i].second]));
        for (int k = 0; k < static_cast<int>(trays[j].slots.size()); k++) {
          min_num = std::min(min_num, GetDist(flops[i].pt, trays[j].slots[k]));
        }
      }
    }

    tot += (min_num / max_den);
  }

  return tot;
}

void MBFF::RunCapacitatedKMeans(const std::vector<Flop>& flops,
                                std::vector<Tray>& trays,
                                int sz,
                                int iter,
                                std::vector<std::pair<int, int>>& cluster)
{
  cluster.clear();
  int num_flops = static_cast<int>(flops.size());
  int rows = GetRows(sz);
  int cols = sz / rows;
  int num_trays = (num_flops + (sz - 1)) / sz;

  float delta = 0;
  for (int i = 0; i < iter; i++) {
    MinCostFlow(flops, trays, sz, cluster);
    delta = RunLP(flops, trays, cluster);

    for (int j = 0; j < num_trays; j++) {
      GetSlots(trays[j].pt, rows, cols, trays[j].slots);
      for (int k = 0; k < rows * cols; k++) {
        trays[j].cand[k] = -1;
      }
    }

    if (delta < 0.5) {
      break;
    }
  }

  MinCostFlow(flops, trays, sz, cluster);
}

std::vector<std::vector<Tray>>& MBFF::RunSilh(
    const std::vector<Flop>& flops,
    std::vector<std::vector<std::vector<Tray>>>& start_trays)
{
  int num_flops = static_cast<int>(flops.size());

  std::vector<std::vector<Tray>>* trays
      = new std::vector<std::vector<Tray>>(NUM_SIZES);

  for (int i = 0; i < NUM_SIZES; i++) {
    int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);
    (*trays)[i].resize(num_trays);
  }

  // add 1-bit trays
  for (int i = 0; i < num_flops; i++) {
    Tray one_bit = GetOneBit(flops[i].pt);
    one_bit.cand.reserve(1);
    one_bit.cand.emplace_back(i);
    (*trays)[0][i] = one_bit;
  }

  std::vector<float> res[NUM_SIZES];
  std::vector<std::pair<int, int>> ind;

  for (int i = 1; i < NUM_SIZES; i++) {
    for (int j = 0; j < 5; j++) {
      ind.push_back(std::make_pair(i, j));
    }
    res[i].resize(5);
  }

  for (int i = 1; i < NUM_SIZES; i++) {
    for (int j = 0; j < 5; j++) {
      int rows = GetRows(GetBitCnt(i));
      int cols = GetBitCnt(i) / rows;
      int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);

      for (int k = 0; k < num_trays; k++) {
        GetSlots(
            start_trays[i][j][k].pt, rows, cols, start_trays[i][j][k].slots);
        start_trays[i][j][k].cand.reserve(rows * cols);
        for (int idx = 0; idx < rows * cols; idx++) {
          start_trays[i][j][k].cand.emplace_back(-1);
        }
      }
    }
  }

  // run multistart_ in parallel
  for (int i = 0; i < static_cast<int>(ind.size()); i++) {
    int bit_idx = ind[i].first;
    int tray_idx = ind[i].second;

    int rows = GetRows(GetBitCnt(bit_idx));
    int cols = GetBitCnt(bit_idx) / rows;

    std::vector<std::pair<int, int>> tmp_cluster;

    RunCapacitatedKMeans(
        flops, start_trays[bit_idx][tray_idx], rows * cols, 8, tmp_cluster);

    res[bit_idx][tray_idx]
        = GetSilh(flops, start_trays[bit_idx][tray_idx], tmp_cluster);
  }

  for (int i = 1; i < NUM_SIZES; i++) {
    int opt_idx = 0;
    float opt_val = -1;
    for (int j = 0; j < 5; j++) {
      if (res[i][j] > opt_val) {
        opt_val = res[i][j];
        opt_idx = j;
      }
    }
    (*trays)[i] = start_trays[i][opt_idx];
  }

  return *trays;
}

// standard K-means++ implementation
void MBFF::KMeans(const std::vector<Flop>& flops,
                  std::vector<std::vector<Flop>>& clusters)
{
  int num_flops = static_cast<int>(flops.size());

  std::set<int> chosen;
  // choose initial center
  int seed = rand() % num_flops;
  chosen.insert(seed);

  std::vector<Flop> centers;
  centers.push_back(flops[seed]);

  std::vector<float> d(num_flops);
  for (int i = 0; i < num_flops; i++) {
    d[i] = GetDist(flops[i].pt, flops[seed].pt);
  }

  // choose remaining K-1 centers
  while (static_cast<int>(chosen.size()) < knn_) {
    double tot_sum = 0;

    for (int i = 0; i < num_flops; i++) {
      if (!chosen.count(i)) {
        for (int j : chosen) {
          d[i] = std::min(d[i], GetDist(flops[i].pt, flops[j].pt));
        }
        tot_sum += (double(d[i]) * double(d[i]));
      }
    }

    int rnd = rand() % (int(tot_sum * 100));
    float prob = rnd / 100.0;

    double cum_sum = 0;
    for (int i = 0; i < num_flops; i++) {
      if (!chosen.count(i)) {
        cum_sum += (double(d[i]) * double(d[i]));
        if (cum_sum >= prob) {
          chosen.insert(i);
          centers.push_back(flops[i]);
          break;
        }
      }
    }
  }

  clusters.resize(knn_);
  float prev = -1;
  while (true) {
    for (int i = 0; i < knn_; i++) {
      clusters[i].clear();
    }

    // remap flops to clusters
    for (int i = 0; i < num_flops; i++) {
      float min_cost = std::numeric_limits<float>::max();
      int idx = 0;

      for (int j = 0; j < knn_; j++) {
        if (GetDist(flops[i].pt, centers[j].pt) < min_cost) {
          min_cost = GetDist(flops[i].pt, centers[j].pt);
          idx = j;
        }
      }

      clusters[idx].push_back(flops[i]);
    }

    // find new center locations
    for (int i = 0; i < knn_; i++) {
      int cur_sz = static_cast<int>(clusters[i].size());
      float cX = 0;
      float cY = 0;

      for (int j = 0; j < static_cast<int>(clusters[i].size()); j++) {
        cX += clusters[i][j].pt.x();
        cY += clusters[i][j].pt.y();
      }

      float new_x = cX / float(cur_sz);
      float new_y = cY / float(cur_sz);
      centers[i].pt = odb::Point(new_x, new_y);
    }

    // get total displacement
    float tot_disp = 0;
    for (int i = 0; i < knn_; i++) {
      for (int j = 0; j < static_cast<int>(clusters[i].size()); j++) {
        tot_disp += GetDist(centers[i].pt, clusters[i][j].pt);
      }
    }

    if (tot_disp == prev) {
      break;
    }
    prev = tot_disp;
  }

  for (int i = 0; i < knn_; i++) {
    clusters[i].push_back(centers[i]);
  }
}

/*
    shreyas (august 2023):
    method to decompose a pointset into multiple "mini"-pointsets of size <=
   MAX_SZ.
    basic implementation of K-means++ (with K = 4) is used.
*/

void MBFF::KMeansDecomp(const std::vector<Flop>& flops,
                        int MAX_SZ,
                        std::vector<std::vector<Flop>>& pointsets)
{
  int num_flops = static_cast<int>(flops.size());
  if (num_flops <= MAX_SZ) {
    pointsets.push_back(flops);
    return;
  }

  std::vector<std::vector<Flop>> tmp_clusters[multistart_];
  std::vector<float> tmp_costs(multistart_);

  // multistart_ K-means++
  for (int i = 0; i < multistart_; i++) {
    KMeans(flops, tmp_clusters[i]);

    /* cur_cost = sum of distances between flops and its
    matching cluster's center */
    float cur_cost = 0;
    for (int j = 0; j < knn_; j++) {
      for (int k = 0; k + 1 < static_cast<int>(tmp_clusters[i][j].size());
           k++) {
        cur_cost
            += GetDist(tmp_clusters[i][j][k].pt, tmp_clusters[i][j].back().pt);
      }
    }
    tmp_costs[i] = cur_cost;
  }

  float best_cost = std::numeric_limits<float>::max();
  std::vector<std::vector<Flop>> k_means_ret;
  for (int i = 0; i < multistart_; i++) {
    if (tmp_costs[i] < best_cost) {
      best_cost = tmp_costs[i];
      k_means_ret = tmp_clusters[i];
    }
  }

  /*
  create edges between all std::pairs of cluster centers
  edge weight = distance between the two centers
  */

  std::vector<std::pair<float, std::pair<int, int>>> cluster_pairs;
  for (int i = 0; i < knn_; i++) {
    for (int j = i + 1; j < knn_; j++) {
      cluster_pairs.push_back(std::make_pair(
          GetDist(k_means_ret[i].back().pt, k_means_ret[j].back().pt),
          std::make_pair(i, j)));
    }
  }
  std::sort(cluster_pairs.begin(), cluster_pairs.end());

  for (int i = 0; i < knn_; i++) {
    k_means_ret[i].pop_back();
  }

  /*
      lines 890-918: basic implementation of DSU
      keep uniting sets (== mini-pointsets with size <= MAX_SZ) while not
      exceeding MAX_SZ
  */

  std::vector<int> id(knn_);
  std::vector<int> sz(knn_);

  for (int i = 0; i < knn_; i++) {
    id[i] = i;
    sz[i] = static_cast<int>(k_means_ret[i].size());
  }

  for (int i = 0; i < static_cast<int>(cluster_pairs.size()); i++) {
    int idx1 = cluster_pairs[i].second.first;
    int idx2 = cluster_pairs[i].second.second;

    if (sz[id[idx1]] < sz[id[idx2]]) {
      std::swap(idx1, idx2);
    }
    if (sz[id[idx1]] + sz[id[idx2]] > MAX_SZ) {
      continue;
    }

    sz[id[idx1]] += sz[id[idx2]];

    // merge the two clusters
    int orig_id = id[idx2];
    for (int j = 0; j < knn_; j++) {
      if (id[j] == orig_id) {
        id[j] = id[idx1];
      }
    }
  }

  std::vector<std::vector<Flop>> nxt_clusters(knn_);
  for (int i = 0; i < knn_; i++) {
    for (Flop f : k_means_ret[i]) {
      nxt_clusters[id[i]].push_back(f);
    }
  }

  // recurse on each new cluster
  for (int i = 0; i < knn_; i++) {
    if (static_cast<int>(nxt_clusters[i].size())) {
      std::vector<std::vector<Flop>> R;
      KMeansDecomp(nxt_clusters[i], MAX_SZ, R);
      for (auto& x : R) {
        pointsets.push_back(x);
      }
    }
  }
}

double MBFF::GetTCPDisplacement(float beta)
{
  double ret = 0;
  for (auto& path : paths_) {
    float diff_x
        = slot_disp_x_[path.start_point] - slot_disp_x_[path.end_point];
    float diff_y
        = slot_disp_y_[path.start_point] - slot_disp_y_[path.end_point];
    ret += (std::max(diff_x, -diff_x) + std::max(diff_y, -diff_y));
  }
  return (beta * ret);
}

void MBFF::Run(int mx_sz, float alpha, float beta)
{
  srand(1);
  omp_set_num_threads(num_threads_);

  // run k-means++ based decomposition
  std::vector<std::vector<Flop>> pointsets;
  KMeansDecomp(flops_, mx_sz, pointsets);

  /*
  (1) DO NOT RUN IN PARALLEL -- MESSES WITH REPRODUCIBILITY
  (2) all_start_trays[t][i][j] = start trays of size 2^i for pointset[t]
  and iteration j of the multistart
  */

  std::vector<std::vector<std::vector<Tray>>>
      all_start_trays[static_cast<int>(pointsets.size())];
  for (int t = 0; t < static_cast<int>(pointsets.size()); t++) {
    all_start_trays[t].resize(NUM_SIZES);
    for (int i = 1; i < NUM_SIZES; i++) {
      int rows = GetRows(GetBitCnt(i));
      int cols = GetBitCnt(i) / rows;
      float AR = (cols * WIDTH * RATIOS[i]) / (rows * HEIGHT);
      int num_trays
          = (static_cast<int>(pointsets[t].size()) + (GetBitCnt(i) - 1))
            / GetBitCnt(i);
      all_start_trays[t][i].resize(5);
      for (int j = 0; j < 5; j++) {
        GetStartTrays(pointsets[t], num_trays, AR, all_start_trays[t][i][j]);
      }
    }
  }

  double ans = 0;
#pragma omp parallel for num_threads(num_threads_)
  for (int t = 0; t < static_cast<int>(pointsets.size()); t++) {
    std::vector<std::vector<Tray>>& cur_trays
        = RunSilh(pointsets[t], all_start_trays[t]);

    // run capacitated k-means per tray size
    int num_flops = static_cast<int>(pointsets[t].size());
    for (int i = 1; i < NUM_SIZES; i++) {
      int rows = GetRows(GetBitCnt(i)), cols = GetBitCnt(i) / rows;
      int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);

      for (int j = 0; j < num_trays; j++) {
        GetSlots(cur_trays[i][j].pt, rows, cols, cur_trays[i][j].slots);
      }

      std::vector<std::pair<int, int>> cluster;
      RunCapacitatedKMeans(
          pointsets[t], cur_trays[i], GetBitCnt(i), 35, cluster);

      MinCostFlow(pointsets[t], cur_trays[i], GetBitCnt(i), cluster);

      for (int j = 0; j < num_trays; j++) {
        GetSlots(cur_trays[i][j].pt, rows, cols, cur_trays[i][j].slots);
      }
    }
    ans += RunILP(pointsets[t], cur_trays, alpha);
    delete &cur_trays;
  }
  ans += GetTCPDisplacement(beta);
  log_->info(utl::GPL, 22, "Total = {}", ans);
}

// ctor
MBFF::MBFF(int num_flops,
           int num_paths,
           const std::vector<odb::Point>& points,
           const std::vector<Path>& paths,
           int threads,
           int knn,
           int multistart,
           utl::Logger* log)
{
  flops_.reserve(num_flops);
  for (int i = 0; i < num_flops; i++) {
    Flop new_flop{points[i], i, 0.0};
    flops_.emplace_back(new_flop);
  }

  paths_.reserve(num_paths);
  for (int i = 0; i < num_paths; i++) {
    paths_.emplace_back(paths[i]);
    flops_in_path_.insert(paths[i].start_point);
    flops_in_path_.insert(paths[i].end_point);
  }

  slot_disp_x_.reserve(num_flops);
  slot_disp_y_.reserve(num_flops);
  for (int i = 0; i < num_flops; i++) {
    slot_disp_x_.emplace_back(0);
    slot_disp_y_.emplace_back(0);
  }

  log_ = log;
  num_threads_ = threads;
  knn_ = knn;
  multistart_ = multistart;
}

// dtor
MBFF::~MBFF() = default;

}  // end namespace gpl
