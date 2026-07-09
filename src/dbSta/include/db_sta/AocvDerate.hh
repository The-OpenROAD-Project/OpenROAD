// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Depth-based (AOCV-style) on-chip-variation derate, first slice.
//
// This is an OpenROAD-side (dbSta) feature that re-uses public OpenSTA APIs to
// recompute a depth-adjusted setup slack on already-found critical paths. It
// does NOT modify the OpenSTA forward search; see AOCV_INVESTIGATION.md for the
// rationale and the documented limitation.

#pragma once

#include <map>
#include <string>
#include <vector>

namespace sta {

class Sta;
class PathEnd;

// A simple depth -> derate-multiplier table, with separate early and late
// columns. depth is the number of combinational logic stages along the data
// path. Lookup uses the entry with the largest tabulated depth that is <= the
// queried depth (i.e. step / hold-last-value), which matches how AOCV tables
// are typically interpreted. If empty, lookups return 1.0 (inactive).
class AocvDerateTable
{
 public:
  void clear();
  bool empty() const { return late_.empty() && early_.empty(); }

  void setLate(int depth, float derate) { late_[depth] = derate; }
  void setEarly(int depth, float derate) { early_[depth] = derate; }

  // Returns 1.0 when no late entries are present (inactive / default).
  float lateDerate(int depth) const;
  float earlyDerate(int depth) const;

  // Load a whitespace table file with lines of the form:
  //   depth late_derate [early_derate]
  // '#' starts a comment. Returns false and sets error on a malformed line.
  bool readFile(const std::string& filename, std::string& error);

 private:
  static float lookup(const std::map<int, float>& tbl, int depth);

  std::map<int, float> late_;
  std::map<int, float> early_;
};

// One row of the AOCV slack report for a single path end.
struct AocvPathResult
{
  std::string endpoint;
  int logic_depth = 0;
  float flat_slack = 0.0f;
  float aocv_slack = 0.0f;
  float late_derate = 1.0f;  // derate used for this depth
};

// Recompute depth-adjusted setup slack for one path end, applying the table's
// late derate (by logic depth) to the data path's combinational cell arcs in
// place of the flat late derate that OpenSTA actually applied.
AocvPathResult aocvAdjustPathEnd(Sta* sta,
                                 PathEnd* path_end,
                                 const AocvDerateTable& table);

}  // namespace sta
