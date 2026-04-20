// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "UnbufferGenerator.hh"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "UnbufferCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

namespace {

using utl::RSZ;

constexpr int kBufferRemovalMaxFanout = 10;

struct UnbufferSelectionContext
{
  Resizer& resizer;
  MoveCommitter& committer;
  const Target& target;
  float setup_slack_margin;

  UnbufferSelectionContext(Resizer& resizer,
                           MoveCommitter& committer,
                           const Target& target,
                           float setup_slack_margin)
      : resizer(resizer),
        committer(committer),
        target(target),
        setup_slack_margin(setup_slack_margin)
  {
  }

  std::unique_ptr<sta::PathExpanded> expanded;
  sta::Pin* drvr_pin{nullptr};
  sta::Instance* drvr{nullptr};
  sta::LibertyPort* drvr_port{nullptr};
  sta::LibertyCell* drvr_cell{nullptr};
  sta::Pin* prev_drvr_pin{nullptr};
  sta::Pin* drvr_input_pin{nullptr};
  const sta::Path* prev_drvr_path{nullptr};
  const sta::Scene* scene{nullptr};
  const sta::Scene* slack_scene{nullptr};
  const sta::MinMax* min_max{nullptr};
};

bool validTarget(const Target& target)
{
  // Buffer removal needs one upstream driver stage and one buffer stage on the
  // path.
  return target.isKind(TargetKind::kPathDriver)
         && target.endpoint_path != nullptr && target.path_index >= 2;
}

bool resolveDriverContext(UnbufferSelectionContext& ctx)
{
  // Resolve the buffer instance and its timing corner before any removal checks
  // run.
  ctx.drvr_pin = ctx.target.driver_pin;
  if (ctx.drvr_pin == nullptr) {
    ctx.drvr_pin = ctx.target.endpoint_path->pin(ctx.resizer.staState());
  }
  if (ctx.drvr_pin == nullptr) {
    return false;
  }

  ctx.drvr = ctx.resizer.network()->instance(ctx.drvr_pin);
  ctx.drvr_port = ctx.resizer.network()->libertyPort(ctx.drvr_pin);
  ctx.drvr_cell
      = ctx.drvr_port != nullptr ? ctx.drvr_port->libertyCell() : nullptr;
  ctx.scene = ctx.target.endpoint_path->scene(ctx.resizer.sta());
  ctx.slack_scene = ctx.scene;
  ctx.min_max = ctx.target.endpoint_path->minMax(ctx.resizer.sta());
  return ctx.drvr != nullptr && ctx.drvr_cell != nullptr
         && ctx.drvr_cell->isBuffer() && ctx.scene != nullptr
         && ctx.min_max != nullptr;
}

bool loadExpandedPath(UnbufferSelectionContext& ctx)
{
  const sta::Path* drvr_input_path = ctx.target.inputPath(ctx.resizer);
  ctx.prev_drvr_path = ctx.target.prevDriverPath(ctx.resizer);
  if (drvr_input_path == nullptr || ctx.prev_drvr_path == nullptr) {
    return false;
  }

  sta::Vertex* drvr_input_vertex = drvr_input_path->vertex(ctx.resizer.sta());
  sta::Vertex* prev_drvr_vertex = ctx.prev_drvr_path->vertex(ctx.resizer.sta());
  ctx.drvr_input_pin
      = drvr_input_vertex != nullptr ? drvr_input_vertex->pin() : nullptr;
  ctx.prev_drvr_pin
      = prev_drvr_vertex != nullptr ? prev_drvr_vertex->pin() : nullptr;
  return ctx.drvr_input_pin != nullptr && ctx.prev_drvr_pin != nullptr;
}

bool passesMoveConflictGuard(const UnbufferSelectionContext& ctx)
{
  std::string reason;
  if (!ctx.committer.hasBlockingBufferRemovalMove(ctx.drvr, reason)) {
    return true;
  }

  debugPrint(ctx.resizer.logger(),
             RSZ,
             "unbuffer_move",
             4,
             "buffer {} is not removed because {}",
             ctx.resizer.dbNetwork()->name(ctx.drvr),
             reason);
  return false;
}

bool passesFanoutGuard(const UnbufferSelectionContext& ctx)
{
  float fanout;
  float limit;
  float slack;
  ctx.resizer.sta()->checkFanout(ctx.prev_drvr_pin,
                                 ctx.scene->mode(),
                                 ctx.resizer.maxAnalysisMode(),
                                 fanout,
                                 limit,
                                 slack);

  const float new_fanout = fanout + ctx.target.fanout - 1;
  if (limit > 0.0) {
    if (new_fanout <= limit) {
      return true;
    }

    debugPrint(
        ctx.resizer.logger(),
        RSZ,
        "unbuffer_move",
        2,
        "buffer {} is not removed because of max fanout limit of {} at {}",
        ctx.resizer.dbNetwork()->name(ctx.drvr),
        limit,
        ctx.resizer.network()->pathName(ctx.prev_drvr_pin));
    return false;
  }

  if (new_fanout <= kBufferRemovalMaxFanout) {
    return true;
  }

  debugPrint(
      ctx.resizer.logger(),
      RSZ,
      "unbuffer_move",
      2,
      "buffer {} is not removed because of default fanout limit of {} at {}",
      ctx.resizer.dbNetwork()->name(ctx.drvr),
      kBufferRemovalMaxFanout,
      ctx.resizer.network()->pathName(ctx.prev_drvr_pin));
  return false;
}

bool passesCapGuard(UnbufferSelectionContext& ctx)
{
  float cap;
  float max_cap;
  float cap_slack;
  const sta::Scene* corner;
  const sta::RiseFall* tr;
  ctx.resizer.sta()->checkCapacitance(ctx.prev_drvr_pin,
                                      ctx.resizer.sta()->scenes(),
                                      ctx.resizer.maxAnalysisMode(),
                                      cap,
                                      max_cap,
                                      cap_slack,
                                      tr,
                                      corner);
  ctx.slack_scene = corner;
  if (max_cap <= 0.0 || corner == nullptr) {
    return true;
  }

  const float drvr_cap = ctx.resizer.sta()->graphDelayCalc()->loadCap(
      ctx.drvr_pin, corner, ctx.resizer.maxAnalysisMode());
  sta::LibertyPort* buffer_input_port = nullptr;
  sta::LibertyPort* buffer_output_port = nullptr;
  ctx.drvr_cell->bufferPorts(buffer_input_port, buffer_output_port);
  const float new_cap
      = cap + drvr_cap - ctx.resizer.portCapacitance(buffer_input_port, corner);
  if (new_cap <= max_cap) {
    return true;
  }

  debugPrint(ctx.resizer.logger(),
             RSZ,
             "unbuffer_move",
             2,
             "buffer {} is not removed because of max cap limit of {} at {}",
             ctx.resizer.dbNetwork()->name(ctx.drvr),
             max_cap,
             ctx.resizer.network()->pathName(ctx.prev_drvr_pin));
  return false;
}

bool passesSlackGuard(const UnbufferSelectionContext& ctx)
{
  SlackEstimatorParams params(ctx.setup_slack_margin, ctx.slack_scene);
  params.driver_pin = ctx.drvr_pin;
  params.prev_driver_pin = ctx.prev_drvr_pin;
  params.driver_input_pin = ctx.drvr_input_pin;
  params.driver = ctx.drvr;
  params.driver_path = ctx.target.driverPath(ctx.resizer);
  params.prev_driver_path = ctx.prev_drvr_path;
  params.driver_cell = ctx.drvr_cell;
  return ctx.resizer.estimatedSlackOK(params);
}

bool isEligible(UnbufferSelectionContext& ctx)
{
  debugPrint(ctx.resizer.logger(),
             RSZ,
             "unbuffer_move",
             4,
             "checking unbuffer eligibility for {}",
             ctx.target.driver_pin != nullptr
                 ? ctx.resizer.network()->pathName(ctx.target.driver_pin)
                 : "");
  if (!validTarget(ctx.target)) {
    return false;
  }
  if (!resolveDriverContext(ctx)) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "unbuffer_move",
               4,
               "buffer target {} is not removable because driver context did "
               "not resolve",
               ctx.target.driver_pin != nullptr
                   ? ctx.resizer.network()->pathName(ctx.target.driver_pin)
                   : "");
    return false;
  }
  if (!loadExpandedPath(ctx)) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "unbuffer_move",
               4,
               "buffer {} is not removed because path context did not resolve",
               ctx.resizer.dbNetwork()->name(ctx.drvr));
    return false;
  }
  if (!passesMoveConflictGuard(ctx) || !passesFanoutGuard(ctx)
      || !passesCapGuard(ctx)) {
    return false;
  }
  if (!passesSlackGuard(ctx)) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "unbuffer_move",
               4,
               "buffer {} is not removed because estimated slack is not OK",
               ctx.resizer.dbNetwork()->name(ctx.drvr));
    return false;
  }
  if (!ctx.resizer.canRemoveBuffer(ctx.drvr, true)) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "unbuffer_move",
               4,
               "buffer {} is not removed because canRemoveBuffer rejected it",
               ctx.resizer.dbNetwork()->name(ctx.drvr));
    return false;
  }
  return true;
}

}  // namespace

UnbufferGenerator::UnbufferGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool UnbufferGenerator::isApplicable(const Target& target) const
{
  return target.isKind(TargetKind::kPathDriver)
         && target.endpoint_path != nullptr && target.path_index >= 0;
}

std::vector<std::unique_ptr<MoveCandidate>> UnbufferGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  UnbufferSelectionContext ctx{
      resizer_,
      committer_,
      target,
      static_cast<float>(run_config_.setup_slack_margin)};
  if (!isEligible(ctx)) {
    return candidates;
  }

  candidates.push_back(
      std::make_unique<UnbufferCandidate>(resizer_, target, ctx.drvr));
  return candidates;
}

}  // namespace rsz
