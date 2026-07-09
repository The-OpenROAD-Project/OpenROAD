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
// slews, and the matching GBA gate-delay sum. Returns recovered = GBA - PBA
// gate-delay total (>= 0). gate_stages counts re-evaluated gate stages.
//
// Algorithm (see PBA_INVESTIGATION.md):
//  - We re-evaluate only COMBINATIONAL gate arcs along the data path. Wire
//    arcs and the launch sequential (clk->Q) / clock arcs are left at their
//    GBA values (path-independent for our slice).
//  - For each such gate we drive the EXISTING OpenSTA arc delay calculator
//    twice with identical load_cap/parasitic:
//      * GBA gate delay: with in_slew = graph (worst-at-pin) slew at the
//        gate's actual fan-in pin on this path, prev->slew().
//      * PBA gate delay: with the PATH-SPECIFIC in_slew, i.e. the driver
//        output slew the previous gate produced ON THIS PATH (seeded from
//        the GBA slew at the first gate's input).
//  - recovered = sum(GBA gate delay - PBA gate delay).
//
// PBA >= GBA guarantee: the path-specific delivered slew is always <= the
// worst-at-pin slew GBA keeps (it is one of the slews contributing to that
// max), and gate delay is monotonic non-decreasing in input slew, so each
// PBA gate delay <= the corresponding GBA gate delay. Computing BOTH via the
// same gateDelay() call (not the stored arrival increments) keeps the
// comparison self-consistent, so recovered >= 0 by construction.
static float recoverGatePessimism(dbSta* sta,
                                  const Path* path,
                                  int& gate_stages)
{
  dbNetwork* network = sta->getDbNetwork();
  GraphDelayCalc* gdc = sta->graphDelayCalc();
  ArcDelayCalc* adc = sta->arcDelayCalc();

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
    const MinMax* min_max = curr->minMax(sta);

    // GBA worst-at-pin input slew at this gate's actual fan-in on the path.
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
                       min_max,
                       /*multi_drvr=*/nullptr,
                       adc,
                       load_cap,
                       parasitic);
    LoadPinIndexMap load_pin_index_map = gdc->makeLoadPinIndexMap(drvr_vertex);

    // GBA gate delay: with the worst-at-pin slew (reproduces GBA).
    ArcDcalcResult gba_res = adc->gateDelay(drvr_pin,
                                            arc,
                                            gba_in_slew,
                                            load_cap,
                                            parasitic,
                                            load_pin_index_map,
                                            scene,
                                            min_max);
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
                                            min_max);
    const float pba_gate_delay = delayAsFloat(pba_res.gateDelay());
    adc->finishDrvrPin();

    // Per-gate recovery (clamped at 0 to defend against calculator
    // non-monotonicity / float noise; the slice never reports negative
    // per-gate recovery).
    float gate_recovered = gba_gate_delay - pba_gate_delay;
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

  // This first slice supports SETUP (max) analysis only. For setup, reducing
  // the data-path gate delay (recovering gate-slew pessimism) reduces the
  // data arrival time and therefore INCREASES slack, giving the monotone
  // pba_slack >= gba_slack guarantee. Hold (min) has the opposite sign and
  // a different (min-slew) pessimism source; it is intentionally out of
  // scope for this slice (see PBA_INVESTIGATION.md / AGENT_REPORT.md).
  if (min_max != MinMax::max()) {
    sta->getLogger()->error(
        utl::STA,
        2101,
        "report_pba_slack first slice supports -setup (max) analysis only.");
  }

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
      MinMaxAll::max(),
      max_paths,  // group_path_count (top-N endpoint groups)
      1,          // endpoint_path_count (one per endpoint)
      true,       // unique_pins
      true,       // unique_edges
      -INF,       // slack_min
      INF,        // slack_max
      true,       // sort_by_slack
      group_names,
      true,    // setup
      false,   // hold
      false,   // recovery
      false,   // removal
      false,   // clk_gating_setup
      false);  // clk_gating_hold

  for (PathEnd* path_end : path_ends) {
    Path* path = path_end->path();

    PbaPathResult r;
    Vertex* end_vertex = path_end->vertex(sta);
    r.endpoint = end_vertex ? network->pathName(end_vertex->pin()) : "?";
    r.gba_slack = delayAsFloat(path_end->slack(sta));

    int gate_stages = 0;
    const float recovered = recoverGatePessimism(sta, path, gate_stages);

    // Setup: reducing data-path gate delay recovers slack:
    //   pba_slack = gba_slack + recovered, recovered >= 0
    // so pba_slack >= gba_slack by construction.
    r.recovered = recovered;
    r.pba_slack = r.gba_slack + recovered;
    r.gate_stages = gate_stages;

    results.push_back(r);
  }

  return results;
}

void reportPbaSlack(dbSta* sta, int max_paths, const MinMax* min_max)
{
  Report* report = sta->report();
  const Unit* time_unit = sta->units()->timeUnit();

  std::vector<PbaPathResult> results = computePbaSlack(sta, max_paths, min_max);

  report->report("Path-Based Analysis pessimism recovery -- setup (max)");
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

}  // namespace sta
