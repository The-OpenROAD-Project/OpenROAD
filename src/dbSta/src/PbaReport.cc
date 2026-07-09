// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db_sta/PbaReport.hh"

#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Network.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/Report.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/StringUtil.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace sta {

// Recompute the gate-delay sum of one expanded path using path-specific
// slews, and the matching GBA gate-delay sum. Returns recovered pessimism
// (>= 0). gate_stages counts re-evaluated gate stages.
//
// Algorithm (see PBA_INVESTIGATION.md):
//  - We re-evaluate only COMBINATIONAL gate arcs along the data path. Wire
//    arcs and the launch sequential (clk->Q) / clock arcs are left at their
//    GBA values (path-independent for our slice).
//  - For each such gate we drive the EXISTING OpenSTA arc delay calculator
//    twice with identical load_cap/parasitic:
//      * GBA gate delay: with in_slew = graph (worst-at-pin for max, best-
//        at-pin for min) slew at the gate's actual fan-in pin on this path,
//        prev->slew().
//      * PBA gate delay: with the PATH-SPECIFIC in_slew, i.e. the driver
//        output slew the previous gate produced ON THIS PATH (seeded from
//        the GBA slew at the first gate's input).
//  - setup (max): recovered = sum(GBA_gd - PBA_gd).
//    hold  (min): recovered = sum(PBA_gd - GBA_gd).
//
// PBA >= GBA guarantee:
//  * max: the path-specific delivered slew is always <= the worst-at-pin
//    slew GBA keeps (it is one of the slews contributing to that max), and
//    gate delay is monotonic non-decreasing in input slew, so each PBA gate
//    delay <= the corresponding GBA gate delay -> recovered >= 0.
//  * min: GBA keeps the MINIMUM-at-pin slew (best/fastest), which yields the
//    minimum gate delay; the path-specific delay is one of the contributors
//    to that min, so each PBA gate delay >= the GBA gate delay -> recovered
//    >= 0.
//  In both cases reducing/increasing the data-path delay relaxes the
//  respective check, so pba_slack = gba_slack + recovered >= gba_slack.
//  Computing BOTH via the same gateDelay() call (not the stored arrival
//  increments) keeps the comparison self-consistent.
static float recoverGatePessimism(dbSta* sta,
                                  const Path* path,
                                  const MinMax* min_max,
                                  int& gate_stages)
{
  dbNetwork* network = sta->getDbNetwork();
  GraphDelayCalc* gdc = sta->graphDelayCalc();
  ArcDelayCalc* adc = sta->arcDelayCalc();

  const bool is_max = (min_max == MinMax::max());

  PathExpanded expanded(path, sta);
  const size_t n = expanded.size();

  gate_stages = 0;
  float recovered = 0.0f;

  // Path-specific slew carried forward along the path, seeded lazily from
  // the GBA slew of the stage feeding the first re-evaluated gate.
  Slew path_in_slew = 0.0;
  bool have_path_slew = false;

  for (size_t i = 1; i < n; i++) {
    const Path* curr = expanded.path(i);
    const Path* prev = expanded.path(i - 1);
    const TimingArc* arc = curr->prevArc(sta);
    if (arc == nullptr) {
      continue;
    }

    // Only re-evaluate combinational gate arcs. Wire, sequential (clk->Q),
    // and clock-network arcs keep their GBA contribution.
    if (arc->role() != TimingRole::combinational()) {
      continue;
    }

    const Pin* drvr_pin = curr->pin(sta);
    Vertex* drvr_vertex = curr->vertex(sta);
    if (drvr_pin == nullptr || drvr_vertex == nullptr
        || !network->isDriver(drvr_pin)) {
      continue;
    }

    const RiseFall* drvr_rf = curr->transition(sta);
    const Scene* scene = curr->scene(sta);
    const MinMax* path_min_max = curr->minMax(sta);

    // GBA at-pin input slew at this gate's actual fan-in on the path. For
    // max this is the worst-at-pin slew, for min the best-at-pin slew; in
    // both cases it is the slew GBA used to derive the bounding gate delay.
    const Slew gba_in_slew = prev->slew(sta);

    // Seed the path-specific slew from the GBA slew at the first gate input.
    if (!have_path_slew) {
      path_in_slew = gba_in_slew;
      have_path_slew = true;
    }

    // Load cap + parasitic for this driver, exactly as GBA obtains them.
    float load_cap = 0.0f;
    const Parasitic* parasitic = nullptr;
    gdc->parasiticLoad(drvr_pin,
                       drvr_rf,
                       scene,
                       path_min_max,
                       /*multi_drvr=*/nullptr,
                       adc,
                       load_cap,
                       parasitic);
    LoadPinIndexMap load_pin_index_map = gdc->makeLoadPinIndexMap(drvr_vertex);

    // GBA gate delay: with the at-pin slew (reproduces GBA).
    ArcDcalcResult gba_res = adc->gateDelay(drvr_pin,
                                            arc,
                                            gba_in_slew,
                                            load_cap,
                                            parasitic,
                                            load_pin_index_map,
                                            scene,
                                            path_min_max);
    const float gba_gate_delay = delayAsFloat(gba_res.gateDelay());
    adc->finishDrvrPin();

    // PBA gate delay: with the path-specific input slew.
    ArcDcalcResult pba_res = adc->gateDelay(drvr_pin,
                                            arc,
                                            path_in_slew,
                                            load_cap,
                                            parasitic,
                                            load_pin_index_map,
                                            scene,
                                            path_min_max);
    const float pba_gate_delay = delayAsFloat(pba_res.gateDelay());
    adc->finishDrvrPin();

    // Per-gate recovery. Sign depends on the check:
    //   setup (max): GBA delay is the larger; recovered = GBA - PBA.
    //   hold  (min): GBA delay is the smaller; recovered = PBA - GBA.
    // Clamped at 0 to defend against calculator non-monotonicity / float
    // noise; the slice never reports negative per-gate recovery.
    float gate_recovered = is_max ? (gba_gate_delay - pba_gate_delay)
                                  : (pba_gate_delay - gba_gate_delay);
    if (gate_recovered < 0.0f) {
      gate_recovered = 0.0f;
    }
    recovered += gate_recovered;
    gate_stages++;

    // The PBA-recomputed driver slew feeds the next gate stage on this path.
    path_in_slew = pba_res.drvrSlew();
  }

  return recovered;
}

