// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ord/Timing.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ord/Design.h"
#include "ord/OpenRoad.hh"
#include "ord/Tech.h"
#include "rsz/Resizer.hh"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathGroup.hh"
#include "sta/PowerClass.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/StringUtil.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "utl/Logger.h"

namespace ord {

Timing::Timing(Design* design) : design_(design)
{
}

sta::dbSta* Timing::getSta()
{
  return design_->getTech()->getSta();
}

std::pair<odb::dbITerm*, odb::dbBTerm*> Timing::staToDBPin(const sta::Pin* pin)
{
  sta::dbNetwork* db_network = getSta()->getDbNetwork();
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::dbModITerm* moditerm;
  db_network->staToDb(pin, iterm, bterm, moditerm);
  return std::make_pair(iterm, bterm);
}

bool Timing::isEndpoint(odb::dbITerm* db_pin)
{
  sta::Pin* sta_pin = getSta()->getDbNetwork()->dbToSta(db_pin);
  return isEndpoint(sta_pin);
}

bool Timing::isEndpoint(odb::dbBTerm* db_pin)
{
  sta::Pin* sta_pin = getSta()->getDbNetwork()->dbToSta(db_pin);
  return isEndpoint(sta_pin);
}

bool Timing::isEndpoint(sta::Pin* sta_pin)
{
  auto search = getSta()->search();
  auto vertex_array = vertices(sta_pin);
  for (auto vertex : vertex_array) {
    if (vertex != nullptr && search->isEndpoint(vertex)) {
      return true;
    }
  }
  return false;
}

float Timing::slewAllCorners(sta::Vertex* vertex, const sta::MinMax* minmax)
{
  auto sta = getSta();
  return sta::delayAsFloat(
      sta->slew(vertex, sta::RiseFallBoth::riseFall(), sta->scenes(), minmax));
}

float Timing::getPinSlew(odb::dbITerm* db_pin, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlew(sta_pin, minmax);
}

float Timing::getPinSlew(odb::dbBTerm* db_pin, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlew(sta_pin, minmax);
}

float Timing::getPinSlew(sta::Pin* sta_pin, MinMax minmax)
{
  auto vertex_array = vertices(sta_pin);
  float pin_slew = (minmax == Max) ? -sta::INF : sta::INF;
  for (auto vertex : vertex_array) {
    if (vertex != nullptr) {
      const float pin_slew_temp = slewAllCorners(vertex, getMinMax(minmax));
      pin_slew = (minmax == Max) ? std::max(pin_slew, pin_slew_temp)
                                 : std::min(pin_slew, pin_slew_temp);
    }
  }
  return pin_slew;
}

sta::Network* Timing::cmdLinkedNetwork()
{
  sta::Network* network = getSta()->cmdNetwork();
  if (network->isLinked()) {
    return network;
  }

  design_->getLogger()->error(utl::ORD, 104, "STA network is not linked.");
}

sta::Graph* Timing::cmdGraph()
{
  cmdLinkedNetwork();
  return getSta()->ensureGraph();
}

std::array<sta::Vertex*, 2> Timing::vertices(const sta::Pin* pin)
{
  sta::Vertex *vertex, *vertex_bidirect_drvr;
  std::array<sta::Vertex*, 2> vertices;

  cmdGraph()->pinVertices(pin, vertex, vertex_bidirect_drvr);
  vertices[0] = vertex;
  vertices[1] = vertex_bidirect_drvr;
  return vertices;
}

bool Timing::isTimeInf(float time)
{
  return (time > 1e+10 || time < -1e+10);
}

float Timing::getPinArrivalTime(sta::Clock* clk,
                                const sta::RiseFall* clk_rf,
                                sta::Vertex* vertex,
                                const sta::RiseFall* rf)
{
  sta::dbSta* sta = getSta();
  (void) clk;
  (void) clk_rf;
  return sta::delayAsFloat(sta->arrival(
      vertex, rf->asRiseFallBoth(), sta->scenes(), sta::MinMax::max()));
}

sta::ClockSeq Timing::findClocksMatching(const char* pattern,
                                         bool regexp,
                                         bool nocase)
{
  auto sta = getSta();
  cmdLinkedNetwork();
  sta::PatternMatch matcher(pattern, regexp, nocase, sta->tclInterp());
  return sta->cmdMode()->sdc()->findClocksMatching(&matcher);
}

float Timing::getPinArrival(odb::dbITerm* db_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinArrival(sta_pin, rf, minmax);
}

float Timing::getPinArrival(odb::dbBTerm* db_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinArrival(sta_pin, rf, minmax);
}

float Timing::getPinArrival(sta::Pin* sta_pin, RiseFall rf, MinMax minmax)
{
  auto vertex_array = vertices(sta_pin);
  float delay = (minmax == Max) ? -sta::INF : sta::INF;
  float d1, d2;
  sta::Clock* default_arrival_clock
      = getSta()->cmdMode()->sdc()->defaultArrivalClock();
  for (auto vertex : vertex_array) {
    if (vertex == nullptr) {
      continue;
    }
    const sta::RiseFall* clk_r = sta::RiseFall::rise();
    const sta::RiseFall* clk_f = sta::RiseFall::fall();
    const sta::RiseFall* arrive_hold = (rf == Rise) ? clk_r : clk_f;
    d1 = getPinArrivalTime(nullptr, clk_r, vertex, arrive_hold);
    d2 = getPinArrivalTime(default_arrival_clock, clk_r, vertex, arrive_hold);
    delay = (minmax == Max) ? std::max({d1, d2, delay})
                            : std::min({d1, d2, delay});
    for (auto clk : findClocksMatching("*", false, false)) {
      d1 = getPinArrivalTime(clk, clk_r, vertex, arrive_hold);
      d2 = getPinArrivalTime(clk, clk_f, vertex, arrive_hold);
      delay = (minmax == Max) ? std::max({d1, d2, delay})
                              : std::min({d1, d2, delay});
    }
  }
  return delay;
}

std::vector<sta::Scene*> Timing::getCorners()
{
  auto& corners = getSta()->scenes();
  return {corners.begin(), corners.end()};
}

sta::Scene* Timing::cmdCorner()
{
  return getSta()->cmdScene();
}

sta::Scene* Timing::findCorner(const char* name)
{
  for (auto* corner : getCorners()) {
    if (strcmp(corner->name().c_str(), name) == 0) {
      return corner;
    }
  }

  return nullptr;
}

float Timing::getPinSlack(odb::dbITerm* db_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlack(sta_pin, rf, minmax);
}

float Timing::getPinSlack(odb::dbBTerm* db_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlack(sta_pin, rf, minmax);
}

float Timing::getPinSlack(sta::Pin* sta_pin, RiseFall rf, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  auto sta_rf = (rf == Rise) ? sta::RiseFall::rise() : sta::RiseFall::fall();
  return sta->slack(
      sta_pin, sta_rf->asRiseFallBoth(), sta->scenes(), getMinMax(minmax));
}

// I'd like to return a std::set but swig gave me way too much grief
// so I just copy the set to a vector.
std::vector<odb::dbMTerm*> Timing::getTimingFanoutFrom(odb::dbMTerm* input)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();

