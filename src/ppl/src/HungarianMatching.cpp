// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "HungarianMatching.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

#include "Core.h"
#include "Netlist.h"
#include "Slots.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace ppl {

HungarianMatching::HungarianMatching(const Section& section,
                                     Netlist* netlist,
                                     Core* core,
                                     std::vector<Slot>& slots,
                                     utl::Logger* logger,
                                     odb::dbDatabase* db)
    : netlist_(netlist),
      core_(core),
      pin_indices_(section.pin_indices),
      pin_groups_(section.pin_groups),
      slots_(slots),
      db_(db)
{
  num_io_pins_ = section.pin_indices.size();
  num_pin_groups_ = netlist_->numIOGroups();
  begin_slot_ = section.begin_slot;
  end_slot_ = section.end_slot;
  num_slots_ = end_slot_ - begin_slot_;
  non_blocked_slots_ = section.num_slots;
  group_slots_ = 0;
  edge_ = section.edge;
  logger_ = logger;
}

void HungarianMatching::findAssignment()
{
  createMatrix();
  if (!hungarian_matrix_.empty()) {
    hungarian_solver_.solve(hungarian_matrix_, assignment_);
  }
}

void HungarianMatching::createMatrix()
{
  hungarian_matrix_.resize(non_blocked_slots_);
  int pin_index = 0;

  for (int idx : pin_indices_) {
    IOPin& io_pin = netlist_->getIoPin(idx);
    if (!io_pin.isInGroup()) {
      bool is_mirrored = false;
      std::vector<int> larger_costs;
      int slot_index = 0;
      for (int i = begin_slot_; i <= end_slot_; ++i) {
        const odb::Point& slot_pos = slots_[i].pos;
        if (slots_[i].blocked) {
          continue;
        }
        hungarian_matrix_[slot_index].resize(num_io_pins_,
                                             std::numeric_limits<int>::max());
        const int io_net_hpwl = netlist_->computeIONetHPWL(idx, slot_pos);
        const int mirrored_cost = getMirroredPinCost(io_pin, slot_pos);
        const int hpwl = io_net_hpwl + mirrored_cost;
        larger_costs.push_back(std::max(io_net_hpwl, mirrored_cost));
        hungarian_matrix_[slot_index][pin_index] = hpwl;
        is_mirrored = is_mirrored || mirrored_cost != 0;
        slot_index++;
      }

      if (is_mirrored) {
        std::vector<uint8_t> rank = getTieBreakRank(larger_costs);
        for (int idx = 0; idx < slot_index; idx++) {
          const int hpwl = hungarian_matrix_[idx][pin_index];
          if ((hpwl >> 24) != 0) {
            logger_->critical(utl::PPL, 210, "Cost for pin exceeds 24 bits.");
          }
          hungarian_matrix_[idx][pin_index] = (hpwl << 8) | rank[idx];
        }
      }
      pin_index++;
    }
  }
}

void HungarianMatching::getFinalAssignment(std::vector<IOPin>& assignment,
                                           bool assign_mirrored)
{
  size_t rows = non_blocked_slots_;
  size_t col = 0;
  int slot_index = 0;
  for (int idx : pin_indices_) {
    IOPin& io_pin = netlist_->getIoPin(idx);

    if (!io_pin.isInGroup()) {
      slot_index = begin_slot_;
      for (size_t row = 0; row < rows; row++) {
        while (slots_[slot_index].blocked && slot_index < slots_.size()) {
          slot_index++;
        }
        if (assignment_[row] != col) {
          slot_index++;
          continue;
        }
        if (hungarian_matrix_[row][col] == hungarian_fail_) {
          logger_->warn(utl::PPL,
                        33,
                        "I/O pin {} cannot be placed in the specified region. "
                        "Not enough space.",
                        io_pin.getName().c_str());
        }

        // Make this check here to avoid messing up the correlation between the
        // pin sorting and the hungarian matrix values
        if ((assign_mirrored && !io_pin.getBTerm()->hasMirroredBTerm())
            || io_pin.isPlaced()) {
          continue;
        }
        io_pin.setPosition(slots_[slot_index].pos);
        io_pin.setLayer(slots_[slot_index].layer);
        io_pin.setPlaced();
        io_pin.setEdge(slots_[slot_index].edge);
        // Set line information only for polygon edges
        if (slots_[slot_index].edge == Edge::polygonEdge) {
          io_pin.setLine(slots_[slot_index].containing_line);
        }
        assignment.push_back(io_pin);
        slots_[slot_index].used = true;

        if (assign_mirrored) {
          assignMirroredPins(io_pin, assignment);
        }
        break;
      }
      col++;
    }
  }
}

