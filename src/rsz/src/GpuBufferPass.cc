// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "GpuBufferPass.hh"

#include <algorithm>
#include <chrono>
#include <memory>
#include <unordered_set>
#include <vector>

#include "Rebuffer.hh"
#include "db_sta/dbNetwork.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Sta.hh"
#include "utl/Logger.h"

namespace rsz {
void printRebufferProfile(utl::Logger* logger)
{
  // Profiling summary is a no-op in the integrated build;
  // per-net timing is already logged by the rebuffer loop.
  (void) logger;
}

namespace {

using utl::RSZ;

struct BufferCandidatePin
{
  sta::Pin* pin = nullptr;
  float slack_s = 0.0f;
  int fanout = 0;
  float load_cap = 0.0f;
  float score = 0.0f;
};

void ensureSetupTiming(Resizer* resizer, sta::Sta* sta)
{
  resizer->updateParasiticsAndTiming();
  for (auto mode : sta->modes()) {
    sta->ensureClkNetwork(mode);
  }
  sta->ensureGraph();
  sta->searchPreamble();
  sta->ensureLevelized();
  sta->updateTiming(true);
  sta->findRequireds();
}

int wireFanout(sta::Graph* graph, sta::Vertex* vertex)
{
  int fanout = 0;
  sta::VertexOutEdgeIterator edge_iter(vertex, graph);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (edge->isWire()) {
      fanout++;
    }
  }
  return fanout;
}

bool drivesTopLevelOutput(sta::Network* network, const sta::Pin* drvr_pin)
{
  sta::Net* net = network->net(drvr_pin);
  if (net == nullptr) {
    return true;
  }
  std::unique_ptr<sta::NetConnectedPinIterator> pin_iter(
      network->connectedPinIterator(net));
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    if (network->isTopLevelPort(pin) && network->direction(pin)->isOutput()) {
      return true;
    }
  }
  return false;
}

bool skipDriverPin(Resizer* resizer, sta::Sta* sta, const sta::Pin* pin)
{
  sta::Network* network = resizer->network();
  if (network->isTopLevelPort(pin) || network->libertyPort(pin) == nullptr) {
    return true;
  }
  if (resizer->dontTouch(pin) || !resizer->okToBufferNet(pin)) {
    return true;
  }
  if (sta->isClock(pin, sta->cmdMode())
      || sta->isConstant(pin, sta->cmdMode())) {
    return true;
  }
  if (drivesTopLevelOutput(network, pin)) {
    return true;
  }
  return false;
}

std::vector<BufferCandidatePin> collectViolatingDrivers(Resizer* resizer,
                                                        float wns_s,
                                                        std::unordered_set<const sta::Pin*>* failed_pins)
{
  std::vector<BufferCandidatePin> candidates;
  sta::Sta* sta = resizer->sta();
  sta::Network* network = resizer->network();
  sta::Graph* graph = sta->graph();
  if (graph == nullptr) {
    return candidates;
  }

  sta::Instance* top = network->topInstance();
  if (top == nullptr) {
    return candidates;
  }

  // Near-WNS drivers only. slack and wns are negative; slack <= 0.5*wns
  // is at least half as critical as WNS.
  const float slack_cut = (wns_s < 0.0f) ? (0.5f * wns_s) : 0.0f;

  std::unordered_set<const sta::Net*> seen_nets;
  std::unique_ptr<sta::InstanceChildIterator> child_iter(
      network->childIterator(top));
  while (child_iter->hasNext()) {
    sta::Instance* inst = child_iter->next();
    if (network->libertyCell(inst) == nullptr) {
      continue;
    }
    std::unique_ptr<sta::InstancePinIterator> pin_iter(
        network->pinIterator(inst));
    while (pin_iter->hasNext()) {
      sta::Pin* pin = pin_iter->next();
      if (!network->direction(pin)->isOutput()) {
        continue;
      }
      sta::Vertex* vertex = graph->pinDrvrVertex(pin);
      if (vertex == nullptr) {
        continue;
      }
      const sta::Slack slack = sta->slack(vertex, sta::MinMax::max());
      if (slack == sta::INF || static_cast<float>(slack) >= 0.0f) {
        continue;
      }
      const float slack_s = static_cast<float>(slack);
      if (slack_s > slack_cut) {
        continue;
      }
      const int fanout = wireFanout(graph, vertex);
      // High fanout is eligible; the apply-count cap (max_nets) bounds work.
      if (fanout <= 1) {
        continue;
      }
      if (skipDriverPin(resizer, sta, pin)) {
        continue;
      }
      const sta::Net* net = network->net(pin);
      if (net == nullptr || !seen_nets.insert(net).second) {
        continue;
      }
      if (failed_pins && failed_pins->count(pin)) {
        continue;
      }
      float load_cap = 0.0f;
      sta::GraphDelayCalc* gdc = sta->graphDelayCalc();
      if (gdc != nullptr && !sta->scenes().empty()) {
        load_cap = static_cast<float>(
            gdc->loadCap(pin, sta->scenes()[0], sta::MinMax::max()));
      }
      candidates.push_back({.pin = pin,
                            .slack_s = slack_s,
                            .fanout = fanout,
                            .load_cap = load_cap,
                            .score = (-slack_s) * static_cast<float>(fanout)});
    }
  }
  return candidates;
}

}  // namespace

