// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db_sta/CpprReport.hh"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Graph.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Report.hh"
#include "sta/Scene.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Units.hh"

namespace sta {

// Collect the pins of a clock path, ordered from the clock root (front) to
// the clock leaf / register CK pin (back). The Path linked list runs
// leaf -> root via prevPath(); we walk it and then reverse. Only pins that
// are part of the clock network are recorded.
static std::vector<const Pin*> clkPathPins(const Path* clk_leaf, dbSta* sta)
{
  std::vector<const Pin*> pins;  // leaf -> root order while walking
  const Path* p = clk_leaf;
  while (p != nullptr) {
    if (!p->isClock(sta)) {
      break;
    }
    const Pin* pin = p->pin(sta);
    if (pin != nullptr) {
      pins.push_back(pin);
    }
    p = p->prevPath();
  }
  std::reverse(pins.begin(), pins.end());  // root -> leaf
  return pins;
}

// Deepest shared clock pin between the launch and capture clock paths: the
// last pin of the common root prefix. Returns nullptr when the paths share
// no clock pin (e.g. cross-domain checks with no common point).
static const Pin* deepestCommonClkPin(const Path* launch_clk_leaf,
                                      const Path* capture_clk_leaf,
                                      dbSta* sta)
{
  if (launch_clk_leaf == nullptr || capture_clk_leaf == nullptr) {
    return nullptr;
  }
  std::vector<const Pin*> launch = clkPathPins(launch_clk_leaf, sta);
  std::vector<const Pin*> capture = clkPathPins(capture_clk_leaf, sta);

  const Pin* common = nullptr;
  const size_t n = std::min(launch.size(), capture.size());
  for (size_t i = 0; i < n; i++) {
    if (launch[i] == capture[i]) {
      common = launch[i];
    } else {
      break;
    }
  }
  return common;
}

std::vector<CpprPathResult> computeCpprSlack(dbSta* sta,
                                             int max_paths,
                                             const MinMax* min_max)
{
  dbNetwork* network = sta->getDbNetwork();
  std::vector<CpprPathResult> results;

  const bool is_max = (min_max == MinMax::max());

  SceneSeq scenes = sta->scenes();
  StringSeq group_names;

  sta->ensureGraph();
  sta->searchPreamble();
  Search* search = sta->search();

  PathEndSeq path_ends = search->findPathEnds(
      nullptr,  // from
      nullptr,  // thrus
      nullptr,  // to
      false,    // unconstrained
      scenes,
      is_max ? MinMaxAll::max() : MinMaxAll::min(),
      max_paths,  // group_path_count (top-N endpoint groups)
      1,          // endpoint_path_count (one per endpoint)
      true,       // unique_pins
      true,       // unique_edges
      -INF,       // slack_min
      INF,        // slack_max
      true,       // sort_by_slack
      group_names,
      is_max,   // setup
      !is_max,  // hold
      false,    // recovery
      false,    // removal
      false,    // clk_gating_setup
      false);   // clk_gating_hold

  for (PathEnd* path_end : path_ends) {
    CpprPathResult r;
    Vertex* end_vertex = path_end->vertex(sta);
    r.endpoint = end_vertex ? network->pathName(end_vertex->pin()) : "?";

    // Authoritative numbers straight from OpenSTA: the credit is the engine's
    // own (slack - slackNoCrpr) so it always matches report_checks. CPPR only
    // relaxes the check, so credit >= 0 by construction.
    r.raw_slack = delayAsFloat(path_end->slackNoCrpr(sta));
    r.cppr_slack = delayAsFloat(path_end->slack(sta));
    r.credit = r.cppr_slack - r.raw_slack;

    // Independently attribute the credit to a concrete branch point: the
    // deepest shared clock pin of the launch and capture clock paths. The
    // launch clock path is reached through PathExpanded::clkPath(); the
    // capture clock path is PathEnd::targetClkPath().
    const Path* data_path = path_end->path();
    PathExpanded expanded(data_path, sta);
    const Path* launch_clk = expanded.clkPath();
    const Path* capture_clk = path_end->targetClkPath();
    const Pin* common = deepestCommonClkPin(launch_clk, capture_clk, sta);
    r.common_pin = common ? network->pathName(common) : "";

    results.push_back(r);
  }

  return results;
}

void reportCppr(dbSta* sta, int max_paths, const MinMax* min_max)
{
  Report* report = sta->report();
  const Unit* time_unit = sta->units()->timeUnit();
  const bool is_max = (min_max == MinMax::max());

  std::vector<CpprPathResult> results
      = computeCpprSlack(sta, max_paths, min_max);

  report->report("Clock Reconvergence Pessimism Removal -- {} ({})",
                 is_max ? "setup" : "hold",
                 is_max ? "max" : "min");
  report->report("{:<32} {:>12} {:>12} {:>12}  {}",
                 "Endpoint",
                 "Raw slack",
                 "CPPR slack",
                 "Credit",
                 "Common clock pin");
  report->report(
      "------------------------------------------------------------"
      "------------------------------------");

  if (results.empty()) {
    report->report("(no constrained paths found)");
    return;
  }

  for (const CpprPathResult& r : results) {
    report->report("{:<32} {:>12} {:>12} {:>12}  {}",
                   r.endpoint,
                   time_unit->asString(r.raw_slack, 4),
                   time_unit->asString(r.cppr_slack, 4),
                   time_unit->asString(r.credit, 4),
                   r.common_pin.empty() ? "(none)" : r.common_pin);
  }
}

std::vector<CpprEndpointResult> computeCpprEndpoints(dbSta* sta,
                                                     int max_paths,
                                                     const MinMax* min_max)
{
  std::vector<CpprPathResult> paths = computeCpprSlack(sta, max_paths, min_max);
  std::vector<CpprEndpointResult> endpoints;
  endpoints.reserve(paths.size());

  // One check per endpoint (endpoint_path_count == 1), so this is a direct
  // 1:1 projection enriched with closure flags. The closure decision keys off
  // the RAW (no-CRPR) slack as the pessimistic baseline and the CPPR-adjusted
  // slack as the closure number: an endpoint that is raw-negative but
  // cppr-positive was failing only because the shared clock-path pessimism
  // was double-counted -- a CRPR artifact, not a genuine violation.
  for (const CpprPathResult& p : paths) {
    CpprEndpointResult e;
    e.endpoint = p.endpoint;
    e.raw_slack = p.raw_slack;
    e.cppr_slack = p.cppr_slack;
    e.credit = p.credit;
    e.common_pin = p.common_pin;
    e.raw_violated = (p.raw_slack < 0.0f);
    e.cppr_violated = (p.cppr_slack < 0.0f);
    e.recovered_to_positive = e.raw_violated && !e.cppr_violated;
    endpoints.push_back(e);
  }
  return endpoints;
}

CpprClosureSummary summarizeCpprClosure(
    const std::vector<CpprEndpointResult>& endpoints)
{
  CpprClosureSummary s;
  s.endpoints = static_cast<int>(endpoints.size());
  for (const CpprEndpointResult& e : endpoints) {
    if (e.raw_violated) {
      s.raw_violations++;
    }
    if (e.cppr_violated) {
      s.cppr_violations++;
    }
    if (e.recovered_to_positive) {
      s.recovered_endpoints++;
    }
  }
  return s;
}

void reportCpprEndpoints(dbSta* sta, int max_paths, const MinMax* min_max)
{
  Report* report = sta->report();
  const Unit* time_unit = sta->units()->timeUnit();
  const bool is_max = (min_max == MinMax::max());

  std::vector<CpprEndpointResult> endpoints
      = computeCpprEndpoints(sta, max_paths, min_max);

  report->report("CPPR endpoint closure -- {} ({})",
                 is_max ? "setup" : "hold",
                 is_max ? "max" : "min");
  report->report("{:<32} {:>12} {:>12} {:>12} {:>9}",
                 "Endpoint",
                 "Raw slack",
                 "CPPR slack",
                 "Credit",
                 "Status");
  report->report(
      "------------------------------------------------------------"
      "------------------------");

  if (endpoints.empty()) {
    report->report("(no constrained paths found)");
    return;
  }

  for (const CpprEndpointResult& e : endpoints) {
    const char* status = e.cppr_violated
                             ? "VIOLATED"
                             : (e.recovered_to_positive ? "RECOVERED" : "ok");
    report->report("{:<32} {:>12} {:>12} {:>12} {:>9}",
                   e.endpoint,
                   time_unit->asString(e.raw_slack, 4),
                   time_unit->asString(e.cppr_slack, 4),
                   time_unit->asString(e.credit, 4),
                   status);
  }

  CpprClosureSummary s = summarizeCpprClosure(endpoints);
  report->report("");
  report->report(
      "Summary: {} endpoints, {} raw-failing, {} cleared by CPPR, "
      "{} genuine violations remaining.",
      s.endpoints,
      s.raw_violations,
      s.recovered_endpoints,
      s.cppr_violations);
}

void reportCpprClosure(dbSta* sta,
                       int max_paths,
                       const MinMax* min_max,
                       bool only_violations)
{
  Report* report = sta->report();
  const Unit* time_unit = sta->units()->timeUnit();
  const bool is_max = (min_max == MinMax::max());

  std::vector<CpprEndpointResult> endpoints
      = computeCpprEndpoints(sta, max_paths, min_max);
  CpprClosureSummary s = summarizeCpprClosure(endpoints);

  report->report("CPPR closure decision -- {} ({})",
                 is_max ? "setup" : "hold",
                 is_max ? "max" : "min");
  report->report(
      "Genuine violations (still failing after CPPR pessimism removal):");
  report->report("{:<32} {:>12} {:>12} {:>12}",
                 "Endpoint",
                 "Raw slack",
                 "CPPR slack",
                 "Credit");
  report->report(
      "------------------------------------------------------------"
      "----------------");

  int listed = 0;
  for (const CpprEndpointResult& e : endpoints) {
    // Closure surface lists genuine (post-CPPR) violations. With
    // only_violations=false also include the raw-failing endpoints CPPR
    // cleared (the common-path-pessimism artifacts) so the user can audit
    // what was credited back.
    const bool show = e.cppr_violated || (!only_violations && e.raw_violated);
    if (!show) {
      continue;
    }
    report->report("{:<32} {:>12} {:>12} {:>12}",
                   e.endpoint,
                   time_unit->asString(e.raw_slack, 4),
                   time_unit->asString(e.cppr_slack, 4),
                   time_unit->asString(e.credit, 4));
    listed++;
  }
  if (listed == 0) {
    report->report("(none -- no endpoints fail after CPPR pessimism removal)");
  }

  report->report("");
  report->report(
      "Closure: {} of {} raw-failing endpoints were clock-reconvergence "
      "pessimism artifacts (cleared by CPPR); {} are genuine violations.",
      s.recovered_endpoints,
      s.raw_violations,
      s.cppr_violations);
}

}  // namespace sta
