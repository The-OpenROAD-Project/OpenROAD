// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "sta/Clock.hh"
#include "sta/Graph.hh"
#include "sta/PatternMatch.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"

namespace odb {
class dbMaster;
class dbMTerm;
class dbITerm;
class dbBTerm;
class dbInst;
class dbNet;
}  // namespace odb

namespace sta {
class dbSta;
class Scene;
class LibertyCell;
class Network;
class Sta;
class RiseFall;
class Vertex;
class Pin;
}  // namespace sta

namespace ord {

class Design;
class OpenRoad;
class Timing
{
 public:
  explicit Timing(Design* design);

  enum RiseFall
  {
    Rise,
    Fall
  };
  enum MinMax
  {
    Min,
    Max
  };

  sta::ClockSeq findClocksMatching(const char* pattern,
                                   bool regexp,
                                   bool nocase);
  float getPinArrival(odb::dbITerm* db_pin, RiseFall rf, MinMax minmax = Max);
  float getPinArrival(odb::dbBTerm* db_pin, RiseFall rf, MinMax minmax = Max);
  bool isTimeInf(float time);

  float getPinSlew(odb::dbITerm* db_pin, MinMax minmax = Max);
  float getPinSlew(odb::dbBTerm* db_pin, MinMax minmax = Max);

  float getPinSlack(odb::dbITerm* db_pin, RiseFall rf, MinMax minmax = Max);
  float getPinSlack(odb::dbBTerm* db_pin, RiseFall rf, MinMax minmax = Max);

  bool isEndpoint(odb::dbITerm* db_pin);
  bool isEndpoint(odb::dbBTerm* db_pin);

  float getNetCap(odb::dbNet* net, sta::Scene* corner, MinMax minmax);
  float getPortCap(odb::dbITerm* pin, sta::Scene* corner, MinMax minmax);
  float getMaxCapLimit(odb::dbMTerm* pin);
  float getMaxSlewLimit(odb::dbMTerm* pin);
  float staticPower(odb::dbInst* inst, sta::Scene* corner);
  float dynamicPower(odb::dbInst* inst, sta::Scene* corner);

  std::vector<odb::dbMTerm*> getTimingFanoutFrom(odb::dbMTerm* input);
  std::vector<sta::Scene*> getCorners();
  sta::Scene* cmdCorner();
  sta::Scene* findCorner(const char* name);

  void makeEquivCells();
  std::vector<odb::dbMaster*> equivCells(odb::dbMaster* master);

 private:
  sta::dbSta* getSta();
  const sta::MinMax* getMinMax(MinMax type);
  sta::LibertyCell* getLibertyCell(odb::dbMaster* master);
  std::array<sta::Vertex*, 2> vertices(const sta::Pin* pin);
  bool isEndpoint(sta::Pin* sta_pin);
  float getPinSlew(sta::Pin* sta_pin, MinMax minmax);
  float getPinArrival(sta::Pin* sta_pin, RiseFall rf, MinMax minmax);
  float getPinSlack(sta::Pin* sta_pin, RiseFall rf, MinMax minmax);
  float slewAllCorners(sta::Vertex* vertex, const sta::MinMax* minmax);
  std::vector<float> arrivalsClk(const sta::RiseFall* rf,
                                 sta::Clock* clk,
                                 const sta::RiseFall* clk_rf,
                                 sta::Vertex* vertex);
  float getPinArrivalTime(sta::Clock* clk,
                          const sta::RiseFall* clk_rf,
                          sta::Vertex* vertex,
                          const sta::RiseFall* arrive_hold);
  sta::Graph* cmdGraph();
  sta::Network* cmdLinkedNetwork();
  std::pair<odb::dbITerm*, odb::dbBTerm*> staToDBPin(const sta::Pin* pin);
  Design* design_;
};

}  // namespace ord
