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

#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <list>
#include <utility>

#include "Hungarian.h"
#include "Netlist.h"
#include "Slots.h"
#include "ppl/IOPlacer.h"

namespace utl {
class Logger;
}

namespace ppl {

using odb::Point;
using odb::Rect;
using utl::Logger;

class HungarianMatching
{
 public:
  HungarianMatching(Section&, Netlist&, std::vector<Slot>&, Logger* logger);
  virtual ~HungarianMatching() = default;
  void findAssignment();
  void findAssignmentForGroups();
  void getFinalAssignment(std::vector<IOPin>&) const;
  void getAssignmentForGroups(std::vector<IOPin>&);

 private:
  std::vector<std::vector<int>> hungarian_matrix_;
  std::vector<int> assignment_;
  HungarianAlgorithm hungarian_solver_;
  Netlist& netlist_;
  std::vector<int>& pin_indices_;
  std::vector<std::vector<int>>& pin_groups_;
  std::vector<Slot>& slots_;
  int begin_slot_;
  int end_slot_;
  int num_slots_;
  int num_io_pins_;
  int num_pin_groups_;
  int non_blocked_slots_;
  int group_slots_;
  int group_size_;
  Edge edge_;
  const int hungarian_fail = std::numeric_limits<int>::max();
  Logger* logger_;

  void createMatrix();
  void createMatrixForGroups();
};

}  // namespace ppl
