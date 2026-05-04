// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace rsz {

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
// each token.  Empty input → empty vector; default substitution is the
// caller's responsibility.  Pure utility — no STA / OpenDB / logger access.
std::vector<PhaseStep> parsePhases(std::string_view phases);

}  // namespace rsz
