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
#include <memory>
#include <random>

namespace gpl {

bool MBFF::IsClockPin(odb::dbITerm* iterm)
{
  const bool yes = (iterm->getSigType() == odb::dbSigType::CLOCK);
  sta::Pin* pin = network_->dbToSta(iterm);
  return yes || sta_->isClock(pin);
}

bool MBFF::IsSupplyPin(odb::dbITerm* iterm)
{
  return iterm->getSigType().isSupply();
}

bool MBFF::IsDPin(odb::dbITerm* iterm)
{
  const bool exclude = (IsClockPin(iterm) || IsSupplyPin(iterm));
  const bool yes = (iterm->getIoType() == odb::dbIoType::INPUT);
  return (yes & !exclude);
}

bool MBFF::IsQPin(odb::dbITerm* iterm)
{
  const bool exclude = (IsClockPin(iterm) || IsSupplyPin(iterm));
  const bool yes = (iterm->getIoType() == odb::dbIoType::OUTPUT);
  return (yes & !exclude);
}

int MBFF::GetNumSlots(odb::dbInst* inst)
{
  int num_d = 0;
  for (auto iterm : inst->getITerms()) {
    num_d += IsDPin(iterm);
  }
  return num_d;
}

int MBFF::GetBitIdx(int bit_cnt)
{
  for (int i = 0; i < num_sizes_; i++) {
    if ((1 << i) == bit_cnt) {
      return i;
    }
  }
  log_->error(utl::GPL, 122, "{} is not in 2^[0,{}]", bit_cnt, num_sizes_);
}

int MBFF::GetBitCnt(int bit_idx)
{
  return (1 << bit_idx);
}

int MBFF::GetRows(int slot_cnt)
{
  int idx = GetBitIdx(slot_cnt);
  int width = int(multiplier_ * tray_width_[idx]);
  int height = int(multiplier_ * (tray_area_[idx] / tray_width_[idx]));
  return (height / std::gcd(width, height));
}

double MBFF::GetDist(const Point& a, const Point& b)
{
  return (abs(a.x - b.x) + abs(a.y - b.y));
}

bool MBFF::IsValidTray(odb::dbInst* tray)
{
  sta::Cell* cell = network_->dbToSta(tray->getMaster());
  if (cell == nullptr) {
    return false;
  }
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  if (lib_cell == nullptr) {
    return false;
  }
  sta::LibertyCellPortIterator port_itr(lib_cell);
  if (port_itr.hasNext() == false) {
    return false;
  }
  int slot_cnt = GetNumSlots(tray);
  if (slot_cnt <= 1) {
    return false;
  }
  return true;
}

std::map<std::string, std::string> MBFF::GetPinMapping(odb::dbInst* tray)
{
  sta::Cell* cell = network_->dbToSta(tray->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::LibertyCellPortIterator port_itr(lib_cell);

  std::vector<std::string> d_pins;
  std::vector<std::string> q_pins;
  while (port_itr.hasNext()) {
    sta::LibertyPort* port = port_itr.next();
    if (network_->staToDb(port) == nullptr) {
      continue;
    }
    if (port->isClock()) {
      continue;
    }
    if (port->direction()->isInput()) {
      d_pins.push_back(port->name());
    }
    if (port->direction()->isOutput()) {
      q_pins.push_back(port->name());
    }
  }

  std::map<std::string, std::string> ret;
  for (size_t i = 0; i < d_pins.size(); i++) {
    ret[d_pins[i]] = q_pins[i];
  }
  return ret;
}

void MBFF::ModifyPinConnections(const std::vector<Flop>& flops,
                                const std::vector<Tray>& trays,
                                std::vector<std::pair<int, int>>& mapping)
{
  const int num_flops = static_cast<int>(flops.size());

  std::map<int, int> old_to_new_idx;
  std::vector<odb::dbInst*> tray_inst;
  for (int i = 0; i < num_flops; i++) {
    int tray_idx = mapping[i].first;
    if (static_cast<int>(trays[tray_idx].slots.size()) == 1) {
      mapping[i].first = std::numeric_limits<int>::max();
      continue;
    }
    if (!old_to_new_idx.count(tray_idx)) {
      old_to_new_idx[tray_idx] = static_cast<int>(tray_inst.size());

      // the chance that two trays with the same size get the same number is
      // rare (long long max is 10^18)
      std::string new_name
          = "_tray_"
            + std::to_string(static_cast<int>(trays[tray_idx].slots.size()))
            + " " + std::to_string(unused_.back());
      unused_.pop_back();
      int bit_idx = GetBitIdx(static_cast<int>(trays[tray_idx].slots.size()));
      auto new_tray = odb::dbInst::create(
          block_, best_master_[bit_idx], new_name.c_str());
      new_tray->setLocation(
          static_cast<int>(multiplier_ * trays[tray_idx].pt.x),
          static_cast<int>(multiplier_ * trays[tray_idx].pt.y));
      new_tray->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      tray_inst.push_back(new_tray);
    }
    mapping[i].first = old_to_new_idx[tray_idx];
  }

  odb::dbBTerm* clk_term = nullptr;
  for (int i = 0; i < num_flops; i++) {
    // single bit flop?
    if (mapping[i].first == std::numeric_limits<int>::max()) {
      continue;
    }

    int tray_idx = mapping[i].first;
    int tray_sz_idx = GetBitIdx(GetNumSlots(tray_inst[tray_idx]));
    int slot_idx = mapping[i].second;

    // find the new port names
    std::string d_pin;
    std::string q_pin;
    int idx = 0;
    for (auto pins : pin_mappings_[tray_sz_idx]) {
      if (idx == slot_idx) {
        d_pin = pins.first;
        q_pin = pins.second;
        break;
      }
      idx++;
    }

    // disconnect / reconnect iterms
    for (auto iterm : insts_[flops[i].idx]->getITerms()) {
      if (IsSupplyPin(iterm)) {
        continue;
      }
      auto net = iterm->getNet();
      if (!net) {
        continue;
      }
      iterm->disconnect();
      if (IsDPin(iterm)) {
        tray_inst[tray_idx]->findITerm(d_pin.c_str())->connect(net);
      }
      if (IsQPin(iterm)) {
        tray_inst[tray_idx]->findITerm(q_pin.c_str())->connect(net);
      }
      if (IsClockPin(iterm)) {
        for (auto bterm : net->getBTerms()) {
          bterm->disconnect();
          clk_term = bterm;
        }
        iterm->disconnect();
        odb::dbNet::destroy(net);
      }
    }
  }

  std::vector<bool> isConnected(static_cast<int>(tray_inst.size()));
  for (int i = 0; i < num_flops; i++) {
    if (mapping[i].first != std::numeric_limits<int>::max()) {
      if (!isConnected[mapping[i].first] && clk_term != nullptr) {
        std::string name = "_net_" + std::to_string(flops[i].idx) + "_to_clk";
        odb::dbNet* net = odb::dbNet::create(block_, name.c_str());
        clk_term->connect(net);
        for (auto iterm : tray_inst[mapping[i].first]->getITerms()) {
          if (IsClockPin(iterm)) {
            iterm->connect(net);
          }
        }
        isConnected[mapping[i].first] = 1;
      }
      odb::dbInst::destroy(insts_[flops[i].idx]);
    }
  }
}

/*
This LP finds new tray centers such that the sum of displacements from the new
trays' slots to their matching flops is minimized
*/

double MBFF::RunLP(const std::vector<Flop>& flops,
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

    const Point& flop = flops[i].pt;
    const Point& tray = trays[tray_idx].pt;
    const Point& slot = trays[tray_idx].slots[slot_idx];

    double shift_x = slot.x - tray.x;
    double shift_y = slot.y - tray.y;

    operations_research::MPConstraint* c1
        = solver->MakeRowConstraint(shift_x - flop.x, inf, "");
    c1->SetCoefficient(disp_x[i], 1);
    c1->SetCoefficient(tray_x[tray_idx], -1);

    operations_research::MPConstraint* c2
        = solver->MakeRowConstraint(flop.x - shift_x, inf, "");
    c2->SetCoefficient(disp_x[i], 1);
    c2->SetCoefficient(tray_x[tray_idx], 1);

    operations_research::MPConstraint* c3
        = solver->MakeRowConstraint(shift_y - flop.y, inf, "");
    c3->SetCoefficient(disp_y[i], 1);
    c3->SetCoefficient(tray_y[tray_idx], -1);

    operations_research::MPConstraint* c4
        = solver->MakeRowConstraint(flop.y - shift_y, inf, "");
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

  double tot_disp = 0;
  for (int i = 0; i < num_trays; i++) {
    double new_x = tray_x[i]->solution_value();
    double new_y = tray_y[i]->solution_value();

    Tray new_tray;
    new_tray.pt = Point{new_x, new_y};

    tot_disp += GetDist(trays[i].pt, new_tray.pt);
    trays[i].pt = new_tray.pt;
  }

  return tot_disp;
}

/*
this ILP finds a set of trays (given all of the tray candidates from capacitated
k-means) such that
(1) each flop gets mapped to exactly one slot (or, stays a single bit flop)
(2) [(a) + (b)] is minimized, where
        (a) = sum of all displacements from a flop to its slot
        (b) = alpha * (sum of the chosen tray costs)

we ignore timing-critical path constraints /
objectives so that the algorithm is scalable
*/

double MBFF::RunILP(const std::vector<Flop>& flops,
                    const std::vector<Tray>& trays,
                    std::vector<std::pair<int, int>>& final_flop_to_slot,
                    double alpha)
{
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
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      disp_x[i].push_back(
          cp_model.NewIntVar(operations_research::Domain(0, inf)));
      disp_y[i].push_back(
          cp_model.NewIntVar(operations_research::Domain(0, inf)));
      mapped[i].push_back(cp_model.NewBoolVar());
    }
  }

