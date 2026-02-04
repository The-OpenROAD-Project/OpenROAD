// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "Core.h"
#include "Hungarian.h"
#include "Netlist.h"
#include "Slots.h"
#include "odb/geom.h"
#include "ppl/IOPlacer.h"
#include "utl/Logger.h"

namespace ppl {

class HungarianMatching
{
 public:
  HungarianMatching(const Section& section,
                    Netlist* netlist,
                    Core* core,
                    std::vector<Slot>& slots,
                    utl::Logger* logger,
                    odb::dbDatabase* db);
  virtual ~HungarianMatching() = default;
  void findAssignment();
  void findAssignmentForGroups();
  void getFinalAssignment(std::vector<IOPin>& assignment, bool assign_mirrored);
  void getAssignmentForGroups(std::vector<IOPin>& assignment,
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
  const int hungarian_fail_ = std::numeric_limits<int>::max();
  utl::Logger* logger_;
  odb::dbDatabase* db_;

  void createMatrix();
  void createMatrixForGroups();
  void assignMirroredPins(IOPin& io_pin, std::vector<IOPin>& assignment);
  int getSlotIdxByPosition(const odb::Point& position, int layer) const;
  bool groupHasMirroredPin(const std::vector<int>& group);
  int getMirroredPinCost(IOPin& io_pin, const odb::Point& position);
  Edge getMirroredEdge(const Edge& edge);
  std::vector<uint8_t> getTieBreakRank(const std::vector<int>& costs);
};

}  // namespace ppl
