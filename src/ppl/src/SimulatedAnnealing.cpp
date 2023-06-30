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

#include "SimulatedAnnealing.h"

#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace ppl {

SimulatedAnnealing::SimulatedAnnealing(Netlist* netlist,
                                       std::vector<Slot>& slots,
                                       Logger* logger,
                                       odb::dbDatabase* db)
    : netlist_(netlist),
      slots_(slots),
      pin_groups_(netlist->getIOGroups()),
      logger_(logger),
      db_(db)
{
  num_slots_ = slots.size();
  num_pins_ = netlist->numIOPins();
  perturb_per_iter_ = static_cast<int>(num_pins_ * 0.8);
}

void SimulatedAnnealing::run()
{
  init();
  randomAssignment();
  int64 pre_cost = 0;
  pre_cost = getAssignmentCost();
  float temperature = init_temperature_;

  for (int iter = 0; iter < max_iterations_; iter++) {
    for (int perturb = 0; perturb < perturb_per_iter_; perturb++) {
      int prev_slot = -1;
      int new_slot = -1;
      int pin1 = -1;
      int pin2 = -1;
      int prev_cost;
      perturbAssignment(prev_slot, new_slot, pin1, pin2, prev_cost);
      const int64 cost = pre_cost + getDeltaCost(prev_cost, pin1, pin2);
      const int delta_cost = cost - pre_cost;
      debugPrint(logger_,
                 utl::PPL,
                 "annealing",
                 1,
                 "iteration: {}; temperature: {}; assignment cost: {}um; delta "
                 "cost: {}um",
                 iter,
                 temperature,
                 dbuToMicrons(cost),
                 dbuToMicrons(delta_cost));

      const float rand_float = distribution_(generator_);
      const float accept_prob = std::exp((-1) * delta_cost / temperature);
      if (delta_cost <= 0 || accept_prob > rand_float) {
        // accept new solution, update cost and slots
        pre_cost = cost;
        if (prev_slot != -1 && new_slot != -1) {
          slots_[prev_slot].used = false;
          slots_[new_slot].used = true;
        }
      } else {
        restorePreviousAssignment(prev_slot, new_slot, pin1, pin2);
      }
    }

    temperature *= alpha_;
  }
}

void SimulatedAnnealing::getAssignment(std::vector<IOPin>& assignment)
{
  for (int i = 0; i < pin_assignment_.size(); i++) {
    IOPin& io_pin = netlist_->getIoPin(i);
    Slot& slot = slots_[pin_assignment_[i]];

    io_pin.setPos(slot.pos);
    io_pin.setLayer(slot.layer);
    io_pin.setPlaced();
    assignment.push_back(io_pin);
    slot.used = true;
  }
}

void SimulatedAnnealing::init()
{
  pin_assignment_.resize(num_pins_);
  slot_indices_.resize(num_slots_);
  std::iota(slot_indices_.begin(), slot_indices_.end(), 0);

  generator_.seed(seed_);
  std::uniform_real_distribution<float> distribution;
  distribution_ = distribution;
}

void SimulatedAnnealing::randomAssignment()
{
  std::mt19937 g;
  g.seed(seed_);
  utl::shuffle(slot_indices_.begin(), slot_indices_.end(), g);
  
  std::set<int> placed_pins;
  int slot_idx = randomAssignmentForGroups(placed_pins);

  for (int i = 0; i < pin_assignment_.size(); i++) {
    if (placed_pins.find(i) != placed_pins.end()) {
      continue;
    }

    int slot = slot_indices_[slot_idx];
    while (slots_[slot].used) {
      slot_idx++;
      slot = slot_indices_[slot_idx];
    }
    pin_assignment_[i] = slot;
    slots_[slot].used = true;
    slot_idx++;
  }
}

int SimulatedAnnealing::randomAssignmentForGroups(std::set<int>& placed_pins)
{
  int slot_idx = 0;

  for (const auto& group : pin_groups_) {
    const auto pin_list = group.pin_indices;
    int group_slot = slot_indices_[slot_idx];
    for (const auto& pin_idx : pin_list) {
      pin_assignment_[pin_idx] = group_slot;
      slots_[group_slot].used = true;
      group_slot++;
      slot_idx++;
      placed_pins.insert(pin_idx);
    }
  }

  return slot_idx;
}

int64 SimulatedAnnealing::getAssignmentCost()
{
  int64 cost = 0;

  for (int i = 0; i < pin_assignment_.size(); i++) {
    cost += getPinCost(i);
  }

  return cost;
}

int SimulatedAnnealing::getDeltaCost(const int prev_cost, int pin1, int pin2)
{
  int new_cost = getPinCost(pin1);
  if (pin2 != -1) {
    new_cost += getPinCost(pin2);
  }

  return new_cost - prev_cost;
}

int SimulatedAnnealing::getPinCost(int pin_idx)
{
  int slot_idx = pin_assignment_[pin_idx];
  const odb::Point& position = slots_[slot_idx].pos;
  return netlist_->computeIONetHPWL(pin_idx, position);
}

void SimulatedAnnealing::perturbAssignment(int& prev_slot,
                                           int& new_slot,
                                           int& pin1,
                                           int& pin2,
                                           int& prev_cost)
{
  const float move = distribution_(generator_);

  if (move < swap_pins_) {
    prev_cost = swapPins(pin1, pin2);
  } else {
    prev_cost = movePinToFreeSlot(prev_slot, new_slot, pin1);
  }
}

int SimulatedAnnealing::swapPins(int& pin1, int& pin2)
{
  pin1 = (int) (std::floor(distribution_(generator_) * num_pins_));
  pin2 = (int) (std::floor(distribution_(generator_) * num_pins_));

  int prev_cost = getPinCost(pin1) + getPinCost(pin2);

  std::swap(pin_assignment_[pin1], pin_assignment_[pin2]);

  return prev_cost;
}

int SimulatedAnnealing::movePinToFreeSlot(int& prev_slot,
                                          int& new_slot,
                                          int& pin)
{
  pin = (int) (std::floor(distribution_(generator_) * num_pins_));
  prev_slot = pin_assignment_[pin];

  int prev_cost = getPinCost(pin);

  bool free_slot = false;
  while (!free_slot) {
    new_slot = (int) (std::floor(distribution_(generator_) * num_slots_));
    free_slot = slots_[new_slot].isAvailable() && new_slot != prev_slot;
  }

  pin_assignment_[pin] = new_slot;

  return prev_cost;
}

void SimulatedAnnealing::restorePreviousAssignment(const int prev_slot,
                                                   const int new_slot,
                                                   const int pin1,
                                                   const int pin2)
{
  if (prev_slot != -1 && new_slot != -1) {
    // restore single pin to previous slot
    pin_assignment_[pin1] = prev_slot;
  } else if (pin1 != -1 && pin2 != -1) {
    // undo pin swapping
    std::swap(pin_assignment_[pin1], pin_assignment_[pin2]);
  }
}

double SimulatedAnnealing::dbuToMicrons(int64_t dbu)
{
  return (double) dbu / (db_->getChip()->getBlock()->getDbUnitsPerMicron());
}

}  // namespace ppl
