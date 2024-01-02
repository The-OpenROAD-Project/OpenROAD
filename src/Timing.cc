/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "ord/Timing.h"

#include <tcl.h>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "sta/Search.hh"
//#include "ord/Tech.h"
#include "ord/Design.h"
#include "sta/Corner.hh"
#include "sta/Liberty.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "utl/Logger.h"

namespace ord {

Timing::Timing(Design* design) : design_(design)
{
}

sta::dbSta* Timing::getSta()
{
  auto app = OpenRoad::openRoad();
  return app->getSta();
}

std::pair<odb::dbITerm*, odb::dbBTerm*> Timing::staToDBPin(const sta::Pin* pin)
{
  ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
  sta::dbNetwork* db_network = openroad->getDbNetwork();
  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  db_network->staToDb(pin, iterm, bterm);
  return std::make_pair(iterm, bterm);
}

bool Timing::isEndpoint(odb::dbITerm* db_pin)
{
  sta::Pin* sta_pin = getSta()->getDbNetwork()->dbToSta(db_pin);
  return isEndpoint_(sta_pin);
}

bool Timing::isEndpoint(odb::dbBTerm* db_pin)
{
  sta::Pin* sta_pin = getSta()->getDbNetwork()->dbToSta(db_pin);
  return isEndpoint_(sta_pin);
}

bool Timing::isEndpoint_(sta::Pin* sta_pin)
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

float Timing::slew(sta::Vertex* vertex, sta::MinMax minmax)
{
  auto sta = getSta();
  float slew = -sta::INF;
  float slew_min = +sta::INF;
  float slew;
  for (auto corner : getCorners()) {
    slew = sta::delayAsFloat(sta->vertexSlew(vertex, sta::RiseFall::rise(), corner, minmax));
    slew_max = std::max(slew_max,
  }
  return slew_max:slew_min;
}

float Timing::getPinSlew(odb::dbITerm* db_pin)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlew_(sta_pin);
}

float Timing::getPinSlew(odb::dbBTerm* db_pin)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinSlew_(sta_pin);
}

float Timing::getPinSlew_(sta::Pin* sta_pin)
{
  auto vertex_array = vertices(sta_pin);
  float pinSlew = -sta::INF;
  for (auto vertex : vertex_array) {
    if (vertex != nullptr) {
      float pinSlewTemp = slew_corner(vertex);
      pinSlew = std::max(pinSlew, pinSlewTemp);
    }
  }
  return pinSlew;
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

std::vector<float> Timing::arrivalsClk(const sta::RiseFall* rf,
                                       sta::Clock* clk,
                                       const sta::RiseFall* clk_rf,
                                       sta::Vertex* vertex)
{
  auto sta = getSta();
  std::vector<float> arrivals;

  const sta::ClockEdge* clk_edge = nullptr;

  if (clk) {
    clk_edge = clk->edge(clk_rf);
  }

  for (auto path_ap : sta->corners()->pathAnalysisPts()) {
    arrivals.push_back(sta::delayAsFloat(
        sta->vertexArrival(vertex, rf, clk_edge, path_ap, nullptr)));
  }
  return arrivals;
}

bool Timing::isTimeInf(float time)
{
  return (time > 1e+10 || time < -1e+10);
}

float Timing::getPinArrivalTime(sta::Clock* clk,
                                const sta::RiseFall* clk_rf,
                                sta::Vertex* vertex,
                                const sta::RiseFall* arrive_hold)
{
  std::vector<float> times = arrivalsClk(arrive_hold, clk, clk_rf, vertex);
  float delay = -sta::INF;
  for (float delay_time : times) {
    if (!isTimeInf(delay_time)) {
      delay = std::max(delay, delay_time);
    }
  }
  return delay;
}

sta::ClockSeq Timing::findClocksMatching(const char* pattern,
                                         bool regexp,
                                         bool nocase)
{
  auto sta = getSta();
  cmdLinkedNetwork();
  sta::PatternMatch matcher(pattern, regexp, nocase, sta->tclInterp());
  return sta->sdc()->findClocksMatching(&matcher);
}

float Timing::getPinArrival(odb::dbITerm* db_pin, RiseFall rf, MinMax mm)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinArrival_(sta_pin, rf, mm);
}

float Timing::getPinArrival(odb::dbBTerm* db_pin, RiseFall rf, MinMax mm)
{
  sta::dbSta* sta = getSta();
  sta::Pin* sta_pin = sta->getDbNetwork()->dbToSta(db_pin);
  return getPinArrival_(sta_pin, rf);
}

float Timing::getPinArrival_(sta::Pin* sta_pin, RiseFall rf, MinMax mm)
{
  auto vertex_array = vertices(sta_pin);
  float delay = -1;
  float d1, d2;
  sta::Clock* defaultArrivalClock
      = getSta()->sdc()->defaultArrivalClock();
  for (auto vertex : vertex_array) {
    if (vertex == nullptr) {
      continue
    }
    const sta::RiseFall* arrive_hold
        = (rf == Rise) ? sta::RiseFall::rise() : sta::RiseFall::fall();
    const sta::RiseFall* clk_rf = sta::RiseFall::rise();
    d1 = getPinArrivalTime(nullptr, clk_rf, vertex, arrive_hold);
    d2 = getPinArrivalTime(defaultArrivalClock, clk_rf, vertex, arrive_hold);
    for (auto clk : findClocksMatching("*", false, false)) {
      delay
          = std::max(delay,
                     getPinArrivalTime(
                         clk, sta::RiseFall::rise(), vertex, arrive_hold));
      delay
          = std::max(delay,
                     getPinArrivalTime(
                         clk, sta::RiseFall::fall(), vertex, arrive_hold));
    }
    
  }
  return delay;
}

std::vector<sta::Corner*> Timing::getCorners()
{
  sta::Corners* corners = getSta()->corners();
  return {corners->begin(), corners->end()};
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
    sta::TimingRole* role = arc_set->role();
    if (role->isTimingCheck() || role->isAsyncTimingCheck()
        || role->isNonSeqTimingCheck() || role->isDataCheck()) {
      continue;
    }
    sta::LibertyPort* to_port = arc_set->to();
    odb::dbMTerm* to_mterm = master->findMTerm(to_port->name());
    if (to_mterm) {
      outputs.insert(to_mterm);
    }
  }
  return {outputs.begin(), outputs.end()};
}

}  // namespace ord
