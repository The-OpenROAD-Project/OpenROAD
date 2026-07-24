// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db_sta/ClosureReport.hh"

#include <string>
#include <unordered_map>
#include <vector>

#include "db_sta/CpprReport.hh"
#include "db_sta/PbaReport.hh"
#include "db_sta/dbSta.hh"
#include "sta/Report.hh"
#include "sta/Units.hh"

namespace sta {

const char* clearedByLabel(ClearedBy cleared_by)
{
  switch (cleared_by) {
    case ClearedBy::kCppr:
      return "CPPR";
    case ClearedBy::kPba:
      return "PBA";
    case ClearedBy::kBoth:
      return "BOTH";
    case ClearedBy::kNone:
    default:
      return "-";
  }
}

std::vector<ClosureEndpointResult> computeClosure(dbSta* sta,
                                                  int max_paths,
                                                  const MinMax* min_max)
{
  // COMPOSE the existing PBA and CPPR endpoint computations -- do NOT
  // reimplement either. Both enumerate the same top-N endpoint set (same
  // findPathEnds parameters) ordered by GBA slack, so a join by endpoint
  // name lines them up. The PBA result carries the GBA slack and the
  // gate-slew recovery; the CPPR result carries the raw (pre-CRPR) slack and
  // the common-path credit.
  std::vector<PbaEndpointResult> pba
      = computePbaEndpoints(sta, max_paths, min_max);
  std::vector<CpprEndpointResult> cppr
      = computeCpprEndpoints(sta, max_paths, min_max);

  // Index the CPPR rows by endpoint for the join.
  std::unordered_map<std::string, const CpprEndpointResult*> cppr_by_endpoint;
  cppr_by_endpoint.reserve(cppr.size());
  for (const CpprEndpointResult& c : cppr) {
    cppr_by_endpoint.emplace(c.endpoint, &c);
  }

  std::vector<ClosureEndpointResult> results;
  results.reserve(pba.size());

  // Drive the order off the PBA result so the unified report preserves the
  // GBA slack ordering (same as report_pba_* / report_cppr_*).
  for (const PbaEndpointResult& p : pba) {
    ClosureEndpointResult r;
    r.endpoint = p.endpoint;

    // From PBA: the CPPR-adjusted GBA slack and the gate-slew recovery.
    r.gba_slack = p.gba_slack;
    r.pba_recovered = p.recovered;
    // PBA slack already builds on the CPPR-adjusted GBA slack, so it IS the
    // fully-recovered (CPPR + PBA) slack.
    r.recovered_slack = p.pba_slack;

    // From CPPR (if the endpoint also appears there): the raw pre-CRPR
    // baseline and the common-path credit. If a CPPR row is missing
    // (defensive -- the two searches enumerate the same set), fall back to
    // the GBA slack as the baseline with zero credit, which makes CPPR a
    // no-op for that endpoint rather than inventing recovery.
    auto it = cppr_by_endpoint.find(p.endpoint);
    if (it != cppr_by_endpoint.end()) {
      const CpprEndpointResult* c = it->second;
      r.raw_slack = c->raw_slack;
      r.cppr_credit = c->credit;
      r.common_pin = c->common_pin;
    } else {
      r.raw_slack = p.gba_slack;
      r.cppr_credit = 0.0f;
      r.common_pin = "";
    }

    // Classification keyed off the RAW pessimistic baseline.
    r.raw_violated = (r.raw_slack < 0.0f);
    r.genuine = r.raw_violated && (r.recovered_slack < 0.0f);
    r.artifact = r.raw_violated && !r.genuine;

    if (r.artifact) {
      // Attribute the clearance. Test each mechanism in isolation against the
      // raw baseline:
      //   CPPR alone : raw_slack + cppr_credit  >= 0
      //   PBA  alone : raw_slack + pba_recovered >= 0
      const bool cppr_clears = (r.raw_slack + r.cppr_credit) >= 0.0f;
      const bool pba_clears = (r.raw_slack + r.pba_recovered) >= 0.0f;
      if (cppr_clears && !pba_clears) {
        r.cleared_by = ClearedBy::kCppr;
      } else if (pba_clears && !cppr_clears) {
        r.cleared_by = ClearedBy::kPba;
      } else if (cppr_clears && pba_clears) {
        // Both independently suffice -- attribute to CPPR (the cheaper,
        // always-on mechanism) as the primary, but report BOTH so the user
        // sees it is doubly covered.
        r.cleared_by = ClearedBy::kBoth;
      } else {
        // Neither alone suffices but the sum does: a joint clearance.
        r.cleared_by = ClearedBy::kBoth;
      }
    } else {
      r.cleared_by = ClearedBy::kNone;
    }

    results.push_back(std::move(r));
  }

  return results;
}

ClosureSummary summarizeClosure(
    const std::vector<ClosureEndpointResult>& endpoints)
{
  ClosureSummary s;
  s.endpoints = static_cast<int>(endpoints.size());
  for (const ClosureEndpointResult& e : endpoints) {
    if (e.raw_violated) {
      s.raw_failing++;
    }
    if (e.genuine) {
      s.genuine++;
    }
    if (e.artifact) {
      switch (e.cleared_by) {
        case ClearedBy::kCppr:
          s.cleared_by_cppr++;
          break;
        case ClearedBy::kPba:
          s.cleared_by_pba++;
          break;
        case ClearedBy::kBoth:
          s.cleared_by_both++;
          break;
        case ClearedBy::kNone:
          break;
      }
    }
  }
  return s;
}

void reportClosure(dbSta* sta,
                   int max_paths,
                   const MinMax* min_max,
                   bool only_violations)
{
  Report* report = sta->report();
  const Unit* time_unit = sta->units()->timeUnit();
  const bool is_max = (min_max == MinMax::max());

  std::vector<ClosureEndpointResult> endpoints
      = computeClosure(sta, max_paths, min_max);
  ClosureSummary s = summarizeClosure(endpoints);

  report->report("Unified pessimism-recovery closure -- {} ({})",
                 is_max ? "setup" : "hold",
                 is_max ? "max" : "min");
  report->report(
      "Endpoints failing under the raw (pre-CPPR, pre-PBA) baseline, "
      "classified after applying BOTH recoveries:");
  report->report("{:<32} {:>12} {:>12} {:>12} {:>9} {:>9}",
                 "Endpoint",
                 "Raw slack",
                 "Recovered",
                 "CPPR cr.",
                 "PBA rec.",
                 "Verdict");
  report->report(
      "------------------------------------------------------------"
      "----------------------------------");

  if (endpoints.empty()) {
    report->report("(no constrained paths found)");
    return;
  }

  int listed = 0;
  for (const ClosureEndpointResult& e : endpoints) {
    // The closure surface lists genuine (post-recovery) violations. With
    // only_violations=false also include the cleared artifacts (with the
    // mechanism label) so the user can audit what each recovery removed.
    const bool show = e.genuine || (!only_violations && e.raw_violated);
    if (!show) {
      continue;
    }
    const char* verdict = e.genuine ? "GENUINE" : clearedByLabel(e.cleared_by);
    report->report("{:<32} {:>12} {:>12} {:>12} {:>9} {:>9}",
                   e.endpoint,
                   time_unit->asString(e.raw_slack, 4),
                   time_unit->asString(e.recovered_slack, 4),
                   time_unit->asString(e.cppr_credit, 4),
                   time_unit->asString(e.pba_recovered, 4),
                   verdict);
    listed++;
  }
  if (listed == 0) {
    report->report("(none -- no endpoints fail after CPPR+PBA recovery)");
  }

  report->report("");
  report->report(
      "Closure: {} endpoints, {} raw-failing -- {} cleared by CPPR, "
      "{} cleared by PBA, {} cleared by CPPR+PBA, {} genuine violations.",
      s.endpoints,
      s.raw_failing,
      s.cleared_by_cppr,
      s.cleared_by_pba,
      s.cleared_by_both,
      s.genuine);
}

}  // namespace sta
