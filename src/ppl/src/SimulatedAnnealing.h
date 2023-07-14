/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <random>

#include "Netlist.h"
#include "Slots.h"
#include "odb/geom.h"
#include "ppl/IOPlacer.h"

namespace utl {
class Logger;
}  // namespace utl

namespace ppl {
using utl::Logger;

class SimulatedAnnealing
{
 public:
  SimulatedAnnealing(Netlist* netlist,
                     std::vector<Slot>& slots,
                     Logger* logger,
                     odb::dbDatabase* db);
  ~SimulatedAnnealing() = default;
  void run(float init_temperature,
           int max_iterations,
           int perturb_per_iter,
           float alpha);
  void getAssignment(std::vector<IOPin>& assignment);

 private:
  void init(float init_temperature,
            int max_iterations,
            int perturb_per_iter,
            float alpha);
  void randomAssignment();
  int randomAssignmentForGroups(std::set<int>& placed_pins,
                                const std::vector<int>& slot_indices);
  int64 getAssignmentCost();
  int getDeltaCost(int prev_cost);
  int getPinCost(int pin_idx);
  int64 getGroupCost(int group_idx);
  void perturbAssignment(int& prev_cost);
  int swapPins();
  int movePinToFreeSlot();
  int moveGroupToFreeSlots(int group_idx);
  void restorePreviousAssignment();
  double dbuToMicrons(int64_t dbu);
  bool isFreeForGroup(int slot_idx, int group_size);

  // [pin] -> slot
  std::vector<int> pin_assignment_;
  std::vector<int> slot_indices_;
  Netlist* netlist_;
  std::vector<Slot>& slots_;
  const std::vector<PinGroupByIndex>& pin_groups_;
  int num_slots_;
  int num_pins_;
  int num_groups_;
  int lone_pins_;

  std::vector<int> prev_slots_;
  std::vector<int> new_slots_;
  std::vector<int> pins_;

  // annealing variables
  float init_temperature_ = 1.0;
  int max_iterations_ = 2000;
  int perturb_per_iter_ = 0;
  float alpha_ = 0.985;
  boost::random::mt19937 generator_;

  // perturbation variables
  const float swap_pins_ = 0.5;
  const int move_fail_ = -1;

  Logger* logger_ = nullptr;
  odb::dbDatabase* db_;
  const int fail_cost_ = std::numeric_limits<int>::max();
  const int seed_ = 42;
};

}  // namespace ppl
