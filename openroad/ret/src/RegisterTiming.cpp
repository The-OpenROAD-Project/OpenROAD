// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "RegisterTiming.h"

#include <algorithm>
#include <optional>

#include "db_sta/dbSta.hh"
#include "sta/ClkNetwork.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/PocvMode.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"

namespace ret {

const sta::Pvt* tablePvt(sta::dbSta* sta,
                         const sta::LibertyCell* cell,
                         const sta::Instance* inst)
{
  const sta::Sdc* sdc = sta->cmdScene()->sdc();
  const sta::MinMax* max = sta::MinMax::max();
  const sta::Pvt* pvt = inst != nullptr ? sdc->pvt(inst, max) : nullptr;
  if (pvt == nullptr) {
    pvt = sdc->operatingConditions(max);
  }
  return pvt != nullptr ? pvt
                        : cell->libertyLibrary()->defaultOperatingConditions();
}

std::optional<TableArc> slowestTableArc(sta::dbSta* sta,
                                        const sta::TimingArcSet* set,
                                        const sta::Pvt* pvt,
                                        const float in_slew,
                                        const float load,
                                        std::optional<TableArc> slowest)
{
  const sta::Scene* scene = sta->cmdScene();
  for (const sta::TimingArc* arc : set->arcs()) {
    const sta::GateTableModel* model
        = arc->gateTableModel(scene, sta::MinMax::max());
    if (model == nullptr) {
      continue;
    }
    float delay = 0.0f;
    float slew = 0.0f;
    model->gateDelay(pvt, in_slew, load, delay, slew);
    if (!slowest || delay > slowest->delay) {
      slowest = TableArc{.delay = delay, .slew = slew};
    }
  }
  return slowest;
}

float registerClockToQ(sta::dbSta* sta,
                       const sta::LibertyCell* cell,
                       const sta::LibertyPort* to,
                       const float clock_slew,
                       const float load)
{
  if (cell == nullptr) {
    return -1.0f;
  }
  const sta::Pvt* pvt = tablePvt(sta, cell);
  const TableArc none{.delay = -1.0f, .slew = 0.0f};
  std::optional<TableArc> worst = none;
  for (const sta::TimingArcSet* set : cell->timingArcSets()) {
    if (set->role() != sta::TimingRole::regClkToQ()
        || (to != nullptr && set->to() != to)) {
      continue;
    }
    worst = slowestTableArc(sta, set, pvt, clock_slew, load, worst);
  }
  return worst.value_or(none).delay;
}

float registerSetupTime(sta::dbSta* sta,
                        const sta::LibertyCell* cell,
                        const float clock_slew,
                        const float data_slew)
{
  if (cell == nullptr) {
    return -1.0f;
  }
  const sta::Pvt* pvt = tablePvt(sta, cell);
  float worst = -1.0f;
  for (const sta::TimingArcSet* set : cell->timingArcSets()) {
    if (set->role() != sta::TimingRole::setup()) {
      continue;
    }
    for (const sta::TimingArc* arc : set->arcs()) {
      const sta::CheckTableModel* model
          = dynamic_cast<const sta::CheckTableModel*>(
              arc->checkModel(sta->cmdScene(), sta::MinMax::max()));
      if (model == nullptr) {
        continue;
      }
      worst = std::max(
          worst,
          sta::delayAsFloat(model->checkDelay(pvt,
                                              clock_slew,
                                              data_slew,
                                              0.0f,
                                              sta::MinMax::max(),
                                              sta::PocvMode::scalar)));
    }
  }
  return worst;
}

float clockPinSlew(sta::dbSta* sta, const sta::Pin* pin)
{
  const sta::Mode* mode = sta->cmdScene()->mode();
  sta->ensureClkNetwork(mode);
  const sta::ClkNetwork* clk_network = mode->clkNetwork();
  if (clk_network->isIdealClock(pin)) {
    const sta::MinMax* max = sta::MinMax::max();
    return std::max(clk_network->idealClkSlew(pin, sta::RiseFall::rise(), max),
                    clk_network->idealClkSlew(pin, sta::RiseFall::fall(), max));
  }
  return pinSlew(sta, pin);
}

float pinSlew(sta::dbSta* sta, const sta::Pin* pin)
{
  const sta::Graph* graph = sta->ensureGraph();
  sta::Vertex* vertex = graph->pinLoadVertex(pin);
  if (vertex == nullptr) {
    return 0.0f;
  }
  return sta::delayAsFloat(sta->slew(vertex,
                                     sta::RiseFallBoth::riseFall(),
                                     {sta->cmdScene()},
                                     sta::MinMax::max()));
}

}  // namespace ret
