// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "GlobalSizingPolicy.hh"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "LRSubproblem.hh"
#include "OptimizationPolicy.hh"
#include "OptimizerTypes.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "sta/Sta.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"
#include "utl/ThreadPool.h"

namespace rsz {

using utl::RSZ;

GlobalSizingPolicy::GlobalSizingPolicy(Resizer& resizer,
                                       MoveCommitter& committer,
                                       RepairSetupContext& setup_context,
                                       const OptimizerRunConfig& config)
    : OptimizationPolicy(resizer, committer, setup_context, config)
{
}

GlobalSizingPolicy::~GlobalSizingPolicy() = default;

bool GlobalSizingPolicy::isDataArc(const sta::Edge* edge) const
{
  const sta::TimingRole* role = edge->role();
  if (role != nullptr && role->isTimingCheck()) {
    return false;
  }
  if (edge->isDisabledLoop()) {
    return false;
  }
  if (role == sta::TimingRole::latchDtoQ()
      || role == sta::TimingRole::latchEnToQ()) {
    return false;
  }
  return true;
}

float GlobalSizingPolicy::edgeMaxArcDelay(sta::Edge* edge) const
{
  sta::TimingArcSet* arc_set = edge->timingArcSet();
  if (arc_set == nullptr) {
    return 0.0f;
  }
  float max_d = 0.0f;
  for (sta::TimingArc* arc : arc_set->arcs()) {
    const sta::ArcDelay d = graph_->arcDelay(edge, arc, dcalc_ap_);
    const float df = sta::delayAsFloat(d);
    max_d = std::max(df, max_d);
  }
  return max_d;
}

void GlobalSizingPolicy::allocate()
{
  // Ensure arc delays and endpoint slacks are up to date before we seed
  sta_->findRequireds();
  // DRC preambles the per-gate subproblem relies on later
  sta_->checkCapacitancesPreamble(sta_->scenes());
  sta_->checkSlewsPreamble();
  sta_->checkFanoutPreamble();

  const sta::Scene* scene = sta_->cmdScene();
  dcalc_ap_ = scene->dcalcAnalysisPtIndex(policy_max_);

  // Walk the graph once to discover max EdgeId (lambda_ is keyed by
  // sta::Edge::id, which is sparse - size to max_id + 1)
  sta::EdgeId max_edge_id = 0;
  int data_edge_count = 0;
  sta::VertexIterator vit(graph_);
  while (vit.hasNext()) {
    sta::Vertex* v = vit.next();
    sta::VertexOutEdgeIterator eit(v, graph_);
    while (eit.hasNext()) {
      sta::Edge* e = eit.next();
      if (!isDataArc(e)) {
        continue;
      }
      const sta::EdgeId id = graph_->id(e);
      max_edge_id = std::max(id, max_edge_id);
      ++data_edge_count;
    }
  }

  const size_t n_edges = static_cast<size_t>(max_edge_id) + 1;
  lambda_.assign(n_edges, 0.0f);

  // Endpoint bookkeeping
  endpoint_vertices_.clear();
  endpoint_index_.clear();
  const sta::VertexSet& eps = sta_->endpoints();
  endpoint_vertices_.reserve(eps.size());
  endpoint_index_.reserve(eps.size());
  for (sta::Vertex* v : eps) {
    endpoint_index_.emplace(v, static_cast<int>(endpoint_vertices_.size()));
    endpoint_vertices_.push_back(v);
  }
  mu_.assign(endpoint_vertices_.size(), 0.0f);

  debugPrint(logger_,
             RSZ,
             "global_sizing",
             2,
             "LR allocate: edges={} (max_id={}), endpoints={}, dcalc_ap={}",
             data_edge_count,
             max_edge_id,
             endpoint_vertices_.size(),
             dcalc_ap_);
}

void GlobalSizingPolicy::seedMultipliers(const LRParams& params)
{
  // λ_e = d_e  (delay-proportional seed, max arc delay across rise/fall)
  float lambda_sum = 0.0f;
  float lambda_max = 0.0f;
  int seeded = 0;
  sta::VertexIterator vit(graph_);
  while (vit.hasNext()) {
    sta::Vertex* v = vit.next();
    sta::VertexOutEdgeIterator eit(v, graph_);
    while (eit.hasNext()) {
      sta::Edge* e = eit.next();
      if (!isDataArc(e)) {
        continue;
      }
      const float d = edgeMaxArcDelay(e);
      const sta::EdgeId id = graph_->id(e);
      const float seed = std::max(d, params.lambda_floor);
      lambda_[id] = seed;
      lambda_sum += seed;
      lambda_max = std::max(lambda_max, seed);
      ++seeded;
    }
  }

  // μ_k = max(0, margin - slack_k)^p  (WNS-biased endpoint seed).
  // Then normalize so max(μ) = 1 - this decouples the LR pressure's scale
  // from the raw slack units so that downstream λ·d terms are predictable.
  float mu_max_raw = 0.0f;
  int mu_nonzero = 0;
  const float margin = params.setup_slack_margin;
  const float p = params.mu_exponent;
  for (size_t k = 0; k < endpoint_vertices_.size(); ++k) {
    const sta::Slack slack = sta_->slack(endpoint_vertices_[k], policy_max_);
    const float slack_f = sta::delayAsFloat(slack);
    const float gap = margin - slack_f;
    float mu = 0.0f;
    if (gap > 0.0f) {
      mu = std::pow(gap, p);
      ++mu_nonzero;
    }
    mu_[k] = mu;
    mu_max_raw = std::max(mu_max_raw, mu);
  }
  if (mu_max_raw > 0.0f) {
    for (float& mu : mu_) {
      mu /= mu_max_raw;
    }
  }
  float mu_sum = 0.0f;
  float mu_max = 0.0f;
  for (const float mu : mu_) {
    mu_sum += mu;
    mu_max = std::max(mu_max, mu);
  }

  debugPrint(logger_,
             RSZ,
             "global_sizing",
             2,
             "LR seed: {} data arcs (λ sum={:.3g}, max={:.3g}, avg={:.3g}); "
             "{}/{} endpoints violating (μ sum={:.3g}, max={:.3g})",
             seeded,
             lambda_sum,
             lambda_max,
             seeded ? lambda_sum / seeded : 0.0f,
             mu_nonzero,
             endpoint_vertices_.size(),
             mu_sum,
             mu_max);
}

void GlobalSizingPolicy::updateMultipliers(const LRParams& params)
{
  // μ: re-seed from current endpoint slacks. Fresh seed (rather than a
  // multiplicative μ update) avoids the lock-in where an endpoint whose μ
  // reached the floor can never re-activate when its slack regresses.
  float mu_max_raw = 0.0f;
  const float margin = params.setup_slack_margin;
  const float p = params.mu_exponent;
  int mu_nonzero = 0;
  for (size_t k = 0; k < endpoint_vertices_.size(); ++k) {
    const sta::Slack slack = sta_->slack(endpoint_vertices_[k], policy_max_);
    const float slack_f = sta::delayAsFloat(slack);
    const float gap = margin - slack_f;
    float mu = 0.0f;
    if (gap > 0.0f) {
      mu = std::pow(gap, p);
      ++mu_nonzero;
    }
    mu_[k] = mu;
    mu_max_raw = std::max(mu_max_raw, mu);
  }
  if (mu_max_raw > 0.0f) {
    for (float& mu : mu_) {
      mu /= mu_max_raw;
    }
  }

  // λ: dual-subgradient ascent.
  //
  //   g_e_norm = (d_e - (a_to - a_from)) / max(d_e, ε)   ∈ [-1, 0]
  //   λ_e ← max(floor, λ_e · (1 + α · g_e_norm))
  //
  // tight arc (g=0)        → λ unchanged
  // full slack (g=−1)      → λ ← (1-α)·λ
  //
  // Arcs touching unconstrained vertices (sentinel arrivals from no-clock
  // PIs/POs) are skipped - those have no meaningful slack and projection
  // alone determines their λ.
  const float alpha = std::clamp(params.beta, 0.0f, 1.0f);
  const float kArrivalSentinel = 1e6f;
  float lam_sum = 0.0f;
  float lam_max = 0.0f;
  int updated = 0;
  int skipped_unconstrained = 0;
  int tight_arcs = 0;
  sta::VertexIterator vit(graph_);
  while (vit.hasNext()) {
    sta::Vertex* v = vit.next();
    sta::VertexOutEdgeIterator eit(v, graph_);
    while (eit.hasNext()) {
      sta::Edge* e = eit.next();
      if (!isDataArc(e)) {
        continue;
      }
      const sta::EdgeId id = graph_->id(e);
      if (static_cast<size_t>(id) >= lambda_.size()) {
        continue;
      }
      const float d = edgeMaxArcDelay(e);
      sta::Vertex* from_v = e->from(graph_);
      sta::Vertex* to_v = e->to(graph_);
      const float a_from = sta::delayAsFloat(sta_->arrival(
          from_v, sta::RiseFallBoth::riseFall(), sta_->scenes(), policy_max_));
      const float a_to = sta::delayAsFloat(sta_->arrival(
          to_v, sta::RiseFallBoth::riseFall(), sta_->scenes(), policy_max_));
      if (std::fabs(a_from) >= kArrivalSentinel
          || std::fabs(a_to) >= kArrivalSentinel) {
        ++skipped_unconstrained;
        lam_sum += lambda_[id];
        lam_max = std::max(lam_max, lambda_[id]);
        continue;
      }
      const float arrival_diff = a_to - a_from;
      const float denom = std::max(d, params.lambda_floor);
      const float g_norm = (d - arrival_diff) / denom;
      const float g_clamped = std::clamp(g_norm, -1.0f, 0.0f);
      if (g_clamped > -1e-6f) {
        ++tight_arcs;
      }
      const float scale = 1.0f + alpha * g_clamped;
      lambda_[id] = std::max(lambda_[id] * scale, params.lambda_floor);
      lam_sum += lambda_[id];
      lam_max = std::max(lam_max, lambda_[id]);
      ++updated;
    }
  }

  debugPrint(logger_,
             RSZ,
             "global_sizing",
             2,
             "LR update: {} arcs subgradient-stepped "
             "({} tight, {} unconstrained skipped); "
             "λ sum={:.3g} max={:.3g}; "
             "{}/{} endpoints violating",
             updated,
             tight_arcs,
             skipped_unconstrained,
             lam_sum,
             lam_max,
             mu_nonzero,
             endpoint_vertices_.size());
}

void GlobalSizingPolicy::projectFlowBalance(const LRParams& params)
{
  // Collect all vertices and sort by level (descending) so we visit endpoints
  // before their predecessors
  std::vector<sta::Vertex*> vertices;
  {
    sta::VertexIterator vit(graph_);
    while (vit.hasNext()) {
      vertices.push_back(vit.next());
    }
  }
  std::ranges::sort(vertices, [](const sta::Vertex* a, const sta::Vertex* b) {
    return a->level() > b->level();
  });

  int rescaled = 0;
  int zero_sum_fallback = 0;
  for (sta::Vertex* v : vertices) {
    // Target flow into v
    float target = 0.0f;
    auto ep_it = endpoint_index_.find(v);
    const bool is_endpoint = ep_it != endpoint_index_.end();
    if (is_endpoint) {
      target = mu_[ep_it->second];
    } else {
      sta::VertexOutEdgeIterator oeit(v, graph_);
      while (oeit.hasNext()) {
        sta::Edge* e = oeit.next();
        if (!isDataArc(e)) {
          continue;
        }
        // lambda_ is sized to the edge-id space captured in allocate(). A
        // sweep can replace cells and the subsequent updateParasitics()/
        // findRequireds() rebuild arcs, minting edge ids beyond that space, so
        // an id may now be >= lambda_.size(). Such arcs carry no multiplier;
        // skip them, matching the guard in updateMultipliers().
        const sta::EdgeId id = graph_->id(e);
        if (static_cast<size_t>(id) >= lambda_.size()) {
          continue;
        }
        target += lambda_[id];
      }
    }

    // Current flow summed over in-data-edges
    float in_sum = 0.0f;
    int in_count = 0;
    {
      sta::VertexInEdgeIterator ieit(v, graph_);
      while (ieit.hasNext()) {
        sta::Edge* e = ieit.next();
        if (!isDataArc(e)) {
          continue;
        }
        const sta::EdgeId id = graph_->id(e);
        if (static_cast<size_t>(id) >= lambda_.size()) {
          continue;
        }
        in_sum += lambda_[id];
        ++in_count;
      }
    }

    if (in_count == 0) {
      continue;
    }

    if (in_sum > 0.0f) {
      const float scale = target / in_sum;
      sta::VertexInEdgeIterator ieit(v, graph_);
      while (ieit.hasNext()) {
        sta::Edge* e = ieit.next();
        if (!isDataArc(e)) {
          continue;
        }
        const sta::EdgeId id = graph_->id(e);
        if (static_cast<size_t>(id) >= lambda_.size()) {
          continue;
        }
        lambda_[id] = std::max(lambda_[id] * scale, params.lambda_floor);
      }
      ++rescaled;
    } else if (target > 0.0f) {
      const float share = target / static_cast<float>(in_count);
      sta::VertexInEdgeIterator ieit(v, graph_);
      while (ieit.hasNext()) {
        sta::Edge* e = ieit.next();
        if (!isDataArc(e)) {
          continue;
        }
        const sta::EdgeId id = graph_->id(e);
        if (static_cast<size_t>(id) >= lambda_.size()) {
          continue;
        }
        lambda_[id] = std::max(share, params.lambda_floor);
      }
      ++zero_sum_fallback;
    }
  }

  debugPrint(logger_,
             RSZ,
             "global_sizing",
             2,
             "LR project: {} vertices rescaled ({} zero-sum fallbacks)",
             rescaled,
             zero_sum_fallback);
}

GlobalSizingPolicy::DesignSnap GlobalSizingPolicy::computeDesignSnap() const
{
  DesignSnap s;
  std::unique_ptr<sta::LeafInstanceIterator> iit(
      network_->leafInstanceIterator());
  while (iit->hasNext()) {
    sta::Instance* inst = iit->next();
    sta::LibertyCell* cell = network_->libertyCell(inst);
    if (cell == nullptr) {
      continue;
    }
    ++s.instances;
    const std::optional<float> leak = resizer_.cellLeakage(cell);
    if (leak.has_value()) {
      s.total_leakage += *leak;
      ++s.with_leakage;
    }
    odb::dbMaster* master = db_network_->staToDb(db_network_->cell(cell));
    if (master != nullptr && master->isCoreAutoPlaceable()) {
      s.total_area += resizer_.dbuToMeters(master->getWidth())
                      * resizer_.dbuToMeters(master->getHeight());
    }
  }
  return s;
}

std::vector<LRSubproblem::GateSnapshot> GlobalSizingPolicy::buildSnapshots()
{
  // Phase A (main thread, delays valid): freeze each evaluable gate's
  // timing/DRC state. snapshot() also reads loadCap/slew and warms the lazy
  // getSwappableCells / cellLeakage / net-driver caches, so the subsequent
  // parallel phase touches none of them.
  const int lambda_size = static_cast<int>(lambda_.size());
  std::vector<LRSubproblem::GateSnapshot> snapshots;
  std::unique_ptr<sta::LeafInstanceIterator> iit(
      network_->leafInstanceIterator());
  while (iit->hasNext()) {
    sta::Instance* inst = iit->next();
    LRSubproblem::GateSnapshot snap;
    if (subproblem_->snapshot(inst, lambda_.data(), lambda_size, snap)) {
      snapshots.push_back(std::move(snap));
    }
  }
  return snapshots;
}

GlobalSizingPolicy::SweepStats GlobalSizingPolicy::applyDecisions(
    const std::vector<LRSubproblem::GateDecision>& decisions,
    const int visited)
{
  // Phase C (main thread, serial): apply accepted replacements in the snapshot
  // vector order so the result is independent of worker scheduling. Pure apply
  // loop - no slack/slew/arrival query may run here, or the single batched
  // timing update in iterate() would fragment into many.
  //
  // Hysteresis on cost improvement before we commit a move:
  // - Upsize moves: 2% - filter LR-cost noise that would otherwise churn
  //   the design without a meaningful timing win.
  // - Downsize moves: 0% - on a non-critical gate λ is at the floor and
  //   the cost is dominated by leakage; any drop is a real leakage gain.
  const float upsize_accept_tol = 0.02f;
  const float downsize_accept_tol = 0.0f;

  int moves = 0;
  int evaluated = 0;
  int downsizes = 0;
  int upsizes = 0;

  for (const LRSubproblem::GateDecision& r : decisions) {
    if (r.best_cell == nullptr) {
      continue;
    }
    ++evaluated;
    const float tol
        = r.best_is_downsize ? downsize_accept_tol : upsize_accept_tol;
    if (r.best_cost < r.baseline_cost * (1.0f - tol)) {
      sta::LibertyCell* prev = network_->libertyCell(r.inst);
      if (subproblem_->applyReplacement(r.inst, r.best_cell)) {
        ++moves;
        const float rel_gain
            = r.baseline_cost > 0.0f
                  ? (r.baseline_cost - r.best_cost) / r.baseline_cost
                  : 0.0f;
        if (r.best_is_downsize) {
          ++downsizes;
        } else {
          ++upsizes;
        }
        debugPrint(logger_,
                   RSZ,
                   "global_sizing",
                   5,
                   "{} {}: {} -> {} (cost {:.3g} -> {:.3g}, gain {:.2f}%)",
                   r.best_is_downsize ? "DOWN" : "UP  ",
                   network_->pathName(r.inst),
                   prev != nullptr ? prev->name() : "?",
                   r.best_cell->name(),
                   r.baseline_cost,
                   r.best_cost,
                   100.0f * rel_gain);
      }
    }
  }

  debugPrint(logger_,
             RSZ,
             "global_sizing",
             2,
             "LR sweep: {} instances visited, "
             "{} with an improving candidate, "
             "{} replacements applied ({} upsize, {} downsize)",
             visited,
             evaluated,
             moves,
             upsizes,
             downsizes);

  return {.moves = moves, .upsizes = upsizes, .downsizes = downsizes};
}

GlobalSizingPolicy::SweepStats GlobalSizingPolicy::singleSweep(
    const float timing_weight)
{
  // Phase A: Freeze per-gate state.
  std::vector<LRSubproblem::GateSnapshot> snapshots = buildSnapshots();

  // Phase B: Score every snapshot independently. Each worker uses its own
  // ArcDelayCalc copy (arc_delay_calc_ is single-threaded shared state); the
  // copy is cached per worker thread and refreshed if the source changes. With
  // a zero-worker pool this runs inline on the calling thread.
  sta::ArcDelayCalc* const src = sta_->arcDelayCalc();
  const std::vector<LRSubproblem::GateDecision> decisions
      = thread_pool_->parallelMap(
          snapshots,
          [this, timing_weight, src](const LRSubproblem::GateSnapshot& snap) {
            static thread_local sta::ArcDelayCalc* cached_src = nullptr;
            static thread_local std::unique_ptr<sta::ArcDelayCalc> adc;
            if (adc == nullptr || cached_src != src) {
              adc.reset(src->copy());
              cached_src = src;
            }
            return subproblem_->evaluateSnapshot(
                snap, timing_weight, adc.get());
          });

  // Phase C: Apply accepted moves serially.
  return applyDecisions(decisions, static_cast<int>(snapshots.size()));
}

float GlobalSizingPolicy::computeAutoTimingWeight(const LRParams& params) const
{
  std::vector<float> leakages;
  std::vector<float> timings;
  const sta::Scene* scene = sta_->cmdScene();
  const int lambda_size = static_cast<int>(lambda_.size());

  std::unique_ptr<sta::LeafInstanceIterator> iit(
      network_->leafInstanceIterator());
  while (iit->hasNext()) {
    sta::Instance* inst = iit->next();
    if (resizer_.dontTouch(inst)) {
      continue;
    }
    sta::LibertyCell* cell = network_->libertyCell(inst);
    if (cell == nullptr) {
      continue;
    }

    leakages.push_back(subproblem_->leakageOrArea(cell));

    // Per-gate timing pressure used to anchor the leakage<->timing scale.
    //
    // This medians ONLY the output-cone term Σλ·d_out, NOT the full cost
    // function. The upstream-Cin term is deliberately excluded here even
    // though it is part of evaluateCell's cost.
    //
    // Reason: computeAutoTimingWeight calibrates a gain, so it must be
    // anchored to the *actionable* timing pressure - the part of the
    // timing cost that varies as a gate is resized and therefore trades
    // against leakage in the argmin. d_out swings 2-3x across candidate
    // cells, so its level is a faithful proxy for that actionable swing.
    // The upstream-Cin term's level is a full upstream gate delay d_U
    // that is almost entirely a constant w.r.t. this gate's cell choice
    // (intrinsic delay + the driver's other-fanout load); its level is
    // a DC offset, not a signal. Folding it in inflated T_med ~2x,
    // collapsed tw ~2x, and starved the output-cone term - see
    // notes_lr/08_upstream_cin_tw_regression.md. tw cancels in
    // evaluateCell's timing-vs-timing comparison, so the upstream-Cin
    // term still works correctly at the output-cone-anchored tw.
    float gate_t = 0.0f;
    bool has_pressure = false;
    std::unique_ptr<sta::InstancePinIterator> pit(network_->pinIterator(inst));
    while (pit->hasNext()) {
      sta::Pin* pin = pit->next();
      const sta::PortDirection* dir = network_->direction(pin);
      if (!dir->isOutput()) {
        continue;
      }
      sta::Vertex* v = graph_->pinDrvrVertex(pin);
      if (v == nullptr) {
        continue;
      }
      float lam_sum = 0.0f;
      sta::VertexInEdgeIterator ieit(v, graph_);
      while (ieit.hasNext()) {
        sta::Edge* e = ieit.next();
        if (!isDataArc(e)) {
          continue;
        }
        const sta::Pin* from_pin = e->from(graph_)->pin();
        if (network_->instance(from_pin) != inst) {
          continue;
        }
        const sta::EdgeId id = graph_->id(e);
        if (std::cmp_greater_equal(id, lambda_size)) {
          continue;
        }
        lam_sum += lambda_[id];
      }
      if (lam_sum <= 4.0f * params.lambda_floor) {
        continue;
      }
      sta::LibertyPort* port = network_->libertyPort(pin);
      if (port == nullptr) {
        continue;
      }
      const float load
          = sta_->graphDelayCalc()->loadCap(pin, scene, policy_max_);
      const float d = sta::delayAsFloat(
          resizer_.gateDelay(port, load, scene, policy_max_));
      gate_t += lam_sum * d;
      has_pressure = true;
    }
    if (has_pressure) {
      timings.push_back(gate_t);
    }
  }

  float l_med = 0.0f;
  float t_med = 0.0f;
  bool degenerate = leakages.empty() || timings.empty();
  if (!degenerate) {
    const auto l_mid = leakages.size() / 2;
    std::nth_element(
        leakages.begin(), leakages.begin() + l_mid, leakages.end());
    l_med = leakages[l_mid];

    const auto t_mid = timings.size() / 2;
    std::nth_element(timings.begin(), timings.begin() + t_mid, timings.end());
    t_med = timings[t_mid];

    if (l_med <= 0.0f || t_med <= 0.0f) {
      degenerate = true;
    }
  }

  if (degenerate) {
    debugPrint(logger_,
               RSZ,
               "global_sizing",
               1,
               "LR auto timing_weight: degenerate "
               "(leakages={}, timings={}, "
               "L_med={:.3g}, T_med={:.3g}); using 1.0",
               leakages.size(),
               timings.size(),
               l_med,
               t_med);
    return 1.0f;
  }

  const float tw = params.timing_bias * l_med / t_med;
  debugPrint(logger_,
             RSZ,
             "global_sizing",
             1,
             "LR auto timing_weight: bias={:.3g} "
             "L_med={:.3g} T_med={:.3g} -> tw={:.3g}",
             params.timing_bias,
             l_med,
             t_med,
             tw);
  return tw;
}

bool GlobalSizingPolicy::start()
{
  if (!OptimizationPolicy::start()) {
    return false;
  }
  db_network_ = resizer_.dbNetwork();
  subproblem_ = std::make_unique<LRSubproblem>(&resizer_);
  // Phase B fans the per-gate evaluations across the OpenROAD thread budget
  // (threadCount()-1 workers; a zero-worker pool runs inline). Each worker
  // reads only the frozen snapshots, read-only Liberty/SDC, and its own
  // ArcDelayCalc copy, so results are independent of worker count and the
  // apply order stays the snapshot vector order.
  thread_pool_ = makeWorkerThreadPool();
  return true;
}

void GlobalSizingPolicy::iterate()
{
  if (converged_) {
    return;
  }

  allocate();
  seedMultipliers(lr_params_);
  projectFlowBalance(lr_params_);

  subproblem_->init();

  const float timing_weight = computeAutoTimingWeight(lr_params_);

  const DesignSnap pre = computeDesignSnap();
  const float wns_pre = sta::delayAsFloat(sta_->worstSlack(policy_max_));
  const float tns_pre
      = sta::delayAsFloat(sta_->totalNegativeSlack(policy_max_));
  debugPrint(logger_,
             RSZ,
             "global_sizing",
             1,
             "Pre-global sizing design: instances={} (with leakage={}) "
             "leakage={:.3g}W area={:.3g}m^2 WNS={} TNS={}",
             pre.instances,
             pre.with_leakage,
             pre.total_leakage,
             pre.total_area,
             sta::delayAsString(wns_pre, 3, sta_),
             sta::delayAsString(tns_pre, 1, sta_));

  const int max_iter = (lr_params_.max_iterations > 0)
                           ? lr_params_.max_iterations
                           : LRParams{}.max_iterations;
  const float wns_eps = 1e-12f;
  LRParams iter_params = lr_params_;

  float best_wns = wns_pre;
  int total_committed = 0;
  int total_attempted = 0;
  int total_upsizes = 0;
  int total_downsizes = 0;
  int accepted_iters = 0;
  int rejected_iters = 0;
  int consec_zero = 0;
  int consec_reject = 0;
  resizer_.journalBegin();
  for (int iter = 0; iter < max_iter; ++iter) {
    if (iter > 0) {
      updateMultipliers(iter_params);
      projectFlowBalance(iter_params);
    }

    const float wns0 = sta::delayAsFloat(sta_->worstSlack(policy_max_));

    const SweepStats sweep = singleSweep(timing_weight);
    const int iter_moves = sweep.moves;
    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();
    const float wns1 = sta::delayAsFloat(sta_->worstSlack(policy_max_));

    const float wns_delta = wns1 - wns0;
    const bool no_benefit = (iter_moves == 0);
    // Small regressions are deliberately allowed
    const bool reject = sta::fuzzyLess(wns_delta, -wns_eps);

    total_attempted += sweep.moves;
    total_upsizes += sweep.upsizes;
    total_downsizes += sweep.downsizes;

    if (reject) {
      ++consec_reject;
      ++rejected_iters;
      iter_params.beta *= 0.5f;
    } else {
      total_committed += iter_moves;
      ++accepted_iters;
      consec_reject = 0;
    }

    // Best-so-far: Keep track of the best WNS so far but don't restore a sweep
    // that worsens WNS just yet to allow oscillation.
    const float current_wns = sta::delayAsFloat(sta_->worstSlack(policy_max_));
    if (!reject && sta::fuzzyGreaterEqual(current_wns, best_wns)
        && !resizer_.overMaxArea()) {
      resizer_.journalEnd();  // checkpoint
      resizer_.journalBegin();
      best_wns = current_wns;
    }

    if (logger_->debugCheck(RSZ, "global_sizing", 1)) {
      const DesignSnap iter_snap = computeDesignSnap();
      const float tns_iter
          = sta::delayAsFloat(sta_->totalNegativeSlack(policy_max_));
      debugPrint(
          logger_,
          RSZ,
          "global_sizing",
          1,
          "LR iter {}/{} {}: leakage={:.3g} (Δ={:+.3g}, {:+.2f}%) "
          "area={:.3g} (Δ={:+.3g}, {:+.2f}%) "
          "WNS={} TNS={}",
          iter + 1,
          max_iter,
          reject ? "REJ" : "ACC",
          iter_snap.total_leakage,
          iter_snap.total_leakage - pre.total_leakage,
          pre.total_leakage > 0.0
              ? 100.0 * (iter_snap.total_leakage - pre.total_leakage)
                    / pre.total_leakage
              : 0.0,
          iter_snap.total_area,
          iter_snap.total_area - pre.total_area,
          pre.total_area > 0.0
              ? 100.0 * (iter_snap.total_area - pre.total_area) / pre.total_area
              : 0.0,
          sta::delayAsString(wns1, 3, sta_),
          sta::delayAsString(tns_iter, 1, sta_));
    }

    if (consec_reject >= 3) {
      debugPrint(logger_,
                 RSZ,
                 "global_sizing",
                 1,
                 "LR stop: 3 consecutive rejections");
      break;
    }
    if (no_benefit && !reject) {
      if (++consec_zero >= 2) {
        debugPrint(logger_,
                   RSZ,
                   "global_sizing",
                   1,
                   "LR stop: 2 consecutive zero-move passes");
        break;
      }
    } else {
      consec_zero = 0;
    }
  }

  // Journal will always be open and regardless of how the loop exited,
  // we need to restore to the best checkpoint here
  resizer_.journalRestore();

  const DesignSnap post = computeDesignSnap();
  const float wns_post = sta::delayAsFloat(sta_->worstSlack(policy_max_));
  const float tns_post
      = sta::delayAsFloat(sta_->totalNegativeSlack(policy_max_));
  const auto rel = [](double after, double before) {
    return before > 0.0 ? 100.0 * (after - before) / before : 0.0;
  };
  const int total_iters = accepted_iters + rejected_iters;

  // Headline: kept moves vs. attempted moves. They diverge when sweeps are
  // rolled back by the catastrophic-WNS guard, or when the end-of-run best-WNS
  // restore reverts some drift past the best iter.
  logger_->info(RSZ,
                400,
                "GLOBAL_SIZING: {} cells replaced (loop); "
                "{}/{} sweeps accepted, {} rolled back; "
                "{} replacements attempted in total "
                "({} upsize, {} downsize).",
                total_committed,
                accepted_iters,
                total_iters,
                rejected_iters,
                total_attempted,
                total_upsizes,
                total_downsizes);

  // QoR before -> after. This is the line that answers "what did it improve
  // and what did it regress" -- read the arrows, not just the deltas.
  logger_->info(RSZ,
                409,
                "GLOBAL_SIZING QoR: "
                "WNS {} -> {} ({}); "
                "TNS {} -> {} ({}); "
                "leakage {:.3g} -> {:.3g}W ({:+.2f}%); "
                "area {:.3g} -> {:.3g}m^2 ({:+.2f}%).",
                sta::delayAsString(wns_pre, 3, sta_),
                sta::delayAsString(wns_post, 3, sta_),
                sta::delayAsString(wns_post - wns_pre, 3, sta_),
                sta::delayAsString(tns_pre, 1, sta_),
                sta::delayAsString(tns_post, 1, sta_),
                sta::delayAsString(tns_post - tns_pre, 1, sta_),
                pre.total_leakage,
                post.total_leakage,
                rel(post.total_leakage, pre.total_leakage),
                pre.total_area,
                post.total_area,
                rel(post.total_area, pre.total_area));

  // Explain the all-zero summary case explicitly: the design did get
  // churned, but every sweep blew the WNS guard so every pass was
  // rolled back and the netlist is back to where it started.
  if (total_committed == 0 && total_attempted > 0) {
    logger_->info(RSZ,
                  412,
                  "GLOBAL_SIZING: nothing kept -- all {} sweeps tripped the "
                  "WNS guard and were rolled back; the netlist is unchanged "
                  "from the start of this phase. "
                  "The {} attempted replacements were tentative only.",
                  rejected_iters,
                  total_attempted);
  }

  markRunComplete(true);
}

}  // namespace rsz
