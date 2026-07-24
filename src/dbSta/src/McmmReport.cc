// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db_sta/McmmReport.hh"

#include <algorithm>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Mode.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/Report.hh"
#include "sta/Scene.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Units.hh"

namespace sta {

// Run a path-end search over a specific set of scenes and return, for each
// endpoint, its worst slack and (for the across-corner case) the scene that
// produced it. One path end per endpoint (endpoint_path_count == 1) is the
// worst path for that endpoint over the searched scenes.
static PathEndSeq findEndpointWorst(dbSta* sta,
                                    const SceneSeq& scenes,
                                    int max_endpoints,
                                    bool is_max)
{
  StringSeq group_names;
  Search* search = sta->search();
  return search->findPathEnds(
      nullptr,  // from
      nullptr,  // thrus
      nullptr,  // to
      false,    // unconstrained
      scenes,
      is_max ? MinMaxAll::max() : MinMaxAll::min(),
      max_endpoints,  // group_path_count (top-N endpoint groups)
      1,              // endpoint_path_count (worst path per endpoint)
      true,           // unique_pins
      true,           // unique_edges
      -INF,           // slack_min
      INF,            // slack_max
      true,           // sort_by_slack
      group_names,
      is_max,   // setup
      !is_max,  // hold
      false,    // recovery
      false,    // removal
      false,    // clk_gating_setup
      false);   // clk_gating_hold
}

std::vector<McmmEndpointResult> computeMcmmSlack(dbSta* sta,
                                                 int max_endpoints,
                                                 const MinMax* min_max)
{
  dbNetwork* network = sta->getDbNetwork();
  const bool is_max = (min_max == MinMax::max());

  sta->ensureGraph();
  sta->searchPreamble();

  const SceneSeq& all_scenes = sta->scenes();

  std::vector<McmmEndpointResult> results;
  // Map endpoint pin name -> index into results, to merge per-corner columns.
  std::unordered_map<std::string, size_t> index_of;

  // Pass 1: cross-corner worst. A single search over ALL scenes returns, per
  // endpoint, the worst path across all corners. PathEnd::path()->scene()
  // names the limiting corner -- this is OpenSTA's own cross-corner minimum,
  // we are only reading it back. This also establishes the top-N ordering.
  PathEndSeq across = findEndpointWorst(sta, all_scenes, max_endpoints, is_max);
  for (PathEnd* path_end : across) {
    Vertex* end_vertex = path_end->vertex(sta);
    const std::string endpoint
        = end_vertex ? network->pathName(end_vertex->pin()) : "?";
    if (index_of.find(endpoint) != index_of.end()) {
      continue;  // already recorded (defensive: unique endpoints expected)
    }
    McmmEndpointResult r;
    r.endpoint = endpoint;
    r.worst_slack = delayAsFloat(path_end->slack(sta));
    const Scene* scene = path_end->path()->scene(sta);
    r.worst_corner = scene ? scene->name() : "";
    // Slice 2: the all-scenes search spans modes, so the limiting scene's mode
    // is the limiting MODE of the true cross-mode x corner worst.
    r.worst_mode = (scene && scene->mode()) ? scene->mode()->name() : "";
    // Pre-size the per-corner columns in scene-index order.
    r.corners.resize(all_scenes.size());
    for (Scene* s : all_scenes) {
      r.corners[s->index()].corner = s->name();
      r.corners[s->index()].mode = s->mode() ? s->mode()->name() : "";
    }
    index_of[endpoint] = results.size();
    results.push_back(std::move(r));
  }

  // Pass 2: per-corner columns. For each scene, search that scene alone and
  // record each endpoint's worst slack in that corner. Only endpoints that
  // are in the top-N cross-corner set are filled (others are ignored).
  for (Scene* scene : all_scenes) {
    SceneSeq one_scene{scene};
    // Enumerate enough endpoints per corner to cover the top-N set; using the
    // same max_endpoints keeps it bounded while covering the reported rows.
    PathEndSeq per = findEndpointWorst(sta, one_scene, max_endpoints, is_max);
    for (PathEnd* path_end : per) {
      Vertex* end_vertex = path_end->vertex(sta);
      const std::string endpoint
          = end_vertex ? network->pathName(end_vertex->pin()) : "?";
      auto it = index_of.find(endpoint);
      if (it == index_of.end()) {
        continue;  // not in the reported top-N set
      }
      McmmCornerSlack& col = results[it->second].corners[scene->index()];
      const float slack = delayAsFloat(path_end->slack(sta));
      // First (and only) path end per endpoint per scene is the worst.
      if (!col.valid) {
        col.slack = slack;
        col.valid = true;
      }
    }
  }

  return results;
}

// Slice 2: per-mode worst-slack breakdown. For each active mode we run a
// path-end search restricted to that mode's scenes (Mode::scenes()). Because
// the search is bounded to one mode, PathEnd::slack is the worst slack for that
// endpoint *within that mode*, and the limiting scene is the limiting corner
// inside the mode. This reuses the same public Search API as slice 1 and does
// not mutate the graph.
std::vector<McmmModeResult> computeMcmmSlackByMode(dbSta* sta,
                                                   int max_endpoints,
                                                   const MinMax* min_max)
{
  dbNetwork* network = sta->getDbNetwork();
  const bool is_max = (min_max == MinMax::max());

  sta->ensureGraph();
  sta->searchPreamble();

  std::vector<McmmModeResult> results;
  // Iterate modes in mode-index order for a stable, reproducible report.
  ModeSeq modes = sta->modes();
  std::sort(modes.begin(), modes.end(), [](const Mode* a, const Mode* b) {
    return a->modeIndex() < b->modeIndex();
  });

  for (Mode* mode : modes) {
    McmmModeResult mr;
    mr.mode = mode->name();
    // Scenes belonging to this mode (the corners analyzed under its SDC).
    SceneSeq mode_scenes = mode->scenes();
    if (mode_scenes.empty()) {
      // A mode with no scenes contributes no timing; record it for visibility.
      results.push_back(std::move(mr));
      continue;
    }
    for (Scene* s : mode_scenes) {
      mr.scenes.push_back(s->name());
    }

    PathEndSeq ends
        = findEndpointWorst(sta, mode_scenes, max_endpoints, is_max);
    std::unordered_map<std::string, bool> seen;
    for (PathEnd* path_end : ends) {
      Vertex* end_vertex = path_end->vertex(sta);
      const std::string endpoint
          = end_vertex ? network->pathName(end_vertex->pin()) : "?";
      if (seen.count(endpoint)) {
        continue;  // first (worst) end per endpoint already recorded
      }
      seen[endpoint] = true;
      McmmModeEndpointSlack es;
      es.endpoint = endpoint;
      es.slack = delayAsFloat(path_end->slack(sta));
      const Scene* scene = path_end->path()->scene(sta);
      es.worst_corner = scene ? scene->name() : "";
      es.valid = true;
      mr.endpoints.push_back(std::move(es));
    }
    results.push_back(std::move(mr));
  }

  return results;
}

void reportMcmmSlack(dbSta* sta,
                     int max_endpoints,
                     const MinMax* min_max,
                     bool by_mode)
{
  Report* report = sta->report();
  const Unit* time_unit = sta->units()->timeUnit();
  const bool is_max = (min_max == MinMax::max());

  sta->ensureGraph();
  sta->searchPreamble();
  const SceneSeq& all_scenes = sta->scenes();

  std::vector<McmmEndpointResult> results
      = computeMcmmSlack(sta, max_endpoints, min_max);

  report->report("Multi-Corner Multi-Mode worst slack -- {} ({})",
                 is_max ? "setup" : "hold",
                 is_max ? "max" : "min");
  report->report("Active corners ({}):", static_cast<int>(all_scenes.size()));
  for (Scene* scene : all_scenes) {
    report->report(
        "  [{}] {}", static_cast<int>(scene->index()), scene->name());
  }

  // Header: Endpoint | per-corner columns | Worst | Limiting corner
  // Slice 2: when -by_mode, the limiting column also names the mode, so the
  // header reads "Limiting (mode, corner)". With the flag off the header and
  // every row are byte-identical to slice 1.
  std::string header = "Endpoint                        ";
  for (Scene* scene : all_scenes) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), " %12.12s", scene->name().c_str());
    header += buf;
  }
  header += by_mode ? "        Worst  Limiting (mode, corner)"
                    : "        Worst  Limiting corner";
  report->report("{}", header);
  report->report(
      "------------------------------------------------------------"
      "------------------------------------");

  if (results.empty()) {
    report->report("(no constrained paths found)");
    return;
  }

  for (const McmmEndpointResult& r : results) {
    std::string line;
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%-32.32s", r.endpoint.c_str());
    line = buf;
    for (const McmmCornerSlack& col : r.corners) {
      if (col.valid) {
        std::snprintf(buf,
                      sizeof(buf),
                      " %12s",
                      time_unit->asString(col.slack, 4).c_str());
      } else {
        std::snprintf(buf, sizeof(buf), " %12s", "-");
      }
      line += buf;
    }
    if (by_mode) {
      // True cross-mode x corner limiter: "(mode, corner)".
      std::snprintf(buf,
                    sizeof(buf),
                    " %12s  (%s, %s)",
                    time_unit->asString(r.worst_slack, 4).c_str(),
                    r.worst_mode.c_str(),
                    r.worst_corner.c_str());
    } else {
      std::snprintf(buf,
                    sizeof(buf),
                    " %12s  %s",
                    time_unit->asString(r.worst_slack, 4).c_str(),
                    r.worst_corner.c_str());
    }
    line += buf;
    report->report("{}", line);
  }

  if (!by_mode) {
    return;
  }

  // ---- Slice 2: per-mode breakdown -------------------------------------
  // One section per active mode: for each mode, the worst slack per endpoint
  // over just that mode's scenes, with the limiting corner inside the mode.
  std::vector<McmmModeResult> mode_results
      = computeMcmmSlackByMode(sta, max_endpoints, min_max);
  report->report("");
  report->report("Per-mode worst slack ({} mode{}):",
                 static_cast<int>(mode_results.size()),
                 mode_results.size() == 1 ? "" : "s");
  for (const McmmModeResult& mr : mode_results) {
    std::string scene_list;
    for (size_t i = 0; i < mr.scenes.size(); ++i) {
      if (i) {
        scene_list += ", ";
      }
      scene_list += mr.scenes[i];
    }
    report->report("  mode {} [corners: {}]",
                   mr.mode,
                   scene_list.empty() ? "-" : scene_list);
    if (mr.endpoints.empty()) {
      report->report("    (no constrained paths in this mode)");
      continue;
    }
    for (const McmmModeEndpointSlack& es : mr.endpoints) {
      char buf[256];
      std::snprintf(buf,
                    sizeof(buf),
                    "    %-32.32s %12s  %s",
                    es.endpoint.c_str(),
                    time_unit->asString(es.slack, 4).c_str(),
                    es.worst_corner.c_str());
      report->report("{}", buf);
    }
  }
}

}  // namespace sta