void HungarianMatching::assignMirroredPins(IOPin& io_pin,
                                           std::vector<IOPin>& assignment)
{
  odb::dbBTerm* mirrored_term = io_pin.getBTerm()->getMirroredBTerm();
  int mirrored_pin_idx = netlist_->getIoPinIdx(mirrored_term);
  IOPin& mirrored_pin = netlist_->getIoPin(mirrored_pin_idx);

  odb::Point mirrored_pos = core_->getMirroredPosition(io_pin.getPosition());
  mirrored_pin.setPosition(mirrored_pos);
  mirrored_pin.setLayer(io_pin.getLayer());
  mirrored_pin.setEdge(getMirroredEdge(io_pin.getEdge()));
  mirrored_pin.setPlaced();
  assignment.push_back(mirrored_pin);
  int slot_index = getSlotIdxByPosition(mirrored_pos, mirrored_pin.getLayer());
  if (slot_index < 0 || slots_[slot_index].used) {
    odb::dbTechLayer* layer
        = db_->getTech()->findRoutingLayer(mirrored_pin.getLayer());
    logger_->error(utl::PPL,
                   82,
                   "Mirrored position ({}, {}) at layer {} is not a "
                   "valid position for pin {} placement.",
                   mirrored_pos.getX(),
                   mirrored_pos.getY(),
                   layer ? layer->getName() : "NA",
                   mirrored_pin.getName());
  }
  slots_[slot_index].used = true;
}

void HungarianMatching::findAssignmentForGroups()
{
  createMatrixForGroups();

  if (!hungarian_matrix_.empty()) {
    hungarian_solver_.solve(hungarian_matrix_, assignment_);
  }
}

void HungarianMatching::createMatrixForGroups()
{
  std::vector<int> group_sizes;
  std::vector<int> group_slot_capacity;
  group_sizes.reserve(pin_groups_.size());
  for (const auto& [pins, order] : pin_groups_) {
    group_sizes.push_back(pins.size());
  }

  std::ranges::sort(group_sizes, std::greater<int>());

  if (!group_sizes.empty()) {
    valid_starting_slots_.clear();
    int i = begin_slot_;
    int new_begin = i;
    for (const int group_size : group_sizes) {
      i = new_begin;
      // end the loop to avoid access invalid positions of slots_.
      const int end_i = (end_slot_ - group_size + 1);
      while (i <= end_i) {
        bool blocked = false;
        for (int pin_cnt = 0; pin_cnt < group_size; pin_cnt++) {
          if (slots_[i + pin_cnt].blocked) {
            blocked = true;
            // Find the next unblocked slot, if any, to try again
            while (++i <= end_i) {
              if (!slots_[i + pin_cnt].blocked) {
                break;
              }
            }
            break;
          }
        }
        if (!blocked) {
          group_slots_++;
          valid_starting_slots_.push_back(i);
          group_slot_capacity.push_back(group_size);
          // We have a legal position so jump ahead to limit the
          // number of times we run the hungarian code.
          i += group_size;
          new_begin = i;
        }
      }
    }

    hungarian_matrix_.resize(group_slots_);
    int group_index = 0;
    for (const auto& [pins, order] : pin_groups_) {
      int slot_index = 0;
      bool is_mirrored = false;
      std::vector<int> larger_costs(valid_starting_slots_.size(), 0);
      for (int i : valid_starting_slots_) {
        int group_hpwl = 0;
        for (const int io_idx : pins) {
          const odb::Point& slot_pos = slots_[i].pos;

          hungarian_matrix_[slot_index].resize(num_pin_groups_,
                                               std::numeric_limits<int>::max());
          IOPin& io_pin = netlist_->getIoPin(io_idx);
          int pin_hpwl = netlist_->computeIONetHPWL(io_idx, slot_pos);
          if (pin_hpwl == hungarian_fail_) {
            group_hpwl = hungarian_fail_;
            break;
          }
          const int mirrored_cost = getMirroredPinCost(io_pin, slot_pos);
          group_hpwl += pin_hpwl + mirrored_cost;
          larger_costs[slot_index] += std::max(pin_hpwl, mirrored_cost);
          is_mirrored = is_mirrored || mirrored_cost != 0;
        }
        if (pins.size() > group_slot_capacity[slot_index]) {
          group_hpwl = std::numeric_limits<int>::max();
        }
        hungarian_matrix_[slot_index][group_index] = group_hpwl;
        slot_index++;
      }

      if (is_mirrored) {
        std::vector<uint8_t> rank = getTieBreakRank(larger_costs);
        for (int idx = 0; idx < slot_index; idx++) {
          const int hpwl = hungarian_matrix_[idx][group_index];
          if ((hpwl >> 24) != 0) {
            logger_->critical(utl::PPL, 211, "Cost for pin exceeds 24 bits.");
          }
          hungarian_matrix_[idx][group_index] = (hpwl << 8) | rank[idx];
        }
      }
      group_index++;
    }

    if (hungarian_matrix_.empty()) {
      logger_->error(utl::PPL,
                     89,
                     "Could not create matrix for groups. Not available slots "
                     "inside section.");
    }
  }
}

