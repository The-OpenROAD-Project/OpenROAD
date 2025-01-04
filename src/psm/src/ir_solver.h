///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
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

#include <Eigen/Sparse>
#include <boost/geometry.hpp>
#include <boost/polygon/polygon.hpp>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "connection.h"
#include "debug_gui.h"
#include "ir_network.h"
#include "node.h"
#include "odb/db.h"
#include "psm/pdnsim.h"
#include "utl/Logger.h"

namespace sta {
class dbSta;
class Corner;
}  // namespace sta

namespace rsz {
class Resizer;
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
  };
  struct EMResults
  {
    Current max_current = 0.0;
    Current avg_current = 0.0;
    std::size_t resistors = 0;
  };
  struct ConnectivityResults
  {
    std::set<Node*, Node::Compare> unconnected_nodes_;
    std::set<ITermNode*, Node::Compare> unconnected_iterms_;
  };

  IRSolver(
      odb::dbNet* net,
      bool floorplanning,
      sta::dbSta* sta,
      rsz::Resizer* resizer,
      utl::Logger* logger,
      const std::map<odb::dbNet*, std::map<sta::Corner*, Voltage>>&
          user_voltages,
      const std::map<odb::dbInst*, std::map<sta::Corner*, Power>>& user_powers,
      const PDNSim::GeneratedSourceSettings& generated_source_settings);

  odb::dbNet* getNet() const { return net_; };

  bool check();

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

  IRNetwork* getNetwork() const { return network_.get(); }

 private:
  template <typename T>
  using ValueNodeMap = std::map<const Node*, T>;

  odb::dbBlock* getBlock() const;
  odb::dbTech* getTech() const;

  bool checkOpen();
  bool checkShort() const;

  std::map<odb::dbInst*, Power> getInstancePower(sta::Corner* corner) const;
  Voltage getPowerNetVoltage(sta::Corner* corner) const;

  Connection::ConnectionMap<Current> generateCurrentMap(
      sta::Corner* corner) const;

  Connection::ConnectionMap<Connection::Conductance> generateConductanceMap(
      sta::Corner* corner) const;
  Voltage generateSourceNodes(
      GeneratedSourceType source_type,
      const std::string& source_file,
      sta::Corner* corner,
      std::vector<std::unique_ptr<SourceNode>>& sources) const;
  std::vector<std::unique_ptr<SourceNode>> generateSourceNodesFromBTerms()
      const;
  std::vector<std::unique_ptr<SourceNode>> generateSourceNodesGenericFull()
      const;
  std::vector<std::unique_ptr<SourceNode>> generateSourceNodesGenericStraps()
      const;
  std::vector<std::unique_ptr<SourceNode>> generateSourceNodesGenericBumps()
      const;
  std::vector<std::unique_ptr<SourceNode>> generateSourceNodesFromShapes(
      const std::set<odb::Rect>& shapes) const;
  Voltage generateSourceNodesFromSourceFile(
      const std::string& source_file,
      sta::Corner* corner,
      std::vector<std::unique_ptr<SourceNode>>& sources) const;

  void reportUnconnectedNodes() const;
  bool wasNodeVisited(const std::unique_ptr<ITermNode>& node) const;
  bool wasNodeVisited(const std::unique_ptr<Node>& node) const;
  bool wasNodeVisited(const Node* node) const;

  std::map<Node*, Connection::ConnectionSet> getNodeConnectionMap(
      const Connection::ConnectionMap<Connection::Conductance>& conductance)
      const;
  void buildNodeCurrentMap(sta::Corner* corner,
                           ValueNodeMap<Current>& currents) const;
  std::map<Node*, std::size_t> assignNodeIDs(const Node::NodeSet& nodes,
                                             std::size_t start = 0) const;
  std::map<Node*, std::size_t> assignNodeIDs(
      const std::vector<std::unique_ptr<SourceNode>>& nodes,
      std::size_t start = 0) const;
  void buildCondMatrixAndVoltages(
      bool is_ground,
      const std::map<Node*, Connection::ConnectionSet>& node_connections,
      const ValueNodeMap<Current>& currents,
      const Connection::ConnectionMap<Connection::Conductance>& conductance,
      const std::map<Node*, std::size_t>& node_index,
      Eigen::SparseMatrix<Connection::Conductance>& G,
      Eigen::VectorXd& J) const;
  void addSourcesToMatrixAndVoltages(
      Voltage src_voltage,
      const std::vector<std::unique_ptr<psm::SourceNode>>& sources,
      const std::map<Node*, std::size_t>& node_index,
      Eigen::SparseMatrix<Connection::Conductance>& G,
      Eigen::VectorXd& J) const;

  std::string getMetricKey(const std::string& key, sta::Corner* corner) const;

  void dumpVector(const Eigen::VectorXd& vector, const std::string& name) const;
  void dumpMatrix(const Eigen::SparseMatrix<Connection::Conductance>& matrix,
                  const std::string& name) const;
  void dumpConductance(
      const Connection::ConnectionMap<Connection::Conductance>& cond,
      const std::string& name) const;

  odb::dbNet* net_;

  utl::Logger* logger_;
  rsz::Resizer* resizer_;
  sta::dbSta* sta_;

  std::unique_ptr<IRNetwork> network_;

  std::unique_ptr<DebugGui> gui_;

  const std::map<odb::dbNet*, std::map<sta::Corner*, Voltage>>& user_voltages_;
  const std::map<odb::dbInst*, std::map<sta::Corner*, Power>>& user_powers_;
  std::map<sta::Corner*, Voltage> solution_voltages_;

  const PDNSim::GeneratedSourceSettings& generated_source_settings_;

  // Holds nodes that were visited during the open net check
  std::set<const Node*> visited_;
  std::optional<bool> connected_;

  std::map<sta::Corner*, ValueNodeMap<Voltage>> voltages_;
  std::map<sta::Corner*, ValueNodeMap<Current>> currents_;

  static constexpr Current spice_file_min_current_ = 1e-18;
};

}  // namespace psm
