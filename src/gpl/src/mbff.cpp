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
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <ctime>
#include <chrono>
#include <memory>
#include <random>
#include <omp.h>
#include <ortools/linear_solver/linear_solver.h>
#include <ortools/base/logging.h>
#include <ortools/sat/cp_model.h>
#include <ortools/sat/cp_model.pb.h>
#include <ortools/sat/cp_model_solver.h>
#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>
#include <lemon/maps.h>

/*
height and width of a flip-flop
ratios are used for:
    (1) finding all slot locations in a tray
    (2) finding power consumption per slot
*/

namespace gpl {

constexpr int NUM_SIZES = 6;
constexpr float WIDTH = 2.448, HEIGHT = 1.2;
constexpr float RATIOS[NUM_SIZES] = { 1, 0.875, 0.854, 0.854, 0.844, 0.844 };

int MBFF::GetBitCnt(int bit_idx) { return (1 << bit_idx); }

int MBFF::GetRows(int slot_cnt) {
    if (slot_cnt == 1 || slot_cnt == 2 || slot_cnt == 4) {
        return 1;
    }
    if (slot_cnt == 8) {
        return 2;
    } else {
        return 4;
    }
}

float MBFF::GetDist(const odb::Point &a, const odb::Point &b) {
    return (abs(a.x() - b.x()) + abs(a.y() - b.y()));
}

/*
This LP finds new tray centers such that the sum of displacements from the new
trays' slots to their matching flops is minimized
*/

float MBFF::RunLP(const std::vector<Flop> &flops, std::vector<Tray> &trays,
                  const std::vector<std::pair<int, int> > &clusters) {

    int num_flops = static_cast<int>(flops.size());
    int num_trays = static_cast<int>(trays.size());

    std::unique_ptr<operations_research::MPSolver> solver(
        operations_research::MPSolver::CreateSolver("GLOP"));
    double inf = solver->infinity();

    std::vector<operations_research::MPVariable *> tray_x(num_trays);
    std::vector<operations_research::MPVariable *> tray_y(num_trays);

    for (int i = 0; i < num_trays; i++) {
        tray_x[i] = solver->MakeNumVar(0, inf, "");
        tray_y[i] = solver->MakeNumVar(0, inf, "");
    }

    // displacement from flop to tray slot
    std::vector<operations_research::MPVariable *> disp_x(num_flops);
    std::vector<operations_research::MPVariable *> disp_y(num_flops);

    for (int i = 0; i < num_flops; i++) {

        disp_x[i] = solver->MakeNumVar(0, inf, "");
        disp_y[i] = solver->MakeNumVar(0, inf, "");

        int tray_idx = clusters[i].first;
        int slot_idx = clusters[i].second;

        const odb::Point &flop = flops[i].pt;
        const odb::Point &tray = trays[tray_idx].pt;
        const odb::Point &slot = trays[tray_idx].slots[slot_idx];

        float shift_x = slot.x() - tray.x();
        float shift_y = slot.y() - tray.y();

        operations_research::MPConstraint *c1 =
            solver->MakeRowConstraint(shift_x - flop.x(), inf, "");
        c1->SetCoefficient(disp_x[i], 1);
        c1->SetCoefficient(tray_x[tray_idx], -1);

        operations_research::MPConstraint *c2 =
            solver->MakeRowConstraint(flop.x() - shift_x, inf, "");
        c2->SetCoefficient(disp_x[i], 1);
        c2->SetCoefficient(tray_x[tray_idx], 1);

        operations_research::MPConstraint *c3 =
            solver->MakeRowConstraint(shift_y - flop.y(), inf, "");
        c3->SetCoefficient(disp_y[i], 1);
        c3->SetCoefficient(tray_y[tray_idx], -1);

        operations_research::MPConstraint *c4 =
            solver->MakeRowConstraint(flop.y() - shift_y, inf, "");
        c4->SetCoefficient(disp_y[i], 1);
        c4->SetCoefficient(tray_y[tray_idx], 1);
    }

    operations_research::MPObjective *objective = solver->MutableObjective();
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
This ILP finds a set of trays (given all of the tray candidates from capacitated
k-means) such that
(1) each flop gets mapped to exactly one slot
(2) [(a) + (b) + (c)] is minimized, where
        (a) = sum of displacements from a flop to its slot
        (b) = beta * (sum of displacements from the start to the end of a
timing-critical path)
        (c) = alpha * (sum of the chosen tray costs)
*/

void MBFF::RunILP(const std::vector<Flop> &flops,
                  const std::vector<Path> &paths,
                  const std::vector<std::vector<Tray> > &all_trays, float alpha,
                  float beta) {
    std::vector<Tray> trays;
    for (int i = 0; i < NUM_SIZES; i++) {
        for (int j = 0; j < static_cast<int>(all_trays[i].size()); j++) {
            trays.push_back(all_trays[i][j]);
        }
    }

    int num_flops = static_cast<int>(flops.size());
    int num_paths = static_cast<int>(paths.size());
    int num_trays = static_cast<int>(trays.size());

    /*
    NOTE: CP-SAT constraints only work with INTEGERS
    so, all coefficients (shift_x and shift_y) are multiplied by 100
    */
    operations_research::sat::CpModelBuilder cp_model;
    int inf = std::numeric_limits<int>::max();

    // has a flop been mapped to a slot?
    std::vector<operations_research::sat::BoolVar> mapped[num_flops][num_trays];

    // displacement from flop to slot
    std::vector<operations_research::sat::IntVar> disp_x[num_flops][num_trays];
    std::vector<operations_research::sat::IntVar> disp_y[num_flops][num_trays];

    for (int i = 0; i < num_flops; i++) {
        for (int j = 0; j < num_trays; j++) {
            disp_x[i][j].resize(static_cast<int>(trays[j].slots.size()));
            disp_y[i][j].resize(static_cast<int>(trays[j].slots.size()));
            mapped[i][j].resize(static_cast<int>(trays[j].slots.size()));

            for (int k = 0; k < static_cast<int>(trays[j].slots.size()); k++) {
                if (trays[j].cand[k] == i) {
                    disp_x[i][j][k] =
                        cp_model.NewIntVar(operations_research::Domain(0, inf));
                    disp_y[i][j][k] =
                        cp_model.NewIntVar(operations_research::Domain(0, inf));
                    mapped[i][j][k] = cp_model.NewBoolVar();
                }
            }
        }
    }

    float max_dist = 0;
    for (int i = 0; i < num_flops; i++) {
        for (int j = 0; j < num_trays; j++) {

            std::vector<odb::Point> slots = trays[j].slots;

            for (int k = 0; k < static_cast<int>(slots.size()); k++) {
                if (trays[j].cand[k] == i) {

                    const float shift_x = slots[k].x() - flops[i].pt.x();
                    const float shift_y = slots[k].y() - flops[i].pt.y();

                    max_dist =
                        std::max(max_dist, std::max(shift_x, -shift_x) +
                                               std::max(shift_y, -shift_y));

                    // absolute value constraints for x
                    cp_model.AddLessOrEqual(
                        0,
                        disp_x[i][j][k] - int(100 * shift_x) * mapped[i][j][k]);
                    cp_model.AddLessOrEqual(
                        0,
                        disp_x[i][j][k] + int(100 * shift_x) * mapped[i][j][k]);

                    // absolute value constraints for y
                    cp_model.AddLessOrEqual(
                        0,
                        disp_y[i][j][k] - int(100 * shift_y) * mapped[i][j][k]);
                    cp_model.AddLessOrEqual(
                        0,
                        disp_y[i][j][k] + int(100 * shift_y) * mapped[i][j][k]);
                }
            }
        }
    }

    for (int i = 0; i < num_flops; i++) {
        for (int j = 0; j < num_trays; j++) {
            for (int k = 0; k < static_cast<int>(trays[j].slots.size()); k++) {
                if (trays[j].cand[k] == i) {
                    cp_model.AddLessOrEqual(disp_x[i][j][k] + disp_y[i][j][k],
                                            int(100 * max_dist));
                }
            }
        }
    }

    // add constraints for Z
    std::vector<operations_research::sat::IntVar> disp_path_x(num_paths);
    std::vector<operations_research::sat::IntVar> disp_path_y(num_paths);
    for (int i = 0; i < num_paths; i++) {
        disp_path_x[i] =
            cp_model.NewIntVar(operations_research::Domain(0, inf));
        disp_path_y[i] =
            cp_model.NewIntVar(operations_research::Domain(0, inf));
    }

    for (int i = 0; i < num_paths; i++) {
        const int flop_a_idx = paths[i].a;
        const int flop_b_idx = paths[i].b;

        operations_research::sat::LinearExpr sum_disp_x_flop_a;
        operations_research::sat::LinearExpr sum_disp_y_flop_a;

        operations_research::sat::LinearExpr sum_disp_x_flop_b;
        operations_research::sat::LinearExpr sum_disp_y_flop_b;

        for (int j = 0; j < num_trays; j++) {
            std::vector<odb::Point> slots = trays[j].slots;
            for (int k = 0; k < static_cast<int>(slots.size()); k++) {
                if (trays[j].cand[k] == flop_a_idx) {
                    float shift_x = slots[k].x() - flops[flop_a_idx].pt.x();
                    float shift_y = slots[k].y() - flops[flop_a_idx].pt.y();
                    sum_disp_x_flop_a +=
                        (int(100 * shift_x) * mapped[flop_a_idx][j][k]);
                    sum_disp_y_flop_a +=
                        (int(100 * shift_y) * mapped[flop_a_idx][j][k]);
                }
                if (trays[j].cand[k] == flop_b_idx) {
                    float shift_x = slots[k].x() - flops[flop_b_idx].pt.x();
                    float shift_y = slots[k].y() - flops[flop_b_idx].pt.y();
                    sum_disp_x_flop_b +=
                        (int(100 * shift_x) * mapped[flop_b_idx][j][k]);
                    sum_disp_y_flop_b +=
                        (int(100 * shift_y) * mapped[flop_b_idx][j][k]);
                }
            }
        }

        // absolute value constraints for x
        cp_model.AddLessOrEqual(0, disp_path_x[i] + sum_disp_x_flop_a -
                                       sum_disp_x_flop_b);
        cp_model.AddLessOrEqual(0, disp_path_x[i] - sum_disp_x_flop_a +
                                       sum_disp_x_flop_b);

        // absolute value constraints for y
        cp_model.AddLessOrEqual(0, disp_path_y[i] + sum_disp_y_flop_a -
                                       sum_disp_y_flop_b);
        cp_model.AddLessOrEqual(0, disp_path_y[i] - sum_disp_y_flop_a +
                                       sum_disp_y_flop_b);
    }

    for (int i = 0; i < num_paths; i++) {
        cp_model.AddLessOrEqual(disp_path_x[i] + disp_path_y[i],
                                int(100 * max_dist));
    }

    // check that each slot is mapped to a maxmimum of one flop
    for (int i = 0; i < num_trays; i++) {
        for (int j = 0; j < static_cast<int>(trays[i].slots.size()); j++) {
            operations_research::sat::LinearExpr mapped_slot;
            for (int k = 0; k < num_flops; k++) {
                if (trays[i].cand[j] == k) {
                    mapped_slot += mapped[k][i][j];
                }
            }
            cp_model.AddLessOrEqual(0, mapped_slot);
            cp_model.AddLessOrEqual(mapped_slot, 1);
        }
    }

    // check that each flop is matched to a single slot
    for (int i = 0; i < num_flops; i++) {
        operations_research::sat::LinearExpr mapped_flop;
        for (int j = 0; j < num_trays; j++) {
            for (int k = 0; k < static_cast<int>(trays[j].slots.size()); k++) {
                if (trays[j].cand[k] == i) {
                    mapped_flop += mapped[i][j][k];
                }
            }
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
        for (int j = 0; j < static_cast<int>(trays[i].slots.size()); j++) {
            for (int k = 0; k < num_flops; k++) {

                if (trays[i].cand[j] == k) {
                    // check that if we map a flop to a slot, that slot's tray
                    // is used
                    cp_model.AddLessOrEqual(0, tray_used[i] - mapped[k][i][j]);
                    slots_used += mapped[k][i][j];
                }
            }
        }

        // check that we use at least one slot if a tray is used
        cp_model.AddLessOrEqual(0, slots_used - tray_used[i]);
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
            tray_cost[i] = ((float)GetBitCnt(bit_idx)) *
                           (RATIOS[bit_idx] - GetBitCnt(bit_idx) * 0.0015);
        }
    }

    /*
        - DoubleLinearExpr support double coefficients
        - can only be used for the objective function
    */

    operations_research::sat::DoubleLinearExpr obj;

    // add the sum of all distances
    for (int i = 0; i < num_flops; i++) {
        for (int j = 0; j < num_trays; j++) {
            for (int k = 0; k < static_cast<int>(trays[j].slots.size()); k++) {
                if (trays[j].cand[k] == i) {
                    obj.AddTerm(disp_x[i][j][k], 0.01);
                    obj.AddTerm(disp_y[i][j][k], 0.01);
                }
            }
        }
    }

    // add the critical-timing path constraints
    for (int i = 0; i < num_paths; i++) {
        obj.AddTerm(disp_path_x[i], beta * 0.01);
        obj.AddTerm(disp_path_y[i], beta * 0.01);
    }

    // add the tray usage constraints
    for (int i = 0; i < num_trays; i++) {
        obj.AddTerm(tray_used[i], alpha * tray_cost[i]);
    }

    cp_model.Minimize(obj);

    operations_research::sat::Model model;
    operations_research::sat::SatParameters parameters;
    parameters.set_num_workers(num_threads);
    parameters.set_relative_gap_limit(0.01);
    model.Add(NewSatParameters(parameters));

    operations_research::sat::CpSolverResponse response =
        operations_research::sat::SolveCpModel(cp_model.Build(), &model);

    if (response.status() ==
            operations_research::sat::CpSolverStatus::FEASIBLE ||
        (int)operations_research::sat::CpSolverStatus::OPTIMAL) {

        log_->info(utl::GPL, 22, "Total = {}", (response.objective_value()));

        float sum_disp = 0;
        for (int i = 0; i < num_flops; i++) {
            for (int j = 0; j < num_trays; j++) {
                for (int k = 0; k < static_cast<int>(trays[j].slots.size());
                     k++) {
                    if (trays[j].cand[k] == i) {
                        sum_disp +=
                            (0.01 *
                             operations_research::sat::SolutionIntegerValue(
                                 response, disp_x[i][j][k]));
                        sum_disp +=
                            (0.01 *
                             operations_research::sat::SolutionIntegerValue(
                                 response, disp_y[i][j][k]));
                    }
                }
            }
        }

        log_->info(utl::GPL, 23, "Sum of displacements from flop to slot: {}",
                   sum_disp);

        float sum_disp_path = 0;
        for (int i = 0; i < num_paths; i++) {
            sum_disp_path +=
                (0.01 * operations_research::sat::SolutionIntegerValue(
                            response, disp_path_x[i]));
            sum_disp_path +=
                (0.01 * operations_research::sat::SolutionIntegerValue(
                            response, disp_path_y[i]));
        }

        log_->info(utl::GPL, 24, "Sum of timing-critical path displacements: {}",
                   sum_disp_path);

        float sum_tray_cost = 0;
        for (int i = 0; i < num_trays; i++) {
            if (operations_research::sat::SolutionIntegerValue(response,
                                                               tray_used[i])) {
                sum_tray_cost += tray_cost[i];
            }
        }

        log_->info(utl::GPL, 25, "Sum of tray costs: {}", sum_tray_cost);


        std::set<int> tray_idx;
        for (int i = 0; i < num_flops; i++) {
            for (int j = 0; j < num_trays; j++) {
                for (int k = 0; k < static_cast<int>(trays[j].slots.size());
                     k++) {
                    if (trays[j].cand[k] == i) {
                        if (operations_research::sat::SolutionIntegerValue(
                                response, mapped[i][j][k]) == 1) {
                            tray_idx.insert(j);
                        }
                    }
                }
            }
        }

        std::map<int, int> tray_sz;
        for (auto x : tray_idx) {
            tray_sz[static_cast<int>(trays[x].slots.size())]++;
        }

        for (auto x : tray_sz) {
            log_->error(utl::GPL, 26, "Tray size = {}: {}", x.first, x.second);
        }

    }

} // end ILP

std::vector<odb::Point> MBFF::GetSlots(const odb::Point &tray, int rows,
                                       int cols) {
    int bit_idx = 0;
    for (int i = 1; i < NUM_SIZES; i++) {
        if (rows * cols == GetBitCnt(i)) {
            bit_idx = i;
        }
    }

    float center_x = tray.x();
    float center_y = tray.y();

    std::vector<odb::Point> slots;
    for (int i = 0; i < rows * cols; i++) {
        int new_col = i % cols;
        int new_row = i / cols;

        float new_x = center_x + WIDTH * RATIOS[bit_idx] *
                                     ((new_col + 0.5) - (cols / 2.0));
        float new_y = center_y + HEIGHT * ((new_row + 0.5) - (rows / 2.0));

        odb::Point new_slot(new_x, new_y);
        if (new_slot.x() >= 0 && new_slot.y() >= 0) {
            slots.push_back(new_slot);
        }
    }

    return slots;
}

Flop MBFF::GetNewFlop(const std::vector<Flop> &prob_dist, float tot_dist) {
    float rand_num = (float)(rand() % 101), cum_sum = 0;
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

std::vector<Tray> MBFF::GetStartTrays(std::vector<Flop> flops, int num_trays,
                                      float AR) {
    int num_flops = static_cast<int>(flops.size());

    /* pick a random flop */
    int rand_idx = rand() % (num_flops);
    Tray tray_zero;
    tray_zero.pt = flops[rand_idx].pt;

    std::set<int> used_flops;
    used_flops.insert(rand_idx);
    std::vector<Tray> trays;
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

        std::sort(std::begin(prob_dist), std::end(prob_dist));

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

    return trays;
}

Tray MBFF::GetOneBit(const odb::Point &pt) {
    float new_x = pt.x() - WIDTH / 2.0;
    float new_y = pt.y() - HEIGHT / 2.0;

    Tray tray;
    tray.pt = odb::Point(new_x, new_y);
    tray.slots.push_back(pt);
    tray.cand.resize(1);

    return tray;
}

std::vector<std::pair<int, int> >
MBFF::MinCostFlow(const std::vector<Flop> &flops, std::vector<Tray> &trays,
                  int sz) {
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

    std::vector<std::pair<int, int> > slot_to_tray;

    for (int i = 0; i < num_trays; i++) {
        std::vector<odb::Point> tray_slots = trays[i].slots;

        for (int j = 0; j < static_cast<int>(tray_slots.size()); j++) {

            lemon::ListDigraph::Node slot_node = graph.addNode();
            nodes.push_back(slot_node);

            // add edges from flop to slot
            for (int k = 0; k < num_flops; k++) {
                lemon::ListDigraph::Arc flop_to_slot =
                    graph.addArc(nodes[k], slot_node);
                edges.push_back(flop_to_slot);

                int edge_cost = (100 * GetDist(flops[k].pt, tray_slots[j]));
                cur_costs.push_back(edge_cost), cur_caps.push_back(1);
            }

            // add edges from slot to sink
            lemon::ListDigraph::Arc slot_to_sink =
                graph.addArc(slot_node, sink);
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
    std::vector<std::pair<int, int> > clusters(num_flops);
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

    return clusters;
}

float MBFF::GetSilh(const std::vector<Flop> &flops,
                    const std::vector<Tray> &trays,
                    const std::vector<std::pair<int, int> > &clusters) {
    int num_flops = static_cast<int>(flops.size());
    int num_trays = static_cast<int>(trays.size());

    float tot = 0;
    for (int i = 0; i < num_flops; i++) {
        float min_num = std::numeric_limits<float>::max();
        float max_den = GetDist(
            flops[i].pt, trays[clusters[i].first].slots[clusters[i].second]);
        for (int j = 0; j < num_trays; j++)
            if (j != clusters[i].first) {
                max_den = std::max(
                    max_den,
                    GetDist(flops[i].pt, trays[j].slots[clusters[i].second]));
                for (int k = 0; k < static_cast<int>(trays[j].slots.size());
                     k++) {
                    min_num = std::min(min_num,
                                       GetDist(flops[i].pt, trays[j].slots[k]));
                }
            }

        tot += (min_num / max_den);
    }

    return tot;
}

// standard K-means++ implementation
std::vector<std::vector<Flop> > MBFF::KMeans(const std::vector<Flop> &flops,
                                             int K) {
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
    while (static_cast<int>(chosen.size()) < K) {

        double tot_sum = 0;
        for (int i = 0; i < num_flops; i++)
            if (!chosen.count(i)) {
                for (int j : chosen) {
                    d[i] = std::min(d[i], GetDist(flops[i].pt, flops[j].pt));
                }
                tot_sum += (double(d[i]) * double(d[i]));
            }

        int rnd = rand() % (int(tot_sum * 100));
        float prob = rnd / 100.0;

        double cum_sum = 0;
        for (int i = 0; i < num_flops; i++)
            if (!chosen.count(i)) {
                cum_sum += (double(d[i]) * double(d[i]));
                if (cum_sum >= prob) {
                    chosen.insert(i);
                    centers.push_back(flops[i]);
                    break;
                }
            }
    }

    std::vector<std::vector<Flop> > clusters(K);

    float prev = -1;
    while (1) {

        for (int i = 0; i < K; i++) {
            clusters[i].clear();
        }

        // remap flops to clusters
        for (int i = 0; i < num_flops; i++) {

            float min_cost = std::numeric_limits<float>::max();
            int idx = -1;

            for (int j = 0; j < K; j++) {
                if (GetDist(flops[i].pt, centers[j].pt) < min_cost) {
                    min_cost = GetDist(flops[i].pt, centers[j].pt);
                    idx = j;
                }
            }

            clusters[idx].push_back(flops[i]);
        }

        // find new center locations
        for (int i = 0; i < K; i++) {

            int cur_sz = static_cast<int>(clusters[i].size());
            float cX = 0, cY = 0;

            for (Flop f : clusters[i]) {
                cX += f.pt.x();
                cY += f.pt.y();
            }

            float new_x = cX / float(cur_sz);
            float new_y = cY / float(cur_sz);

            Flop new_flop;
            new_flop.pt = odb::Point(new_x, new_y);

            centers[i] = new_flop;
        }

        // get total displacement
        float tot_disp = 0;
        for (int i = 0; i < K; i++) {
            for (Flop f : clusters[i]) {
                tot_disp += GetDist(centers[i].pt, f.pt);
            }
        }

        if (tot_disp == prev) {
            break;
        }
        prev = tot_disp;
    }

    for (int i = 0; i < K; i++) {
        clusters[i].push_back(centers[i]);
    }

    return clusters;
}

/*
    shreyas (august 2023):
    method to decompose a pointset into multiple "mini"-pointsets of size <=
   MAX_SZ.
    basic implementation of K-means++ (with K = 4) is used.
*/

std::vector<std::vector<Flop> >
MBFF::KMeansDecomp(const std::vector<Flop> &flops, int MAX_SZ) {

    int num_flops = static_cast<int>(flops.size());

    std::vector<std::vector<Flop> > ret;

    if (num_flops <= MAX_SZ) {
        ret.push_back(flops);
        return ret;
    }

    std::vector<std::vector<Flop> > tmp_clusters[10];
    std::vector<float> tmp_costs(10);

    // multistart K-means++
    for (int i = 0; i < 10; i++) {
        std::vector<std::vector<Flop> > tmp = KMeans(flops, 4);

        /* cur_cost = sum of distances between flops and its
        matching cluster's center */
        float cur_cost = 0;
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k + 1 < static_cast<int>(tmp[j].size()); k++) {
                cur_cost += GetDist(tmp[j][k].pt, tmp[j].back().pt);
            }
        }

        tmp_clusters[i] = tmp;
        tmp_costs[i] = cur_cost;
    }

    float best_cost = std::numeric_limits<float>::max();
    std::vector<std::vector<Flop> > k_means_ret;
    for (int i = 0; i < 10; i++) {
        if (tmp_costs[i] < best_cost) {
            best_cost = tmp_costs[i];
            k_means_ret = tmp_clusters[i];
        }
    }

    /*
    create edges between all std::pairs of cluster centers
    edge weight = distance between the two centers
    */

    std::vector<std::pair<float, std::pair<int, int> > > cluster_pairs;
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            cluster_pairs.push_back(std::make_pair(
                GetDist(k_means_ret[i].back().pt, k_means_ret[j].back().pt),
                std::make_pair(i, j)));
        }
    }
    std::sort(std::begin(cluster_pairs), std::end(cluster_pairs));

    for (int i = 0; i < 4; i++) {
        k_means_ret[i].pop_back();
    }

    /*
        lines 810-830: basic implementation of DSU
        keep uniting sets (== mini-pointsets with size <= MAX_SZ) while not
        exceeding MAX_SZ
    */

    std::vector<int> id(4);
    std::vector<int> sz(4);

    for (int i = 0; i < 4; i++) {
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
        for (int j = 0; j < 4; j++) {
            if (id[j] == orig_id) {
                id[j] = id[idx1];
            }
        }
    }

    std::vector<std::vector<Flop> > nxt_clusters(4);
    for (int i = 0; i < 4; i++) {
        for (Flop f : k_means_ret[i]) {
            nxt_clusters[id[i]].push_back(f);
        }
    }

    // recurse on each new cluster
    std::vector<Flop> Q1 = nxt_clusters[0];
    std::vector<Flop> Q2 = nxt_clusters[1];
    std::vector<Flop> Q3 = nxt_clusters[2];
    std::vector<Flop> Q4 = nxt_clusters[3];

    std::vector<std::vector<Flop> > R1, R2, R3, R4;
    if (static_cast<int>(Q1.size())) {
        R1 = KMeansDecomp(Q1, MAX_SZ);
    }
    if (static_cast<int>(Q2.size())) {
        R2 = KMeansDecomp(Q2, MAX_SZ);
    }
    if (static_cast<int>(Q3.size())) {
        R3 = KMeansDecomp(Q3, MAX_SZ);
    }
    if (static_cast<int>(Q4.size())) {
        R4 = KMeansDecomp(Q4, MAX_SZ);
    }

    for (auto x : R1) {
        ret.push_back(x);
    }
    for (auto x : R2) {
        ret.push_back(x);
    }
    for (auto x : R3) {
        ret.push_back(x);
    }
    for (auto x : R4) {
        ret.push_back(x);
    }

    return ret;
}

std::vector<std::pair<int, int> >
MBFF::RunCapacitatedKMeans(const std::vector<Flop> &flops,
                           std::vector<Tray> &trays, int sz, int iter) {
    int num_flops = static_cast<int>(flops.size());
    int rows = GetRows(sz);
    int cols = sz / rows;
    int num_trays = (num_flops + (sz - 1)) / sz;

    float delta = 0;
    std::vector<std::pair<int, int> > cluster;

    for (int i = 0; i < iter; i++) {
        cluster = MinCostFlow(flops, trays, sz);
        delta = RunLP(flops, trays, cluster);

        for (int j = 0; j < num_trays; j++) {
            trays[j].slots = GetSlots(trays[j].pt, rows, cols);
        }

        if (delta < 0.5) {
            break;
        }
    }

    cluster = MinCostFlow(flops, trays, sz);
    return cluster;
}

std::vector<std::vector<Tray> > MBFF::RunSilh(const std::vector<Flop> &flops) {
    int num_flops = static_cast<int>(flops.size());

    std::vector<std::vector<Tray> > trays(NUM_SIZES);

    for (int i = 0; i < NUM_SIZES; i++) {
        int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);
        trays[i].resize(num_trays);
    }

