// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include "Timing.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

#include "db_sta/dbSta.hh"
#include "sta/Clock.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Path.hh"
#include "sta/Scene.hh"
#include "sta/Search.hh"
#include "sta/Transition.hh"

namespace wmk {

bool isConstrainedSlack(float slack)
{
  return std::isfinite(slack) && slack > -sta::INF && slack < sta::INF;
}

ClockSkews clockSkews(sta::dbSta* sta)
{
  sta->ensureClkArrivals();
  struct Latencies
  {
    float earliest = std::numeric_limits<float>::max();
    float latest = std::numeric_limits<float>::lowest();
  };
  std::map<ClockSkewKey, Latencies> latencies;
  for (sta::Vertex* vertex : sta->graph()->regClkVertices()) {
    sta::VertexPathIterator paths(vertex, sta);
    while (paths.hasNext()) {
      const sta::Path* path = paths.next();
      const sta::Clock* clock = path->clock(sta);
      if (!path->isClock(sta) || clock == nullptr || !clock->isPropagated()) {
        continue;
      }
      const sta::ClockEdge* edge = path->clkEdge(sta);
      const sta::Scene* scene = path->scene(sta);
      const ClockSkewKey key{{scene->mode()->modeIndex(), clock->index()},
                             scene->index(),
                             edge->transition()->index()};
      const float latency = path->arrival() - edge->time();
      if (!isConstrainedSlack(latency)) {
        return {};
      }
      auto& range = latencies[key];
      range.earliest = std::min(range.earliest, latency);
      range.latest = std::max(range.latest, latency);
    }
  }
  ClockSkews skews;
  for (const auto& [key, range] : latencies) {
    skews.emplace(key, range.latest - range.earliest);
  }
  return skews;
}

bool haveClockSkews(const ClockIdentities& clocks, const ClockSkews& skews)
{
  return !clocks.empty()
         && std::ranges::all_of(clocks, [&](const ClockIdentity& clock) {
              return std::ranges::any_of(skews, [&](const auto& entry) {
                return std::get<0>(entry.first) == clock;
              });
            });
}

bool clockSkewsWithin(const ClockSkews& before,
                      const ClockSkews& after,
                      float margin)
{
  return !before.empty() && std::ranges::all_of(before, [&](const auto& entry) {
    const auto it = after.find(entry.first);
    return it != after.end() && std::isfinite(it->second)
           && it->second <= entry.second + margin;
  });
}

std::vector<EndpointSlack> endpointSlacks(sta::dbSta* sta)
{
  sta->findRequireds();
  std::vector<EndpointSlack> result;
  for (sta::Vertex* vertex : sta->search()->endpoints()) {
    for (sta::Scene* scene : sta->scenes()) {
      for (const sta::MinMax* min_max : sta::MinMax::range()) {
        for (const sta::RiseFall* transition : sta::RiseFall::range()) {
          const float slack = sta->slack(
              vertex, transition->asRiseFallBoth(), {scene}, min_max);
          if (isConstrainedSlack(slack)) {
            result.push_back({.vertex = vertex,
                              .scene = scene,
                              .transition = transition,
                              .min_max = min_max,
                              .slack = slack});
          }
        }
      }
    }
  }
  return result;
}

bool endpointSlacksWithin(sta::dbSta* sta,
                          const std::vector<EndpointSlack>& before,
                          float margin)
{
  return std::ranges::all_of(before, [&](const EndpointSlack& entry) {
    const float slack = sta->slack(entry.vertex,
                                   entry.transition->asRiseFallBoth(),
                                   {entry.scene},
                                   entry.min_max);
    return isConstrainedSlack(slack) && slack >= entry.slack - margin;
  });
}

}  // namespace wmk
