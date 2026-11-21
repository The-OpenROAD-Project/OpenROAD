// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RerouteGenerator.hh"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "RerouteCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "rsz/Resizer.hh"
#include "sta/Network.hh"
#include "sta/Scene.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

namespace {

constexpr float kMaxPromotedLayerUsage = 0.70f;
// Realistic total wire + current via + extra via-stack resistance reduction
// threshold for upper-layer promotion. Unlike Resizer's 50% rough wire-only
// heuristic (which omits via stacks and assumes all segments move to the single
// lowest-R layer), promoting an already-res-aware net up one layer pair
// typically saves 10-45% total resistance; actual commits remain strictly gated
// by post-GRT STA slack improvement.
constexpr float kMinPromotedReduction = 0.10f;
constexpr int kLayerUsageRefreshInterval = 32;

double wireResPerMeter(est::EstimateParasitics* est,
                       const sta::Scene* corner,
                       odb::dbTechLayer* layer,
                       const double dbu_per_m)
{
  if (layer == nullptr) {
    return 0.0;
  }
  double res = 0.0;
  double cap = 0.0;
  if (est != nullptr && corner != nullptr) {
    est->layerRC(layer, corner, res, cap);
  }
  if (res <= 0.0) {
    const double width_m = layer->getWidth() / dbu_per_m;
    if (width_m > 0.0 && layer->getResistance() > 0.0) {
      res = layer->getResistance() / width_m;
    }
  }
  return res;
}

double viaResBetween(est::EstimateParasitics* est,
                     const sta::Scene* corner,
                     odb::dbTech* tech,
                     const int l1,
                     const int l2)
{
  const int lo = std::min(l1, l2);
  const int hi = std::max(l1, l2);
  double total = 0.0;
  for (int l = lo; l < hi; ++l) {
    odb::dbTechLayer* rlayer = tech->findRoutingLayer(l);
    if (rlayer == nullptr) {
      continue;
    }
    odb::dbTechLayer* cut = rlayer->getUpperLayer();
    if (cut == nullptr) {
      continue;
    }
    double r = 0.0;
    double c = 0.0;
    if (est != nullptr && corner != nullptr) {
      est->layerRC(cut, corner, r, c);
    }
    if (r <= 0.0) {
      r = cut->getResistance();
    }
    total += r;
  }
  return total;
}

}  // namespace

