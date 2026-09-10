// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

#include <cstddef>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

namespace sta {
class dbSta;
class Scene;
class Vertex;
class RiseFall;
class MinMax;
}  // namespace sta

namespace wmk {

// STA uses a finite sentinel for unconstrained slack.
bool isConstrainedSlack(float slack);

// Clock indices are allocated independently by each mode's SDC. Keep the
// mode index with the clock index for both membership and timing availability.
using ClockIdentity = std::pair<size_t, int>;
using ClockIdentities = std::vector<ClockIdentity>;

// Clock identity, scene index, source edge. Separating source edges avoids
// counting the clock's duty cycle as skew.
using ClockSkewKey = std::tuple<ClockIdentity, size_t, size_t>;
using ClockSkews = std::map<ClockSkewKey, float>;

// Propagated clock latency spread at sequential clock pins, in seconds.
// Each clock, scene and source edge has its own nonnegative measurement.
ClockSkews clockSkews(sta::dbSta* sta);
bool haveClockSkews(const ClockIdentities& clocks, const ClockSkews& skews);
bool clockSkewsWithin(const ClockSkews& before,
                      const ClockSkews& after,
                      float margin);

// A fixed baseline for every constrained endpoint, scene, transition and
// setup/hold check. Placement may change unselected cells during legalization.
struct EndpointSlack
{
  sta::Vertex* vertex;
  sta::Scene* scene;
  const sta::RiseFall* transition;
  const sta::MinMax* min_max;
  float slack;
};

std::vector<EndpointSlack> endpointSlacks(sta::dbSta* sta);
bool endpointSlacksWithin(sta::dbSta* sta,
                          const std::vector<EndpointSlack>& before,
                          float margin);

}  // namespace wmk
