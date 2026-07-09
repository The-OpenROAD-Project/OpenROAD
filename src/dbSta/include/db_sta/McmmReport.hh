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
//
// ----------------------------------------------------------------------------
// Slice 2 -- the MODE dimension (additive, flag-gated, report-only)
// ----------------------------------------------------------------------------
// OpenSTA models mode x scene: a `Mode` (sta::Mode) holds an SDC, and each
// `Scene` (sta::Scene) belongs to exactly one Mode (Scene::mode()). The full
// list of modes/scenes is StaState::modes()/scenes(); multiMode() reports
// whether more than one mode is active. The user-facing setup surface already
// exists upstream (`set_mode`, `define_scene -mode`, `read_sdc -mode`,
// `get_modes`, `get_scenes -modes`). Slice 2 does NOT add any mode-setup
// command and makes NO src/sta edit -- it consumes the existing model.
//
// What slice 1 already gave us for free: its cross-scene path-end search runs
// over ALL scenes, and because scenes span modes that search ALREADY yields
// the true cross-MODE x CORNER worst per endpoint. What slice 2 surfaces:
//   1. the limiting MODE (scene->mode()->name()) of that worst, alongside the
//      limiting corner -- i.e. the true (mode, corner) pair, and
//   2. a PER-MODE breakdown: for each mode, the worst slack per endpoint over
//      just that mode's scenes (so a func vs test mode can be compared).
// All slice-2 output is gated behind report_mcmm_slack's -by_mode flag; with
// the flag off (default) the report is byte-identical to slice 1.

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
  std::string mode;    // mode the scene belongs to (slice 2)
  float slack = 0.0f;  // worst slack of this endpoint in this corner (seconds)
  bool valid = false;  // false if the endpoint has no constrained path here
};

// Per-endpoint cross-corner worst-slack result.
struct McmmEndpointResult
{
  std::string endpoint;
  // Worst (minimum) slack across all active corners (seconds). This matches
  // OpenSTA's own cross-corner minimum. With multiple modes active this is the
  // true cross-mode x corner minimum (the all-scenes search spans modes).
  float worst_slack = 0.0f;
  // Name of the corner (scene) that produced worst_slack (limiting corner).
  std::string worst_corner;
  // Name of the mode that produced worst_slack (limiting mode -- slice 2).
  std::string worst_mode;
  // Per-corner slacks in scene-index order (one entry per active corner).
  std::vector<McmmCornerSlack> corners;
};

// Worst slack of one endpoint within a single mode (over that mode's scenes).
struct McmmModeEndpointSlack
{
  std::string endpoint;
  float slack = 0.0f;        // worst slack across this mode's scenes (s)
  std::string worst_corner;  // limiting corner (scene) within this mode
  bool valid = false;
};

// Per-mode worst-slack breakdown (one section per active mode).
struct McmmModeResult
{
  std::string mode;                              // mode name
  std::vector<std::string> scenes;               // scene names in this mode
  std::vector<McmmModeEndpointSlack> endpoints;  // top-N within this mode
};

// Compute the per-endpoint cross-corner (and, with multiple modes, cross-mode)
// worst slack for the top critical endpoints.
//  max_endpoints : number of endpoint groups to enumerate (top-N), ordered
//                  by the cross-corner worst slack.
//  min_max       : MinMax::max() for setup, MinMax::min() for hold.
// Read-only: does not mutate the graph. The worst slack and limiting
// corner/mode are taken straight from OpenSTA's path-end search; the per-corner
// columns are filled from per-scene searches.
std::vector<McmmEndpointResult> computeMcmmSlack(dbSta* sta,
                                                 int max_endpoints,
                                                 const MinMax* min_max);

// Slice 2: per-mode worst-slack breakdown. For each active mode, runs a
// path-end search restricted to that mode's scenes and records the worst slack
// (and limiting corner within the mode) for the top-N endpoints. Read-only.
std::vector<McmmModeResult> computeMcmmSlackByMode(dbSta* sta,
                                                   int max_endpoints,
                                                   const MinMax* min_max);

// Compute + print the cross-corner worst-slack table to the report stream.
// When by_mode is true (slice 2), also prints a per-mode breakdown section and
// the limiting (mode, corner) pair. When false (default), output is identical
// to slice 1.
void reportMcmmSlack(dbSta* sta,
                     int max_endpoints,
                     const MinMax* min_max,
                     bool by_mode = false);

}  // namespace sta
