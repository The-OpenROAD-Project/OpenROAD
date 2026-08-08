// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Unified pessimism-recovery closure report (additive, report-only).
//
// This is an ADDITIVE, OpenROAD-side (dbSta) diagnostic. It does NOT change
// the default graph-based-analysis (GBA) results produced by report_checks,
// and it does NOT mutate the timing graph.
//
// Motivation
// ----------
// Two independent pessimism-recovery surfaces already exist in this fork:
//   * PBA  (PbaReport)  -- path-based gate-slew pessimism recovery; recovers
//                          the gate-stage delay over-estimate the worst-case
//                          graph slew bakes into GBA.
//   * CPPR (CpprReport) -- clock-reconvergence pessimism removal; credits
//                          back the double-counted derate on the shared
//                          launch/capture clock segment.
// Each classifies endpoints separately. A user closing a design wants ONE
// verdict: for every endpoint that is failing under the most pessimistic
// view, does it STILL fail after BOTH recoveries are applied (a GENUINE
// violation to fix), or does some recovery mechanism clear it (an ARTIFACT)?
//
// Composition (NOT reimplementation)
// ----------------------------------
// We COMPOSE the existing computePbaEndpoints() and computeCpprEndpoints()
// results, joined per endpoint. The pessimism stack, from most to least
// pessimistic, is:
//
//   raw_slack            = PathEnd::slackNoCrpr  (CpprEndpointResult.raw_slack)
//     + cppr_credit      = CPPR common-path credit (>= 0)
//   = gba_slack          = PathEnd::slack = report_checks slack
//                          (== CpprEndpointResult.cppr_slack
//                           == PbaEndpointResult.gba_slack)
//     + pba_recovered    = PBA gate-slew pessimism recovery (>= 0)
//   = recovered_slack    = PbaEndpointResult.pba_slack
//
// So the fully-recovered slack is exactly the PBA slack (PBA already builds
// on the CPPR-adjusted GBA slack), and the combined recovery over the raw
// baseline is cppr_credit + pba_recovered. Both terms are >= 0 by the
// construction proved in the PBA and CPPR modules, so the recovery is
// monotone: recovered_slack >= raw_slack.
//
// Classification (keyed off the RAW pessimistic baseline)
// -------------------------------------------------------
//   raw_violated   : raw_slack < 0  (failing under the pessimistic view)
//   genuine        : recovered_slack < 0  (still failing after BOTH
//                    recoveries -- the real violation to fix)
//   artifact       : raw_violated && !genuine  (cleared by some recovery),
//                    labeled with WHICH mechanism cleared it:
//                      CPPR  -- CPPR credit alone lifts it to >= 0,
//                      PBA   -- PBA recovery alone lifts it to >= 0,
//                      BOTH  -- neither alone suffices but together they do.

#pragma once

#include <string>
#include <vector>

#include "sta/MinMax.hh"

namespace sta {

class dbSta;

// Which recovery mechanism cleared a raw-failing endpoint (for artifacts).
enum class ClearedBy
{
  kNone,  // not an artifact (either not raw-failing, or still genuine)
  kCppr,  // CPPR common-path credit alone lifts raw_slack to >= 0
  kPba,   // PBA gate-slew recovery alone lifts raw_slack to >= 0
  kBoth   // neither mechanism alone suffices; both together clear it
};

// Human-readable mechanism label for ClearedBy (stable, used in reports and
// the machine-readable test surface). "-" for kNone.
const char* clearedByLabel(ClearedBy cleared_by);

// Per-endpoint unified closure result. Times are in seconds (STA internal
// units). Composed from the PBA and CPPR endpoint results.
struct ClosureEndpointResult
{
  std::string endpoint;
  // Most pessimistic baseline: pre-CPPR, pre-PBA (PathEnd::slackNoCrpr).
  float raw_slack = 0.0f;
  // GBA slack as reported by report_checks (CPPR applied, no PBA):
  //   gba_slack = raw_slack + cppr_credit.
  float gba_slack = 0.0f;
  // Fully recovered slack after BOTH CPPR and PBA:
  //   recovered_slack = gba_slack + pba_recovered = raw_slack + total recovery.
  float recovered_slack = 0.0f;
  // CPPR common-path credit (>= 0).
  float cppr_credit = 0.0f;
  // PBA gate-slew pessimism recovery (>= 0).
  float pba_recovered = 0.0f;
  // Deepest shared clock pin the CPPR credit is attributed to (or "").
  std::string common_pin;
  // Failing under the raw pessimistic baseline (raw_slack < 0).
  bool raw_violated = false;
  // GENUINE violation: still failing after BOTH recoveries
  // (recovered_slack < 0).
  bool genuine = false;
  // ARTIFACT: raw-failing but cleared by some recovery (raw_violated &&
  // !genuine).
  bool artifact = false;
  // For artifacts, which mechanism cleared it. kNone otherwise.
  ClearedBy cleared_by = ClearedBy::kNone;
};

// Summary counters for the unified closure verdict.
struct ClosureSummary
{
  int endpoints = 0;        // endpoints analyzed
  int raw_failing = 0;      // failing under the raw pessimistic baseline
  int cleared_by_cppr = 0;  // artifacts cleared by CPPR alone
  int cleared_by_pba = 0;   // artifacts cleared by PBA alone
  int cleared_by_both = 0;  // artifacts cleared only by CPPR+PBA together
  int genuine = 0;          // still failing after both recoveries
};

// Compute the unified per-endpoint closure classification by COMPOSING the
// existing PBA and CPPR endpoint computations (it calls computePbaEndpoints
// and computeCpprEndpoints internally and joins them by endpoint name).
//  max_paths : number of endpoint groups to enumerate (top-N), forwarded to
//              both underlying computations.
//  min_max   : MinMax::max() for setup, MinMax::min() for hold.
// Read-only: does not mutate the graph and does not change crpr_enabled.
std::vector<ClosureEndpointResult> computeClosure(dbSta* sta,
                                                  int max_paths,
                                                  const MinMax* min_max);

// Summarize endpoint results into the unified closure counters.
ClosureSummary summarizeClosure(
    const std::vector<ClosureEndpointResult>& endpoints);

// Unified closure decision surface: classify every raw-failing endpoint into
// a CPPR/PBA/BOTH artifact (cleared by pessimism recovery) vs a genuine
// post-recovery violation, print the combined recovered slack per endpoint
// and a one-line verdict summary.
//  only_violations : when true (default) list only the genuine violations;
//                    when false also list the cleared artifacts (with the
//                    mechanism label).
void reportClosure(dbSta* sta,
                   int max_paths,
                   const MinMax* min_max,
                   bool only_violations);

}  // namespace sta
