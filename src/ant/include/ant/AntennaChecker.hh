// BSD 3-Clause License
//
// Copyright (c) 2020, MICL, DD-Lab, University of Michigan
// All rights reserved.
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

#pragma once

#include <tcl.h>

#include <map>

#include "odb/db.h"
#include "odb/dbWireGraph.h"
#include "utl/Logger.h"

namespace ant {

using odb::dbInst;
using odb::dbITerm;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbSWire;
using odb::dbTechLayer;
using odb::dbWire;
using odb::dbWireGraph;

using utl::Logger;

struct PARinfo;
struct ARinfo;
struct ANTENNAmodel;

struct ViolationInfo
{
  int routing_level;
  std::vector<odb::dbITerm*> iterms;
  int required_diode_count;
};

class AntennaChecker
{
 public:
  AntennaChecker();
  ~AntennaChecker();

  void init(odb::dbDatabase* db, utl::Logger* logger);

  void loadAntennaRules();

  int checkAntennas(std::string report_filename, bool simple_report);

  void checkMaxLength(const char* net_name, int layer);

  void findMaxWireLength();

  std::vector<ViolationInfo> getNetAntennaViolations(dbNet* net,
                                                     odb::dbMTerm* diode_mterm);

 private:
  double dbuToMicrons(int value);

  dbWireGraph::Node* findSegmentRoot(dbWireGraph::Node* node_info,
                                     int wire_level);
  dbWireGraph::Node* findSegmentStart(dbWireGraph::Node* node);
  bool ifSegmentRoot(dbWireGraph::Node* node, int wire_level);

  void findWireBelowIterms(dbWireGraph::Node* node,
                           double iterm_areas[2],
                           int wire_level,
                           std::set<dbITerm*>& iv,
                           std::set<dbWireGraph::Node*>& nv);
  std::pair<double, double> calculateWireArea(
      dbWireGraph::Node* node,
      int wire_level,
      std::set<dbWireGraph::Node*>& nv,
      std::set<dbWireGraph::Node*>& level_nodes);

  double getViaArea(dbWireGraph::Edge* edge);
  dbTechLayer* getViaLayer(dbWireGraph::Edge* edge);
  std::string getViaName(dbWireGraph::Edge* edge);
  double calculateViaArea(dbWireGraph::Node* node, int wire_level);
  dbWireGraph::Edge* findVia(dbWireGraph::Node* node, int wire_level);

  void findCarPath(dbWireGraph::Node* node,
                   int wire_level,
                   dbWireGraph::Node* goal,
                   std::vector<dbWireGraph::Node*>& current_path,
                   std::vector<dbWireGraph::Node*>& path_found);

  void printGraphInfo(dbWireGraph graph);
  void calculateParInfo(PARinfo& PARtable);
  bool checkIterm(dbWireGraph::Node* node, double iterm_areas[2]);
  double getPwlFactor(odb::dbTechLayerAntennaRule::pwl_pair pwl_info,
                      double ref_val,
                      double def);

  void buildWireParTable(std::vector<PARinfo>& PARtable,
                         std::vector<dbWireGraph::Node*> wireroots_info);
  void buildWireCarTable(std::vector<ARinfo>& CARtable,
                         std::vector<PARinfo> PARtable,
                         std::vector<PARinfo> VIA_PARtable,
                         std::vector<dbWireGraph::Node*> gate_iterms);
  void buildViaParTable(std::vector<PARinfo>& VIA_PARtable,
                        std::vector<dbWireGraph::Node*> wireroots_info);
  void buildViaCarTable(std::vector<ARinfo>& VIA_CARtable,
                        std::vector<PARinfo> PARtable,
                        std::vector<PARinfo> VIA_PARtable,
                        std::vector<dbWireGraph::Node*> gate_iterms);

  std::vector<dbWireGraph::Node*> getWireroots(dbWireGraph graph);

  std::pair<bool, bool> checkWirePar(ARinfo AntennaRatio,
                                     bool simple_report,
                                     bool print);
  std::pair<bool, bool> checkWireCar(ARinfo AntennaRatio,
                                     bool par_checked,
                                     bool simple_report,
                                     bool print);
  bool checkViaPar(ARinfo AntennaRatio, bool simple_report, bool print);
  bool checkViaCar(ARinfo AntennaRatio, bool simple_report, bool print);

  std::vector<int> getAntennaRatio(std::string path, bool simple_report);

  void checkDiodeCell();

  bool checkViolation(PARinfo &par_info, dbTechLayer* layer);

  void findWirerootIterms(dbWireGraph::Node* node,
                          int wire_level,
                          std::vector<dbITerm*>& gates);
  double maxDiffArea(dbMTerm *mterm);

  std::vector<std::pair<double, std::vector<dbITerm*>>> parMaxWireLength(
      dbNet* net,
      int layer);
  std::vector<std::pair<double, std::vector<dbITerm*>>> getViolatedWireLength(
      dbNet* net,
      int routing_level);

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  FILE* stream_;
  std::map<odb::dbTechLayer*, ANTENNAmodel> layer_info;

  static constexpr int repair_max_diode_count = 10;
};

}  // namespace ant
