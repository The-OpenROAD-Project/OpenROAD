// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "OptimizerTypes.hh"
#include "RepairSetupAnalysis.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"

namespace rsz {
namespace {

std::optional<double> finiteValue(const sta::Delay& value,
                                  const sta::StaState* sta)
{
  if (sta::delayInf(value, sta) || !std::isfinite(sta::delayAsFloat(value))) {
    return std::nullopt;
  }
  return sta::delayAsFloat(value);
}

std::string rfName(const sta::RiseFall* rf)
{
  return rf == sta::RiseFall::rise() ? "rise" : "fall";
}

std::string masterName(const sta::Pin* pin, sta::Network* network)
{
  const sta::LibertyCell* cell = network->libertyCell(network->instance(pin));
  return cell != nullptr ? cell->name() : "";
}

std::string rolesString(const std::set<std::string>& roles)
{
  std::string text;
  for (const std::string& role : roles) {
    if (!text.empty()) {
      text += '|';
    }
    text += role;
  }

  return text;
}

}  // namespace

void RepairSetupAnalysis::captureSelectedWindow(const Target& target)
{
  sta::Network* network = resizer_.network();
  const sta::Path* path = target.driverPath(resizer_);
  if (path != nullptr) {
    selected_rf_ = rfName(path->transition(resizer_.staState()));
    const sta::Path* previous = path->prevPath();
    if (previous != nullptr) {
      selected_input_ = network->pathName(previous->pin(resizer_.staState()));
    }
  }

  if (!target.estimator_context.has_value()) {
    return;
  }

  const EstimatorContext& context = *target.estimator_context;
  for (size_t index = 0; index < context.path_stages.size(); ++index) {
    const DelayStageState& stage = context.path_stages[index];
    const sta::Pin* input = network->findPin(
        network->instance(stage.driver_pin), stage.arc.inputPort()->name());
    selected_window_.push_back(
        {network->pathName(stage.driver_pin),
         input != nullptr ? network->pathName(input) : "",
         stage.arc.currentCell()->name(),
         stage.arc.scene->name(),
         rfName(stage.arc.inputRiseFall()),
         rfName(stage.arc.outputRiseFall()),
         static_cast<int>(index) == context.target_stage_index,
         stage.input_slew,
         stage.load_cap,
         stage.current_model_delay,
         stage.current_delay,
         stage.current_wire_delay,
         stage.current_slew});
    AnalysisWindowStage& recorded = selected_window_.back();
    const sta::TimingArcSet* arc_set = stage.arc.ref_arc->set();
    recorded.role = arc_set->role()->to_string();
    recorded.condition
        = arc_set->cond() != nullptr ? arc_set->cond()->to_string() : "";
    recorded.mode_name = arc_set->modeName();
    recorded.mode_value = arc_set->modeValue();
  }
}

void RepairSetupAnalysis::watchPin(const sta::Pin* pin, const std::string& role)
{
  sta::Network* network = resizer_.network();
  if (pin != nullptr && !network->direction(pin)->isPowerGround()) {
    watched_pins_[network->pathName(pin)].insert(role);
  }
}

void RepairSetupAnalysis::watchNeighborhood()
{
  sta::Network* network = resizer_.network();
  const sta::Pin* target = network->findPin(target_pin_name_);
  if (target == nullptr) {
    return;
  }

  const sta::Instance* instance = network->instance(target);
  std::unique_ptr<sta::InstancePinIterator> pins(
      network->pinIterator(instance));
  while (pins->hasNext()) {
    const sta::Pin* pin = pins->next();
    if (network->direction(pin)->isPowerGround()) {
      continue;
    }

    watchPin(pin, network->isLoad(pin) ? "target_input" : "target_output");
    const sta::Net* net = network->net(pin);
    if (net != nullptr) {
      watched_nets_.insert(network->pathName(net));
    }
  }

  // Include every incident input net, its drivers and direct sibling outputs.
  for (const std::string& name : watched_nets_) {
    const sta::Net* net = network->findNet(name);
    if (net == nullptr) {
      continue;
    }

    std::unique_ptr<sta::NetConnectedPinIterator> connected(
        network->connectedPinIterator(net));
    while (connected->hasNext()) {
      const sta::Pin* pin = connected->next();
      const bool driver = network->isDriver(pin);
      watchPin(pin, driver ? "incident_driver" : "incident_load");
      const sta::Instance* other = network->instance(pin);
      if (network->isTopLevelPort(pin) || other == instance) {
        continue;
      }

      std::unique_ptr<sta::InstancePinIterator> neighbors(
          network->pinIterator(other));
      while (neighbors->hasNext()) {
        const sta::Pin* neighbor = neighbors->next();
        if (driver || network->isDriver(neighbor)) {
          watchPin(neighbor, driver ? "fanin_cell" : "sibling_output");
        }
      }
    }
  }
}

AnalysisPathState RepairSetupAnalysis::capturePath(
    const sta::Path* path,
    const std::string& kind) const
{
  const sta::StaState* sta = resizer_.staState();
  sta::Network* network = resizer_.network();
  sta::PathExpanded expanded(path, true, sta);
  AnalysisPathState result;
  result.kind = kind;
  result.scene = path->scene(sta)->name();
  result.rf = rfName(path->transition(sta));
  result.sense = path->minMax(sta) == sta::MinMax::max() ? "max" : "min";
  result.startpoint = network->pathName(expanded.startPath()->pin(sta));
  result.end_pin = network->pathName(path->pin(sta));
  result.launch_clock
      = path->clock(sta) != nullptr ? path->clock(sta)->name() : "";
  result.slack = finiteValue(path->slack(sta), sta);
  result.required = finiteValue(path->required(), sta);
  for (size_t index = 0; index < expanded.size(); ++index) {
    const sta::Path* step = expanded.path(index);
    const sta::Pin* pin = step->pin(sta);
    const sta::Net* net = network->net(pin);
    const sta::Edge* edge = step->prevEdge(sta);
    AnalysisPathPoint point;
    point.pin = network->pathName(pin);
    point.master = masterName(pin, network);
    point.net = net != nullptr ? network->pathName(net) : "";
    point.rf = rfName(step->transition(sta));
    point.role = edge != nullptr ? edge->role()->to_string() : "start";
    point.arrival = finiteValue(step->arrival(), sta);
    point.slew = finiteValue(step->slew(sta), sta);
    if (edge != nullptr && step->prevArc(sta) != nullptr) {
      point.delay = finiteValue(
          resizer_.graph()->arcDelay(
              edge, step->prevArc(sta), step->dcalcAnalysisPtIndex(sta)),
          sta);
    }

    result.points.push_back(std::move(point));
  }

  return result;
}

AnalysisPinState RepairSetupAnalysis::captureElectrical(
    const sta::Pin* pin,
    const std::set<std::string>& roles,
    const sta::Scene* scene,
    const sta::RiseFall* rf,
    const sta::MinMax* sense) const
{
  sta::Network* network = resizer_.network();
  AnalysisPinState state;
  state.pin = network->pathName(pin);
  state.instance = network->pathName(network->instance(pin));
  state.master = masterName(pin, network);
  const sta::Net* net = network->net(pin);
  state.net = net != nullptr ? network->pathName(net) : "";
  state.role = rolesString(roles);
  state.scene = scene->name();
  state.rf = rfName(rf);
  state.sense = sense == sta::MinMax::max() ? "max" : "min";
  const sta::LibertyPort* port = network->libertyPort(pin);
  if (port != nullptr) {
    port = port->scenePort(scene->libertyIndex(sense));
  }

  if (port != nullptr && network->isLoad(pin)) {
    state.input_cap = port->capacitance(rf, sense);
  }

  if (network->isDriver(pin) && net != nullptr
      && resizer_.graph()->pinDrvrVertex(pin) != nullptr) {
    float pin_cap, wire_cap, fanout;
    bool has_net_load;
    resizer_.sta()->graphDelayCalc()->netCaps(
        pin, rf, scene, sense, pin_cap, wire_cap, fanout, has_net_load);

    // Include extracted RC, respecting set_load precedence and Pi pin caps.
    resizer_.sta()->graphDelayCalc()->loadCap(
        pin, rf, scene, sense, pin_cap, wire_cap);
    state.pin_cap = pin_cap;
    state.wire_cap = wire_cap;
    state.fanout = fanout;
    if (port != nullptr) {
      state.drive_resistance = port->driveResistance(rf, sta::MinMax::max());
      state.drive_resistance_max = port->driveResistance();
    }
  }

  return state;
}

void RepairSetupAnalysis::capturePinStates(AnalysisSnapshot& snapshot) const
{
  sta::Network* network = resizer_.network();
  const sta::StaState* sta = resizer_.staState();
  for (const std::pair<const std::string, std::set<std::string>>& watched :
       watched_pins_) {
    const sta::Pin* pin = network->findPin(watched.first);
    if (pin == nullptr) {
      continue;
    }

    sta::Vertex* vertex;
    sta::Vertex* bidirect;
    resizer_.graph()->pinVertices(pin, vertex, bidirect);
    using PathKey = std::tuple<std::string, int, bool, bool>;
    std::map<PathKey, const sta::Path*> paths;
    for (sta::Vertex* observed : {vertex, bidirect}) {
      if (observed == nullptr) {
        continue;
      }

      sta::VertexPathIterator iterator(observed, sta);
      while (iterator.hasNext()) {
        const sta::Path* path = iterator.next();
        if (path->minMax(sta) != resizer_.maxAnalysisMode()
            && !path->isClock(sta)) {
          continue;
        }

        const PathKey key(path->scene(sta)->name(),
                          path->rfIndex(sta),
                          path->minMax(sta) == sta::MinMax::max(),
                          path->isClock(sta));
        const sta::Path*& best = paths[key];
        const std::optional<double> slack = finiteValue(path->slack(sta), sta);
        const std::optional<double> best_slack
            = best != nullptr ? finiteValue(best->slack(sta), sta)
                              : std::nullopt;
        if (best == nullptr
            || (slack.has_value()
                && (!best_slack.has_value() || *slack < *best_slack))
            || (!slack.has_value() && !best_slack.has_value()
                && (path->minMax(sta) == sta::MinMax::max()
                        ? path->arrival() > best->arrival()
                        : path->arrival() < best->arrival()))) {
          best = path;
        }
      }
    }

    for (const std::pair<const PathKey, const sta::Path*>& entry : paths) {
      const sta::Path* path = entry.second;
      const sta::RiseFall* rf = path->transition(sta);
      const sta::Scene* scene = path->scene(sta);
      const sta::MinMax* sense = path->minMax(sta);
      AnalysisPinState state
          = captureElectrical(pin, watched.second, scene, rf, sense);
      state.clock = path->isClock(sta);
      state.launch_clock
          = path->clock(sta) != nullptr ? path->clock(sta)->name() : "";
      state.arrival = finiteValue(path->arrival(), sta);
      state.required = finiteValue(path->required(), sta);
      state.slack = finiteValue(path->slack(sta), sta);
      state.slew = finiteValue(path->slew(sta), sta);
      if (watched.first == target_pin_name_) {
        AnalysisPathState prefix = capturePath(path, "target_prefix");
        state.startpoint = prefix.startpoint;
        snapshot.paths.push_back(std::move(prefix));
      } else if (watched.first == endpoint_pin_name_ && !state.clock) {
        AnalysisPathState endpoint
            = capturePath(path, "selected_endpoint_datapath");
        state.startpoint = endpoint.startpoint;
        snapshot.paths.push_back(std::move(endpoint));
      } else if (state.clock && watched.second.contains("endpoint_cell")) {
        AnalysisPathState clock = capturePath(path, "endpoint_clockpath");
        state.startpoint = clock.startpoint;
        snapshot.paths.push_back(std::move(clock));
      } else {
        const sta::PathExpanded prefix(path, sta);
        state.startpoint = network->pathName(prefix.startPath()->pin(sta));
      }

      snapshot.pins.push_back(std::move(state));
    }

    if (paths.empty()) {
      for (const sta::Scene* scene : resizer_.staState()->scenes()) {
        for (const sta::RiseFall* rf :
             {sta::RiseFall::rise(), sta::RiseFall::fall()}) {
          AnalysisPinState state = captureElectrical(
              pin, watched.second, scene, rf, resizer_.maxAnalysisMode());
          state.role += "|electrical_only";
          snapshot.pins.push_back(std::move(state));
        }
      }
    }
  }
}

void RepairSetupAnalysis::captureArcStates(AnalysisSnapshot& snapshot) const
{
  sta::Network* network = resizer_.network();
  sta::Graph* graph = resizer_.graph();
  for (const std::pair<const std::string, std::set<std::string>>& watched :
       watched_pins_) {
    const sta::Pin* pin = network->findPin(watched.first);
    sta::Vertex* vertex = pin != nullptr ? graph->pinLoadVertex(pin) : nullptr;
    if (vertex == nullptr) {
      continue;
    }

    sta::VertexOutEdgeIterator edges(vertex, graph);
    while (edges.hasNext()) {
      const sta::Edge* edge = edges.next();
      if (edge->isWire() || edge->role()->isTimingCheck()) {
        continue;
      }

      const sta::Pin* output = edge->to(graph)->pin();
      for (const sta::TimingArc* arc : edge->timingArcSet()->arcs()) {
        const sta::RiseFall* from_rf = arc->fromEdge()->asRiseFall();
        const sta::RiseFall* to_rf = arc->toEdge()->asRiseFall();
        if (from_rf == nullptr || to_rf == nullptr) {
          continue;
        }

        for (const sta::Scene* scene : resizer_.staState()->scenes()) {
          snapshot.arcs.push_back(
              {watched.first,
               network->pathName(output),
               masterName(pin, network),
               scene->name(),
               rfName(from_rf),
               rfName(to_rf),
               edge->role()->to_string(),
               finiteValue(graph->arcDelay(edge,
                                           arc,
                                           scene->dcalcAnalysisPtIndex(
                                               resizer_.maxAnalysisMode())),
                           resizer_.staState()),
               arc});
          AnalysisArcState& recorded = snapshot.arcs.back();
          const sta::TimingArcSet* arc_set = arc->set();
          recorded.condition
              = arc_set->cond() != nullptr ? arc_set->cond()->to_string() : "";
          recorded.mode_name = arc_set->modeName();
          recorded.mode_value = arc_set->modeValue();
          recorded.default_condition = arc_set->isCondDefault();
        }
      }
    }
  }
}

AnalysisSnapshot RepairSetupAnalysis::snapshot()
{
  AnalysisSnapshot result;
  result.metrics = sample();
  watchNeighborhood();
  sta::Network* network = resizer_.network();
  const sta::Pin* target = network->findPin(target_pin_name_);
  if (target != nullptr) {
    result.target_instance = network->pathName(network->instance(target));
    result.target_master = masterName(target, network);
  }

  sta::Slack wns;
  sta::Vertex* worst;
  resizer_.sta()->worstSlack(resizer_.maxAnalysisMode(), wns, worst);
  const sta::Path* path = worst != nullptr
                              ? resizer_.sta()->vertexWorstSlackPath(
                                    worst, resizer_.maxAnalysisMode())
                              : nullptr;
  if (path != nullptr) {
    result.paths.push_back(capturePath(path, "global_endpoint_path"));
    for (const AnalysisPathPoint& point : result.paths.back().points) {
      watchPin(network->findPin(point.pin), "global_path");
    }
  }

  for (const sta::Pin* endpoint :
       {network->findPin(endpoint_pin_name_),
        path != nullptr ? path->pin(resizer_.staState()) : nullptr}) {
    if (endpoint == nullptr) {
      continue;
    }

    watchPin(endpoint, "endpoint_cell");
    if (!network->isTopLevelPort(endpoint)) {
      std::unique_ptr<sta::InstancePinIterator> pins(
          network->pinIterator(network->instance(endpoint)));
      while (pins->hasNext()) {
        watchPin(pins->next(), "endpoint_cell");
      }
    }
  }

  for (const std::string& name : watched_nets_) {
    const sta::Net* net = network->findNet(name);
    if (net == nullptr) {
      continue;
    }

    std::unique_ptr<sta::NetConnectedPinIterator> pins(
        network->connectedPinIterator(net));
    while (pins->hasNext()) {
      const sta::Pin* pin = pins->next();
      if (!network->direction(pin)->isPowerGround()) {
        result.connections.push_back(
            {name,
             network->pathName(pin),
             network->pathName(network->instance(pin)),
             network->isDriver(pin) ? "driver" : "load"});
      }
    }
  }

  capturePinStates(result);
  captureArcStates(result);
  return result;
}

}  // namespace rsz
