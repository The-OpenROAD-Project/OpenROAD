// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Path-Based Analysis (PBA) pessimism-recovery reporting.
//
// This is an ADDITIVE, OpenROAD-side (dbSta) diagnostic. It does NOT change
// the default graph-based-analysis (GBA) results produced by report_checks.
// For the top-N GBA critical paths it re-derives the gate-stage delays using
// the path-specific slew at each stage (driving the EXISTING OpenSTA arc
// delay calculator with the incoming slew from the previous arc ON THIS
// PATH, rather than the worst-case graph slew) and reports GBA slack vs PBA
// slack (the recovered pessimism) per path.
//
// Both SETUP (max) and HOLD (min) checks are supported:
//  - setup: path-specific slew <= worst-at-pin slew, so the recomputed gate
//    delay is <= the GBA gate delay; smaller data delay -> earlier arrival
//    -> larger setup slack. recovered = sum(GBA_gd - PBA_gd) >= 0.
//  - hold: GBA holds the MINIMUM gate delay (best/fastest slew); the
//    path-specific gate delay is >= that minimum; larger data delay ->
//    later arrival -> larger hold slack. recovered = sum(PBA_gd - GBA_gd)
//    >= 0. In both cases pba_slack = gba_slack + recovered >= gba_slack.
//
// On top of the per-path view it aggregates per-endpoint recovery and a
// closure decision surface that separates genuine post-PBA violations from
// GBA-pessimism artifacts (endpoints that move negative -> positive).
//

#pragma once

#include <string>
#include <vector>

#include "sta/MinMax.hh"

namespace sta {

class dbSta;

// Per-path PBA result. Times are in seconds (STA internal units).
struct PbaPathResult
{
  std::string endpoint;
  float gba_slack = 0.0f;
  float pba_slack = 0.0f;
  // Recovered pessimism (pba_slack - gba_slack), always >= 0 by construction.
  float recovered = 0.0f;
  // Number of gate stages that were re-evaluated with path-specific slew.
  int gate_stages = 0;
};

// Per-endpoint aggregation of PBA recovery. For the slice we enumerate one
// path per endpoint, so this is a 1:1 view of PbaPathResult enriched with
// the negative->positive transition flags used by the closure decision.
struct PbaEndpointResult
{
  std::string endpoint;
  float gba_slack = 0.0f;
  float pba_slack = 0.0f;
  float recovered = 0.0f;
  int gate_stages = 0;
  // GBA reports the endpoint as failing (slack < 0).
  bool gba_violated = false;
  // PBA still reports the endpoint as failing after recovery (slack < 0):
  // this is a GENUINE violation, not a GBA-pessimism artifact.
  bool pba_violated = false;
  // GBA-failing but PBA-passing: the negative slack was GBA pessimism.
  bool recovered_to_positive = false;
};

// Summary counters for the closure decision surface.
struct PbaClosureSummary
{
  int endpoints = 0;            // endpoints analyzed
  int gba_violations = 0;       // endpoints failing under GBA
  int pba_violations = 0;       // endpoints still failing after PBA (genuine)
  int recovered_endpoints = 0;  // GBA-failing that PBA recovered to passing
};

// Compute PBA pessimism recovery for the top-N critical paths.
//  max_paths   : number of endpoint groups to enumerate (top-N).
//  min_max     : MinMax::max() for setup, MinMax::min() for hold.
// The returned vector preserves the GBA slack ordering.
std::vector<PbaPathResult> computePbaSlack(dbSta* sta,
                                           int max_paths,
                                           const MinMax* min_max);

// Per-endpoint aggregation (setup or hold) with closure flags populated.
std::vector<PbaEndpointResult> computePbaEndpoints(dbSta* sta,
                                                   int max_paths,
                                                   const MinMax* min_max);

// Summarize endpoint results into closure counters.
PbaClosureSummary summarizeClosure(
    const std::vector<PbaEndpointResult>& endpoints);

// Compute + print a per-path report table to the STA report stream.
void reportPbaSlack(dbSta* sta, int max_paths, const MinMax* min_max);

// Compute + print a per-endpoint report plus the negative->positive summary.
void reportPbaEndpoints(dbSta* sta, int max_paths, const MinMax* min_max);

// Closure decision surface: print only the endpoints that are still failing
// after PBA recovery (the genuine violations) plus a one-line summary that
// separates genuine violations from GBA-pessimism artifacts.
//  only_violations : when true (default) list only PBA-failing endpoints.
void reportPbaClosure(dbSta* sta,
                      int max_paths,
                      const MinMax* min_max,
                      bool only_violations);

}  // namespace sta
