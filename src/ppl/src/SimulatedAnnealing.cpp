// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "SimulatedAnnealing.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include "AbstractIOPlacerRenderer.h"
#include "Core.h"
#include "Netlist.h"
#include "Slots.h"
#include "boost/random/uniform_int_distribution.hpp"
#include "boost/random/uniform_real_distribution.hpp"
#include "odb/db.h"
#include "odb/geom.h"
#include "ppl/IOPlacer.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace ppl {

SimulatedAnnealing::SimulatedAnnealing(
    Netlist* netlist,
    Core* core,
    std::vector<Slot>& slots,
    const std::vector<Constraint>& constraints,
    utl::Logger* logger,
    odb::dbDatabase* db)
    : netlist_(netlist),
      core_(core),
      slots_(slots),
      pin_groups_(netlist->getIOGroups()),
      constraints_(constraints),
      logger_(logger),
      db_(db),
      debug_(std::make_unique<DebugSettings>())
{
  num_slots_ = slots.size();
  num_pins_ = netlist->numIOPins();
  num_groups_ = pin_groups_.size();
  countLonePins();
  perturb_per_iter_ = static_cast<int>(lone_pins_ * 0.8 + num_groups_ * 10);
}

void SimulatedAnnealing::run(float init_temperature,
                             int max_iterations,
                             int perturb_per_iter,
                             float alpha)
{
  init(init_temperature, max_iterations, perturb_per_iter, alpha);
  randomAssignment();
  int64 pre_cost = 0;
  pre_cost = getAssignmentCost();
  float temperature = init_temperature_;
  odb::dbBlock* block = db_->getChip()->getBlock();

  boost::random::uniform_real_distribution<float> distribution;
  for (int iter = 0; iter < max_iterations_; iter++) {
    for (int perturb = 0; perturb < perturb_per_iter_; perturb++) {
      int prev_cost;
      perturbAssignment(prev_cost);

      const int64 cost = pre_cost + getDeltaCost(prev_cost);
      const int delta_cost = cost - pre_cost;
      debugPrint(logger_,
                 utl::PPL,
                 "annealing",
                 2,
                 "iteration: {}; perturb: {}; temperature: {}; assignment "
                 "cost: {}um; delta "
                 "cost: {}um",
                 iter,
                 perturb,
                 temperature,
                 block->dbuToMicrons(cost),
                 block->dbuToMicrons(delta_cost));

      const float rand_float = distribution(generator_);
      const float accept_prob = std::exp((-1) * delta_cost / temperature);
      if (delta_cost <= 0 || accept_prob > rand_float) {
        // accept new solution, update cost and slots
        pre_cost = cost;
        if (!prev_slots_.empty() && !new_slots_.empty()) {
          for (int prev_slot : prev_slots_) {
            slots_[prev_slot].used = false;
          }
          for (int new_slot : new_slots_) {
            slots_[new_slot].used = true;
          }
        }
      } else {
        for (const int prev_slot : prev_slots_) {
          slots_[prev_slot].used = true;
        }
        restorePreviousAssignment();
      }
      prev_slots_.clear();
      new_slots_.clear();
      pins_.clear();
    }

    temperature *= alpha_;

    if (debug_->isOn()) {
      std::vector<ppl::IOPin> pins;
      getAssignment(pins);

      std::vector<std::vector<ppl::InstancePin>> all_sinks;

      for (int pin_idx = 0; pin_idx < pins.size(); pin_idx++) {
        std::vector<ppl::InstancePin> pin_sinks;
        netlist_->getSinksOfIO(pin_idx, pin_sinks);
        all_sinks.push_back(std::move(pin_sinks));
      }

      annealingStateVisualization(pins, all_sinks, iter);
    }
  }
}

void SimulatedAnnealing::getAssignment(std::vector<IOPin>& assignment)
{
  for (int i = 0; i < pin_assignment_.size(); i++) {
    IOPin& io_pin = netlist_->getIoPin(i);
    Slot& slot = slots_[pin_assignment_[i]];

    io_pin.setPosition(slot.pos);
    io_pin.setLayer(slot.layer);
    io_pin.setPlaced();
    io_pin.setEdge(slot.edge);
    assignment.push_back(io_pin);
    slot.used = true;
  }
}