  odb::dbMaster* master = input->getMaster();
  sta::Cell* cell = network->dbToSta(master);
  if (!cell) {
    return {};
  }

  sta::LibertyCell* lib_cell = network->libertyCell(cell);
  if (!lib_cell) {
    return {};
  }

  sta::Port* port = network->dbToSta(input);
  sta::LibertyPort* lib_port = network->libertyPort(port);

  std::set<odb::dbMTerm*> outputs;
  for (auto arc_set : lib_cell->timingArcSets(lib_port, /* to */ nullptr)) {
    const sta::TimingRole* role = arc_set->role();
    if (role->isTimingCheck() || role->isAsyncTimingCheck()
        || role->isNonSeqTimingCheck() || role->isDataCheck()) {
      continue;
    }
    sta::LibertyPort* to_port = arc_set->to();
    odb::dbMTerm* to_mterm = master->findMTerm(to_port->name().c_str());
    if (to_mterm) {
      outputs.insert(to_mterm);
    }
  }
  return {outputs.begin(), outputs.end()};
}

const sta::MinMax* Timing::getMinMax(MinMax type)
{
  return type == Max ? sta::MinMax::max() : sta::MinMax::min();
}

float Timing::getNetCap(odb::dbNet* net, sta::Scene* corner, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::Net* sta_net = sta->getDbNetwork()->dbToSta(net);

  float pin_cap;
  float wire_cap;
  sta->connectedCap(sta_net, corner, getMinMax(minmax), pin_cap, wire_cap);
  return pin_cap + wire_cap;
}

