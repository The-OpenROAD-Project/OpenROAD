// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string_view>

namespace rsz {

class MoveCommitter;
class RepairTargetCollector;
class Resizer;
struct RepairSetupContext;

// The repair_timing progress table of the setup policies: the column header,
// a row per reported iteration and the closing row.  A row shows the move
// totals, the area growth and the WNS/TNS/endpoint counts of a collector.
class ProgressReporter
{
 public:
  ProgressReporter(Resizer& resizer,
                   MoveCommitter& committer,
                   RepairSetupContext& context);

  // The column header: once per repair_timing call, and again when
  // show_startpoint_metrics changes the column layout.
  void printHeader(bool show_startpoint_metrics) const;
  // label is the iteration number with the phase marker; profile_point names
  // the profiler QoR sample, or nullptr.  show_startpoint_metrics adds the
  // StTNS column; worst_by_startpoint names the worst startpoint instead.
  void printRow(std::string_view label,
                RepairTargetCollector& collector,
                const char* profile_point,
                bool show_startpoint_metrics,
                bool worst_by_startpoint) const;
  // The closing row of the run, in the layout of the last header; with no
  // header yet, show_startpoint_metrics picks the layout.
  void printFinal(RepairTargetCollector& collector,
                  bool show_startpoint_metrics) const;

 private:
  // Recomputed only when the committer's edit count changed, because
  // computeDesignArea() walks every instance.
  double areaGrowthPercent() const;

  Resizer& resizer_;
  MoveCommitter& committer_;
  RepairSetupContext& context_;
};

}  // namespace rsz
