// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "BottomUpBuffering.h"

namespace cts {

struct BufferingCellChoice
{
  BufferingChoice buffer;
  double nominal_input_cap = 0;
  double area = 0;
  // One entry per scene and transition, in a stable caller-defined order.
  std::vector<double> delays;
};

struct BufferingSearchModel
{
  double wire_cap = 0;
  double wire_res = 0;
  double max_wire_length = 0;
  size_t dimensions = 2;
  size_t candidate_limit = 16;
};

struct BufferingPlan
{
  // Empty master means a wire-only junction, not a zero-capacitance buffer.
  // Edges through junctions are retained for RC and arrival propagation.
  std::vector<BufferingNode> nodes;
  std::vector<std::pair<double, double>> arrivals;
  size_t depth = 0;
  size_t buffers = 0;
  double area = 0;
  size_t peak_candidates = 0;
  bool reference = false;

  double delay() const;
  double skew() const;
};

using BufferingChoices = std::function<std::vector<BufferingCellChoice>(
    const BufferingLoad&,
    const std::string& fixed_master)>;
using BufferingDummy
    = std::function<std::optional<BufferingCellChoice>(float actual_pin_cap,
                                                       float ideal_pin_cap,
                                                       size_t fanout)>;

// Evaluate a complete plan, including the caller's unchanged dummy policy.
// Infeasible plans return nullopt. Invalid geometry/model data throws.
std::optional<BufferingPlan> evaluateBuffering(
    const std::vector<BufferingNode>& nodes,
    size_t root,
    const BufferingSearchModel& model,
    const BufferingChoices& choices,
    const BufferingDummy& dummy = {});

// Bounded heuristic, not a global optimizer. The independently evaluated
// reference is outside the frontier limit and cannot be pruned. An
// independently feasible complete input plan is also retained, subject to the
// same safeguard: a result must not worsen any reference scene/transition's
// latest arrival or spread.
BufferingPlan searchBuffering(const std::vector<BufferingNode>& sites,
                              size_t root,
                              const BufferingSearchModel& model,
                              const BufferingChoices& choices,
                              const BufferingDummy& dummy = {},
                              const std::optional<BufferingPlan>& reference
                              = {});

}  // namespace cts
