/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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
    : netlist_(netlist), slots_(slots), logger_(logger), db_(db)
{
  num_slots_ = slots.size();
  num_pins_ = netlist->numIOPins();
  perturb_per_iter_ = static_cast<int>(num_pins_ * 0.8);
}

void SimulatedAnnealing::init()
{
  pin_assignment_.resize(num_pins_);
  slot_indices_.resize(num_slots_);
  std::iota(slot_indices_.begin(), slot_indices_.end(), 0);

  std::mt19937 rand_gen(seed_);
  generator_ = rand_gen;
  std::uniform_real_distribution<float> distribution(0.0, 1.0);
  distribution_ = distribution;
}

void SimulatedAnnealing::run()
{
  init();
  randomAssignment();
  int pre_cost = getAssignmentCost();
  float temperature = init_temperature_;

  for (int iter = 0; iter < max_iterations_; iter++) {
    for (int perturb = 0; perturb < perturb_per_iter_; perturb++) {
      int prev_slot = -1;
      int new_slot = -1;
      int pin1 = -1;
      int pin2 = -1;
      perturbAssignment(prev_slot, new_slot, pin1, pin2);
      int cost = getAssignmentCost();
      int delta_cost = cost - pre_cost;
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
        // rejects new result, restore previous assignment
        if (prev_slot != -1 && new_slot != -1) {
          // restore single pin to previous slot
          pin_assignment_[pin1] = prev_slot;
        } else if (pin1 != -1 && pin2 != -1) {
          // undo pin swapping
          std::swap(pin_assignment_[pin1], pin_assignment_[pin2]);
        }
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

void SimulatedAnnealing::randomAssignment()
{
  std::mt19937 g;
  g.seed(seed_);
  utl::shuffle(slot_indices_.begin(), slot_indices_.end(), g);
  for (int i = 0; i < pin_assignment_.size(); i++) {
    int slot_idx = slot_indices_[i];
    pin_assignment_[i] = slot_idx;
    slots_[slot_idx].used = true;
  }
}

int SimulatedAnnealing::getAssignmentCost()
{
  int cost = 0;

  for (int i = 0; i < pin_assignment_.size(); i++) {
    int slot_idx = pin_assignment_[i];
    const odb::Point& position = slots_[slot_idx].pos;
    cost += netlist_->computeIONetHPWL(i, position);
  }

  return cost;
}

void SimulatedAnnealing::perturbAssignment(int& prev_slot,
                                           int& new_slot,
                                           int& pin1,
                                           int& pin2)
{
  const float move = distribution_(generator_);

  if (move < swap_pins_) {
    swapPins(pin1, pin2);
  } else {
    movePinToFreeSlot(prev_slot, new_slot, pin1);
  }
}

void SimulatedAnnealing::swapPins(int& pin1, int& pin2)
{
  pin1 = (int) (std::floor(distribution_(generator_) * num_pins_));
  pin2 = (int) (std::floor(distribution_(generator_) * num_pins_));
  std::swap(pin_assignment_[pin1], pin_assignment_[pin2]);
}

void SimulatedAnnealing::movePinToFreeSlot(int& prev_slot,
                                           int& new_slot,
                                           int& pin)
{
  pin = (int) (std::floor(distribution_(generator_) * num_pins_));
  prev_slot = pin_assignment_[pin];

  bool free_slot = false;
  while (!free_slot) {
    new_slot = (int) (std::floor(distribution_(generator_) * num_slots_));
    free_slot = slots_[new_slot].isAvailable() && new_slot != prev_slot;
  }

  pin_assignment_[pin] = new_slot;
}

std::vector<int> SimulatedAnnealing::placeSubsetOfPins(
    const std::vector<int>& pin_assignment,
    float subset_percent)
{
  std::vector<int> assignment;

  return assignment;
}

double SimulatedAnnealing::dbuToMicrons(int64_t dbu)
{
  return (double) dbu / (db_->getChip()->getBlock()->getDbUnitsPerMicron());
}

}  // namespace ppl
