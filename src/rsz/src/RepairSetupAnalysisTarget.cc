// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RepairSetupAnalysisTarget.hh"

#include <optional>

#include "RepairSetupProfiler.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"

namespace rsz {

RepairSetupAnalysisTarget::RepairSetupAnalysisTarget(Resizer& resizer,
                                                     const Target& source)
    : resizer_(resizer), source_(source)
{
  if (RepairSetupProfiler::analysis() && source.endpoint_pin != nullptr) {
    observed_target_ = source;
  }
}

bool RepairSetupAnalysisTarget::refresh()
{
  if (!observed_target_) {
    return true;
  }

  const ProfileScope scope("analysis_observation");
  const sta::StaState* sta = resizer_.staState();
  const sta::MinMax* max = resizer_.maxAnalysisMode();
  std::optional<Target> selected;
  sta::Slack selected_slack = sta::MinMax::min()->initValue();
  sta::Vertex* endpoint = resizer_.graph()->pinLoadVertex(source_.endpoint_pin);

  // Resolve every path field from one compatible, current unfiltered chain.
  if (endpoint != nullptr) {
    sta::VertexPathIterator paths(endpoint, sta);
    while (paths.hasNext()) {
      const sta::Path* path = paths.next();
      if (path->minMax(sta) != max || path->scene(sta) != source_.scene
          || sta::delayInf(path->required(), sta)) {
        continue;
      }

      sta::PathExpanded expanded(path, sta);
      visitPathSegments(
          path,
          expanded,
          sta,
          [&](const sta::Path* segment, sta::PathExpanded& chain) {
            for (size_t index = chain.startIndex(); index < chain.size();
                 ++index) {
              const sta::Path* driver = chain.path(index);
              if (driver->pin(sta) != source_.driver_pin
                  || driver->transition(sta) != source_.driver_rf) {
                continue;
              }
              const sta::Slack slack = path->slack(sta);
              if (sta::delayLess(slack, selected_slack, sta)) {
                selected = makePathDriverTarget(segment,
                                                chain,
                                                static_cast<int>(index),
                                                source_.slack,
                                                resizer_,
                                                source_.violated_endpoint_pin);
                selected_slack = slack;
              }
            }
            return false;
          });
    }
  }

  sta::Vertex* driver = resizer_.graph()->pinDrvrVertex(source_.driver_pin);
  if (!selected && driver != nullptr) {
    // A driver prefix preserves upstream context without inventing an endpoint
    // suffix that STA no longer retains through this driver.
    sta::VertexPathIterator paths(
        driver, source_.scene, max, source_.driver_rf, sta);
    while (paths.hasNext()) {
      const sta::Path* path = paths.next();
      if (sta::delayInf(path->required(), sta)) {
        continue;
      }

      const sta::Slack slack = path->slack(sta);
      if (sta::delayLess(slack, selected_slack, sta)) {
        sta::PathExpanded expanded(path, sta);
        selected = makePathDriverTarget(path,
                                        expanded,
                                        static_cast<int>(expanded.size() - 1),
                                        source_.slack,
                                        resizer_,
                                        source_.violated_endpoint_pin);
        selected_slack = slack;
      }
    }

    if (selected) {
      RepairSetupProfiler::event("analysis_serial_prefix_fallback");
    }
  }

  if (!selected) {
    RepairSetupProfiler::event("analysis_serial_target_unavailable");
    return false;
  }

  if (driver != nullptr) {
    sta::VertexOutEdgeIterator edges(driver, resizer_.graph());
    while (edges.hasNext()) {
      if (edges.next()->isWire()) {
        ++selected->fanout;
      }
    }
  }

  *observed_target_ = *selected;
  RepairSetupProfiler::event("analysis_serial_target_refresh");
  return true;
}

}  // namespace rsz
