// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ant/AntennaChecker.hh"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbWireGraph.h"

namespace utl {
class Logger;
}

namespace ant {

struct PARinfo;
struct ARinfo;
struct AntennaModel;
struct PinAntenna;
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

  // Accumulates per-layer ratios for CAR/CSR; iterm areas are per-gate
  // constants and must not be summed (see calculateAreas)
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

using LayerToNodeInfo = odb::PtrMap<odb::dbTechLayer, NodeInfo>;
using GraphNodes = std::vector<std::unique_ptr<GraphNode>>;
using LayerToGraphNodes = odb::PtrMap<odb::dbTechLayer, GraphNodes>;
using GateToLayerToNodeInfo = odb::PtrMap<odb::dbITerm, LayerToNodeInfo>;
using PinAntennas = odb::PtrMap<odb::dbMTerm, PinAntenna>;
using GateToViolationLayers
    = odb::PtrMap<odb::dbITerm, odb::PtrSet<odb::dbTechLayer>>;

class AntennaChecker::Impl
{
 public:
  Impl(odb::dbDatabase* db, utl::Logger* logger);

  DesignCheck check(odb::dbNet* net, int num_threads);
  int checkAntennas(odb::dbNet* net, int num_threads, bool verbose);
  int antennaViolationCount() const;
  int pinViolationCount() const;
  NetCheck checkNet(odb::dbNet* net, float ratio_margin);
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
  std::vector<std::pair<double, std::vector<odb::dbITerm*>>> parMaxWireLength(
      odb::dbNet* net,
      int layer);
  std::vector<std::pair<double, std::vector<odb::dbITerm*>>>
  getViolatedWireLength(odb::dbNet* net, int routing_level);
  void buildLayerMaps(odb::dbNet* net, LayerToGraphNodes& node_by_layer_map);
  int analyzeNet(odb::dbNet* net,
                 odb::dbMTerm* diode_mterm,
                 float ratio_margin,
                 Violations& antenna_violations,
                 NetCheck& net_check);
  void saveGates(odb::dbNet* db_net,
                 LayerToGraphNodes& node_by_layer_map,
                 int node_count);
  void calculateAreas(const LayerToGraphNodes& node_by_layer_map,
                      PinAntennas& pins,
                      GateToLayerToNodeInfo& gate_info);
  void calculatePAR(GateToLayerToNodeInfo& gate_info);
  void calculateCAR(const PinAntennas& pins, GateToLayerToNodeInfo& gate_info);
  LayerCheck checkRatioViolations(odb::dbTechLayer* layer,
                                  NodeInfo& node_info,
                                  float ratio_margin);
  int checkGates(odb::dbNet* db_net,
                 odb::dbMTerm* diode_mterm,
                 float ratio_margin,
                 GateToLayerToNodeInfo& gate_info,
                 Violations& antenna_violations,
                 NetCheck& net_check);
  void calculateViaPar(odb::dbTechLayer* tech_layer, NodeInfo& info);
  void calculateWirePar(odb::dbTechLayer* tech_layer, NodeInfo& info);
  // Each check compares a node's ratio with the layer rule.  checkPAR and
  // checkPSR also record the excess ratio in info.
  RatioCheck checkPAR(odb::dbTechLayer* tech_layer,
                      NodeInfo& info,
                      float ratio_margin);
  RatioCheck checkPSR(odb::dbTechLayer* tech_layer,
                      NodeInfo& info,
                      float ratio_margin);
  RatioCheck checkCAR(odb::dbTechLayer* tech_layer,
                      const NodeInfo& info,
                      float ratio_margin);
  RatioCheck checkCSR(odb::dbTechLayer* tech_layer,
                      const NodeInfo& info,
                      float ratio_margin);

  odb::dbDatabase* db_{nullptr};
  odb::dbBlock* block_{nullptr};
  utl::Logger* logger_{nullptr};
  odb::PtrMap<odb::dbTechLayer, AntennaModel> layer_info_;
  // The counts of the last checkAntennas.
  int net_violation_count_{0};
  int pin_violation_count_{0};
  std::string report_file_name_;
  // consts
  static constexpr int kMaxDiodeCountPerGate = 10;
};

}  // namespace ant
