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

#include "Core.h"
#include "Netlist.h"
#include "Slots.h"
#include "odb/geom.h"
#include "ppl/IOPlacer.h"

namespace utl {
class Logger;
}  // namespace utl

namespace ppl {

struct DebugSettings
{
  std::unique_ptr<AbstractIOPlacerRenderer> renderer;

  bool isOn() const { return renderer != nullptr; }
};

using utl::Logger;

using GroupLimits = std::pair<int, int>;

struct Constraint;

class SimulatedAnnealing
{
 public:
  SimulatedAnnealing(Netlist* netlist,
                     Core* core,
                     std::vector<Slot>& slots,
                     const std::vector<Constraint>& constraints,
                     Logger* logger,
                     odb::dbDatabase* db);
  ~SimulatedAnnealing() = default;
  void run(float init_temperature,
           int max_iterations,
           int perturb_per_iter,
           float alpha,
           bool random);
  void getAssignment(std::vector<IOPin>& assignment);

  // debug functions
  void setDebugOn(std::unique_ptr<AbstractIOPlacerRenderer> renderer);

  void annealingStateVisualization(
      const std::vector<IOPin>& assignment,
      const std::vector<std::vector<InstancePin>>& sinks,
      const int& current_iteration);

  AbstractIOPlacerRenderer* getDebugRenderer();

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
  int movePinToFreeSlot(bool lone_pin = false);
  int moveGroupToFreeSlots(int group_idx);
  int shiftGroup(int group_idx);
  void shiftGroupToPosition(const std::vector<int>& pin_indices,
                            int free_slots_count,
                            int min_slot,
                            bool move_to_max);
  int rearrangeConstrainedGroups(int constraint_idx);
  int moveGroup(int pin_idx);
  void restorePreviousAssignment();
  double dbuToMicrons(int64_t dbu);
  bool isFreeForGroup(int& slot_idx, int group_size, int last_slot);
  void getSlotsRange(const IOPin& io_pin, int& first_slot, int& last_slot);
  int getSlotIdxByPosition(const odb::Point& position, int layer) const;
  bool isFreeForMirrored(int slot_idx, int& mirrored_idx) const;
  int getMirroredSlotIdx(int slot_idx) const;
  void updateSlotsFromGroup(const std::vector<int>& prev_slots_, bool block);
  int computeGroupPrevCost(int group_idx);
  void updateGroupSlots(const std::vector<int>& pin_indices, int& new_slot);
  void countLonePins();

  // [pin] -> slot
  std::vector<int> pin_assignment_;
  std::vector<int> slot_indices_;
  Netlist* netlist_;
  Core* core_;
  std::vector<Slot>& slots_;
  const std::vector<PinGroupByIndex>& pin_groups_;
  const std::vector<Constraint>& constraints_;
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
  const float shift_group_ = 0.8;
  const float group_to_free_slots_ = 0.7;
  const float pins_per_slot_limit_ = 0.5;

  Logger* logger_ = nullptr;
  odb::dbDatabase* db_;
  const int fail_cost_ = std::numeric_limits<int>::max();
  const int seed_ = 42;

  // debug variables
  std::unique_ptr<DebugSettings> debug_;
};

}  // namespace ppl