std::vector<PbaPathResult> computePbaSlack(dbSta* sta,
                                           int max_paths,
                                           const MinMax* min_max)
{
  dbNetwork* network = sta->getDbNetwork();
  std::vector<PbaPathResult> results;

  // Supports SETUP (max) and HOLD (min). For setup, reducing the data-path
  // gate delay (recovering gate-slew pessimism) reduces the data arrival
  // time and INCREASES slack. For hold, the path-specific delay is >= the
  // GBA min-slew delay, increasing the launch data delay and INCREASING hold
  // slack. Both give the monotone pba_slack >= gba_slack guarantee.
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
    Path* path = path_end->path();

    PbaPathResult r;
    Vertex* end_vertex = path_end->vertex(sta);
    r.endpoint = end_vertex ? network->pathName(end_vertex->pin()) : "?";
    r.gba_slack = delayAsFloat(path_end->slack(sta));

    int gate_stages = 0;
    const float recovered
        = recoverGatePessimism(sta, path, min_max, gate_stages);

    // Recovered pessimism always relaxes the check:
    //   pba_slack = gba_slack + recovered, recovered >= 0
    // so pba_slack >= gba_slack by construction (setup and hold).
    r.recovered = recovered;
    r.pba_slack = r.gba_slack + recovered;
    r.gate_stages = gate_stages;

    results.push_back(r);
  }

  return results;
}

std::vector<PbaEndpointResult> computePbaEndpoints(dbSta* sta,
                                                   int max_paths,
                                                   const MinMax* min_max)
{
  std::vector<PbaPathResult> paths = computePbaSlack(sta, max_paths, min_max);
  std::vector<PbaEndpointResult> endpoints;
  endpoints.reserve(paths.size());

  // One path per endpoint (endpoint_path_count == 1), so this is a direct
  // 1:1 projection enriched with closure flags.
  for (const PbaPathResult& p : paths) {
    PbaEndpointResult e;
    e.endpoint = p.endpoint;
    e.gba_slack = p.gba_slack;
    e.pba_slack = p.pba_slack;
    e.recovered = p.recovered;
    e.gate_stages = p.gate_stages;
    e.gba_violated = (p.gba_slack < 0.0f);
    e.pba_violated = (p.pba_slack < 0.0f);
    e.recovered_to_positive = e.gba_violated && !e.pba_violated;
    endpoints.push_back(e);
  }
  return endpoints;
}

PbaClosureSummary summarizeClosure(
    const std::vector<PbaEndpointResult>& endpoints)
{
  PbaClosureSummary s;
  s.endpoints = static_cast<int>(endpoints.size());
  for (const PbaEndpointResult& e : endpoints) {
    if (e.gba_violated) {
      s.gba_violations++;
    }
    if (e.pba_violated) {
      s.pba_violations++;
    }
    if (e.recovered_to_positive) {
      s.recovered_endpoints++;
    }
  }
  return s;
}