void HungarianMatching::getAssignmentForGroups(std::vector<IOPin>& assignment,
                                               bool only_mirrored)
{
  if (hungarian_matrix_.empty()) {
    return;
  }

  size_t rows = group_slots_;
  size_t col = 0;
  int slot_index = 0;
  for (const auto& [pins, order] : pin_groups_) {
    bool assigned = false;
    if ((only_mirrored && !groupHasMirroredPin(pins))
        || (!only_mirrored && groupHasMirroredPin(pins))) {
      continue;
    }

    slot_index = begin_slot_;
    for (size_t row = 0; row < rows; row++) {
      if (assignment_[row] != col) {
        continue;
      }
      slot_index = valid_starting_slots_.at(row);

      int pin_cnt = (edge_ == Edge::top || edge_ == Edge::left) && order
                        ? pins.size() - 1
                        : 0;

      for (int pin_idx : pins) {
        IOPin& io_pin = netlist_->getIoPin(pin_idx);
        io_pin.setPosition(slots_[slot_index + pin_cnt].pos);
        io_pin.setLayer(slots_[slot_index + pin_cnt].layer);
        io_pin.setEdge(slots_[slot_index + pin_cnt].edge);
        // Set line information only for polygon edges
        if (slots_[slot_index + pin_cnt].edge == Edge::polygonEdge) {
          io_pin.setLine(slots_[slot_index + pin_cnt].containing_line);
        }
        assignment.push_back(io_pin);
        slots_[slot_index + pin_cnt].used = true;
        slots_[slot_index + pin_cnt].blocked = true;
        if ((slot_index + pin_cnt) <= end_slot_) {
          non_blocked_slots_--;
        }
        pin_cnt = (edge_ == Edge::top || edge_ == Edge::left) && order
                      ? pin_cnt - 1
                      : pin_cnt + 1;
        if (io_pin.getBTerm()->hasMirroredBTerm()) {
          assignMirroredPins(io_pin, assignment);
        }
      }
      assigned = true;
      break;
    }
    col++;
    if (!assigned) {
      logger_->error(
          utl::PPL, 86, "Pin group of size {} was not assigned.", pins.size());
    }
  }

  hungarian_matrix_.clear();
  assignment_.clear();
}

int HungarianMatching::getSlotIdxByPosition(const odb::Point& position,
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

bool HungarianMatching::groupHasMirroredPin(const std::vector<int>& group)
{
  for (int pin_idx : group) {
    IOPin& io_pin = netlist_->getIoPin(pin_idx);
    if (io_pin.getBTerm()->hasMirroredBTerm()) {
      return true;
    }
  }

  return false;
}

int HungarianMatching::getMirroredPinCost(IOPin& io_pin,
                                          const odb::Point& position)
{
  if (io_pin.getBTerm()->hasMirroredBTerm()) {
    odb::Point mirrored_pos = core_->getMirroredPosition(position);
    return netlist_->computeIONetHPWL(io_pin.getMirrorPinIdx(), mirrored_pos);
  }

  return 0;
}

Edge HungarianMatching::getMirroredEdge(const Edge& edge)
{
  Edge mirrored_edge = Edge::invalid;
  if (edge == Edge::bottom) {
    mirrored_edge = Edge::top;
  } else if (edge == Edge::top) {
    mirrored_edge = Edge::bottom;
  } else if (edge == Edge::left) {
    mirrored_edge = Edge::right;
  } else if (edge == Edge::right) {
    mirrored_edge = Edge::left;
  } else {
    mirrored_edge = Edge::invalid;
  }

  return mirrored_edge;
}

std::vector<uint8_t> HungarianMatching::getTieBreakRank(
    const std::vector<int>& costs)
{
  std::vector<uint8_t> rank(costs.size());
  uint8_t ranking = 1;
  for (int i : sortIndexes(costs)) {
    rank[i] = ranking;
    ranking++;
  }

  return rank;
}

}  // namespace ppl