PromotedLayerEval evaluateReroutePromotion(
    Resizer& resizer,
    odb::dbNet* db_net,
    const std::vector<float>& layer_usage,
    const sta::Scene* corner)
{
  PromotedLayerEval result;
  grt::GlobalRouter* grt = resizer.globalRouter();
  if (grt == nullptr || db_net == nullptr || resizer.block() == nullptr) {
    return result;
  }
  if (db_net->isSpecial() || db_net->getSigType().isSupply()
      || db_net->getSigType() == odb::dbSigType::CLOCK
      || db_net->getNonDefaultRule() != nullptr
      || grt->isNonLeafClock(db_net)) {
    return result;
  }

  const grt::NetRouteMap& routes = grt->getRoutes();
  auto it = routes.find(db_net);
  if (it == routes.end() || it->second.empty()) {
    return result;
  }

  odb::dbTech* tech = resizer.block()->getTech();
  const double dbu_per_m = tech->getDbUnitsPerMicron() * 1.0e6;
  est::EstimateParasitics* est = resizer.estimateParasitics();
  const sta::Scene* active_corner
      = (corner != nullptr) ? corner : resizer.sta()->cmdScene();

  const int min_routing_layer = grt->getMinRoutingLayer();
  const int max_routing_layer = grt->getMaxRoutingLayer();
  if (min_routing_layer < 1 || max_routing_layer <= min_routing_layer) {
    return result;
  }

  std::vector<double> r_per_m(max_routing_layer + 1, 0.0);
  std::vector<odb::dbTechLayerDir> dir(max_routing_layer + 1,
                                       odb::dbTechLayerDir::NONE);
  for (int l = 1; l <= max_routing_layer; ++l) {
    odb::dbTechLayer* layer = tech->findRoutingLayer(l);
    if (layer != nullptr) {
      r_per_m[l] = wireResPerMeter(est, active_corner, layer, dbu_per_m);
      dir[l] = layer->getDirection();
    }
  }

  double len_h_m = 0.0;
  double len_v_m = 0.0;
  double curr_wire_r = 0.0;
  double curr_via_r = 0.0;
  double weighted_layer_sum = 0.0;
  int min_used_layer = max_routing_layer;
  for (const grt::GSegment& seg : it->second) {
    if (seg.isVia()) {
      curr_via_r += viaResBetween(
          est, active_corner, tech, seg.init_layer, seg.final_layer);
    } else {
      const double len_m = seg.length() / dbu_per_m;
      if (len_m <= 0.0 || seg.init_layer < 1
          || seg.init_layer > max_routing_layer) {
        continue;
      }
      if (seg.init_y == seg.final_y) {
        len_h_m += len_m;
      } else {
        len_v_m += len_m;
      }
      curr_wire_r += r_per_m[seg.init_layer] * len_m;
      weighted_layer_sum += seg.init_layer * len_m;
      min_used_layer = std::min(min_used_layer, seg.init_layer);
    }
  }

  const double total_len_m = len_h_m + len_v_m;
  const double curr_total_r = curr_wire_r + curr_via_r;
  if (total_len_m <= 0.0 || curr_total_r <= 0.0) {
    return result;
  }

  const int avg_curr_layer = std::clamp(
      static_cast<int>(std::floor(weighted_layer_sum / total_len_m)),
      min_routing_layer,
      max_routing_layer);

  const int num_pins = std::max(2, static_cast<int>(db_net->getTermCount()));
  const double via_stacks = 2.0 + 0.5 * std::max(0, num_pins - 2);

  double best_est_total_r = curr_total_r;
  int best_min_layer = 0;

  const int start_cand = std::max(min_routing_layer + 1, min_used_layer + 1);
  for (int cand = start_cand; cand <= max_routing_layer - 1; ++cand) {
    const float u0
        = std::cmp_less(cand, layer_usage.size()) ? layer_usage[cand] : 1.0f;
    const float u1 = std::cmp_less(cand + 1, layer_usage.size())
                         ? layer_usage[cand + 1]
                         : 1.0f;
    if (u0 > kMaxPromotedLayerUsage || u1 > kMaxPromotedLayerUsage) {
      continue;
    }
    int l_h = 0;
    int l_v = 0;
    if (dir[cand] == odb::dbTechLayerDir::HORIZONTAL
        && dir[cand + 1] == odb::dbTechLayerDir::VERTICAL) {
      l_h = cand;
      l_v = cand + 1;
    } else if (dir[cand] == odb::dbTechLayerDir::VERTICAL
               && dir[cand + 1] == odb::dbTechLayerDir::HORIZONTAL) {
      l_h = cand + 1;
      l_v = cand;
    } else {
      continue;
    }
    if (r_per_m[l_h] <= 0.0 || r_per_m[l_v] <= 0.0) {
      continue;
    }

    const double est_wire_r = len_h_m * r_per_m[l_h] + len_v_m * r_per_m[l_v];
    const double extra_via_r
        = via_stacks
          * viaResBetween(
              est, active_corner, tech, std::min(avg_curr_layer, cand), cand);
    const double est_total_r = est_wire_r + curr_via_r + extra_via_r;
    if (est_total_r < best_est_total_r) {
      best_est_total_r = est_total_r;
      best_min_layer = cand;
    }
  }

  if (best_min_layer <= 0) {
    return result;
  }
  const double ratio = (curr_total_r - best_est_total_r) / curr_total_r;
  if (ratio < kMinPromotedReduction) {
    return result;
  }

  result.promoted_min_layer = best_min_layer;
  result.curr_resistance = static_cast<float>(curr_total_r);
  result.est_resistance = static_cast<float>(best_est_total_r);
  result.reduction_ratio = static_cast<float>(ratio);
  return result;
}

RerouteGenerator::RerouteGenerator(const GeneratorContext& context)
    : MoveGenerator(context), net_destroy_cbk_(this)
{
  committer_.addJournalObserver(this);
  pending_levels_.resize(committer_.journalDepth());
  if (resizer_.block() != nullptr) {
    net_destroy_cbk_.addOwner(resizer_.block());
  }
}

RerouteGenerator::~RerouteGenerator()
{
  committer_.removeJournalObserver(this);
  net_destroy_cbk_.removeOwner();
}

void RerouteGenerator::recordAttempt(odb::dbNet* db_net)
{
  attempted_nets_.insert(db_net);
  if (!pending_levels_.empty()) {
    pending_levels_.back().push_back(
        {.net = db_net, .speculative = committer_.inSpeculativePass()});
  }
}

