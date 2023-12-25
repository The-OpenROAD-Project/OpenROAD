/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
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

#pragma once

#include <vector>

#include "sta/Clock.hh"
#include "sta/Graph.hh"
#include "sta/PatternMatch.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"

namespace odb {
// class dbBlock;
class dbMaster;
class dbMTerm;
// class dbNet;
// class dbInst;
class dbITerm;
class dbBTerm;
}  // namespace odb

namespace sta {
class dbSta;
class Corner;
// class MinMax;
class LibertyCell;
class Network;
class Sta;
class RiseFall;
class Vertex;
class Pin;
// class PinSet;
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

  sta::ClockSeq findClocksMatching(const char* pattern,
                                   bool regexp,
                                   bool nocase);
  float getPinArrival(odb::dbITerm* db_pin, RiseFall rf);
  float getPinArrival(odb::dbBTerm* db_pin, RiseFall rf);
  std::vector<float> arrivalsClk(const sta::RiseFall* rf,
                                 sta::Clock* clk,
                                 const sta::RiseFall* clk_rf,
                                 sta::Vertex* vertex);
  float getPinArrivalTime(sta::Clock* clk,
                          const sta::RiseFall* clk_rf,
                          sta::Vertex* vertex,
                          const sta::RiseFall* arrive_or_hold);
  sta::Graph* cmdGraph();
  sta::Network* cmdLinkedNetwork();
  std::array<sta::Vertex*, 2> vertices(const sta::Pin* pin);
  bool isTimeInf(float time);

  float slew_corner(sta::Vertex* vertex);
  float getPinSlew(odb::dbITerm* db_pin);
  float getPinSlew(odb::dbBTerm* db_pin);

  std::pair<odb::dbITerm*, odb::dbBTerm*> staToDBPin(const sta::Pin* pin);

  bool isStartpoint(odb::dbITerm* db_pin);
  bool isStartpoint(odb::dbBTerm* db_pin);
  bool isEndpoint(odb::dbITerm* db_pin);
  bool isEndpoint(odb::dbBTerm* db_pin);

  std::vector<odb::dbMTerm*> getTimingFanoutFrom(odb::dbMTerm* input);
  std::vector<sta::Corner*> getCorners();

 private:
  sta::LibertyCell* getLibertyCell(odb::dbMaster* master);
  sta::dbSta* getSta();
  bool isStartpoint_(sta::Pin* sta_pin);
  bool isEndpoint_(sta::Pin* sta_pin);
  float getPinSlew_(sta::Pin* sta_pin);
  float getPinArrival_(sta::Pin* sta_pin, RiseFall rf);
  Design* design_;
};

}  // namespace ord
