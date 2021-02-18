/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include "HungarianMatching.h"

#include "utility/Logger.h"

namespace ppl {

HungarianMatching::HungarianMatching(Section& section,
                                     SlotVector& slots,
                                     Logger* logger)
    : netlist_(section.net), slots_(slots)
{
  num_io_pins_ = netlist_.numIOPins();
  num_pin_groups_ = netlist_.numIOGroups();
  begin_slot_ = section.begin_slot;
  end_slot_ = section.end_slot;
  num_slots_ = end_slot_ - begin_slot_;
  non_blocked_slots_ = section.num_slots;
  group_slots_ = 0;
  group_size_ = -1;
  edge_ = section.edge;
  logger_ = logger;
}

void HungarianMatching::findAssignment(std::vector<Constraint>& constraints)
{
  createMatrix(constraints);
  hungarian_solver_.solve(hungarian_matrix_, assignment_);
}

void HungarianMatching::createMatrix(std::vector<Constraint>& constraints)
{
  hungarian_matrix_.resize(non_blocked_slots_);
  int slot_index = 0;
  for (int i = begin_slot_; i < end_slot_; ++i) {
    int pinIndex = 0;
    Point newPos = slots_[i].pos;
    if (slots_[i].blocked) {
      continue;
    }
    hungarian_matrix_[slot_index].resize(num_io_pins_);
    int idx = 0;
    for (IOPin& io_pin : netlist_.getIOPins()) {
      if (!io_pin.isInGroup()) {
        int hpwl = netlist_.computeIONetHPWL(idx, newPos, edge_, constraints);
        hungarian_matrix_[slot_index][pinIndex] = hpwl;
        pinIndex++;
      }
      idx++;
    }
    slot_index++;
  }
}

inline bool samePos(Point& a, Point& b)
{
  return (a.x() == b.x() && a.y() == b.y());
}

void HungarianMatching::getFinalAssignment(std::vector<IOPin>& assigment) const
{
  size_t rows = non_blocked_slots_;
  size_t col = 0;
  int slot_index = 0;
  for (IOPin& io_pin : netlist_.getIOPins()) {
    if (!io_pin.isInGroup()) {
      slot_index = begin_slot_;
      for (size_t row = 0; row < rows; row++) {
        while (slots_[slot_index].blocked && slot_index < slots_.size())
          slot_index++;
        if (assignment_[row] != col) {
          slot_index++;
          continue;
        }
        if (hungarian_matrix_[row][col] == hungarian_fail) {
          logger_->warn(utl::PPL,
                        33,
                        "I/O pin {} cannot be placed in the specified region. "
                        "Not enough space",
                        io_pin.getName().c_str());
        }
        io_pin.setPos(slots_[slot_index].pos);
        io_pin.setLayer(slots_[slot_index].layer);
        assigment.push_back(io_pin);
        slots_[slot_index].used = true;
        break;
      }
      col++;
    }
  }
}

void HungarianMatching::findAssignmentForGroups(std::vector<Constraint>& constraints)
{
  createMatrixForGroups(constraints);

  if (hungarian_matrix_.size() > 0)
    hungarian_solver_.solve(hungarian_matrix_, assignment_);
}

void HungarianMatching::createMatrixForGroups(std::vector<Constraint>& constraints)
{
  for (std::set<int>& io_group : netlist_.getIOGroups()) {
    group_size_ = std::max((int)io_group.size(), group_size_);
  }

  if (group_size_ > 0) {
    for (int i = begin_slot_; i < end_slot_; i+=group_size_) {
      bool blocked = false;
      for (int pin_cnt = 0; pin_cnt < group_size_; pin_cnt++) {
        if (slots_[i + pin_cnt].blocked) {
          blocked = true;
        }
      }
      if (!blocked) {
        group_slots_++;
      }
    }

    hungarian_matrix_.resize(group_slots_);
    int slot_index = 0;
    for (int i = begin_slot_; i < end_slot_; i+=group_size_) {
      int groupIndex = 0;
      Point newPos = slots_[i].pos;
      
      bool blocked = false;
      for (int pin_cnt = 0; pin_cnt < group_size_; pin_cnt++) {
        if (slots_[i + pin_cnt].blocked) {
          blocked = true;
        }
      }
      if (blocked) {
        continue;
      }

      hungarian_matrix_[slot_index].resize(num_pin_groups_);
      for (std::set<int>& io_group : netlist_.getIOGroups()) {
        int hpwl = 0;
        int pin_count = 0;
        for (int io_idx : io_group) {
          hpwl += netlist_.computeIONetHPWL(io_idx, newPos, edge_, constraints);
          hungarian_matrix_[slot_index][groupIndex] = hpwl;
        }
        groupIndex++;
      }
      slot_index++;
    }
  }
}

void HungarianMatching::getAssignmentForGroups(std::vector<IOPin>& assigment)
{
  if (hungarian_matrix_.size() <= 0)
    return;

  size_t rows = group_slots_;
  size_t col = 0;
  int slot_index = 0;
  for (std::set<int>& io_group : netlist_.getIOGroups()) {
    slot_index = begin_slot_;
    for (size_t row = 0; row < rows; row++) {
      while (slots_[slot_index].blocked && slot_index < slots_.size())
        slot_index++;
      if (assignment_[row] != col) {
        slot_index++;
        continue;
      }
      int pin_cnt = 0;
      for (int pin_idx : io_group) {
        IOPin io_pin = netlist_.getIoPin(pin_idx);
        io_pin.setPos(slots_[slot_index + pin_cnt].pos);
        io_pin.setLayer(slots_[slot_index + pin_cnt].layer);
        assigment.push_back(io_pin);
        slots_[slot_index + pin_cnt].used = true;
        slots_[slot_index + pin_cnt].blocked = true;
        non_blocked_slots_--;
        pin_cnt++;
      }
      break;
    }
    col++;
  }

  hungarian_matrix_.clear();
  assignment_.clear();
}

}  // namespace ppl
