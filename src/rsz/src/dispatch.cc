// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "dispatch.hh"

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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

}  // namespace rsz
