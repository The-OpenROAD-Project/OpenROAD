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

#include "Netlist.h"

#include "Slots.h"
#include "ppl/IOPlacer.h"

#include <algorithm>

namespace ppl {

Netlist::Netlist()
{
  net_pointer_.push_back(0);
}

void Netlist::addIONet(const IOPin& io_pin,
                       const std::vector<InstancePin>& inst_pins)
{
  _db_pin_idx_map[io_pin.getBTerm()] = io_pins_.size();
  io_pins_.push_back(io_pin);
  inst_pins_.insert(inst_pins_.end(), inst_pins.begin(), inst_pins.end());
  net_pointer_.push_back(inst_pins_.size());
}

int Netlist::createIOGroup(const std::vector<odb::dbBTerm*>& pin_list)
{
  int pin_cnt = 0;
  std::vector<int> pin_indices;
  for (odb::dbBTerm* bterm : pin_list) {
    int pin_idx = _db_pin_idx_map[bterm];
    if (pin_idx < 0) {
      return pin_cnt;
    }
    io_pins_[pin_idx].setInGroup();
    pin_indices.push_back(pin_idx);
    pin_cnt++;
  }

  io_groups_.push_back(pin_indices);
  return pin_indices.size();
}

void Netlist::addIOGroup(const std::vector<int>& pin_group)
{
  io_groups_.push_back(pin_group);
}

void Netlist::getSinksOfIO(int idx, std::vector<InstancePin>& sinks)
{
  int net_start = net_pointer_[idx];
  int net_end = net_pointer_[idx + 1];
  for (int sink_idx = net_start; sink_idx < net_end; ++sink_idx) {
    sinks.push_back(inst_pins_[sink_idx]);
  }
}

int Netlist::numSinksOfIO(int idx)
{
  int net_start = net_pointer_[idx];
  int net_end = net_pointer_[idx + 1];
  return net_end - net_start;
}

int Netlist::numIOPins()
{
  return io_pins_.size();
}

Rect Netlist::getBB(int idx, const Point& slot_pos)
{
  int net_start = net_pointer_[idx];
  int net_end = net_pointer_[idx + 1];

  int min_x = slot_pos.x();
  int min_y = slot_pos.y();
  int max_x = slot_pos.x();
  int max_y = slot_pos.y();

  for (int idx = net_start; idx < net_end; ++idx) {
    Point pos = inst_pins_[idx].getPos();
    min_x = std::min(min_x, pos.x());
    max_x = std::max(max_x, pos.x());
    min_y = std::min(min_y, pos.y());
    max_y = std::max(max_y, pos.y());
  }

  Point upper_bounds = Point(max_x, max_y);
  Point lower_bounds = Point(min_x, min_y);

  Rect net_b_box(lower_bounds, upper_bounds);
  return net_b_box;
}

int Netlist::computeIONetHPWL(int idx, const Point& slot_pos)
{
  int net_start = net_pointer_[idx];
  int net_end = net_pointer_[idx + 1];

  int min_x = slot_pos.x();
  int min_y = slot_pos.y();
  int max_x = slot_pos.x();
  int max_y = slot_pos.y();

  for (int idx = net_start; idx < net_end; ++idx) {
    Point pos = inst_pins_[idx].getPos();
    min_x = std::min(min_x, pos.x());
    max_x = std::max(max_x, pos.x());
    min_y = std::min(min_y, pos.y());
    max_y = std::max(max_y, pos.y());
  }

  int x = max_x - min_x;
  int y = max_y - min_y;

  return (x + y);
}

int Netlist::computeDstIOtoPins(int idx, const Point& slot_pos)
{
  int net_start = net_pointer_[idx];
  int net_end = net_pointer_[idx + 1];

  int total_distance = 0;

  for (int idx = net_start; idx < net_end; ++idx) {
    Point pin_pos = inst_pins_[idx].getPos();
    total_distance += std::abs(pin_pos.x() - slot_pos.x())
                      + std::abs(pin_pos.y() - slot_pos.y());
  }

  return total_distance;
}

void Netlist::clear()
{
  inst_pins_.clear();
  net_pointer_.clear();
  io_pins_.clear();
  io_groups_.clear();
  _db_pin_idx_map.clear();
}

}  // namespace ppl