float Timing::getPortCap(odb::dbITerm* pin, sta::Scene* corner, MinMax minmax)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Pin* sta_pin = network->dbToSta(pin);
  sta::LibertyPort* lib_port = network->libertyPort(sta_pin);
  return sta->capacitance(lib_port, corner, getMinMax(minmax));
}

float Timing::getMaxCapLimit(odb::dbMTerm* pin)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Port* port = network->dbToSta(pin);
  sta::LibertyPort* lib_port = network->libertyPort(port);
  sta::LibertyLibrary* lib = network->defaultLibertyLibrary();
  float max_cap = 0.0;
  bool max_cap_exists = false;
  if (!pin->getSigType().isSupply()) {
    lib_port->capacitanceLimit(sta::MinMax::max(), max_cap, max_cap_exists);
    if (!max_cap_exists) {
      lib->defaultMaxCapacitance(max_cap, max_cap_exists);
    }
  }
  return max_cap;
}

float Timing::getMaxSlewLimit(odb::dbMTerm* pin)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Port* port = network->dbToSta(pin);
  sta::LibertyPort* lib_port = network->libertyPort(port);
  sta::LibertyLibrary* lib = network->defaultLibertyLibrary();
  float max_slew = 0.0;
  bool max_slew_exists = false;
  if (!pin->getSigType().isSupply()) {
    lib_port->slewLimit(sta::MinMax::max(), max_slew, max_slew_exists);
    if (!max_slew_exists) {
      lib->defaultMaxSlew(max_slew, max_slew_exists);
    }
  }
  return max_slew;
}

float Timing::staticPower(odb::dbInst* inst, sta::Scene* corner)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();

  sta::Instance* sta_inst = network->dbToSta(inst);
  if (!sta_inst) {
    return 0.0;
  }
  sta::PowerResult power = sta->power(sta_inst, corner);
  return power.leakage();
}

float Timing::dynamicPower(odb::dbInst* inst, sta::Scene* corner)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();

  sta::Instance* sta_inst = network->dbToSta(inst);
  if (!sta_inst) {
    return 0.0;
  }
  sta::PowerResult power = sta->power(sta_inst, corner);
  return (power.internal() + power.switching());
}

void Timing::makeEquivCells()
{
  rsz::Resizer* resizer = design_->getResizer();
  resizer->makeEquivCells();
}

std::vector<odb::dbMaster*> Timing::equivCells(odb::dbMaster* master)
{
  sta::dbSta* sta = getSta();
  sta::dbNetwork* network = sta->getDbNetwork();
  sta::Cell* cell = network->dbToSta(master);
  std::vector<odb::dbMaster*> master_seq;
  if (cell) {
    sta::LibertyCell* libcell = network->libertyCell(cell);
    sta::LibertyCellSeq* equiv_cells = sta->equivCells(libcell);
    if (equiv_cells) {
      for (sta::LibertyCell* equiv_cell : *equiv_cells) {
        odb::dbMaster* equiv_master = network->staToDb(equiv_cell);
        master_seq.emplace_back(equiv_master);
      }
    } else {
      master_seq.emplace_back(master);
    }
  }
  return master_seq;
}
float Timing::getWorstSlack(MinMax minmax)
{
  sta::dbSta* sta = getSta();
  cmdLinkedNetwork();
  return sta->worstSlack(getMinMax(minmax));
}

