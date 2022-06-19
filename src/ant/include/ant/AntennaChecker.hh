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

#include <map>
#include <unordered_set>

#include "odb/db.h"
#include "odb/dbWireGraph.h"
#include "utl/Logger.h"

namespace grt {
class GlobalRouter;
}

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
struct AntennaModel;

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

  void init(odb::dbDatabase* db,
            grt::GlobalRouter* global_router,
            utl::Logger* logger);

  // net_name nullptr -> check all nets
  int checkAntennas(const char *net_name,
                    bool verbose);
  int antennaViolationCount() const;

  void checkMaxLength(const char* net_name, int layer);

  void findMaxWireLength();

  std::vector<ViolationInfo> getAntennaViolations(dbNet* net,
                                                  odb::dbMTerm* diode_mterm);
  void initAntennaRules();

 private:
  bool haveRoutedNets();
  double dbuToMicrons(int value);

  dbWireGraph::Node* findSegmentRoot(dbWireGraph::Node* node_info,
                                     int wire_level);
  dbWireGraph::Node* findSegmentStart(dbWireGraph::Node* node);
  bool ifSegmentRoot(dbWireGraph::Node* node, int wire_level);

  void findWireBelowIterms(dbWireGraph::Node* node,
                           double &iterm_gate_area,
                           double &iterm_diff_area,
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

  void calculateParInfo(PARinfo& PARtable);
  double getPwlFactor(odb::dbTechLayerAntennaRule::pwl_pair pwl_info,
                      double ref_val,
                      double def);

  void buildWireParTable(std::vector<PARinfo>& PARtable,
                         std::vector<dbWireGraph::Node*> wire_roots);
  void buildWireCarTable(std::vector<ARinfo>& CARtable,
                         std::vector<PARinfo> PARtable,
                         std::vector<PARinfo> VIA_PARtable,
                         std::vector<dbWireGraph::Node*> gate_iterms);
  void buildViaParTable(std::vector<PARinfo>& VIA_PARtable,
                        std::vector<dbWireGraph::Node*> wire_roots);
  void buildViaCarTable(std::vector<ARinfo>& VIA_CARtable,
                        std::vector<PARinfo> PARtable,
                        std::vector<PARinfo> VIA_PARtable,
                        std::vector<dbWireGraph::Node*> gate_iterms);

  std::vector<dbWireGraph::Node*> findWireRoots(dbWire* wire);
  void findWireRoots(dbWire* wire,
                     // Return values.
                     std::vector<dbWireGraph::Node*> &wire_roots,
                     std::vector<dbWireGraph::Node*> &gate_iterms);

  std::pair<bool, bool> checkWirePar(ARinfo AntennaRatio,
                                     bool verbose,
                                     bool report);
  std::pair<bool, bool> checkWireCar(ARinfo AntennaRatio,
                                     bool par_checked,
                                     bool verbose,
                                     bool report);
  bool checkViaPar(ARinfo AntennaRatio, bool verbose, bool report);
  bool checkViaCar(ARinfo AntennaRatio, bool verbose, bool report);

  void checkNet(dbNet* net,
                bool report_if_no_violation,
                bool verbose,
                // Return values.
                int &net_violation_count,
                int &pin_violation_count);
  void checkGate(dbWireGraph::Node* gate,
                 std::vector<ARinfo> &CARtable,
                 std::vector<ARinfo> &VIA_CARtable,
                 bool report,
                 bool verbose,
                 // Return values.
                 bool &violation,
                 std::unordered_set<dbWireGraph::Node*> &violated_gates);
  void checkDiodeCell();
  bool checkViolation(PARinfo &par_info, dbTechLayer* layer);
  bool antennaRatioDiffDependent(dbTechLayer* layer);

  void findWireRootIterms(dbWireGraph::Node* node,
                          int wire_level,
                          std::vector<dbITerm*>& gates);
  double maxDiffArea(dbMTerm* mterm);
  double maxGateArea(dbMTerm* mterm);

  std::vector<std::pair<double, std::vector<dbITerm*>>> parMaxWireLength(
      dbNet* net,
      int layer);
  std::vector<std::pair<double, std::vector<dbITerm*>>> getViolatedWireLength(
      dbNet* net,
      int routing_level);

  odb::dbDatabase* db_;
  odb::dbBlock* block_;
  int dbu_per_micron_;
  grt::GlobalRouter* global_router_;
  utl::Logger* logger_;
  std::map<odb::dbTechLayer*, AntennaModel> layer_info_;
  int net_violation_count_;

  static constexpr int repair_max_diode_count = 10;
};

}  // namespace ant
