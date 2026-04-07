// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors
//
// Fmt formatter for sta::Delay (and aliases: ArcDelay, Slew, Arrival,
// Required, Slack) so they can be used with spdlog/fmt formatting.

#pragma once

#include "spdlog/fmt/fmt.h"  // IWYU pragma: keep
#include "sta/Delay.hh"

template <>
struct fmt::formatter<sta::Delay> : fmt::formatter<float>
{
  auto format(const sta::Delay& d, format_context& ctx) const
  {
    return fmt::formatter<float>::format(sta::delayAsFloat(d), ctx);
  }
};
