// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db_sta/AocvDerate.hh"

#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"

namespace sta {

void AocvDerateTable::clear()
{
  late_.clear();
  early_.clear();
}

float AocvDerateTable::lookup(const std::map<int, float>& tbl, int depth)
{
  if (tbl.empty()) {
    return 1.0f;
  }
  // Largest tabulated depth <= queried depth; if depth is below the first
  // entry, use the first (shallowest) entry.
  auto it = tbl.upper_bound(depth);
  if (it == tbl.begin()) {
    return it->second;
  }
  --it;
  return it->second;
}

float AocvDerateTable::lateDerate(int depth) const
{
  return lookup(late_, depth);
}

float AocvDerateTable::earlyDerate(int depth) const
{
  return lookup(early_, depth);
}

bool AocvDerateTable::readFile(const std::string& filename, std::string& error)
{
  std::ifstream in(filename);
  if (!in.is_open()) {
    error = "cannot open file " + filename;
    return false;
  }
  std::map<int, float> late, early;
  std::string line;
  int line_no = 0;
  while (std::getline(in, line)) {
    line_no++;
    // Strip comments.
    const size_t hash = line.find('#');
    if (hash != std::string::npos) {
      line = line.substr(0, hash);
    }
    std::istringstream ss(line);
    int depth;
    float late_d;
    if (!(ss >> depth)) {
      continue;  // blank / comment-only line
    }
    if (!(ss >> late_d)) {
      error = "malformed line " + std::to_string(line_no) + ": expected "
              "'depth late_derate [early_derate]'";
      return false;
    }
    late[depth] = late_d;
    float early_d;
    if (ss >> early_d) {
      early[depth] = early_d;
    }
  }
  late_ = std::move(late);
  early_ = std::move(early);
  return true;
}

AocvPathResult aocvAdjustPathEnd(Sta* sta,
                                 PathEnd* path_end,
                                 const AocvDerateTable& table)
{
  AocvPathResult result;
  const Path* end_path = path_end->path();
  result.endpoint = sta->network()->pathName(end_path->pin(sta));
  result.flat_slack = delayAsFloat(path_end->slack(sta));
  result.aocv_slack = result.flat_slack;

  Search* search = sta->search();
  Graph* graph = sta->graph();

  PathExpanded expanded(end_path, sta);
  const size_t size = expanded.size();
  // path(0) is the path root (clock source / startpoint side);
  // path(size-1) is the endpoint.

  int logic_depth = 0;
  // Sum of (aocv_derated - flat_derated) over data combinational cell arcs.
  // For setup, increasing data arrival hurts slack; AOCV late derate < flat
  // late derate reduces the derated delay, so delta is negative and slack
  // improves.
  float delay_delta = 0.0f;

  // First pass: count combinational logic stages on the data path.
  for (size_t i = 1; i < size; i++) {
    const Path* p = expanded.path(i);
    const TimingArc* arc = p->prevArc(sta);
    if (arc != nullptr && arc->role() == TimingRole::combinational()) {
      logic_depth++;
    }
  }
  result.logic_depth = logic_depth;

  const float aocv_late = table.lateDerate(logic_depth);
  result.late_derate = aocv_late;

  // No table loaded -> inactive, aocv_slack == flat_slack exactly.
  if (table.empty()) {
    return result;
  }

  // Second pass: re-scale the combinational cell-delay arcs of the data path.
  for (size_t i = 1; i < size; i++) {
    const Path* p = expanded.path(i);
    const Path* prev = p->prevPath();
    const TimingArc* arc = p->prevArc(sta);
    const Edge* edge = p->prevEdge(sta);
    if (arc == nullptr || edge == nullptr || prev == nullptr) {
      continue;
    }
    const TimingRole* role = arc->role();
    // Only adjust data-path combinational cell arcs (the logic stages).
    if (role != TimingRole::combinational()) {
      continue;
    }
    const Vertex* from_vertex = prev->vertex(sta);
    const MinMax* min_max = p->minMax(sta);
    const DcalcAPIndex dcalc_ap = p->dcalcAnalysisPtIndex(sta);
    const Sdc* sdc = p->sdc(sta);

    const ArcDelay flat_derated
        = search->deratedDelay(from_vertex, arc, edge, /*is_clk=*/false,
                               min_max, dcalc_ap, sdc);
    const ArcDelay raw = graph->arcDelay(edge, arc, dcalc_ap);
    const float flat_derated_f = delayAsFloat(flat_derated);
    const float raw_f = delayAsFloat(raw);
    // Replace the flat late derate with the depth-based AOCV late derate for
    // this arc. raw_f * flat_factor == flat_derated_f, so the new derated delay
    // is raw_f * aocv_late. (Handles per-cell flat derates because raw is the
    // un-derated arc delay.)
    const float aocv_derated_f = raw_f * aocv_late;
    delay_delta += aocv_derated_f - flat_derated_f;
  }

  // Setup: slack = required - data_arrival. data_arrival changes by delay_delta.
  result.aocv_slack = result.flat_slack - delay_delta;
  return result;
}

}  // namespace sta
