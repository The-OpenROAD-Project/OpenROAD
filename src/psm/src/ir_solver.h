// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "Eigen/Sparse"
#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "connection.h"
#include "debug_gui.h"
#include "ir_network.h"
#include "node.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "psm/pdnsim.h"
#include "utl/Logger.h"

namespace sta {
class dbSta;
class Corner;
}  // namespace sta

namespace est {
class EstimateParasitics;
}

namespace psm {
class IRNetwork;

class IRSolver
{
 public:
  using Voltage = double;
  using Current = double;
  using Power = float;

  struct Results
  {
    Voltage net_voltage = 0.0;
    Voltage worst_voltage = 0.0;
    Voltage worst_ir_drop = 0.0;
    Voltage avg_voltage = 0.0;
    Voltage avg_ir_drop = 0.0;
    float max_percent = 0.0;
    Power total_power = 0.0;
  };
  struct EMResults
  {
    Current max_current = 0.0;
    Current avg_current = 0.0;
    std::size_t resistors = 0;
  };
  struct ConnectivityResults
  {
    std::set<Node*, Node::Compare> unconnected_nodes;
    std::set<ITermNode*, Node::Compare> unconnected_iterms;
  };

  using UserVoltages = std::map<odb::dbNet*, std::map<sta::Corner*, Voltage>>;
  using UserPowers = std::map<odb::dbInst*, std::map<sta::Corner*, Power>>;

  IRSolver(odb::dbNet* net,
           bool floorplanning,
           sta::dbSta* sta,
           est::EstimateParasitics* estimate_parasitics,
           utl::Logger* logger,
           const UserVoltages& user_voltages,
           const UserPowers& user_powers,
           const PDNSim::GeneratedSourceSettings& generated_source_settings);

  odb::dbNet* getNet() const { return net_; };

  bool check(bool check_bterms);

  void solve(sta::Corner* corner,
             GeneratedSourceType source_type,
             const std::string& source_file);

  void report(sta::Corner* corner) const;
  void reportEM(sta::Corner* corner) const;

  Results getSolution(sta::Corner* corner) const;
  EMResults getEMSolution(sta::Corner* corner) const;
  PDNSim::IRDropByPoint getIRDrop(odb::dbTechLayer* layer,
                                  sta::Corner* corner) const;
  ConnectivityResults getConnectivityResults() const;

  void enableGui(bool enable);

  void writeErrorFile(const std::string& error_file) const;
  void writeInstanceVoltageFile(const std::string& voltage_file,
                                sta::Corner* corner) const;
  void writeEMFile(const std::string& em_file, sta::Corner* corner) const;
  void writeSpiceFile(GeneratedSourceType source_type,
                      const std::string& spice_file,
                      sta::Corner* corner,
                      const std::string& voltage_source_file) const;

  bool belongsTo(Node* node) const;
  bool belongsTo(Connection* connection) const;

  std::vector<sta::Corner*> getCorners() const;
  bool hasSolution(sta::Corner* corner) const;
  Voltage getNetVoltage(sta::Corner* corner) const;
  std::optional<Voltage> getVoltage(sta::Corner* corner, Node* node) const;

  std::optional<Voltage> getSDCVoltage(sta::Corner* corner,
                                       odb::dbNet* net) const;
  std::optional<Voltage> getPVTVoltage(sta::Corner* corner) const;
  std::optional<Voltage> getUserVoltage(sta::Corner* corner,
                                        odb::dbNet* net) const;
  std::optional<Voltage> getSolutionVoltage(sta::Corner* corner) const;

  odb::dbNet* getPowerNet() const;

  Connection::ResistanceMap getResistanceMap(sta::Corner* corner) const;
  void assertResistanceMap(sta::Corner* corner) const;

  IRNetwork* getNetwork() const { return network_.get(); }

 private:
  template <typename T>
  using ValueNodeMap = std::map<const Node*, T>;

  odb::dbBlock* getBlock() const;
  odb::dbTech* getTech() const;

  bool checkOpen();
  bool checkBTerms() const;
  bool checkShort() const;