void RerouteGenerator::onNetDestroyed(odb::dbNet* db_net)
{
  attempted_nets_.erase(db_net);
  for (auto& level : pending_levels_) {
    std::erase_if(level, [db_net](const PendingAttempt& attempt) {
      return attempt.net == db_net;
    });
  }
}

void RerouteGenerator::onBeginJournal()
{
  pending_levels_.emplace_back();
}

void RerouteGenerator::onCommitJournal(const bool nested)
{
  if (pending_levels_.empty()) {
    return;
  }
  std::vector<PendingAttempt> top = std::move(pending_levels_.back());
  pending_levels_.pop_back();
  if (nested && !pending_levels_.empty()) {
    auto& parent = pending_levels_.back();
    parent.insert(parent.end(), top.begin(), top.end());
  }
}

void RerouteGenerator::onRestoreJournal()
{
  if (pending_levels_.empty()) {
    return;
  }
  const std::vector<PendingAttempt> top = std::move(pending_levels_.back());
  pending_levels_.pop_back();
  grt::GlobalRouter* global_router = resizer_.globalRouter();
  for (const PendingAttempt& attempt : top) {
    if (global_router != nullptr) {
      // Clear both promoted_min_layers_ and pending reroute_nets_ on rollback
      // (covering pre-updateParasitics() rollbacks without touching
      // dirty_nets_).
      global_router->clearNetPromotedMinLayer(attempt.net);
    }
    if (attempt.speculative) {
      attempted_nets_.erase(attempt.net);
    }
  }
}

bool RerouteGenerator::isApplicable(const Target& target) const
{
  return MoveGenerator::isApplicable(target)
         && resizer_.globalRouter() != nullptr
         && !resizer_.globalRouter()->isUseCUGR()
         && resizer_.estimateParasitics()->getParasiticsSrc()
                == est::ParasiticsSrc::kGlobalRouting;
}

std::vector<std::unique_ptr<MoveCandidate>> RerouteGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  if (!isApplicable(target)) {
    return candidates;
  }

  sta::Pin* driver_pin = target.resolvedPin(resizer_);
  sta::Instance* driver_inst = target.inst(resizer_);
  if (driver_pin == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove: No driver pin");
    return candidates;
  }

  if (resizer_.dontTouch(driver_pin)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: pin/net/instance is \"don't touch\"",
               resizer_.network()->pathName(driver_pin));
    return candidates;
  }

  sta::Net* net = resizer_.network()->net(driver_pin);
  if (net == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: No net found for driver pin",
               resizer_.network()->pathName(driver_pin));
    return candidates;
  }

  odb::dbNet* db_net = resizer_.dbNetwork()->flatNet(net);
  if (db_net == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: No db net found",
               resizer_.network()->pathName(driver_pin));
    return candidates;
  }

  if (db_net->isSpecial()) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: Net is special",
               resizer_.network()->pathName(driver_pin));
    return candidates;
  }

  if (attempted_nets_.contains(db_net)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: Net already attempted in this phase",
               resizer_.network()->pathName(driver_pin));
    return candidates;
  }

  grt::GlobalRouter* global_router = resizer_.globalRouter();
  const int committed_reroutes = committer_.committedMoves(MoveType::kReroute);
  const bool needs_usage_refresh
      = layer_usage_cache_.empty()
        || (!usage_cache_from_initialized_ && global_router->isInitialized())
        || (last_usage_refresh_reroutes_ < 0)
        || (committed_reroutes - last_usage_refresh_reroutes_
            >= kLayerUsageRefreshInterval);
  if (needs_usage_refresh) {
    layer_usage_cache_ = global_router->getLayerUsageRatios();
    last_usage_refresh_reroutes_ = committed_reroutes;
    usage_cache_from_initialized_ = global_router->isInitialized();
  }

  const PromotedLayerEval eval = evaluateReroutePromotion(
      resizer_, db_net, layer_usage_cache_, target.activeScene(resizer_));
  if (eval.promoted_min_layer <= 0) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: No beneficial underutilized promotion "
               "layer above current route (threshold {:.1f}%)",
               resizer_.network()->pathName(driver_pin),
               100.0f * kMinPromotedReduction);
    return candidates;
  }

  candidates.push_back(std::make_unique<RerouteCandidate>(
      resizer_,
      target,
      driver_pin,
      driver_inst,
      db_net,
      eval.curr_resistance,
      eval.est_resistance,
      eval.promoted_min_layer,
      &attempted_nets_,
      [this](odb::dbNet* net) { recordAttempt(net); }));
  return candidates;
}

}  // namespace rsz
