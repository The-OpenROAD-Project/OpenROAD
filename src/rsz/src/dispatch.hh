// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rsz {

class MoveCommitter;
class OptPolicy;
class Resizer;
class SetupLegacyPolicy;

// One token from the `phases` argument of repair_timing.
//
// `phases` is the token list driving the sequencer.  Each token names either
// a legacy repair phase (LEGACY, WNS_PATH, ...) or a top-level policy run as
// a single phase (LEGACY_MT, MT1, ...).  Conceptually: phases = policy list.
struct PhaseStep
{
  std::string name;
  // Single-character marker used in log prefixes (e.g. "LEGACY{} Phase: ...").
  // Assigned by parsePhases() based on the position in the token list so that
  // identical token names at different positions get distinct markers.
  char marker{'?'};
};

// Whitespace-tokenize `phases` and assign a per-position marker character to
// each token.  Empty input -> empty vector; default substitution is the
// caller's responsibility.  Pure utility  -  no STA / OpenDB / logger access.
std::vector<PhaseStep> parsePhases(std::string_view phases);

// Cross-phase state shared by the sequencer with each phase invocation.
//
// All members (except `step`) are sequencer-owned references  -  phases mutate
// these through `ctx` so accumulators survive across phase boundaries.
struct PhaseRunContext
{
  int& opto_iteration;
  float& initial_tns;
  float& prev_tns;
  int& num_viols;

  // Per-step descriptor (name + dynamic marker char).
  PhaseStep step;
};

// Optional log labels that the sequencer prints around a phase's iterate()
// call.  `phase_summary` is the legacy "Phase: <name>" prefix; `profiler` is
// the matching debug::ScopedDebugProfile label.  Both are passed through
// std::string_view for zero-copy literal use.
struct PhaseSummaryLabels
{
  std::string_view phase_summary;
  std::string_view profiler;
};

// Cross-cutting hooks the sequencer applies around a phase's iterate() call.
// `capture_pre_slack` requests pre-iteration TNS sampling for accumulator
// updates; `summary` opts in to log/profiler scopes.  Empty defaults mean the
// phase opts out of all sequencer-side hooks (e.g. LEGACY).
struct PhaseHooks
{
  bool capture_pre_slack = false;
  std::optional<PhaseSummaryLabels> summary;
};

// Build the OptPolicy implementation that handles `token`.  Phase tokens
// (LEGACY, WNS/WNS_PATH/WNS_CONE, TNS, ENDPOINT_FANIN, STARTPOINT_FANOUT,
// LAST_GASP) are dispatched to PhasePolicies.cc wrappers; top-level policy
// tokens (LEGACY_MT, MT1, MEASURED_VT_SWAP) wire directly to their existing
// OptPolicy classes.  Returns nullptr for unknown tokens  -  caller is
// responsible for the user-facing error message.
//
// `legacy_parent` is required for phase tokens (which delegate back to
// SetupLegacyPolicy helpers) and ignored for top-level tokens.  Until Step 8
// renames SetupLegacyPolicy -> LegacyRepairContext, callers pass `this` from
// SetupLegacyPolicy::runSetup().
std::unique_ptr<OptPolicy> makeOptPolicyByToken(
    std::string_view token,
    Resizer& resizer,
    MoveCommitter& committer,
    SetupLegacyPolicy* legacy_parent);

// Stable list of all token names accepted by makeOptPolicyByToken().  Used
// for help text and unknown-token error messages.
const std::vector<std::string_view>& knownOptPolicyTokens();

}  // namespace rsz
