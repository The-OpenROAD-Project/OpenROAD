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
  int pre_cost = getAssignmentCost(pin_assignment_);
  std::vector<int> assignment;
  float temperature = init_temperature_;

  for (int iter = 0; iter < max_iterations_; iter++) {
    for (int perturb = 0; perturb < perturb_per_iter_; perturb++) {
      assignment = perturbAssignment();
      int cost = getAssignmentCost(assignment);
      int delta_cost = cost - pre_cost;
      const float rand_float = distribution_(generator_);
      const float accept_prob = std::exp((-1) * delta_cost / temperature);
      if (delta_cost <= 0 || accept_prob > rand_float) {
        pin_assignment_ = assignment;
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
    pin_assignment_[i] = slot_indices_[i];
  }
}

int SimulatedAnnealing::getAssignmentCost(const std::vector<int>& assignment)
{
  int cost = 0;

  for (int i = 0; i < assignment.size(); i++) {
    int slot_idx = assignment[i];
    const odb::Point& position = slots_[slot_idx].pos;
    cost += netlist_->computeIONetHPWL(i, position);
  }

  debugPrint(logger_, utl::PPL, "annealing", 1, "assignment cost: {}", cost);

  return cost;
}

std::vector<int> SimulatedAnnealing::perturbAssignment()
{
  std::vector<int> assignment;
  const float move = distribution_(generator_);

  if (move < swap_pins_) {
    assignment = swapPins(pin_assignment_);
  } else {
    assignment = placeSubsetOfPins(pin_assignment_, pins_subset_percent_);
  }

  return assignment;
}

std::vector<int> SimulatedAnnealing::swapPins(
    const std::vector<int>& pin_assignment)
{
  std::vector<int> assignment = pin_assignment;

  const int idx1 = (int) (std::floor(distribution_(generator_) * num_pins_));
  const int idx2 = (int) (std::floor(distribution_(generator_) * num_pins_));
  std::swap(assignment[idx1], assignment[idx2]);

  return assignment;
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