  // add constraints for displacements
  double max_dist = 0;
  for (int i = 0; i < num_flops; i++) {
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      const int tray_idx = cand_tray[i][j];
      const int slot_idx = cand_slot[i][j];

      const double shift_x = trays[tray_idx].slots[slot_idx].x - flops[i].pt.x;
      const double shift_y = trays[tray_idx].slots[slot_idx].y - flops[i].pt.y;

      max_dist = std::max(
          max_dist, std::max(shift_x, -shift_x) + std::max(shift_y, -shift_y));

      // absolute value constraints for x
      cp_model.AddLessOrEqual(
          0, disp_x[i][j] - int(multiplier_ * shift_x) * mapped[i][j]);
      cp_model.AddLessOrEqual(
          0, disp_x[i][j] + int(multiplier_ * shift_x) * mapped[i][j]);

      // absolute value constraints for y
      cp_model.AddLessOrEqual(
          0, disp_y[i][j] - int(multiplier_ * shift_y) * mapped[i][j]);
      cp_model.AddLessOrEqual(
          0, disp_y[i][j] + int(multiplier_ * shift_y) * mapped[i][j]);
    }
  }

  for (int i = 0; i < num_flops; i++) {
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      cp_model.AddLessOrEqual(disp_x[i][j] + disp_y[i][j],
                              int(multiplier_ * max_dist));
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
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
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
    for (size_t j = 0; j < trays[i].slots.size(); j++) {
      if (trays[i].cand[j] >= 0) {
        flop_ind.push_back(trays[i].cand[j]);
      }
    }

    for (size_t j = 0; j < flop_ind.size(); j++) {
      // TODO: SWITCH TO BINARY SEARCH
      int tray_idx = 0;
      for (size_t k = 0; k < cand_tray[flop_ind[j]].size(); k++) {
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
  std::vector<double> tray_cost(num_trays);
  for (int i = 0; i < num_trays; i++) {
    int bit_idx = 0;
    for (int j = 0; j < num_sizes_; j++) {
      if (best_master_[j] != nullptr) {
        if (GetBitCnt(j) == static_cast<int>(trays[i].slots.size())) {
          bit_idx = j;
        }
      }
    }
    if (GetBitCnt(bit_idx) == 1) {
      tray_cost[i] = 1.00;
    } else {
      tray_cost[i] = (GetBitCnt(bit_idx) * ratios_[bit_idx]);
    }
  }

  /*
    DoubleLinearExpr supports double coefficients
    can only be used for the objective function
  */

  operations_research::sat::DoubleLinearExpr obj;

  // add the sum of all distances
  for (int i = 0; i < num_flops; i++) {
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      obj.AddTerm(disp_x[i][j], coeff[i] * (1 / multiplier_));
      obj.AddTerm(disp_y[i][j], coeff[i] * (1 / multiplier_));
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
      for (size_t j = 0; j < cand_tray[i].size(); j++) {
        if (operations_research::sat::SolutionIntegerValue(response,
                                                           mapped[i][j])
            == 1) {
          slot_disp_x_[flops[i].idx]
              = trays[cand_tray[i][j]].slots[cand_slot[i][j]].x - flops[i].pt.x;
          slot_disp_y_[flops[i].idx]
              = trays[cand_tray[i][j]].slots[cand_slot[i][j]].y - flops[i].pt.y;
          final_flop_to_slot[i]
              = std::make_pair(cand_tray[i][j], cand_slot[i][j]);
        }
      }
    }
    return static_cast<double>(response.objective_value());
  }
  return 0;
}

void MBFF::GetSlots(const Point& tray,
                    int rows,
                    int cols,
                    std::vector<Point>& slots)
{
  slots.clear();
  int idx = GetBitIdx(rows * cols);
  for (int i = 0; i < rows * cols; i++) {
    Point pt = Point{tray.x + slot_to_tray_x_[idx][i],
                     tray.y + slot_to_tray_y_[idx][i]};
    slots.push_back(pt);
  }
}

Flop MBFF::GetNewFlop(const std::vector<Flop>& prob_dist, double tot_dist)
{
  double rand_num = (double) (rand() % 101), cum_sum = 0;
  Flop new_flop;
  for (size_t i = 0; i < prob_dist.size(); i++) {
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
                         double AR,
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

  double tot_dist = 0;
  for (int i = 0; i < num_flops; i++) {
    double contr = GetDist(flops[i].pt, tray_zero.pt) / AR;
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
      double new_contr = GetDist(flops[i].pt, new_tray.pt) / AR;
      flops[i].prob += new_contr, tot_dist += new_contr;
    }
  }
}

Tray MBFF::GetOneBit(const Point& pt)
{
  double new_x = pt.x;
  double new_y = pt.y;

  Tray tray;
  tray.pt = Point{new_x, new_y};
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
    std::vector<Point> tray_slots = trays[i].slots;

    for (size_t j = 0; j < tray_slots.size(); j++) {
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

  for (size_t i = 0; i < edges.size(); i++) {
    costs[edges[i]] = cur_costs[i], caps[edges[i]] = cur_caps[i];
  }

  labels[src] = -1;
  for (size_t i = 0; i < nodes.size(); i++) {
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

double MBFF::GetSilh(const std::vector<Flop>& flops,
                     const std::vector<Tray>& trays,
                     const std::vector<std::pair<int, int>>& clusters)
{
  int num_flops = static_cast<int>(flops.size());
  int num_trays = static_cast<int>(trays.size());

  double tot = 0;
  for (int i = 0; i < num_flops; i++) {
    double min_num = std::numeric_limits<double>::max();
    double max_den = GetDist(
        flops[i].pt, trays[clusters[i].first].slots[clusters[i].second]);
    for (int j = 0; j < num_trays; j++) {
      if (j != clusters[i].first) {
        max_den = std::max(
            max_den, GetDist(flops[i].pt, trays[j].slots[clusters[i].second]));
        for (size_t k = 0; k < trays[j].slots.size(); k++) {
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

  double delta = 0;
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

void MBFF::RunSilh(std::vector<std::vector<Tray>>& trays,
                   const std::vector<Flop>& flops,
                   std::vector<std::vector<std::vector<Tray>>>& start_trays)
{
  int num_flops = static_cast<int>(flops.size());
  trays.resize(num_sizes_);
  for (int i = 0; i < num_sizes_; i++) {
    int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);
    trays[i].resize(num_trays);
  }

  // add 1-bit trays
  for (int i = 0; i < num_flops; i++) {
    Tray one_bit = GetOneBit(flops[i].pt);
    one_bit.cand.reserve(1);
    one_bit.cand.emplace_back(i);
    trays[0][i] = one_bit;
  }

  std::vector<double> res[num_sizes_];
  std::vector<std::pair<int, int>> ind;

  for (int i = 1; i < num_sizes_; i++) {
    if (best_master_[i] != nullptr) {
      for (int j = 0; j < 5; j++) {
        ind.push_back(std::make_pair(i, j));
      }
      res[i].resize(5);
    }
  }

  for (int i = 1; i < num_sizes_; i++) {
    if (best_master_[i] != nullptr) {
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
  }

  // run multistart_ in parallel
  for (size_t i = 0; i < ind.size(); i++) {
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

  for (int i = 1; i < num_sizes_; i++) {
    if (best_master_[i] != nullptr) {
      int opt_idx = 0;
      double opt_val = -1;
      for (int j = 0; j < 5; j++) {
        if (res[i][j] > opt_val) {
          opt_val = res[i][j];
          opt_idx = j;
        }
      }
      trays[i] = start_trays[i][opt_idx];
    }
  }
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

  std::vector<double> d(num_flops);
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
    double prob = rnd / 100.0;

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
  double prev = -1;
  while (true) {
    for (int i = 0; i < knn_; i++) {
      clusters[i].clear();
    }

    // remap flops to clusters
    for (int i = 0; i < num_flops; i++) {
      double min_cost = std::numeric_limits<double>::max();
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
      double cX = 0;
      double cY = 0;

      for (size_t j = 0; j < clusters[i].size(); j++) {
        cX += clusters[i][j].pt.x;
        cY += clusters[i][j].pt.y;
      }

      double new_x = cX / double(cur_sz);
      double new_y = cY / double(cur_sz);
      centers[i].pt = Point{new_x, new_y};
    }

    // get total displacement
    double tot_disp = 0;
    for (int i = 0; i < knn_; i++) {
      for (size_t j = 0; j < clusters[i].size(); j++) {
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
  if (MAX_SZ == -1 || num_flops <= MAX_SZ) {
    pointsets.push_back(flops);
    return;
  }

  std::vector<std::vector<Flop>> tmp_clusters[multistart_];
  std::vector<double> tmp_costs(multistart_);

  // multistart_ K-means++
  for (int i = 0; i < multistart_; i++) {
    KMeans(flops, tmp_clusters[i]);

    /* cur_cost = sum of distances between flops and its
    matching cluster's center */
    double cur_cost = 0;
    for (int j = 0; j < knn_; j++) {
      for (size_t k = 0; k + 1 < tmp_clusters[i][j].size(); k++) {
        cur_cost
            += GetDist(tmp_clusters[i][j][k].pt, tmp_clusters[i][j].back().pt);
      }
    }
    tmp_costs[i] = cur_cost;
  }

  double best_cost = std::numeric_limits<double>::max();
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

  std::vector<std::pair<double, std::pair<int, int>>> cluster_pairs;
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

  for (size_t i = 0; i < cluster_pairs.size(); i++) {
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

double MBFF::GetTCPDisplacement()
{
  double ret = 0;
  for (const auto& path : paths_) {
    double diff_x = 0;
    double diff_y = 0;
    diff_x += slot_disp_x_[path.start_point];
    diff_y += slot_disp_y_[path.start_point];
    diff_x -= slot_disp_x_[path.end_point];
    diff_y -= slot_disp_y_[path.end_point];
    ret += (std::max(diff_x, -diff_x) + std::max(diff_y, -diff_y));
  }
  return ret;
}

double MBFF::doit(const std::vector<Flop>& flops,
                  int mx_sz,
                  double alpha,
                  double beta)
{
  // run k-means++ based decomposition
  std::vector<std::vector<Flop>> pointsets;
  KMeansDecomp(flops, mx_sz, pointsets);

  // all_start_trays[t][i][j]: start trays of size 2^i, multistart = j for
  // pointset[t]
  std::vector<std::vector<std::vector<Tray>>>
      all_start_trays[static_cast<int>(pointsets.size())];
  for (int t = 0; t < static_cast<int>(pointsets.size()); t++) {
    all_start_trays[t].resize(num_sizes_);
    for (int i = 1; i < num_sizes_; i++) {
      if (best_master_[i] != nullptr) {
        int rows = GetRows(GetBitCnt(i));
        int cols = GetBitCnt(i) / rows;
        double AR = (cols * width_ * ratios_[i]) / (rows * height_);
        int num_trays
            = (static_cast<int>(pointsets[t].size()) + (GetBitCnt(i) - 1))
              / GetBitCnt(i);
        all_start_trays[t][i].resize(5);
        for (int j = 0; j < 5; j++) {
          // running in parallel ==> not reproducible
          GetStartTrays(pointsets[t], num_trays, AR, all_start_trays[t][i][j]);
        }
      }
    }
  }

  double ans = 0;
  std::vector<std::pair<int, int>>
      all_mappings[static_cast<int>(pointsets.size())];
  std::vector<Tray> all_final_trays[static_cast<int>(pointsets.size())];

  int num_pointsets = static_cast<int>(pointsets.size());
#pragma omp parallel for num_threads(num_threads_)
  for (int t = 0; t < num_pointsets; t++) {
    std::vector<std::vector<Tray>> cur_trays;
    RunSilh(cur_trays, pointsets[t], all_start_trays[t]);

    // run capacitated k-means per tray size
    int num_flops = static_cast<int>(pointsets[t].size());
    for (int i = 1; i < num_sizes_; i++) {
      if (best_master_[i] != nullptr) {
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
    }
    for (int i = 0; i < num_sizes_; i++) {
      if (!i || best_master_[i] != nullptr) {
        all_final_trays[t].insert(
            all_final_trays[t].end(), cur_trays[i].begin(), cur_trays[i].end());
      }
    }
    std::vector<std::pair<int, int>> mapping(
        static_cast<int>(pointsets[t].size()));
    double cur_ans = RunILP(pointsets[t], all_final_trays[t], mapping, alpha);
    all_mappings[t] = mapping;
    ans += cur_ans;
  }

  for (int t = 0; t < num_pointsets; t++) {
    ModifyPinConnections(pointsets[t], all_final_trays[t], all_mappings[t]);
  }

  return ans;
}

void MBFF::SetVars(const std::vector<Flop>& flops)
{
  // get min height and width
  height_ = std::numeric_limits<double>::max();
  width_ = std::numeric_limits<double>::max();
  for (size_t i = 0; i < flops.size(); i++) {
    height_ = std::min(
        height_, insts_[flops_[i].idx]->getMaster()->getHeight() / multiplier_);
    width_ = std::min(
        width_, insts_[flops_[i].idx]->getMaster()->getWidth() / multiplier_);
  }
}

void MBFF::SetRatios()
{
  ratios_.clear();
  ratios_.push_back(1.00);
  for (int i = 1; i < num_sizes_; i++) {
    ratios_.push_back(std::numeric_limits<float>::max());
    if (best_master_[i] != nullptr) {
      int slot_cnt = GetBitCnt(i);
      int height = GetRows(i);
      int slot_width = slot_cnt / height;
      ratios_[i]
          = (tray_width_[i] / (width_ * (static_cast<float>(slot_width))));
      log_->info(utl::GPL,
                 1002,
                 "Ratio for tray size {}: {}",
                 GetBitCnt(i),
                 ratios_[i]);
    }
  }
}

void MBFF::SeparateFlops(std::vector<std::vector<Flop>>& ffs)
{
  // group by block clock name
  std::map<std::string, std::vector<int>> clk_terms;
  for (size_t i = 0; i < flops_.size(); i++) {
    if (insts_[i]->isDoNotTouch()) {
      continue;
    }
    for (auto iterm : insts_[i]->getITerms()) {
      if (IsClockPin(iterm)) {
        auto net = iterm->getNet();
        if (!net) {
          continue;
        }
        std::string name = (*(net->getBTerms().begin()))->getName();
        clk_terms[name].push_back(i);
      }
    }
  }

  for (auto clks : clk_terms) {
    std::vector<Flop> flops;
    for (int idx : clks.second) {
      flops.push_back(flops_[idx]);
    }
    ffs.push_back(flops);
  }
}

void MBFF::SetTrayNames()
{
  for (size_t i = 0; i < 2 * (flops_.size()); i++) {
    unused_.push_back(i);
  }
}

void MBFF::Run(int mx_sz, double alpha, double beta)
{
  srand(1);
  omp_set_num_threads(num_threads_);

  ReadFFs();
  ReadPaths();
  ReadLibs();
  SetTrayNames();

  std::vector<std::vector<Flop>> FFs;
  SeparateFlops(FFs);
  int num_chunks = static_cast<int>(FFs.size());
  double tot_ilp = 0;
  for (int i = 0; i < num_chunks; i++) {
    SetVars(FFs[i]);
    SetRatios();
    tot_ilp += doit(FFs[i], mx_sz, alpha, beta);
  }
  double tcp_disp = (beta * GetTCPDisplacement());

  // round
  tot_ilp = std::ceil(tot_ilp * multiplier_);
  tot_ilp /= multiplier_;

  tcp_disp = std::ceil(tcp_disp * multiplier_);
  tcp_disp /= multiplier_;

  log_->report("Alpha = {}, Beta = {}, max size = {}", alpha, beta, mx_sz);
  log_->report("Total ILP Cost: {}", tot_ilp);
  log_->report("Total Timing Critical Path Displacement: {}", tcp_disp);
  log_->report("Final Objective Value: {}", tot_ilp + tcp_disp);
}

void MBFF::ReadLibs()
{
  best_master_.resize(num_sizes_, nullptr);
  tray_area_.resize(num_sizes_, std::numeric_limits<double>::max());
  tray_width_.resize(num_sizes_);
  pin_mappings_.resize(num_sizes_);

  slot_to_tray_x_.resize(num_sizes_);
  slot_to_tray_y_.resize(num_sizes_);

  for (auto lib : db_->getLibs()) {
    for (auto master : lib->getMasters()) {
      odb::dbInst* tmp_tray = odb::dbInst::create(block_, master, "test_tray");
      if (!IsValidTray(tmp_tray)) {
        odb::dbInst::destroy(tmp_tray);
        continue;
      }
      int num_slots = GetNumSlots(tmp_tray);
      int idx = GetBitIdx(num_slots);
      double cur_area = (master->getHeight() / multiplier_)
                        * (master->getWidth() / multiplier_);
      if (tray_area_[idx] > cur_area) {
        tray_area_[idx] = cur_area;
        best_master_[idx] = master;
        pin_mappings_[idx] = GetPinMapping(tmp_tray);
        tray_width_[idx]
            = static_cast<double>(master->getWidth()) / multiplier_;

        // save slot info
        tmp_tray->setLocation(0, 0);
        tmp_tray->setPlacementStatus(odb::dbPlacementStatus::PLACED);

        slot_to_tray_x_[idx].clear();
        slot_to_tray_y_[idx].clear();
        std::vector<Point> d(num_slots);
        std::vector<Point> q(num_slots);

        int itr = 0;
        for (auto p : pin_mappings_[idx]) {
          auto d_pin = tmp_tray->findITerm(p.first.c_str());
          auto q_pin = tmp_tray->findITerm(p.second.c_str());
          d[itr] = Point{
              d_pin->getBBox().xCenter() / multiplier_,
              d_pin->getBBox().yCenter() / multiplier_,
          };
          q[itr] = Point{
              q_pin->getBBox().xCenter() / multiplier_,
              q_pin->getBBox().yCenter() / multiplier_,
          };
          itr++;
        }
        for (int i = 0; i < num_slots; i++) {
          slot_to_tray_x_[idx].push_back(
              (std::max(d[i].x, q[i].x) + std::min(d[i].x, q[i].x)) / 2.0);
          slot_to_tray_y_[idx].push_back(
              (std::max(d[i].y, q[i].y) + std::min(d[i].y, q[i].y)) / 2.0);
        }
      }
      odb::dbInst::destroy(tmp_tray);
    }
  }

  for (int i = 1; i < num_sizes_; i++) {
    if (best_master_[i] != nullptr) {
      log_->info(utl::GPL,
                 1001,
                 "Name of master with minimum area for size = {}: {}",
                 GetBitCnt(i),
                 best_master_[i]->getName());
      for (size_t j = 0; j < slot_to_tray_x_[i].size(); j++) {
        log_->info(utl::GPL,
                   1010,
                   "delta x: {}, delta y: {}",
                   slot_to_tray_x_[i][j],
                   slot_to_tray_y_[i][j]);
      }
    }
  }
}

void MBFF::ReadFFs()
{
  int num_flops = 0;
  for (auto inst : block_->getInsts()) {
    sta::Cell* cell = network_->dbToSta(inst->getMaster());
    sta::LibertyCell* lib_cell = network_->libertyCell(cell);
    if (lib_cell->hasSequentials()) {
      int x_i;
      int y_i;
      inst->getOrigin(x_i, y_i);
      Point pt{x_i / multiplier_, y_i / multiplier_};
      flops_.push_back({pt, num_flops, 0.0});
      insts_.push_back(inst);
      num_flops++;
    }
  }
  slot_disp_x_.resize(num_flops, 0.0);
  slot_disp_y_.resize(num_flops, 0.0);
}

// COMPLETE
void MBFF::ReadPaths()
{
  return;
}

// ctor
MBFF::MBFF(odb::dbDatabase* db,
           sta::dbSta* sta,
           utl::Logger* log,
           int threads,
           int knn,
           int multistart)
{
  // max tray size: 1 << (num_sizes_ - 1)
  num_sizes_ = 7;
  db_ = db;
  block_ = db_->getChip()->getBlock();
  sta_ = sta;
  network_ = sta_->getDbNetwork();
  log_ = log;
  multiplier_ = static_cast<double>(block_->getDbUnitsPerMicron());
  num_threads_ = threads;
  knn_ = knn;
  multistart_ = multistart;
}

// dtor
MBFF::~MBFF() = default;

}  // end namespace gpl
