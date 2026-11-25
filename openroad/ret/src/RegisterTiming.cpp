// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "RegisterTiming.h"

#include <algorithm>

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

namespace {

// nullptr is nominal, as in the delay calculator.
const sta::Pvt* tablePvt(sta::dbSta* sta, const sta::LibertyCell* cell)
{
  const sta::Pvt* pvt
      = sta->cmdScene()->sdc()->operatingConditions(sta::MinMax::max());
  return pvt != nullptr ? pvt
                        : cell->libertyLibrary()->defaultOperatingConditions();
}

}  // namespace

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
  float worst = -1.0f;
  for (const sta::TimingArcSet* set : cell->timingArcSets()) {
    if (set->role() != sta::TimingRole::regClkToQ()
        || (to != nullptr && set->to() != to)) {
      continue;
    }
    for (const sta::TimingArc* arc : set->arcs()) {
      const sta::GateTableModel* model
          = dynamic_cast<const sta::GateTableModel*>(arc->model());
      if (model == nullptr) {
        continue;
      }
      float delay = 0.0f;
      float slew = 0.0f;
      model->gateDelay(pvt, clock_slew, load, delay, slew);
      worst = std::max(worst, delay);
    }
  }
  return worst;
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
          = dynamic_cast<const sta::CheckTableModel*>(arc->model());
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
  sta::Graph* graph = sta->ensureGraph();
  const int ap = sta->cmdScene()->dcalcAnalysisPtIndex(sta::MinMax::max());
  sta::Vertex* vertex = nullptr;
  sta::Vertex* bidirect_drvr = nullptr;
  graph->pinVertices(pin, vertex, bidirect_drvr);
  if (vertex == nullptr) {
    return 0.0f;
  }
  const float rise
      = sta::delayAsFloat(graph->slew(vertex, sta::RiseFall::rise(), ap));
  const float fall
      = sta::delayAsFloat(graph->slew(vertex, sta::RiseFall::fall(), ap));
  return std::max(rise, fall);
}

}  // namespace ret
