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

#include "Core.h"
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
  HungarianMatching(const Section& section,
                    Netlist* netlist,
                    Core* core,
                    std::vector<Slot>& slots,
                    Logger* logger,
                    odb::dbDatabase* db);
  virtual ~HungarianMatching() = default;
  void findAssignment();
  void findAssignmentForGroups();
  void getFinalAssignment(std::vector<IOPin>& assignment,
                          MirroredPins& mirrored_pins,
                          bool assign_mirrored);
  void getAssignmentForGroups(std::vector<IOPin>& assignment,
                              MirroredPins& mirrored_pins,
                              bool only_mirrored);

 private:
  std::vector<std::vector<int>> hungarian_matrix_;
  std::vector<int> assignment_;
  std::vector<int> valid_starting_slots_;
  HungarianAlgorithm hungarian_solver_;
  Netlist* netlist_;
  Core* core_;
  const std::vector<int>& pin_indices_;
  const std::vector<PinGroupByIndex>& pin_groups_;
  std::vector<Slot>& slots_;
  int begin_slot_;
  int end_slot_;
  int num_slots_;
  int num_io_pins_;
  int num_pin_groups_;
  int non_blocked_slots_;
  int group_slots_;
  Edge edge_;
  const int hungarian_fail = std::numeric_limits<int>::max();
  Logger* logger_;
  odb::dbDatabase* db_;

  void createMatrix();
  void createMatrixForGroups();
  void assignMirroredPins(IOPin& io_pin,
                          MirroredPins& mirrored_pins,
                          std::vector<IOPin>& assignment);
  int getSlotIdxByPosition(const odb::Point& position, int layer) const;
  bool groupHasMirroredPin(const std::vector<int>& group,
                           MirroredPins& mirrored_pins);
  Edge getMirroredEdge(const Edge& edge);
};

}  // namespace ppl