void SimulatedAnnealing::setDebugOn(
    std::unique_ptr<AbstractIOPlacerRenderer> renderer)
{
  debug_->renderer = std::move(renderer);
}

AbstractIOPlacerRenderer* SimulatedAnnealing::getDebugRenderer()
{
  return debug_->renderer.get();
}

void SimulatedAnnealing::annealingStateVisualization(
    const std::vector<IOPin>& assignment,
    const std::vector<std::vector<InstancePin>>& sinks,
    const int& current_iteration)
{
  if (!debug_->isOn()) {
    return;
  }
  getDebugRenderer()->setCurrentIteration(current_iteration);
  getDebugRenderer()->setSinks(sinks);
  getDebugRenderer()->setPinAssignment(assignment);
  getDebugRenderer()->redrawAndPause();
}

void SimulatedAnnealing::init(float init_temperature,
                              int max_iterations,
                              int perturb_per_iter,
                              float alpha)
{
  init_temperature_
      = init_temperature != 0 ? init_temperature : init_temperature_;
  max_iterations_ = max_iterations != 0 ? max_iterations : max_iterations_;
  perturb_per_iter_
      = perturb_per_iter != 0 ? perturb_per_iter : perturb_per_iter_;
  alpha_ = alpha != 0 ? alpha : alpha_;

  pin_assignment_.resize(num_pins_);
  slot_indices_.resize(num_slots_);
  std::iota(slot_indices_.begin(), slot_indices_.end(), 0);

  debugPrint(logger_,
             utl::PPL,
             "annealing",
             1,
             "init_temperature_: {}; max_iterations_: {}; perturb_per_iter_: "
             "{}; alpha_: {}",
             init_temperature_,
             max_iterations_,
             perturb_per_iter_,
             alpha_);

  generator_.seed(seed_);
}

void SimulatedAnnealing::randomAssignment()
{
  std::mt19937 g;
  g.seed(seed_);

  std::vector<int> slot_indices = slot_indices_;
  utl::shuffle(slot_indices.begin(), slot_indices.end(), g);

  std::set<int> placed_pins;
  int slot_idx = randomAssignmentForGroups(placed_pins, slot_indices);

  for (bool only_constraints : {true, false}) {
    for (int i = 0; i < pin_assignment_.size(); i++) {
      if (placed_pins.find(i) != placed_pins.end()) {
        continue;
      }

      const IOPin& io_pin = netlist_->getIoPin(i);
      if (io_pin.isInConstraint() && only_constraints) {
        int first_slot = 0;
        int last_slot = num_slots_ - 1;
        getSlotsRange(io_pin, first_slot, last_slot);
        boost::random::uniform_int_distribution<int> distribution(first_slot,
                                                                  last_slot);

        int slot = distribution(generator_);
        int mirrored_slot;
        bool free = slots_[slot].isAvailable();
        if (io_pin.isMirrored()) {
          free = free && isFreeForMirrored(slot, mirrored_slot);
        }
        while (!free) {
          slot = distribution(generator_);
          free = slots_[slot].isAvailable();
          if (io_pin.isMirrored()) {
            free = free && isFreeForMirrored(slot, mirrored_slot);
          }
        }
        pin_assignment_[i] = slot;
        slots_[slot].used = true;
        if (io_pin.isMirrored()) {
          pin_assignment_[io_pin.getMirrorPinIdx()] = mirrored_slot;
          slots_[mirrored_slot].used = true;
        }
        placed_pins.insert(io_pin.getMirrorPinIdx());
      }

      if (!io_pin.isInConstraint() && !only_constraints) {
        int slot = slot_indices[slot_idx];
        int mirrored_slot;
        bool free = slots_[slot].isAvailable();
        if (io_pin.isMirrored()) {
          free = free && isFreeForMirrored(slot, mirrored_slot);
        }
        while (!free) {
          slot_idx++;
          slot = slot_indices[slot_idx];
          free = slots_[slot].isAvailable();
          if (io_pin.isMirrored()) {
            free = free && isFreeForMirrored(slot, mirrored_slot);
          }
        }
        pin_assignment_[i] = slot;
        slots_[slot].used = true;
        if (io_pin.isMirrored()) {
          pin_assignment_[io_pin.getMirrorPinIdx()] = mirrored_slot;
          slots_[mirrored_slot].used = true;
        }
        placed_pins.insert(io_pin.getMirrorPinIdx());

        slot_idx++;
      }
    }
  }
}

