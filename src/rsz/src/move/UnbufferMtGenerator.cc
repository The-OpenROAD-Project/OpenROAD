// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "UnbufferMtGenerator.hh"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "UnbufferMtCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

namespace {

using utl::RSZ;

constexpr int kBufferRemovalMaxFanout = 10;

// The buffer, its upstream driver and the scene the removal checks read.
struct UnbufferSelectionContext
{
  Resizer& resizer;
  const Target& target;
  float setup_slack_margin;

  UnbufferSelectionContext(Resizer& resizer,
                           const Target& target,
                           float setup_slack_margin)
      : resizer(resizer), target(target), setup_slack_margin(setup_slack_margin)
  {
  }

  sta::Pin* drvr_pin{nullptr};
  sta::Instance* drvr{nullptr};
  sta::LibertyPort* drvr_port{nullptr};
  sta::LibertyCell* drvr_cell{nullptr};
  sta::Pin* prev_drvr_pin{nullptr};
  sta::Pin* drvr_input_pin{nullptr};
  const sta::Path* prev_drvr_path{nullptr};
  const sta::Scene* scene{nullptr};
  // The scene of the upstream driver's worst capacitance check; isEligible()
  // sets it for the slack guard and the gain estimate.
  const sta::Scene* slack_scene{nullptr};
  const sta::MinMax* min_max{nullptr};
};

bool resolveDriverContext(UnbufferSelectionContext& ctx)
{
  // The buffer instance and its timing corner; isApplicable() checked that
  // the driver is a buffer.
  ctx.drvr_pin = ctx.target.driver_pin;
  ctx.drvr = ctx.resizer.network()->instance(ctx.drvr_pin);
  ctx.drvr_port = ctx.resizer.network()->libertyPort(ctx.drvr_pin);
  ctx.drvr_cell = ctx.drvr_port->libertyCell();
  ctx.scene = ctx.target.activeScene(ctx.resizer);
  ctx.min_max = ctx.target.minMax(ctx.resizer);
  return ctx.scene != nullptr && ctx.min_max != nullptr;
}

// The buffer's input pin and the upstream driver on the target path.
bool resolvePathContext(UnbufferSelectionContext& ctx)
{
  const sta::Path* drvr_path = ctx.target.driverPath(ctx.resizer);
  const sta::TimingArc* in_arc
      = drvr_path != nullptr ? drvr_path->prevArc(ctx.resizer.staState())
                             : nullptr;
  const sta::LibertyPort* in_lib_port
      = in_arc != nullptr ? in_arc->from() : nullptr;
  if (in_lib_port == nullptr) {
    return false;
  }

  ctx.prev_drvr_path = ctx.target.prevDriverPath(ctx.resizer);
  if (ctx.prev_drvr_path == nullptr) {
    return false;
  }

  sta::Vertex* prev_drvr_vertex = ctx.prev_drvr_path->vertex(ctx.resizer.sta());
  ctx.drvr_input_pin = ctx.resizer.network()->findPin(ctx.drvr, in_lib_port);
  ctx.prev_drvr_pin
      = prev_drvr_vertex != nullptr ? prev_drvr_vertex->pin() : nullptr;
  return ctx.drvr_input_pin != nullptr && ctx.prev_drvr_pin != nullptr;
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
  if (limit > 0.0 && limit < sta::INF) {
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

// Whether the upstream driver stays within its max-cap limit after the merge.
// corner returns the scene of the driver's worst capacitance check, null when
// it has no limit.
bool passesCapGuard(const UnbufferSelectionContext& ctx,
                    const sta::Scene*& corner)
{
  float cap;
  float max_cap;
  float cap_slack;
  const sta::RiseFall* tr;
  ctx.resizer.sta()->checkCapacitance(ctx.prev_drvr_pin,
                                      ctx.resizer.sta()->scenes(),
                                      ctx.resizer.maxAnalysisMode(),
                                      cap,
                                      max_cap,
                                      cap_slack,
                                      tr,
                                      corner);
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

// The delay the removal saves: the buffer delay less the upstream driver's
// slowdown from the merged load and the longer wire.  A repeater on a long
// wire thus estimates at or below zero and is not offered.
float estimateRemovalGain(const UnbufferSelectionContext& ctx)
{
  Resizer& resizer = ctx.resizer;
  sta::ArcDelay old_delay[sta::RiseFall::index_count];
  sta::ArcDelay new_delay[sta::RiseFall::index_count];
  sta::Slew old_slew[sta::RiseFall::index_count];
  sta::Slew new_slew[sta::RiseFall::index_count];
  float old_cap;
  float new_cap;
  if (!resizer.computeNewDelaysSlews(ctx.prev_drvr_pin,
                                     ctx.drvr,
                                     ctx.slack_scene,
                                     old_delay,
                                     new_delay,
                                     old_slew,
                                     new_slew,
                                     old_cap,
                                     new_cap)) {
    return 0.0f;
  }

  sta::Sta* sta = resizer.sta();
  const int prev_rf = ctx.prev_drvr_path->transition(sta)->index();
  const float slowdown = new_delay[prev_rf] - old_delay[prev_rf];
  const sta::Path* drvr_path = ctx.target.driverPath(resizer);
  const float saved = resizer.bufferDelay(
      ctx.drvr_cell,
      drvr_path->transition(sta),
      sta->graphDelayCalc()->loadCap(
          ctx.drvr_pin, ctx.slack_scene, sta::MinMax::max()),
      ctx.slack_scene,
      sta::MinMax::max());

  // The Elmore delay of the extra wire the upstream driver drives after the
  // merge, beyond its longest load today.
  sta::dbNetwork* db_network = resizer.dbNetwork();
  const odb::Point prev_loc = db_network->location(ctx.prev_drvr_pin);
  const int old_dist = resizer.maxLoadManhattenDistance(
      resizer.graph()->pinDrvrVertex(ctx.prev_drvr_pin));
  int new_dist = old_dist;
  sta::Net* out_net = resizer.network()->net(ctx.drvr_pin);
  std::unique_ptr<sta::NetConnectedPinIterator> pin_iter(
      resizer.network()->connectedPinIterator(out_net));
  while (pin_iter->hasNext()) {
    const sta::Pin* load_pin = pin_iter->next();
    if (load_pin == ctx.drvr_pin || !resizer.network()->isLoad(load_pin)) {
      continue;
    }

    const int dist = odb::Point::manhattanDistance(
        prev_loc, db_network->location(load_pin));
    new_dist = std::max(new_dist, dist);
  }

  const double wire_res
      = resizer.estimateParasitics()->wireSignalResistance(ctx.slack_scene);
  const double wire_cap
      = resizer.estimateParasitics()->wireSignalCapacitance(ctx.slack_scene);

  auto wire_delay = [&](const int dist) {
    const double length = resizer.dbuToMeters(dist);
    return 0.5 * (length * wire_res) * (length * wire_cap);
  };
  const float longer_wire = wire_delay(new_dist) - wire_delay(old_dist);
  debugPrint(resizer.logger(),
             RSZ,
             "unbuffer_move",
             3,
             "buffer {} removal estimate: saved {} slowdown {} wire {} -> {}",
             db_network->name(ctx.drvr),
             saved,
             slowdown,
             longer_wire,
             saved - slowdown - longer_wire);

  return saved - slowdown - longer_wire;
}

bool isEligible(UnbufferSelectionContext& ctx)
{
  debugPrint(ctx.resizer.logger(),
             RSZ,
             "unbuffer_move",
             4,
             "checking unbuffer eligibility for {}",
             ctx.resizer.network()->pathName(ctx.target.driver_pin));
  if (!resolveDriverContext(ctx)) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "unbuffer_move",
               4,
               "buffer target {} is not removable because driver context did "
               "not resolve",
               ctx.resizer.network()->pathName(ctx.target.driver_pin));
    return false;
  }
  if (!resolvePathContext(ctx)) {
    debugPrint(ctx.resizer.logger(),
               RSZ,
               "unbuffer_move",
               4,
               "buffer {} is not removed because path context did not resolve",
               ctx.resizer.dbNetwork()->name(ctx.drvr));
    return false;
  }
  const sta::Scene* cap_corner = nullptr;
  if (!passesFanoutGuard(ctx) || !passesCapGuard(ctx, cap_corner)) {
    return false;
  }
  // The slack guard and the gain estimate read the scene of the upstream
  // driver's worst capacitance check.
  ctx.slack_scene = cap_corner;
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

UnbufferMtGenerator::UnbufferMtGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool UnbufferMtGenerator::isApplicable(const Target& target) const
{
  // The path has the buffer and the stage that drives it.
  if (!MoveGenerator::isApplicable(target) || target.path_index < 2) {
    return false;
  }

  sta::Network* network = resizer_.network();
  const sta::LibertyPort* port = network->libertyPort(target.driver_pin);
  sta::Instance* buffer = network->instance(target.driver_pin);
  if (port == nullptr || !port->libertyCell()->isBuffer()
      || buffer == nullptr) {
    return false;
  }

  // The moves that block the serial removal, except a resize: batches size
  // most buffers on the worst paths, and the batch STA judges their removal.
  for (const MoveType type : {MoveType::kSwapPins,
                              MoveType::kClone,
                              MoveType::kSplitLoad,
                              MoveType::kBuffer}) {
    if (committer_.hasMoves(type, buffer)) {
      return false;
    }
  }
  return true;
}

void UnbufferMtGenerator::prepareInputs(Target& target) const
{
  UnbufferSelectionContext ctx{
      resizer_, target, static_cast<float>(run_config_.setup_slack_margin)};
  if (!isEligible(ctx)) {
    return;
  }

  target.unbuffer_gain = estimateRemovalGain(ctx);
}

std::vector<std::unique_ptr<MoveCandidate>> UnbufferMtGenerator::generate(
    const Target& target)
{
  // The batch ranks the removal by its estimate and skips one that estimates
  // as a loss.
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  if (!target.unbuffer_gain.has_value() || *target.unbuffer_gain <= 0.0f) {
    return candidates;
  }

  candidates.push_back(std::make_unique<UnbufferMtCandidate>(resizer_, target));
  return candidates;
}

}  // namespace rsz
