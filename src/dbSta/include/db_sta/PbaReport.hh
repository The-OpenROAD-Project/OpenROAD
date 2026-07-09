// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Path-Based Analysis (PBA) pessimism-recovery reporting (first slice).
//
// This is an ADDITIVE, OpenROAD-side (dbSta) diagnostic. It does NOT change
// the default graph-based-analysis (GBA) results produced by report_checks.
// For the top-N GBA critical paths it re-derives the gate-stage delays using
// the path-specific slew at each stage (driving the EXISTING OpenSTA arc
// delay calculator with the incoming slew from the previous arc ON THIS
// PATH, rather than the worst-case graph slew) and reports GBA slack vs PBA
// slack (the recovered pessimism) per path.
//
// See PBA_INVESTIGATION.md for the API trace and the PBA >= GBA argument.

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

// Compute PBA pessimism recovery for the top-N critical paths.
//  max_paths   : number of endpoint groups to enumerate (top-N).
//  min_max     : MinMax::max() for setup, MinMax::min() for hold.
// The returned vector preserves the GBA slack ordering.
std::vector<PbaPathResult> computePbaSlack(dbSta* sta,
                                           int max_paths,
                                           const MinMax* min_max);

// Compute + print a report table to the STA report stream.
void reportPbaSlack(dbSta* sta, int max_paths, const MinMax* min_max);

}  // namespace sta
