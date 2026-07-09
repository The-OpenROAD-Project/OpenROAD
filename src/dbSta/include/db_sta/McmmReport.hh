// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// MCMM -- Multi-Corner Multi-Mode cross-corner worst-slack reporting
// (additive, report-only diagnostic).
//
// Background
// ----------
// Real designs are signed off across multiple CORNERS (process / voltage /
// temperature + RC corners). In OpenSTA each corner is a "scene" (the class
// was renamed Corner -> Scene; `define_corners` / `read_liberty -corner`
// remain as the user-facing aliases). Sign-off requires the WORST slack
// across all active corners per endpoint, and -- to be actionable -- which
// corner is the limiting one.
//
// What OpenSTA already provides vs. what this adds
// ------------------------------------------------
// OpenSTA ALREADY computes cross-corner worst slack and per-corner worst
// slack end to end:
//   * Sta::worstSlack(min_max)              -- global worst across all scenes
//   * Sta::worstSlack(scene, min_max, ...)  -- per-scene worst
//   * Search::findPathEnds(scenes, ...)      -- path ends across given scenes
//   * Path::scene(sta)                       -- the scene a path belongs to
// and report_checks already prints "Corner: <name>" when multiScene().
//
// This module does NOT reimplement any of that. It SURFACES, in a single
// auditable table, the per-endpoint slack for EACH active corner side by
// side, the worst (minimum) slack across corners, and the name of the
// limiting corner. It uses only the public Search / Path / PathEnd API and
// does NOT mutate the timing graph or change report_checks / GBA results.
//
// This is the actionable MCMM output: "for endpoint X the worst setup slack
// is S, limited by corner C". It is built entirely in the OpenROAD (dbSta)
// layer; there is no src/sta edit.

#pragma once

#include <string>
#include <vector>

#include "sta/MinMax.hh"

namespace sta {

class dbSta;

// Worst slack of one endpoint in one corner (scene).
struct McmmCornerSlack
{
  std::string corner;  // corner / scene name
  float slack = 0.0f;  // worst slack of this endpoint in this corner (seconds)
  bool valid = false;  // false if the endpoint has no constrained path here
};

// Per-endpoint cross-corner worst-slack result.
struct McmmEndpointResult
{
  std::string endpoint;
  // Worst (minimum) slack across all active corners (seconds). This matches
  // OpenSTA's own cross-corner minimum.
  float worst_slack = 0.0f;
  // Name of the corner that produced worst_slack (the limiting corner).
  std::string worst_corner;
  // Per-corner slacks in scene-index order (one entry per active corner).
  std::vector<McmmCornerSlack> corners;
};

// Compute the per-endpoint cross-corner worst slack for the top critical
// endpoints.
//  max_endpoints : number of endpoint groups to enumerate (top-N), ordered
//                  by the cross-corner worst slack.
//  min_max       : MinMax::max() for setup, MinMax::min() for hold.
// Read-only: does not mutate the graph. The worst slack and limiting corner
// are taken straight from OpenSTA's path-end search; the per-corner columns
// are filled from per-scene searches.
std::vector<McmmEndpointResult> computeMcmmSlack(dbSta* sta,
                                                 int max_endpoints,
                                                 const MinMax* min_max);

// Compute + print the cross-corner worst-slack table to the report stream.
void reportMcmmSlack(dbSta* sta, int max_endpoints, const MinMax* min_max);

}  // namespace sta
