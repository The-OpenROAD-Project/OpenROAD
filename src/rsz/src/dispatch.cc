// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "dispatch.hh"

#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "MeasuredVtSwapPolicy.hh"
#include "OptPolicy.hh"
#include "PhasePolicies.hh"
#include "SetupLegacyMtPolicy.hh"
#include "SetupMt1Policy.hh"

namespace rsz {

namespace {

// Single-character marker used in phase log prefixes.
// Order: 8 special chars, 26 lowercase, 26 uppercase, then '?' fallback.
// Identical to the original SetupLegacyPolicy::runSetup lambda so byte-equal
// regression on existing golden logs is preserved.
char getPhaseMarker(int phase_index)
{
  constexpr char special_markers[] = "*+^&@!-=";
  constexpr int num_special = 8;
  if (phase_index < num_special) {
    return special_markers[phase_index];
  }
  phase_index -= num_special;
  if (phase_index < 26) {
    return 'a' + phase_index;
  }
  phase_index -= 26;
  if (phase_index < 26) {
    return 'A' + phase_index;
  }
  return '?';
}

}  // namespace

std::vector<PhaseStep> parsePhases(const std::string_view phases)
{
  std::vector<PhaseStep> steps;
  // istringstream takes string; copy to a local std::string.
  std::istringstream stream{std::string(phases)};
  std::string token;
  int phase_index = 0;
  while (stream >> token) {
    steps.push_back({.name = token, .marker = getPhaseMarker(phase_index)});
    ++phase_index;
  }
  return steps;
}

std::unique_ptr<OptPolicy> makeOptPolicyByToken(
    const std::string_view token,
    Resizer& resizer,
    MoveCommitter& committer,
    SetupLegacyPolicy* const legacy_parent)
{
  // Phase tokens  -  wrappers in PhasePolicies.cc that delegate back to the
  // legacy host (SetupLegacyPolicy) for the actual repair work.
  // LEGACY and LEGACY_MT both dispatch to the same wrapper; the actual
  // single-thread vs MT behavior is determined by the legacy_parent class
  // (SetupLegacyPolicy vs SetupLegacyMtPolicy) selected by the sequencer
  // before dispatch starts.
  if (token == "LEGACY" || token == "LEGACY_MT") {
    return std::make_unique<MainRepairPhasePolicy>(
        resizer, committer, legacy_parent);
  }
  if (token == "WNS" || token == "WNS_PATH") {
    return std::make_unique<WnsPhasePolicy>(
        resizer, committer, legacy_parent, /*use_cone=*/false);
  }
  if (token == "WNS_CONE") {
    return std::make_unique<WnsPhasePolicy>(
        resizer, committer, legacy_parent, /*use_cone=*/true);
  }
  if (token == "TNS") {
    return std::make_unique<TnsPhasePolicy>(resizer, committer, legacy_parent);
  }
  if (token == "ENDPOINT_FANIN") {
    return std::make_unique<DirectionalPhasePolicy>(
        resizer, committer, legacy_parent, /*use_starts=*/false);
  }
  if (token == "STARTPOINT_FANOUT") {
    return std::make_unique<DirectionalPhasePolicy>(
        resizer, committer, legacy_parent, /*use_starts=*/true);
  }
  if (token == "LAST_GASP") {
    return std::make_unique<LastGaspPhasePolicy>(
        resizer, committer, legacy_parent);
  }
  // Self-contained top-level policy tokens.  LEGACY_MT is dispatched above
  // alongside LEGACY (both -> MainRepairPhasePolicy); the actual MT
  // behavior is determined by the legacy_parent class chosen by the
  // sequencer when it sees LEGACY_MT in the token list.
  if (token == "MT1") {
    return std::make_unique<SetupMt1Policy>(resizer, committer);
  }
  if (token == "MEASURED_VT_SWAP") {
    return std::make_unique<MeasuredVtSwapPolicy>(resizer, committer);
  }
  return nullptr;
}

const std::vector<std::string_view>& knownOptPolicyTokens()
{
  // Order kept stable for deterministic help / error output.
  static const std::vector<std::string_view> kTokens{
      "LEGACY",
      "LEGACY_MT",
      "WNS",
      "WNS_PATH",
      "WNS_CONE",
      "TNS",
      "ENDPOINT_FANIN",
      "STARTPOINT_FANOUT",
      "LAST_GASP",
      "MT1",
      "MEASURED_VT_SWAP",
  };
  return kTokens;
}

}  // namespace rsz
