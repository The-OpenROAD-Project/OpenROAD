// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Incremental STA -- prove that after a bounded netlist edit (resize / cell
// swap / buffer insert) only the AFFECTED fanout cone needs re-evaluation,
// and surface the resulting slacks so they can be checked against a full
// from-scratch STA re-run (additive, report-only diagnostic).
//
// What OpenSTA already provides vs. what this adds
// ------------------------------------------------
// OpenSTA ALREADY does incremental, lazy, levelized timing update end to end.
// When a cell is swapped/resized or a pin is connected/disconnected, the
// dbSta ODB callbacks call the Sta edit hooks (replaceCellBefore/After,
// connectPinAfter, disconnectPinBefore, ...). Those hooks mark ONLY the
// affected vertices invalid (Search::arrivalInvalid / requiredInvalid,
// GraphDelayCalc::delayInvalid) into the invalid_arrivals_ / invalid_delays_
// / invalid_requireds_ sets. The next timing query (worstSlack /
// totalNegativeSlack / slack / report_checks) runs a levelized BFS that only
// recomputes the changed cone:
//   * GraphDelayCalc::findDelays  -- forward BFS from invalid delays, stops
//     where load slews stop changing
//   * Search::findArrivals1       -- forward BFS from invalid arrivals, stops
//     where arrivals stop changing
//   * Search::findRequireds       -- backward BFS, only re-seeds endpoints
//     with changed arrivals
// dbSta inherits all of this; the resizer ECO/repair loop already benefits
// from it implicitly.
//
// This module does NOT reimplement timing propagation and does NOT change the
// default full-STA / report_checks path. It is purely additive. It:
//   (1) identifies the AFFECTED fanout cone of a bounded set of edited
//       instances by walking the timing graph forward from their pins,
//   (2) reports how many endpoints fall in that cone vs. the full endpoint
//       count (the "incremental work" proof), and
//   (3) reports the post-edit slacks (WNS/TNS + per-endpoint), taken straight
//       from OpenSTA's own incrementally-updated query API, so a caller/test
//       can assert they are bit-for-bit identical to a full from-scratch STA
//       re-run after the same edits (the correctness oracle).
//
// It is built entirely in the OpenROAD (dbSta) layer; there is no src/sta
// edit.

#pragma once

#include <string>
#include <vector>

#include "sta/MinMax.hh"

namespace sta {

class dbSta;

// Worst slack of one endpoint after the edit (post-incremental-update).
struct IncrementalEndpointSlack
{
  std::string endpoint;  // endpoint pin path
  float slack = 0.0f;    // worst slack at this endpoint (seconds)
  bool in_cone = false;  // true if endpoint is in the affected fanout cone
};

// Result of an incremental-update analysis after editing `seed` instances.
struct IncrementalStaResult
{
  // The set of edited instances that seeded the analysis.
  std::vector<std::string> seed_instances;

  // Affected-cone accounting (the "only the cone was re-evaluated" proof).
  int affected_vertices = 0;   // graph vertices in the forward cone
  int affected_endpoints = 0;  // timing endpoints in the forward cone
  int total_endpoints = 0;     // total timing endpoints in the design

  // Post-edit design timing, taken from OpenSTA's incremental query API.
  float wns = 0.0f;  // worst negative slack (worst endpoint slack)
  float tns = 0.0f;  // total negative slack

  // Per-endpoint worst slack (all endpoints), with in_cone flag.
  std::vector<IncrementalEndpointSlack> endpoints;
};

// Compute the affected fanout cone of `seed_insts` and the post-edit timing.
//
//   seed_insts : instance path names that were edited (resized / swapped /
//                rebuffered). The affected cone is the transitive forward
//                fanout of all output (driver) pins of these instances.
//   min_max    : MinMax::max() for setup, MinMax::min() for hold.
//
// This is read-only: it does NOT mutate the graph and does NOT call
// delaysInvalid. The slacks are whatever OpenSTA's incremental engine has
// already computed (the caller is expected to have applied the edits first,
// which marked the cone invalid; querying slack triggers the incremental
// recompute). The affected-cone set is derived purely by graph traversal so
// it can be compared against the full endpoint count.
IncrementalStaResult computeIncrementalSta(
    dbSta* sta,
    const std::vector<std::string>& seed_insts,
    const MinMax* min_max);

// Compute + print the incremental-update report to the report stream.
void reportIncrementalSta(dbSta* sta,
                          const std::vector<std::string>& seed_insts,
                          const MinMax* min_max);

}  // namespace sta