    // add 1-bit trays
    for (int i = 0; i < num_flops; i++) {
        Tray one_bit = GetOneBit(flops[i].pt);
        one_bit.cand[0] = i;
        trays[0][i] = one_bit;
    }

    std::vector<std::vector<Tray> > start_trays[NUM_SIZES];
    std::vector<float> res[NUM_SIZES];
    std::vector<std::pair<int, int> > ind;

    for (int i = 1; i < NUM_SIZES; i++) {
        for (int j = 0; j < 5; j++) {
            ind.push_back(std::make_pair(i, j));
        }
        start_trays[i].resize(5);
        res[i].resize(5);
    }

    /*
    DO NOT run GetStartTrays in parallel
    runnning in parallel messes with reproducibility even with a fixed seed
    */

    for (int i = 1; i < NUM_SIZES; i++) {
        for (int j = 0; j < 5; j++) {
            int rows = GetRows(GetBitCnt(i));
            int cols = GetBitCnt(i) / rows;
            float AR = (cols * WIDTH * RATIOS[i]) / (rows * HEIGHT);

            int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);
            start_trays[i][j] = GetStartTrays(flops, num_trays, AR);
        }
    }

#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(ind.size()); i++) {

        int bit_idx = ind[i].first;
        int tray_idx = ind[i].second;

        int rows = GetRows(GetBitCnt(bit_idx));
        int cols = GetBitCnt(bit_idx) / rows;

        int num_trays =
            (num_flops + (GetBitCnt(bit_idx) - 1)) / GetBitCnt(bit_idx);

        for (int j = 0; j < num_trays; j++) {
            start_trays[bit_idx][tray_idx][j].slots =
                GetSlots(start_trays[bit_idx][tray_idx][j].pt, rows, cols);
            start_trays[bit_idx][tray_idx][j].cand.resize(rows * cols);
        }

        float delta = 0;
        std::vector<std::pair<int, int> > tmp_cluster;
        for (int j = 0; j < 8; j++) {

            tmp_cluster = MinCostFlow(flops, start_trays[bit_idx][tray_idx],
                                      GetBitCnt(bit_idx));
            delta = RunLP(flops, start_trays[bit_idx][tray_idx], tmp_cluster);

            for (int k = 0; k < num_trays; k++) {
                start_trays[bit_idx][tray_idx][k].slots =
                    GetSlots(start_trays[bit_idx][tray_idx][k].pt, rows, cols);
            }

            if (delta < 0.5) {
                break;
            }
        }

        tmp_cluster = MinCostFlow(flops, start_trays[bit_idx][tray_idx],
                                  GetBitCnt(bit_idx));
        res[bit_idx][tray_idx] =
            GetSilh(flops, start_trays[bit_idx][tray_idx], tmp_cluster);
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
        trays[i] = start_trays[i][opt_idx];
    }

    return trays;
}

