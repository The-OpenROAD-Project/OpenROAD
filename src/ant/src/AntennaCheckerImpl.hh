// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "ant/AntennaChecker.hh"
#include "odb/db.h"
#include "odb/dbWireGraph.h"

namespace utl {
class Logger;
}

namespace ant {

struct PARinfo;
struct ARinfo;
struct AntennaModel;
class WireBuilder;

///////////////////////////////////////
struct GraphNode;

struct NodeInfo
{
  double par = 0.0;
  double psr = 0.0;
  double diff_par = 0.0;
  double diff_psr = 0.0;
  double area = 0.0;
  double side_area = 0.0;
  double iterm_gate_area = 0.0;
  double iterm_diff_area = 0.0;

  double car = 0.0;
  double csr = 0.0;
  double diff_car = 0.0;
  double diff_csr = 0.0;

  // Defines the ratio between the current PAR and the allowed PAR
  double excess_ratio_par = 0.0;
  // Defines the ratio between the current PSR and the allowed PSR
  double excess_ratio_psr = 0.0;

  std::vector<odb::dbITerm*> iterms;

  NodeInfo& operator+=(const NodeInfo& a)
  {
    par += a.par;
    psr += a.psr;
    diff_par += a.diff_par;
    diff_psr += a.diff_psr;
    area += a.area;
    side_area += a.side_area;
    return *this;
  }
};

struct ViolationReport
{
  bool violated;
  std::string report;
  ViolationReport() { violated = false; }
};

using LayerToNodeInfo = std::map<odb::dbTechLayer*, NodeInfo>;
using GraphNodes = std::vector<std::unique_ptr<GraphNode>>;
using LayerToGraphNodes = std::map<odb::dbTechLayer*, GraphNodes>;
using GateToLayerToNodeInfo = std::map<odb::dbITerm*, LayerToNodeInfo>;
using GateToViolationLayers
    = std::map<odb::dbITerm*, std::set<odb::dbTechLayer*>>;

class AntennaChecker::Impl
{
 public:
  Impl(odb::dbDatabase* db, utl::Logger* logger);

  // net nullptr -> check all nets
  int checkAntennas(odb::dbNet* net, int num_threads, bool verbose);
  int antennaViolationCount() const;
  Violations getAntennaViolations(odb::dbNet* net,
                                  odb::dbMTerm* diode_mterm,
                                  float ratio_margin);
  void initAntennaRules();
  void setReportFileName(const char* file_name);
  void makeNetWiresFromGuides(const std::vector<odb::dbNet*>& nets);

 private:
  bool haveRoutedNets();
  bool designIsPlaced();
  bool haveGuides();
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
  int checkNet(odb::dbNet* net,
               bool verbose,
               bool save_report,
               odb::dbMTerm* diode_mterm,
               float ratio_margin,
               Violations& antenna_violations);
  void saveGates(odb::dbNet* db_net,
                 LayerToGraphNodes& node_by_layer_map,
                 int node_count);
  void calculateAreas(const LayerToGraphNodes& node_by_layer_map,
                      GateToLayerToNodeInfo& gate_info);
  void calculatePAR(GateToLayerToNodeInfo& gate_info);
  void calculateCAR(GateToLayerToNodeInfo& gate_info);
  bool checkRatioViolations(odb::dbNet* db_net,
                            odb::dbTechLayer* layer,
                            NodeInfo& node_info,
                            float ratio_margin,
                            bool verbose,
                            bool report,
                            ViolationReport& net_report);
  void writeReport(std::ofstream& report_file, bool verbose);
  void printReport(odb::dbNet* db_net);
  int checkGates(odb::dbNet* db_net,
                 bool verbose,
                 bool save_report,
                 odb::dbMTerm* diode_mterm,
                 float ratio_margin,
                 GateToLayerToNodeInfo& gate_info,
                 Violations& antenna_violations);
  void calculateViaPar(odb::dbTechLayer* tech_layer, NodeInfo& info);
  void calculateWirePar(odb::dbTechLayer* tech_layer, NodeInfo& info);
  bool checkPAR(odb::dbNet* db_net,
                odb::dbTechLayer* tech_layer,
                NodeInfo& info,
                float ratio_margin,
                bool verbose,
                bool report,
                ViolationReport& net_report);
  bool checkPSR(odb::dbNet* db_net,
                odb::dbTechLayer* tech_layer,
                NodeInfo& info,
                float ratio_margin,
                bool verbose,
                bool report,
                ViolationReport& net_report);
  bool checkCAR(odb::dbNet* db_net,
                odb::dbTechLayer* tech_layer,
                const NodeInfo& info,
                bool verbose,
                bool report,
                ViolationReport& net_report);
  bool checkCSR(odb::dbNet* db_net,
                odb::dbTechLayer* tech_layer,
                const NodeInfo& info,
                bool verbose,
                bool report,
                ViolationReport& net_report);

  odb::dbDatabase* db_{nullptr};
  odb::dbBlock* block_{nullptr};
  utl::Logger* logger_{nullptr};
  std::map<odb::dbTechLayer*, AntennaModel> layer_info_;
  int net_violation_count_{0};
  std::string report_file_name_;
  std::vector<odb::dbNet*> nets_;
  std::map<odb::dbNet*, ViolationReport> net_to_report_;
  absl::Mutex map_mutex_;
  // consts
  static constexpr int kMaxDiodeCountPerGate = 10;
};

}  // namespace ant