GpuBufferPassStats runGpuBufferPass(Resizer* resizer,
                                    int max_nets,
                                    float wns_s,
                                                        std::unordered_set<const sta::Pin*>* failed_pins)
{
  GpuBufferPassStats stats;
  if (resizer == nullptr || max_nets <= 0) {
    return stats;
  }

  using Clock = std::chrono::steady_clock;
  auto ms_since = [](Clock::time_point t) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
  };

  utl::Logger* logger = resizer->logger();
  sta::Sta* sta = resizer->sta();
  logger->info(RSZ,
               1030,
               "Starting capped buffer insertion (OpenROAD Rebuffer).");

  auto t0 = Clock::now();
  resizer->initBlock();
  
  resizer->resizePreamble();
  const double preamble_ms = ms_since(t0);
  logger->report("PHASE_BUFFER_PREAMBLE_MS: {:.3f}", preamble_ms);

  t0 = Clock::now();
  Rebuffer& rebuffer = resizer->rebuffer();
  const double rebuffer_init_ms = 0.0;

  t0 = Clock::now();
  ensureSetupTiming(resizer, sta);
  stats.sta_ms = ms_since(t0);

  t0 = Clock::now();
  std::vector<BufferCandidatePin> ranked
      = collectViolatingDrivers(resizer, wns_s, failed_pins);
  stats.candidates = static_cast<int>(ranked.size());
  if (ranked.empty()) {
    stats.rank_ms = ms_since(t0);
    logger->info(RSZ,
                 1031,
                 "No rebuffer candidates (setup violators with fanout >= 2).");
    return stats;
  }

  std::ranges::sort(ranked, [](const BufferCandidatePin& a,
                               const BufferCandidatePin& b) {
    return a.score > b.score;
  });
  stats.rank_ms = ms_since(t0);

  const int apply_count = std::min(max_nets, stats.candidates);
  logger->info(RSZ,
               1032,
               "Rebuffer candidates: {}, applying top {} (ranked by "
               "|slack|*fanout, slack <= 0.5*WNS).",
               stats.candidates,
               apply_count);

  t0 = Clock::now();
  est::IncrementalParasiticsGuard guard(resizer->estimateParasitics());
  for (int i = 0; i < apply_count; i++) {
    sta::Pin* pin = ranked[i].pin;
    const auto t_net = Clock::now();
    const int inserted = rebuffer.rebufferPin(pin);
    const double net_ms = ms_since(t_net);
    logger->report(
        "BUFFER_NET: {} slack_ns={:.4g} fanout={} load={:.4g} "
        "inserted={} ms={:.3f}",
        resizer->network()->pathName(pin),
        ranked[i].slack_s * 1e9f,
        ranked[i].fanout,
        ranked[i].load_cap,
        inserted,
        net_ms);
    if (inserted > 0) {
      stats.nets_buffered++;
      stats.buffers_inserted += inserted;
    } else {
      if (failed_pins) {
        failed_pins->insert(pin);
      }
    }
  }
  stats.rebuffer_ms = preamble_ms + rebuffer_init_ms + ms_since(t0);

  t0 = Clock::now();
  if (stats.buffers_inserted > 0) {
    resizer->invalidateVertexOrdering();
  }
  stats.post_ms = ms_since(t0);

  printRebufferProfile(logger);
  logger->info(RSZ,
               1033,
               "Buffer pass complete: {} nets buffered, {} buffers inserted.",
               stats.nets_buffered,
               stats.buffers_inserted);
  return stats;
}

}  // namespace rsz
