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
#include <queue>
#include <unordered_set>

#include "odb/db.h"
#include "odb/dbWireGraph.h"
#include "utl/Logger.h"

namespace grt {
class GlobalRouter;
}

namespace ant {

using utl::Logger;

struct PARinfo;
struct ARinfo;
struct AntennaModel;

///////////////////////////////////////
struct GraphNode;

struct NodeInfo
{
  double PAR;
  double PSR;
  double diff_PAR;
  double diff_PSR;
  double area;
  double side_area;
  double iterm_gate_area;
  double iterm_diff_area;

  double CAR;
  double CSR;
  double diff_CAR;
  double diff_CSR;

  std::vector<odb::dbITerm*> iterms;

  NodeInfo& operator+=(const NodeInfo& a)
  {
    PAR += a.PAR;
    PSR += a.PSR;
    diff_PAR += a.diff_PAR;
    diff_PSR += a.diff_PSR;
    area += a.area;
    side_area += a.side_area;
    return *this;
  }
  NodeInfo()
  {
    PAR = 0.0;
    PSR = 0.0;
    diff_PAR = 0.0;
    diff_PSR = 0.0;

    area = 0.0;
    side_area = 0.0;
    iterm_gate_area = 0.0;
    iterm_diff_area = 0.0;

    CAR = 0.0;
    CSR = 0.0;
    diff_CAR = 0.0;
    diff_CSR = 0.0;
  }
};

class GlobalRouteSource
{
 public:
  virtual ~GlobalRouteSource() = default;

  virtual bool haveRoutes() = 0;
  virtual void makeNetWires() = 0;
  virtual void destroyNetWires() = 0;
};

struct Violation
{
  int routing_level;
  std::vector<odb::dbITerm*> gates;
  int diode_count_per_gate;
};

using LayerToNodeInfo = std::map<odb::dbTechLayer*, NodeInfo>;
using GraphNodes = std::vector<GraphNode*>;
using LayerToGraphNodes = std::unordered_map<odb::dbTechLayer*, GraphNodes>;
using GateToLayerToNodeInfo = std::map<std::string, LayerToNodeInfo>;
using Violations = std::vector<Violation>;
using GateToViolationLayers
    = std::unordered_map<std::string, std::unordered_set<odb::dbTechLayer*>>;

class AntennaChecker
{
 public:
  AntennaChecker();
  ~AntennaChecker();

  void init(odb::dbDatabase* db,
            GlobalRouteSource* global_route_source,
            utl::Logger* logger);

  // net nullptr -> check all nets
  int checkAntennas(odb::dbNet* net = nullptr,
                    int num_threads = 1,
                    bool verbose = false);
  int antennaViolationCount() const;
  Violations getAntennaViolations(odb::dbNet* net,
                                  odb::dbMTerm* diode_mterm,
                                  float ratio_margin);
  void initAntennaRules();
  void setReportFileName(const char* file_name);

 private:
  bool haveRoutedNets();
  double getPwlFactor(odb::dbTechLayerAntennaRule::pwl_pair pwl_info,
                      double ref_val,
                      double def);
  double diffArea(odb::dbMTerm* mterm);
  double gateArea(odb::dbMTerm* mterm);
  std::vector<std::pair<double, std::vector<odb::dbITerm*>>> parMaxWireLength(
      odb::dbNet* net,
      int layer);
  std::vector<std::pair<double, std::vector<odb::dbITerm*>>>
  getViolatedWireLength(odb::dbNet* net, int routing_level);
  bool isValidGate(odb::dbMTerm* mterm);
  void buildLayerMaps(odb::dbNet* net, LayerToGraphNodes& node_by_layer_map);
  void checkNet(odb::dbNet* net,
                bool verbose,
                bool report_if_no_violation,
                std::ofstream& report_file,
                odb::dbMTerm* diode_mterm,
                float ratio_margin,
                int& net_violation_count,
                int& pin_violation_count,
                Violations& antenna_violations);
  void saveGates(odb::dbNet* db_net,
                 LayerToGraphNodes& node_by_layer_map,
                 int node_count);
  void calculateAreas(const LayerToGraphNodes& node_by_layer_map,
                      GateToLayerToNodeInfo& gate_info);
  void calculatePAR(GateToLayerToNodeInfo& gate_info);
  void calculateCAR(GateToLayerToNodeInfo& gate_info);
  bool checkRatioViolations(odb::dbTechLayer* layer,
                            const NodeInfo& node_info,
                            bool verbose,
                            bool report,
                            std::ofstream& report_file);
  void reportNet(odb::dbNet* db_net,
                 GateToLayerToNodeInfo& gate_info,
                 GateToViolationLayers& gates_with_violations,
                 bool verbose,
                 std::ofstream& report_file);
  int checkGates(odb::dbNet* db_net,
                 bool verbose,
                 bool report_if_no_violation,
                 std::ofstream& report_file,
                 odb::dbMTerm* diode_mterm,
                 float ratio_margin,
                 GateToLayerToNodeInfo& gate_info,
                 Violations& antenna_violations);
  void calculateViaPar(odb::dbTechLayer* tech_layer, NodeInfo& info);
  void calculateWirePar(odb::dbTechLayer* tech_layer, NodeInfo& info);
  bool checkPAR(odb::dbTechLayer* tech_layer,
                const NodeInfo& info,
                bool verbose,
                bool report,
                std::ofstream& report_file);
  bool checkPSR(odb::dbTechLayer* tech_layer,
                const NodeInfo& info,
                bool verbose,
                bool report,
                std::ofstream& report_file);
  bool checkCAR(odb::dbTechLayer* tech_layer,
                const NodeInfo& info,
                bool verbose,
                bool report,
                std::ofstream& report_file);
  bool checkCSR(odb::dbTechLayer* tech_layer,
                const NodeInfo& info,
                bool verbose,
                bool report,
                std::ofstream& report_file);

  odb::dbDatabase* db_{nullptr};
  odb::dbBlock* block_{nullptr};
  GlobalRouteSource* global_route_source_{nullptr};
  utl::Logger* logger_{nullptr};
  std::map<odb::dbTechLayer*, AntennaModel> layer_info_;
  int net_violation_count_{0};
  float ratio_margin_{0};
  std::string report_file_name_;
  odb::dbTechLayer* min_layer_{nullptr};
  std::vector<odb::dbNet*> nets_;
  // consts
  static constexpr int max_diode_count_per_gate = 10;
};

}  // namespace ant
