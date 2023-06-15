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
#include <cstring>
#include <memory>

#include "SteinerTree.hh"
#include "utl/Logger.h"
#include "db_sta/dbSta.hh"

namespace sta {
class LibertyCell;
class PathRef;
class PathExpanded;
}  // namespace sta

namespace rsz {

using sta::Instance;
using sta::InstancePinIterator;
using sta::PathAnalysisPt;
using sta::PathRef;
using sta::PathExpanded;

class PathPoint
{
 public:
  explicit PathPoint(Pin* path_pin = nullptr, bool is_rise = false,
		     float path_arrival = 0, float path_required = 0,
		     float path_slack = 0, sta::PathAnalysisPt* pt = nullptr);
  Pin*                 pin() const;
  bool                 isRise() const;
  float                arrival() const;
  float                required() const;
  float                slack() const;
  int                  analysisPointIndex() const;
  sta::PathAnalysisPt* analysisPoint() const;

 private:
  Pin*                 pin_;
  bool                 is_rise_;
  float                arrival_;
  float                required_;
  float                slack_;
  sta::PathAnalysisPt *path_ap_;
};

class GateCloner
{
 public:
  explicit GateCloner(Resizer *resizer);
  int gateClone(const Pin *drvr_pin, PathRef* drvr_path, int drvr_index,
                PathExpanded* expanded, float cap_factor,
                bool clone_largest_only);
  int run(const Pin* drvr_pin, PathRef* drvr_path, int drvr_index,
          PathExpanded* expanded);
  
 private:
  std::vector<Pin*> levelDriverPins(const bool reverse = false,
                                    const std::unordered_set<Pin*> &filter_pins
                                    = std::unordered_set<Pin*>()) const;
  Cell* largestLibraryCell(Cell* cell);
  float maxLoad(Cell* cell);

  std::vector<const Pin*> fanoutPins(Net* net,
                                     bool include_top_level = false) const;
  Vertex* vertex(Pin* term) const;
  std::vector<PathPoint> expandPath(sta::Path* path, bool enumed) const;
  std::vector<PathPoint> worstSlackPath(const Pin* term,
                                        bool trim = false) const;
  float required(Pin* term, bool is_rise, PathAnalysisPt* path_ap) const;
  bool violatesMaximumCapacitance(Pin* term, float limit_scale_factor = 1.0);
  bool violatesMaximumTransition(Pin* term, float limit_scale_factor = 1.0);
  std::vector<const Pin*> filterPins(std::vector<const Pin*>& terms,
                                     sta::PortDirection* direction,
                                     bool include_top_level) const;
  bool isSingleOutputCombinational(Instance* inst) const;
  bool isSingleOutputCombinational(LibertyCell* cell) const;
  bool isCombinational(LibertyCell* cell) const;

  std::vector<sta::LibertyPort *> libraryPins(Instance* inst) const;
  std::vector<sta::LibertyPort *> libraryPins(LibertyCell* cell) const;
  std::vector<sta::LibertyPort *> libraryInputPins(LibertyCell* cell) const;
  std::vector<sta::LibertyPort *> libraryOutputPins(LibertyCell* cell) const;

  LibertyCell* halfDrivingPowerCell(Instance* inst);
  LibertyCell* halfDrivingPowerCell(LibertyCell* cell);
  LibertyCell* closestDriver(LibertyCell* cell, LibertyCellSeq *candidates,
                             float scale);
  void cloneTree(Instance* inst, float cap_factor, bool clone_largest_only);
  void topDownClone(SteinerTree* tree, SteinerPt current, SteinerPt prev,
                    float c_limit, LibertyCell* driver_cell);
  void topDownConnect(SteinerTree* tree, SteinerPt current, Net* net);
  void cloneInstance(SteinerTree* tree, SteinerPt current,
                     SteinerPt prev, LibertyCell* driver_cell);

  int clone_count_;
  Logger* logger_;
  dbSta* sta_;
  dbNetwork* db_network_;
  Network* network_;
  sta::Graph *graph_;
  Resizer* resizer_;
  const sta::MinMax* min_max_;
  //const Corner *corner_;

};

}  // namespace rsz