float Timing::getTotalNegativeSlack(MinMax minmax)
{
  sta::dbSta* sta = getSta();
  cmdLinkedNetwork();
  return sta->totalNegativeSlack(getMinMax(minmax));
}

int Timing::getEndpointCount()
{
  sta::dbSta* sta = getSta();
  cmdLinkedNetwork();
  return sta->endpoints().size();
}

std::vector<EndpointSlack> Timing::getEndpointSlacks(MinMax minmax)
{
  sta::dbSta* sta = getSta();
  cmdLinkedNetwork();

  std::vector<EndpointSlack> result;
  for (sta::Vertex* vertex : sta->endpoints()) {
    const sta::Pin* pin = vertex->pin();
    float slack = sta->slack(
        pin, sta::RiseFallBoth::riseFall(), sta->scenes(), getMinMax(minmax));
    auto [iterm, bterm] = staToDBPin(pin);
    result.push_back({iterm, bterm, slack});
  }
  return result;
}

std::vector<ClockInfo> Timing::getClockInfo()
{
  sta::dbSta* sta = getSta();
  cmdLinkedNetwork();

  std::vector<ClockInfo> result;
  for (const sta::Clock* clk : sta->cmdMode()->sdc()->clocks()) {
    ClockInfo info;
    info.name = clk->name();
    info.period = clk->period();
    if (clk->waveform()) {
      info.waveform = *clk->waveform();
    }
    for (const sta::Pin* pin : clk->pins()) {
      auto [iterm, bterm] = staToDBPin(pin);
      if (iterm) {
        info.source_iterms.push_back(iterm);
      }
      if (bterm) {
        info.source_bterms.push_back(bterm);
      }
    }
    result.push_back(std::move(info));
  }
  return result;
}

