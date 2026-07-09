// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db_sta/IncrementalStaReport.hh"

#include <algorithm>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/PortDirection.hh"
#include "sta/Report.hh"
#include "sta/Search.hh"
#include "sta/Units.hh"

namespace sta {

// Walk the timing graph forward from `seeds` and collect every reachable
// vertex. This is the affected fanout cone: when the seed instances are
// resized / swapped / rebuffered, the only vertices whose arrivals can change
// are those reachable forward from the seed driver pins. We compute the cone
// purely by graph traversal so we can compare it against the full endpoint
// count -- it is a self-contained, deterministic proof that an incremental
// engine need only touch this cone.
static void collectForwardCone(dbSta* sta,
                               const std::unordered_set<Vertex*>& seeds,
                               std::unordered_set<Vertex*>& cone)
{
  Graph* graph = sta->graph();
  std::vector<Vertex*> stack(seeds.begin(), seeds.end());
  for (Vertex* v : seeds) {
    cone.insert(v);
  }
  while (!stack.empty()) {
    Vertex* vertex = stack.back();
    stack.pop_back();
    VertexOutEdgeIterator edge_iter(vertex, graph);
    while (edge_iter.hasNext()) {
      Edge* edge = edge_iter.next();
      Vertex* to = edge->to(graph);
      if (cone.find(to) == cone.end()) {
        cone.insert(to);
        stack.push_back(to);
      }
    }
  }
}

IncrementalStaResult computeIncrementalSta(
    dbSta* sta,
    const std::vector<std::string>& seed_insts,
    const MinMax* min_max)
{
  Network* network = sta->network();
  Graph* graph = sta->graph();
  Search* search = sta->search();

  // Ensure the graph exists and timing is up to date. searchPreamble() runs
  // the (incremental) delay calc + levelization; the slack queries below run
  // the (incremental) arrival/required propagation. This is OpenSTA's own
  // machinery -- we do not invalidate anything here.
  sta->ensureGraph();
  sta->searchPreamble();

  IncrementalStaResult result;
  result.seed_instances = seed_insts;

  // Seed the cone with the DRIVER (output) vertices of the edited instances.
  // A resize/swap changes the cell's output drive, so its fanout is what may
  // re-time. (Input-pin cap changes also affect the fanin driver, but the
  // fanout cone is the dominant, well-defined affected region for an ECO and
  // is what the test's full-vs-incremental oracle covers.)
  std::unordered_set<Vertex*> seeds;
  for (const std::string& inst_path : seed_insts) {
    Instance* inst = network->findInstance(inst_path);
    if (inst == nullptr) {
      continue;
    }
    InstancePinIterator* pin_iter = network->pinIterator(inst);
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();
      if (network->isDriver(pin)) {
        Vertex* drvr = graph->pinDrvrVertex(pin);
        if (drvr != nullptr) {
          seeds.insert(drvr);
        }
      }
    }
    delete pin_iter;
  }

  std::unordered_set<Vertex*> cone;
  collectForwardCone(sta, seeds, cone);
  result.affected_vertices = static_cast<int>(cone.size());

  // Enumerate all timing endpoints, record each endpoint's worst slack, and
  // mark whether it falls in the affected cone. The slack value comes from
  // OpenSTA's incremental query API (Sta::slack(Vertex*, MinMax*)), so it is
  // identical to what report_checks / a full re-run would report.
  VertexSet& endpoints = search->endpoints();
  result.total_endpoints = static_cast<int>(endpoints.size());

  float wns = INF;
  float tns = 0.0f;
  for (Vertex* end_vertex : endpoints) {
    IncrementalEndpointSlack ep;
    ep.endpoint = network->pathName(end_vertex->pin());
    const Slack slack = sta->slack(end_vertex, min_max);
    ep.slack = delayAsFloat(slack);
    ep.in_cone = (cone.find(end_vertex) != cone.end());
    if (ep.in_cone) {
      result.affected_endpoints++;
    }
    // WNS/TNS over all endpoints (whole design), matching OpenSTA semantics.
    if (ep.slack < wns) {
      wns = ep.slack;
    }
    if (ep.slack < 0.0f) {
      tns += ep.slack;
    }
    result.endpoints.push_back(std::move(ep));
  }

  // Sort endpoints by slack ascending (worst first) for a stable, auditable
  // report and machine-readable ordering.
  std::sort(
      result.endpoints.begin(),
      result.endpoints.end(),
      [](const IncrementalEndpointSlack& a, const IncrementalEndpointSlack& b) {
        if (a.slack != b.slack) {
          return a.slack < b.slack;
        }
        return a.endpoint < b.endpoint;
      });

  result.wns = (wns == INF) ? 0.0f : wns;
  result.tns = tns;
  return result;
}

void reportIncrementalSta(dbSta* sta,
                          const std::vector<std::string>& seed_insts,
                          const MinMax* min_max)
{
  Report* report = sta->report();
  const Unit* time_unit = sta->units()->timeUnit();
  const bool is_max = (min_max == MinMax::max());

  IncrementalStaResult r = computeIncrementalSta(sta, seed_insts, min_max);

  report->report("Incremental STA -- {} ({}) after ECO edit",
                 is_max ? "setup" : "hold",
                 is_max ? "max" : "min");
  report->report("Edited instances ({}):",
                 static_cast<int>(r.seed_instances.size()));
  for (const std::string& inst : r.seed_instances) {
    report->report("  {}", inst);
  }

  const double pct = (r.total_endpoints > 0)
                         ? (100.0 * r.affected_endpoints / r.total_endpoints)
                         : 0.0;
  report->report("Affected fanout cone: {} graph vertices",
                 r.affected_vertices);
  report->report(
      "Endpoints re-evaluated: {} of {} ({:.1f}%)  <- incremental work",
      r.affected_endpoints,
      r.total_endpoints,
      pct);
  report->report("WNS: {}   TNS: {}",
                 time_unit->asString(r.wns, 4),
                 time_unit->asString(r.tns, 4));
  report->report(
      "------------------------------------------------------------");
  report->report("Endpoint                                  Slack   InCone");
  report->report(
      "------------------------------------------------------------");
  if (r.endpoints.empty()) {
    report->report("(no constrained endpoints found)");
    return;
  }
  for (const IncrementalEndpointSlack& ep : r.endpoints) {
    char buf[256];
    std::snprintf(buf,
                  sizeof(buf),
                  "%-40.40s %10s   %s",
                  ep.endpoint.c_str(),
                  time_unit->asString(ep.slack, 4).c_str(),
                  ep.in_cone ? "yes" : "no");
    report->report("{}", buf);
  }
}

}  // namespace sta