int SimulatedAnnealing::randomAssignmentForGroups(
    std::set<int>& placed_pins,
    const std::vector<int>& slot_indices)
{
  int slot_idx = 0;
  for (const auto& group : pin_groups_) {
    int group_slot;
    const IOPin& io_pin = netlist_->getIoPin(group.pin_indices[0]);
    if (io_pin.getConstraintIdx() != -1) {
      int first_slot = 0;
      int last_slot = num_slots_ - 1;
      getSlotsRange(io_pin, first_slot, last_slot);
      boost::random::uniform_int_distribution<int> distribution(first_slot,
                                                                last_slot);

      int slot = distribution(generator_);
      while (!isFreeForGroup(slot, group.pin_indices.size(), last_slot)) {
        slot = distribution(generator_);
      }
      group_slot = slot;
    } else {
      int slot = slot_indices[slot_idx];
      while (!isFreeForGroup(slot, group.pin_indices.size(), num_slots_ - 1)) {
        slot_idx++;
        slot = slot_indices[slot_idx];
      }
      group_slot = slot_indices[slot_idx];
    }

    const auto& pin_list = group.pin_indices;
    for (const auto& pin_idx : pin_list) {
      const IOPin& io_pin = netlist_->getIoPin(pin_idx);
      pin_assignment_[pin_idx] = group_slot;
      slots_[group_slot].used = true;
      if (io_pin.isMirrored()) {
        int mirrored_slot = getMirroredSlotIdx(group_slot);
        pin_assignment_[io_pin.getMirrorPinIdx()] = mirrored_slot;
        slots_[mirrored_slot].used = true;
        placed_pins.insert(io_pin.getMirrorPinIdx());
      }
      group_slot++;
      placed_pins.insert(pin_idx);
    }
    slot_idx++;
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

int SimulatedAnnealing::getDeltaCost(const int prev_cost)
{
  int new_cost = 0;
  for (int pin_idx : pins_) {
    new_cost += getPinCost(pin_idx);
  }

  return new_cost - prev_cost;
}

int SimulatedAnnealing::getPinCost(int pin_idx)
{
  int slot_idx = pin_assignment_[pin_idx];
  const odb::Point& position = slots_[slot_idx].pos;
  return netlist_->computeIONetHPWL(pin_idx, position);
}

int64 SimulatedAnnealing::getGroupCost(int group_idx)
{
  int64 cost = 0;
  for (int pin_idx : pin_groups_[group_idx].pin_indices) {
    int slot_idx = pin_assignment_[pin_idx];
    const odb::Point& position = slots_[slot_idx].pos;
    cost += netlist_->computeIONetHPWL(pin_idx, position);
  }

  return cost;
}

void SimulatedAnnealing::perturbAssignment(int& prev_cost)
{
  boost::random::uniform_real_distribution<float> distribution;
  const float move = distribution(generator_);

  // to perform pin swapping, at least two pins that are not inside a group are
  // necessary
  if (move < swap_pins_ && lone_pins_ > 1) {
    prev_cost = swapPins();
  }

  // move single pin when swapping a single constrained pin is not possible
  if (move >= swap_pins_ || lone_pins_ <= 1 || prev_cost == move_fail_) {
    prev_cost = movePinToFreeSlot();
    // move single pin when moving a group is not possible
    if (prev_cost == move_fail_) {
      prev_cost = movePinToFreeSlot(true);
    }
  }
}

int SimulatedAnnealing::swapPins()
{
  boost::random::uniform_int_distribution<int> distribution(0, num_pins_ - 1);
  int pin1 = distribution(generator_);
  int pin2;
  while (netlist_->getIoPin(pin1).isInGroup()
         || netlist_->getIoPin(pin1).isMirrored()) {
    pin1 = distribution(generator_);
  }

  int constraint_idx = netlist_->getIoPin(pin1).getConstraintIdx();
  if (constraint_idx != -1) {
    const std::vector<int>& pin_indices
        = constraints_[constraint_idx].pin_indices;
    // if there is only one pin in the constraint, do not swap and fallback to
    // move a pin to a free slot
    const int pin_count = pin_indices.size();
    const int mirrored_pins_count
        = constraints_[constraint_idx].mirrored_pins_count;
    if (pin_count == 1 || (pin_count - mirrored_pins_count) <= 1) {
      return move_fail_;
    }
    distribution = boost::random::uniform_int_distribution<int>(
        0, pin_indices.size() - 1);

    int pin_idx = distribution(generator_);
    pin2 = pin_indices[pin_idx];
    while (pin1 == pin2 || netlist_->getIoPin(pin2).isInGroup()
           || netlist_->getIoPin(pin2).isMirrored()) {
      pin_idx = distribution(generator_);
      pin2 = pin_indices[pin_idx];
    }
  } else {
    pin2 = distribution(generator_);
    while (pin1 == pin2 || netlist_->getIoPin(pin2).isInGroup()
           || netlist_->getIoPin(pin2).isMirrored()
           || netlist_->getIoPin(pin2).getConstraintIdx() != -1) {
      pin2 = distribution(generator_);
    }
  }

  pins_.push_back(pin1);
  pins_.push_back(pin2);

  int prev_slot1 = pin_assignment_[pin1];
  prev_slots_.push_back(prev_slot1);

  int prev_slot2 = pin_assignment_[pin2];
  prev_slots_.push_back(prev_slot2);

  int prev_cost = getPinCost(pin1) + getPinCost(pin2);

  std::swap(pin_assignment_[pin1], pin_assignment_[pin2]);

  return prev_cost;
}

int SimulatedAnnealing::movePinToFreeSlot(bool lone_pin)
{
  boost::random::uniform_int_distribution<int> distribution(0, num_pins_ - 1);
  int pin = distribution(generator_);

  if (lone_pin) {
    while (netlist_->getIoPin(pin).isInGroup()
           || netlist_->getIoPin(pin).isMirrored()) {
      pin = distribution(generator_);
    }
  } else {
    const IOPin& io_pin = netlist_->getIoPin(pin);
    if (io_pin.isInGroup()) {
      int prev_cost = moveGroup(pin);
      return prev_cost;
    }

    if (io_pin.isMirrored()) {
      const IOPin& mirrored_pin = netlist_->getIoPin(io_pin.getMirrorPinIdx());
      if (mirrored_pin.isInGroup()) {
        int prev_cost = moveGroup(io_pin.getMirrorPinIdx());
        return prev_cost;
      }
    }
  }

  const IOPin& io_pin = netlist_->getIoPin(pin);
  pins_.push_back(pin);

  int prev_slot = pin_assignment_[pin];
  prev_slots_.push_back(prev_slot);

  int prev_cost = getPinCost(pin);
  if (io_pin.isMirrored()) {
    prev_cost += getPinCost(io_pin.getMirrorPinIdx());
  }

  bool free_slot = false;
  int new_slot;
  int mirrored_slot;

  int first_slot = 0;
  int last_slot = num_slots_ - 1;
  getSlotsRange(netlist_->getIoPin(pin), first_slot, last_slot);

  distribution
      = boost::random::uniform_int_distribution<int>(first_slot, last_slot);
  while (!free_slot) {
    new_slot = distribution(generator_);
    if (io_pin.isMirrored()) {
      free_slot = isFreeForMirrored(new_slot, mirrored_slot);
    } else {
      free_slot = slots_[new_slot].isAvailable() && new_slot != prev_slot;
    }
  }

  new_slots_.push_back(new_slot);
  pin_assignment_[pin] = new_slot;

  if (io_pin.isMirrored()) {
    pins_.push_back(io_pin.getMirrorPinIdx());
    int prev_mirrored_slot = pin_assignment_[io_pin.getMirrorPinIdx()];
    prev_slots_.push_back(prev_mirrored_slot);
    new_slots_.push_back(mirrored_slot);
    pin_assignment_[io_pin.getMirrorPinIdx()] = mirrored_slot;
  }

  return prev_cost;
}

int SimulatedAnnealing::moveGroupToFreeSlots(const int group_idx)
{
  const PinGroupByIndex& group = pin_groups_[group_idx];
  int prev_cost = computeGroupPrevCost(group_idx);
  updateSlotsFromGroup(prev_slots_, false);

  const IOPin& io_pin = netlist_->getIoPin(pins_[0]);

  bool free_slot = false;
  bool same_edge_slot = false;
  int new_slot;

  int first_slot = 0;
  int last_slot = num_slots_ - 1;
  getSlotsRange(io_pin, first_slot, last_slot);
  boost::random::uniform_int_distribution<int> distribution(first_slot,
                                                            last_slot);

  // add max number of iterations to find available slots for group to avoid
  // infinite loop in cases where there are not available contiguous slots
  int iter = 0;
  int max_iters = num_slots_ * 10;
  while ((!free_slot || !same_edge_slot) && iter < max_iters) {
    new_slot = distribution(generator_);
    if ((new_slot + pins_.size() >= num_slots_ - 1)) {
      continue;
    }

    int slot = new_slot;
    int mirrored_slot;
    const Edge& edge = slots_[slot].edge;
    for (int i = 0; i < group.pin_indices.size(); i++) {
      free_slot = slots_[slot].isAvailable();
      if (io_pin.isMirrored()) {
        free_slot = free_slot && isFreeForMirrored(slot, mirrored_slot);
      }
      same_edge_slot = slots_[slot].edge == edge;
      if (!free_slot || !same_edge_slot) {
        break;
      }

      slot++;
    }
    iter++;
  }

  if (free_slot && same_edge_slot) {
    netlist_->sortPinsFromGroup(group_idx, slots_[new_slot].edge);
    updateGroupSlots(group.pin_indices, new_slot);
  } else {
    prev_slots_.clear();
    new_slots_.clear();
    pins_.clear();
    updateSlotsFromGroup(prev_slots_, true);
    return move_fail_;
  }

  return prev_cost;
}

int SimulatedAnnealing::shiftGroup(int group_idx)
{
  const PinGroupByIndex& group = pin_groups_[group_idx];
  const std::vector<int>& pin_indices = group.pin_indices;
  bool is_mirrored = false;
  for (const int pin_idx : pin_indices) {
    const IOPin& io_pin = netlist_->getIoPin(pin_idx);
    if (io_pin.isMirrored()) {
      is_mirrored = true;
      break;
    }
  }
  int prev_cost = computeGroupPrevCost(group_idx);
  updateSlotsFromGroup(prev_slots_, false);

  const int min_slot = std::min(pin_assignment_[pin_indices.front()],
                                pin_assignment_[pin_indices.back()]);

  bool free_slot = true;
  bool same_edge_slot = true;
  const Edge& edge = slots_[min_slot].edge;

  int min_count = 0;
  int slot = min_slot - 1;
  while (free_slot && same_edge_slot && slot >= 0) {
    free_slot = slots_[slot].isAvailable();
    int mirrored_slot;
    if (is_mirrored) {
      free_slot = free_slot && isFreeForMirrored(slot, mirrored_slot);
    }
    same_edge_slot = slots_[slot].edge == edge;
    if (!free_slot || !same_edge_slot) {
      break;
    }

    min_count++;
    slot--;
  }

  free_slot = true;
  same_edge_slot = true;

  int max_count = 0;
  slot = min_slot + 1;
  while (free_slot && same_edge_slot && slot < slots_.size()) {
    free_slot = slots_[slot].isAvailable();
    int mirrored_slot;
    if (is_mirrored) {
      free_slot = free_slot && isFreeForMirrored(slot, mirrored_slot);
    }
    same_edge_slot = slots_[slot].edge == edge;
    if (!free_slot || !same_edge_slot) {
      break;
    }

    max_count++;
    slot++;
  }
  max_count -= pin_indices.size();

  if (min_count + max_count > 0) {
    if (min_count > max_count) {
      shiftGroupToPosition(pin_indices, min_count, min_slot, false);
    } else {
      shiftGroupToPosition(pin_indices, max_count, min_slot, true);
    }
  } else {
    prev_cost = move_fail_;
  }

  return prev_cost;
}

void SimulatedAnnealing::shiftGroupToPosition(
    const std::vector<int>& pin_indices,
    int free_slots_count,
    const int min_slot,
    bool move_to_max)
{
  boost::random::uniform_int_distribution<int> distribution(1,
                                                            free_slots_count);
  int move_count = distribution(generator_);
  int new_slot = move_to_max ? min_slot + move_count : min_slot - move_count;
  updateGroupSlots(pin_indices, new_slot);
}

int SimulatedAnnealing::rearrangeConstrainedGroups(int constraint_idx)
{
  int prev_cost = 0;
  const Constraint& constraint = constraints_[constraint_idx];
  std::vector<int> group_indices;
  std::vector<int> aux_indices;
  std::vector<GroupLimits> group_limits_list;

  if (constraint.pin_groups.size() > 1) {
    int groups_cnt = 0;
    for (int group_idx : constraint.pin_groups) {
      group_indices.push_back(group_idx);
      aux_indices.push_back(groups_cnt);
      const PinGroupByIndex& group = pin_groups_[group_idx];
      prev_cost += computeGroupPrevCost(group_idx);

      const std::vector<int>& pin_indices = group.pin_indices;

      const int min_slot = std::min(pin_assignment_[pin_indices.front()],
                                    pin_assignment_[pin_indices.back()]);
      const int max_slot = std::max(pin_assignment_[pin_indices.front()],
                                    pin_assignment_[pin_indices.back()]);

      GroupLimits group_limits(min_slot, max_slot);
      group_limits_list.push_back(group_limits);
      groups_cnt++;
    }

    std::ranges::sort(group_limits_list);
    utl::shuffle(aux_indices.begin(), aux_indices.end(), generator_);

    int cnt = 0;
    int new_slot = group_limits_list[cnt].first;
    for (int idx : aux_indices) {
      int group_idx = group_indices[idx];
      const PinGroupByIndex& group = pin_groups_[group_idx];
      netlist_->sortPinsFromGroup(group_idx, slots_[new_slot].edge);
      updateGroupSlots(group.pin_indices, new_slot);
      cnt++;
      if (cnt < group_limits_list.size()) {
        new_slot += group_limits_list[cnt].first
                    - group_limits_list[cnt - 1].second - 1;
      }
    }
  } else {
    for (int group_idx : constraint.pin_groups) {
      prev_cost = shiftGroup(group_idx);
    }
  }

  return prev_cost;
}

int SimulatedAnnealing::moveGroup(int pin_idx)
{
  const IOPin& io_pin = netlist_->getIoPin(pin_idx);
  int group_idx = io_pin.getGroupIdx();
  if (io_pin.getConstraintIdx() != -1) {
    const Constraint& constraint = constraints_[io_pin.getConstraintIdx()];
    if (constraint.pins_per_slots <= pins_per_slot_limit_) {
      boost::random::uniform_real_distribution<float> distribution;
      const float move = distribution(generator_);
      if (move <= group_to_free_slots_) {
        return moveGroupToFreeSlots(group_idx);
      }

      return shiftGroup(group_idx);
    }
    boost::random::uniform_real_distribution<float> distribution;
    const float move = distribution(generator_);
    if (move > shift_group_ && constraint.pin_groups.size() > 1) {
      return rearrangeConstrainedGroups(io_pin.getConstraintIdx());
    }

    return shiftGroup(group_idx);
  }

  return moveGroupToFreeSlots(group_idx);
}

void SimulatedAnnealing::restorePreviousAssignment()
{
  if (!prev_slots_.empty()) {
    // restore single pin to previous slot
    int cnt = 0;
    for (int pin : pins_) {
      pin_assignment_[pin] = prev_slots_[cnt];
      cnt++;
    }
  }
}

bool SimulatedAnnealing::isFreeForGroup(int& slot_idx,
                                        int group_size,
                                        int last_slot)
{
  bool free_slot = false;
  while (!free_slot) {
    if ((slot_idx + group_size >= last_slot)) {
      free_slot = false;
      break;
    }

    int slot = slot_idx;
    int mirrored_slot = getMirroredSlotIdx(slot);
    free_slot = true;  // Assume the slots are free and prove otherwise
    for (int i = 0; i < group_size; i++) {
      if (!slots_[slot].isAvailable() || !slots_[mirrored_slot].isAvailable()) {
        free_slot = false;
        break;
      }

      slot++;
    }

    if (!free_slot) {
      slot_idx++;  // Move to the next slot if the current one is not free
    }
  }

  return free_slot;
}

void SimulatedAnnealing::getSlotsRange(const IOPin& io_pin,
                                       int& first_slot,
                                       int& last_slot)
{
  const int pin_constraint_idx = io_pin.getConstraintIdx();
  if (pin_constraint_idx != -1) {
    const Constraint& constraint = constraints_[pin_constraint_idx];
    first_slot = constraint.first_slot;
    last_slot = constraint.last_slot;
  } else if (io_pin.isMirrored()) {
    const IOPin& mirrored_pin = netlist_->getIoPin(io_pin.getMirrorPinIdx());
    const int mirrored_constraint_idx = mirrored_pin.getConstraintIdx();
    if (mirrored_constraint_idx != -1) {
      const Constraint& constraint = constraints_[mirrored_constraint_idx];
      getMirroredSlotRange(
          constraint.first_slot, constraint.last_slot, first_slot, last_slot);
      if (first_slot > last_slot) {
        std::swap(first_slot, last_slot);
      }
    }
  }
}

int SimulatedAnnealing::getSlotIdxByPosition(const odb::Point& position,
                                             int layer) const
{
  int slot_idx = -1;
  for (int i = 0; i < slots_.size(); i++) {
    if (slots_[i].pos == position && slots_[i].layer == layer) {
      slot_idx = i;
      break;
    }
  }

  return slot_idx;
}

bool SimulatedAnnealing::isFreeForMirrored(const int slot_idx,
                                           int& mirrored_idx) const
{
  mirrored_idx = getMirroredSlotIdx(slot_idx);
  const Slot& slot = slots_[slot_idx];
  const Slot& mirrored_slot = slots_[mirrored_idx];

  return slot.isAvailable() && mirrored_slot.isAvailable();
}

int SimulatedAnnealing::getMirroredSlotIdx(int slot_idx) const
{
  const Slot& slot = slots_[slot_idx];
  const int layer = slot.layer;
  const odb::Point& position = slot.pos;
  odb::Point mirrored_pos = core_->getMirroredPosition(position);

  int mirrored_idx = getSlotIdxByPosition(mirrored_pos, layer);

  if (mirrored_idx < 0) {
    odb::dbTechLayer* tech_layer = db_->getTech()->findRoutingLayer(layer);
    logger_->error(utl::PPL,
                   112,
                   "Mirrored position ({}, {}) at layer {} is not a "
                   "valid position.",
                   mirrored_pos.getX(),
                   mirrored_pos.getY(),
                   tech_layer ? tech_layer->getName() : "NA");
  }

  return mirrored_idx;
}

void SimulatedAnnealing::getMirroredSlotRange(const int slot_idx1,
                                              const int slot_idx2,
                                              int& mirrored_slot1,
                                              int& mirrored_slot2) const
{
  const Slot& slot1 = slots_[slot_idx1];
  const Slot& slot2 = slots_[slot_idx2];
  if (slot1.edge != slot2.edge) {
    logger_->error(
        utl::PPL, 36, "Constraint cannot have positions in different edges.");
  }
  const Edge edge = slot1.edge;
  const odb::Point& mirrored_pos1 = core_->getMirroredPosition(slot1.pos);
  const odb::Point& mirrored_pos2 = core_->getMirroredPosition(slot2.pos);

  // find the approximated mirrored position of the original slot range
  // endpoints. this is only necessary because when using multiple layers, the
  // mirrored position of the original slots endpoints are not in the beginning
  // of the mirrored range.
  if (slot1.edge == Edge::bottom || slot1.edge == Edge::right) {
    for (int i = slots_.size() - 1; i >= 0; i--) {
      const Slot& slot = slots_[i];
      if ((edge == Edge::bottom && slot.pos.getX() >= mirrored_pos1.getX()
           && slot.pos.getY() == mirrored_pos1.getY())
          || (edge == Edge::right && slot.pos.getX() == mirrored_pos1.getX()
              && slot.pos.getY() >= mirrored_pos1.getY())) {
        mirrored_slot1 = i;
        break;
      }
    }

    for (int i = 0; i < slots_.size(); i++) {
      const Slot& slot = slots_[i];
      if ((edge == Edge::bottom && slot.pos.getX() <= mirrored_pos2.getX()
           && slot.pos.getY() == mirrored_pos2.getY())
          || (edge == Edge::right && slot.pos.getX() == mirrored_pos2.getX()
              && slot.pos.getY() <= mirrored_pos2.getY())) {
        mirrored_slot2 = i;
        break;
      }
    }
  } else {
    for (int i = 0; i < slots_.size(); i++) {
      const Slot& slot = slots_[i];
      if ((edge == Edge::top && slot.pos.getX() >= mirrored_pos1.getX()
           && slot.pos.getY() == mirrored_pos1.getY())
          || (edge == Edge::left && slot.pos.getX() == mirrored_pos1.getX()
              && slot.pos.getY() >= mirrored_pos1.getY())) {
        mirrored_slot1 = i;
        break;
      }
    }

    for (int i = slots_.size() - 1; i >= 0; i--) {
      const Slot& slot = slots_[i];
      if ((edge == Edge::top && slot.pos.getX() <= mirrored_pos2.getX()
           && slot.pos.getY() == mirrored_pos2.getY())
          || (edge == Edge::left && slot.pos.getX() == mirrored_pos2.getX()
              && slot.pos.getY() <= mirrored_pos2.getY())) {
        mirrored_slot2 = i;
        break;
      }
    }
  }
}

void SimulatedAnnealing::updateSlotsFromGroup(
    const std::vector<int>& prev_slots_,
    bool block)
{
  for (int slot : prev_slots_) {
    slots_[slot].used = block;
  }
}

int SimulatedAnnealing::computeGroupPrevCost(int group_idx)
{
  const PinGroupByIndex& group = pin_groups_[group_idx];
  int prev_cost = getGroupCost(group_idx);
  for (int pin_idx : group.pin_indices) {
    prev_slots_.push_back(pin_assignment_[pin_idx]);
    pins_.push_back(pin_idx);
    if (netlist_->getIoPin(pin_idx).isMirrored()) {
      int mirrored_idx = netlist_->getIoPin(pin_idx).getMirrorPinIdx();
      prev_slots_.push_back(pin_assignment_[mirrored_idx]);
      pins_.push_back(mirrored_idx);
      prev_cost += getPinCost(mirrored_idx);
    }
  }

  return prev_cost;
}

void SimulatedAnnealing::updateGroupSlots(const std::vector<int>& pin_indices,
                                          int& new_slot)
{
  for (int pin_idx : pin_indices) {
    const IOPin& io_pin = netlist_->getIoPin(pin_idx);
    pin_assignment_[pin_idx] = new_slot;
    new_slots_.push_back(new_slot);
    if (io_pin.isMirrored()) {
      int mirrored_slot = getMirroredSlotIdx(new_slot);
      pin_assignment_[io_pin.getMirrorPinIdx()] = mirrored_slot;
      new_slots_.push_back(mirrored_slot);
    }
    new_slot++;
  }
}

void SimulatedAnnealing::countLonePins()
{
  int pins_in_groups = 0;
  for (const auto& group : pin_groups_) {
    for (const int pin_idx : group.pin_indices) {
      pins_in_groups++;
      if (netlist_->getIoPin(pin_idx).isMirrored()) {
        pins_in_groups++;
      }
    }
  }

  lone_pins_ = num_pins_ - pins_in_groups;
}

}  // namespace ppl