void reportPbaSlack(dbSta* sta, int max_paths, const MinMax* min_max)
{
  Report* report = sta->report();
  const Unit* time_unit = sta->units()->timeUnit();
  const bool is_max = (min_max == MinMax::max());

  std::vector<PbaPathResult> results = computePbaSlack(sta, max_paths, min_max);

  report->report("Path-Based Analysis pessimism recovery -- {} ({})",
                 is_max ? "setup" : "hold",
                 is_max ? "max" : "min");
  report->report("{:<40} {:>12} {:>12} {:>12} {:>7}",
                 "Endpoint",
                 "GBA slack",
                 "PBA slack",
                 "Recovered",
                 "Gates");
  report->report(
      "------------------------------------------------------------"
      "---------------------------");

  if (results.empty()) {
    report->report("(no constrained paths found)");
    return;
  }

  for (const PbaPathResult& r : results) {
    report->report("{:<40} {:>12} {:>12} {:>12} {:>7}",
                   r.endpoint,
                   time_unit->asString(r.gba_slack, 4),
                   time_unit->asString(r.pba_slack, 4),
                   time_unit->asString(r.recovered, 4),
                   r.gate_stages);
  }
}

void reportPbaEndpoints(dbSta* sta, int max_paths, const MinMax* min_max)
{
  Report* report = sta->report();
  const Unit* time_unit = sta->units()->timeUnit();
  const bool is_max = (min_max == MinMax::max());

  std::vector<PbaEndpointResult> endpoints
      = computePbaEndpoints(sta, max_paths, min_max);

  report->report("Path-Based Analysis endpoint recovery -- {} ({})",
                 is_max ? "setup" : "hold",
                 is_max ? "max" : "min");
  report->report("{:<40} {:>12} {:>12} {:>12} {:>9}",
                 "Endpoint",
                 "GBA slack",
                 "PBA slack",
                 "Recovered",
                 "Status");
  report->report(
      "------------------------------------------------------------"
      "-----------------------------");

  if (endpoints.empty()) {
    report->report("(no constrained paths found)");
    return;
  }

  for (const PbaEndpointResult& e : endpoints) {
    const char* status = e.pba_violated
                             ? "VIOLATED"
                             : (e.recovered_to_positive ? "RECOVERED" : "ok");
    report->report("{:<40} {:>12} {:>12} {:>12} {:>9}",
                   e.endpoint,
                   time_unit->asString(e.gba_slack, 4),
                   time_unit->asString(e.pba_slack, 4),
                   time_unit->asString(e.recovered, 4),
                   status);
  }

  PbaClosureSummary s = summarizeClosure(endpoints);
  report->report("");
  report->report(
      "Summary: {} endpoints, {} GBA-failing, {} recovered by PBA, "
      "{} genuine violations remaining.",
      s.endpoints,
      s.gba_violations,
      s.recovered_endpoints,
      s.pba_violations);
}

void reportPbaClosure(dbSta* sta,
                      int max_paths,
                      const MinMax* min_max,
                      bool only_violations)
{
  Report* report = sta->report();
  const Unit* time_unit = sta->units()->timeUnit();
  const bool is_max = (min_max == MinMax::max());

  std::vector<PbaEndpointResult> endpoints
      = computePbaEndpoints(sta, max_paths, min_max);
  PbaClosureSummary s = summarizeClosure(endpoints);

  report->report("PBA closure decision -- {} ({})",
                 is_max ? "setup" : "hold",
                 is_max ? "max" : "min");
  report->report(
      "Genuine violations (still failing after PBA pessimism recovery):");
  report->report("{:<40} {:>12} {:>12} {:>12}",
                 "Endpoint",
                 "GBA slack",
                 "PBA slack",
                 "Recovered");
  report->report(
      "------------------------------------------------------------"
      "----------------------");

  int listed = 0;
  for (const PbaEndpointResult& e : endpoints) {
    // Closure surface lists genuine violations. With only_violations=false
    // also include the GBA-pessimism artifacts (recovered endpoints) so the
    // user can see what PBA cleared.
    const bool show = e.pba_violated || (!only_violations && e.gba_violated);
    if (!show) {
      continue;
    }
    report->report("{:<40} {:>12} {:>12} {:>12}",
                   e.endpoint,
                   time_unit->asString(e.gba_slack, 4),
                   time_unit->asString(e.pba_slack, 4),
                   time_unit->asString(e.recovered, 4));
    listed++;
  }
  if (listed == 0) {
    report->report("(none -- no endpoints fail after PBA recovery)");
  }

  report->report("");
  report->report(
      "Closure: {} of {} GBA-failing endpoints were GBA-pessimism artifacts "
      "(cleared by PBA); {} are genuine violations.",
      s.recovered_endpoints,
      s.gba_violations,
      s.pba_violations);
}

}  // namespace sta
