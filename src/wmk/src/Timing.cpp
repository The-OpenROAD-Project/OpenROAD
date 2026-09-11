// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include "Timing.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

#include "db_sta/dbSta.hh"
#include "sta/ClkNetwork.hh"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Search.hh"
#include "sta/Transition.hh"

namespace wmk {

bool isConstrainedSlack(float slack)
{
  return std::isfinite(slack) && slack > -sta::INF && slack < sta::INF;
}

namespace {

bool withinHeadroom(float value, float limit, double fraction)
{
  // Zero is a real constraint, not the absence of a limit.
  return std::isfinite(value) && std::isfinite(limit) && limit >= 0.0f
         && value <= (1.0 - fraction) * limit;
}

bool slewHasHeadroom(sta::dbSta* sta,
                     const sta::Pin* pin,
                     const sta::Scene* scene,
                     double fraction)
{
  const sta::Sdc* sdc = scene->sdc();
  if (sdc->isDisabledConstraint(pin)
      || scene->mode()->clkNetwork()->isIdealClock(pin)) {
    return true;
  }
  sta::Vertex* vertex = sta->graph()->pinDrvrVertex(pin);
  if (vertex == nullptr) {
    return false;
  }
  sta::ConstClockSet clocks;
  sta::VertexPathIterator paths(vertex, sta);
  while (paths.hasNext()) {
    const sta::Path* path = paths.next();
    if (path->scene(sta) == scene && path->clock(sta) != nullptr) {
      clocks.insert(path->clock(sta));
    }
  }

  const sta::MinMax* max = sta::MinMax::max();
  float base_limit;
  bool base_exists;
  // This public STA API resolves the design constraint, scene-specific
  // Liberty port limit and default_max_transition. Add clock constraints for
  // each edge, as STA's slew checker does for non-ideal instance outputs.
  sta->findSlewLimit(
      sta->network()->libertyPort(pin), scene, max, base_limit, base_exists);
  for (const sta::RiseFall* rf : sta::RiseFall::range()) {
    float limit = base_limit;
    bool exists = base_exists;
    for (const sta::Clock* clock : clocks) {
      float clock_limit;
      bool clock_exists;
      sdc->slewLimit(
          clock, rf, sta::PathClkOrData::data, max, clock_limit, clock_exists);
      if (clock_exists && (!exists || clock_limit < limit)) {
        limit = clock_limit;
        exists = true;
      }
    }
    const float slew = sta::delayAsFloat(
        sta->graph()->slew(vertex, rf, scene->dcalcAnalysisPtIndex(max)));
    if (exists && !withinHeadroom(slew, limit, fraction)) {
      return false;
    }
  }
  return true;
}

}  // namespace

bool driverHasHeadroom(sta::dbSta* sta,
                       const sta::Pin* pin,
                       double slew_fraction,
                       double capacitance_fraction)
{
  sta->checkFanoutPreamble();
  for (const sta::Mode* mode : sta->modes()) {
    float fanout, limit, slack;
    // STA resolves SDC/Liberty limits and weighted fanout loads for each mode.
    sta->checkFanout(pin, mode, sta::MinMax::max(), fanout, limit, slack);
    if (slack < 0.0f) {
      return false;
    }
  }
  sta->checkSlewsPreamble();
  sta->checkCapacitancesPreamble(sta->scenes());
  for (sta::Scene* scene : sta->scenes()) {
    if (!slewHasHeadroom(sta, pin, scene, slew_fraction)) {
      return false;
    }
    float cap, limit, slack;
    const sta::RiseFall* rf = nullptr;
    const sta::Scene* checked_scene = nullptr;
    // STA's capacitance limit/load is edge-independent, but differs by scene.
    sta->checkCapacitance(
        pin, {scene}, sta::MinMax::max(), cap, limit, slack, rf, checked_scene);
    if (checked_scene != nullptr
        && !withinHeadroom(cap, limit, capacitance_fraction)) {
      return false;
    }
  }
  return true;
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