  std::map<odb::dbInst*, Power> getInstancePower(sta::Corner* corner) const;
  Voltage getPowerNetVoltage(sta::Corner* corner) const;

  Connection::ConnectionMap<Current> generateCurrentMap(
      sta::Corner* corner) const;

  Connection::ConnectionMap<Connection::Conductance> generateConductanceMap(
      sta::Corner* corner,
      const Connections& connections) const;
  Voltage generateSourceNodes(GeneratedSourceType source_type,
                              const std::string& source_file,
                              sta::Corner* corner,
                              SourceNodes& sources) const;
  SourceNodes generateSourceNodesFromBTerms() const;
  SourceNodes generateSourceNodesGenericFull() const;
  SourceNodes generateSourceNodesGenericStraps() const;
  SourceNodes generateSourceNodesGenericBumps() const;
  SourceNodes generateSourceNodesFromShapes(
      const std::set<odb::Rect>& shapes) const;
  Voltage generateSourceNodesFromSourceFile(const std::string& source_file,
                                            sta::Corner* corner,
                                            SourceNodes& sources) const;

  void reportUnconnectedNodes() const;
  void reportMissingBTerm() const;
  bool wasNodeVisited(const std::unique_ptr<ITermNode>& node) const;
  bool wasNodeVisited(const std::unique_ptr<Node>& node) const;
  bool wasNodeVisited(const Node* node) const;

  std::map<Node*, Connection::ConnectionSet> getNodeConnectionMap(
      const Connection::ConnectionMap<Connection::Conductance>& conductance)
      const;
  IRSolver::Power buildNodeCurrentMap(sta::Corner* corner,
                                      ValueNodeMap<Current>& currents) const;
  std::map<Node*, std::size_t> assignNodeIDs(const Node::NodeSet& nodes,
                                             std::size_t start = 0) const;
  std::map<Node*, std::size_t> assignNodeIDs(const SourceNodes& nodes,
                                             std::size_t start = 0) const;
  void buildCondMatrixAndVoltages(
      bool is_ground,
      const std::map<Node*, Connection::ConnectionSet>& node_connections,
      const ValueNodeMap<Current>& currents,
      const Connection::ConnectionMap<Connection::Conductance>& conductance,
      const std::map<Node*, std::size_t>& node_index,
      Eigen::SparseMatrix<Connection::Conductance>& g_matrix,
      Eigen::VectorXd& j_vector) const;
  void addSourcesToMatrixAndVoltages(
      Voltage src_voltage,
      const SourceNodes& sources,
      const std::map<Node*, std::size_t>& node_index,
      Eigen::SparseMatrix<Connection::Conductance>& g_matrix,
      Eigen::VectorXd& j_vector) const;

  std::string getMetricKey(const std::string& key, sta::Corner* corner) const;

  void dumpVector(const Eigen::VectorXd& vector, const std::string& name) const;
  void dumpMatrix(const Eigen::SparseMatrix<Connection::Conductance>& matrix,
                  const std::string& name) const;
  void dumpConductance(
      const Connection::ConnectionMap<Connection::Conductance>& cond,
      const std::string& name) const;

  odb::dbNet* net_;

  utl::Logger* logger_;
  est::EstimateParasitics* estimate_parasitics_;
  sta::dbSta* sta_;

  std::unique_ptr<IRNetwork> network_;

  std::unique_ptr<DebugGui> gui_;

  const UserVoltages& user_voltages_;
  const UserPowers& user_powers_;
  std::map<sta::Corner*, Voltage> solution_voltages_;
  std::map<sta::Corner*, Power> solution_power_;

  const PDNSim::GeneratedSourceSettings& generated_source_settings_;

  // Holds nodes that were visited during the open net check
  std::set<const Node*> visited_;
  std::optional<bool> connected_;

  std::map<sta::Corner*, ValueNodeMap<Voltage>> voltages_;
  std::map<sta::Corner*, ValueNodeMap<Current>> currents_;

  static constexpr Current kSpiceFileMinCurrent = 1e-18;
};

}  // namespace psm