void MBFF::Remap(const std::vector<Flop> &flops,
                 std::vector<std::vector<Tray> > &trays) {
    int num_flops = static_cast<int>(flops.size());

    std::map<int, int> small_to_large;
    for (int i = 0; i < num_flops; i++) {
        small_to_large[i] = flops[i].idx;
    }

    for (int i = 0; i < NUM_SIZES; i++) {
        for (int j = 0; j < static_cast<int>(trays[i].size()); j++) {
            for (int k = 0; k < static_cast<int>(trays[i][j].cand.size()); k++) {
                trays[i][j].cand[k] = small_to_large[trays[i][j].cand[k]];
            }
        }
    }
}

void MBFF::Run(int mx_sz, float alpha, float beta) {
    srand(1);
    omp_set_num_threads(num_threads);

    // run k-means++ based decomposition
    std::vector<std::vector<Flop> > pointsets = KMeansDecomp(all_flops, mx_sz);

    std::vector<std::vector<Tray> > all_trays(NUM_SIZES);

    for (int t = 0; t < static_cast<int>(pointsets.size()); t++) {

        int num_flops = static_cast<int>(pointsets[t].size());

        // run silhouette metric
        std::vector<std::vector<Tray> > trays = RunSilh(pointsets[t]);

// run capacitated k-means per tray size
#pragma omp parallel for
        for (int i = 1; i < NUM_SIZES; i++) {
            int rows = GetRows(GetBitCnt(i)), cols = GetBitCnt(i) / rows;
            int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);
            for (int j = 0; j < num_trays; j++) {
                trays[i][j].slots = GetSlots(trays[i][j].pt, rows, cols);
            }
            RunCapacitatedKMeans(pointsets[t], trays[i], GetBitCnt(i), 35);
            MinCostFlow(pointsets[t], trays[i], GetBitCnt(i));
            for (int j = 0; j < num_trays; j++) {
                trays[i][j].slots = GetSlots(trays[i][j].pt, rows, cols);
            }
        }

        Remap(pointsets[t], trays);

        // append trays
        for (int i = 0; i < NUM_SIZES; i++) {
            for (int j = 0; j < static_cast<int>(trays[i].size()); j++) {
                all_trays[i].push_back(trays[i][j]);
            }
        }
    }

    RunILP(all_flops, all_paths, all_trays, alpha, beta);
}

// ctor
MBFF::MBFF(int num_flops, int num_paths, const std::vector<float> &x,
           const std::vector<float> &y,
           const std::vector<std::pair<int, int> > &paths, int threads,
           utl::Logger *logger) {
    all_flops.resize(num_flops);
    for (int i = 0; i < num_flops; i++) {
        odb::Point new_pt(x[i] + 70.0, y[i] + 70.0);

        Flop new_flop;
        new_flop.pt = new_pt, new_flop.idx = i;
        all_flops[i] = new_flop;
    }

    all_paths.resize(num_paths);
    for (int i = 0; i < num_paths; i++) {
        Path new_path;
        new_path.a = paths[i].first, new_path.b = paths[i].second;
        all_paths[i] = new_path;
    }

    num_threads = threads;

    log_ = logger;
}

// dtor
MBFF::~MBFF() {}

} // end namespace gpl
