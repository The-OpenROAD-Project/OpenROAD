// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// CPPR / CRPR -- Clock Reconvergence Pessimism Removal reporting.
//
// This is an ADDITIVE, OpenROAD-side (dbSta) diagnostic. It does NOT change
// the default graph-based-analysis (GBA) results produced by report_checks,
// and it does NOT mutate the timing graph.
//
// Background
// ----------
// For a setup/hold check the launch and capture clock paths usually share a
// common clock-tree segment from the clock root down to a branch point. When
// on-chip-variation (OCV / AOCV / set_timing_derate) is applied, the derate
// is applied INDEPENDENTLY to the launch and the capture clock path -- but
// the shared segment is the SAME physical silicon, so deratng it on both
// sides DOUBLE-COUNTS the pessimism on the common path. CPPR (a.k.a. CRPR)
// credits that pessimism back.
//
// What OpenSTA already provides vs. what this adds
// ------------------------------------------------
// OpenSTA already implements CRPR end-to-end (search/Crpr.cc, CheckCrpr) and
// it is ON by default in OCV analysis (Variables::crpr_enabled_ == true). The
// authoritative credit for a check is therefore already available through the
// public PathEnd API:
//   * PathEnd::slackNoCrpr(sta) -- slack WITHOUT the common-path credit (the
//     pessimistic, double-counted number),
//   * PathEnd::slack(sta)       -- slack WITH the credit (the GBA result),
//   * PathEnd::checkCrpr(sta)    -- the signed CRPR delta folded into the
//     required time.
// This module does NOT reimplement CRPR. It SURFACES the raw-vs-adjusted
// split per check in one auditable table, and -- using only the public
// Path / PathExpanded API -- independently identifies the deepest shared
// clock pin (the common point) so the credit is attributable to a concrete
// branch point. The reported credit is taken from OpenSTA (slack -
// slackNoCrpr), so it always matches the engine.

#pragma once

#include <string>
#include <vector>

#include "sta/MinMax.hh"

namespace sta {

class dbSta;

// Per-check CPPR result. Times are in seconds (STA internal units).
struct CpprPathResult
{
  std::string endpoint;
  // Raw slack with the common clock-path pessimism still double-counted
  // (PathEnd::slackNoCrpr). This is the conservative / pessimistic slack.
  float raw_slack = 0.0f;
  // CPPR-adjusted slack with the common-path credit applied
  // (PathEnd::slack -- the default GBA result).
  float cppr_slack = 0.0f;
  // Pessimism credited back on the common clock path:
  //   credit = cppr_slack - raw_slack, always >= 0 (CPPR only relaxes).
  float credit = 0.0f;
  // Name of the deepest shared clock pin between the launch and capture
  // clock paths (the common point), or "" if none was identified.
  std::string common_pin;
};

// Compute the CPPR raw-vs-adjusted slack split for the top-N critical checks.
//  max_paths : number of endpoint groups to enumerate (top-N).
//  min_max   : MinMax::max() for setup, MinMax::min() for hold.
// Ordering follows the GBA slack ordering. Read-only: does not mutate the
// graph and does not change crpr_enabled.
std::vector<CpprPathResult> computeCpprSlack(dbSta* sta,
                                             int max_paths,
                                             const MinMax* min_max);

// Compute + print a per-check report table to the STA report stream.
void reportCppr(dbSta* sta, int max_paths, const MinMax* min_max);

}  // namespace sta
