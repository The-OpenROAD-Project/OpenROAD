// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <vector>

namespace wmk {

struct RoutingStat;

// Evaluate canonical wrong-way fractions, independently of the database and
// key selection. Values are in [0, 1], counts fit in int, and trials is
// positive. Sorting the groups makes both the observation and null independent
// of net iteration order. Empty groups return no evidence.
RoutingStat routingStatistics(std::vector<double> marked,
                              std::vector<double> rest,
                              std::uint64_t seed,
                              int trials);

}  // namespace wmk