std::vector<TimingPathInfo> Timing::getTimingPaths(MinMax minmax,
                                                   int max_paths,
                                                   float slack_threshold)
{
  sta::dbSta* sta = getSta();
  cmdLinkedNetwork();
  sta::dbNetwork* network = sta->getDbNetwork();

  const bool is_setup = (minmax == Max);
  sta::SceneSeq scenes = sta->scenes();
  sta::StringSeq group_names;

  sta->ensureGraph();
  sta->searchPreamble();

  sta::Search* search = sta->search();
  sta::PathEndSeq path_ends = search->findPathEnds(
      nullptr,  // from
      nullptr,  // thrus
      nullptr,  // to
      false,    // unconstrained
      scenes,
      is_setup ? sta::MinMaxAll::max() : sta::MinMaxAll::min(),
      max_paths,        // group_count
      1,                // endpoint_count (one per endpoint)
      true,             // unique_pins
      true,             // unique_edges
      -sta::INF,        // slack_min
      slack_threshold,  // slack_max
      true,             // sort_by_slack
      group_names,
      is_setup,   // setup
      !is_setup,  // hold
      false,      // recovery
      false,      // removal
      false,      // clk_gating_setup
      false);     // clk_gating_hold

  std::vector<TimingPathInfo> result;
  auto* graph = sta->graph();
  const sta::Sdc* sdc = sta->cmdScene()->sdc();
  sta::Mode* mode = sta->cmdScene()->mode();
  sta::GraphDelayCalc* gdc = sta->graphDelayCalc();
  sta::dbNetwork* db_network = sta->getDbNetwork();

  for (auto& path_end : path_ends) {
    TimingPathInfo path_info;
    sta::Path* path = path_end->path();

    path_info.slack = path_end->slack(sta);
    path_info.arrival = path_end->dataArrivalTime(sta);
    path_info.required = path_end->requiredTime(sta);
    path_info.skew = path_end->clkSkew(sta);

    auto* path_delay = path_end->pathDelay();
    path_info.path_delay = path_delay ? path_delay->delay() : 0.0f;

    auto* start_clk_edge = path_end->sourceClkEdge(sta);
    path_info.start_clock
        = start_clk_edge ? start_clk_edge->clock()->name() : "";

    auto* end_clk = path_end->targetClk(sta);
    path_info.end_clock = end_clk ? end_clk->name() : "";

    auto* path_group = path_end->pathGroup();
    path_info.path_group = path_group ? path_group->name() : "";

    // Expand path to get arc detail
    sta::PathExpanded expand(path, sta);
    float arrival_prev = 0.0f;
    float logic_delay_total = 0.0f;
    int logic_depth_count = 0;
    int max_fanout = 0;
    std::unordered_set<sta::Instance*> logic_insts;

    for (size_t i = 0; i < expand.size(); i++) {
      const auto* ref = expand.path(i);
      sta::Vertex* vertex = ref->vertex(sta);
      const sta::Pin* pin = vertex->pin();
      const bool is_rising = ref->transition(sta) == sta::RiseFall::rise();
      const float arr = sta::delayAsFloat(ref->arrival());
      const float slw = sta::delayAsFloat(ref->slew(sta));
      const float pin_delay = arr - arrival_prev;

      // Compute fanout
      int node_fanout = 0;
      sta::VertexOutEdgeIterator iter(vertex, graph);
      while (iter.hasNext()) {
        sta::Edge* edge = iter.next();
        if (edge->isWire()) {
          const sta::Pin* to_pin = edge->to(graph)->pin();
          if (network->isTopLevelPort(to_pin)) {
            sta::Port* port = network->port(to_pin);
            node_fanout += sdc->portExtFanout(port, sta::MinMax::max()) + 1;
          } else {
            node_fanout++;
          }
        }
      }
      max_fanout = std::max(node_fanout, max_fanout);

      // Compute load capacitance
      float cap = 0.0f;
      const bool is_driver = network->isDriver(pin);
      if (is_driver && i > 0) {
        cap = gdc->loadCap(
            pin, ref->transition(sta), ref->scene(sta), ref->minMax(sta));
      }

      // Determine master, net arcs, logic depth, and build arc info
      if (i > 0) {
        const auto* prev_ref = expand.path(i - 1);
        sta::Vertex* prev_vertex = prev_ref->vertex(sta);
        const sta::Pin* prev_pin = prev_vertex->pin();
        sta::Instance* inst = network->instance(pin);
        sta::Instance* prev_inst = network->instance(prev_pin);

        const bool same_inst = (inst == prev_inst && inst != nullptr);

        // Track logic depth (non-clock, non-net arcs)
        bool pin_is_clock = sta->isClock(pin, mode);
        if (same_inst && !pin_is_clock) {
          if (logic_insts.find(inst) == logic_insts.end()) {
            logic_insts.insert(inst);
            logic_depth_count++;
            logic_delay_total += pin_delay;
          }
        }

        TimingArcInfo arc;
        odb::dbModITerm* mod_iterm;
        db_network->staToDb(
            prev_pin, arc.from_iterm, arc.from_bterm, mod_iterm);
        db_network->staToDb(pin, arc.to_iterm, arc.to_bterm, mod_iterm);
        if (same_inst && arc.to_iterm) {
          arc.master = arc.to_iterm->getInst()->getMaster();
        }
        arc.delay = pin_delay;
        arc.slew = slw;
        arc.load = cap;
        arc.fanout = node_fanout;
        arc.is_rising = is_rising;
        path_info.arcs.push_back(arc);
      }

      arrival_prev = arr;
    }

    // Get startpoint/endpoint objects
    odb::dbModITerm* mod_iterm;
    db_network->staToDb(expand.path(0)->vertex(sta)->pin(),
                        path_info.start_iterm,
                        path_info.start_bterm,
                        mod_iterm);
    db_network->staToDb(path_end->vertex(sta)->pin(),
                        path_info.end_iterm,
                        path_info.end_bterm,
                        mod_iterm);

    path_info.logic_delay = logic_delay_total;
    path_info.logic_depth = logic_depth_count;
    path_info.fanout = max_fanout;

    result.push_back(std::move(path_info));
  }
  return result;
}

}  // namespace ord
