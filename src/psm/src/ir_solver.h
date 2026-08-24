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
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "psm/pdnsim.h"
#include "utl/Logger.h"

namespace sta {
class dbSta;
class Scene;
}  // namespace sta

namespace est {
class EstimateParasitics;
}

namespace psm {
class IRNetwork;
class IRShort;

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

  using UserVoltages = odb::PtrMap<odb::dbNet, std::map<sta::Scene*, Voltage>>;
  using UserPowers = odb::PtrMap<odb::dbInst, std::map<sta::Scene*, Power>>;

  IRSolver(odb::dbNet* net,
           bool floorplanning,
           sta::dbSta* sta,
           est::EstimateParasitics* estimate_parasitics,
           utl::Logger* logger,
           const UserVoltages& user_voltages,
           const UserPowers& user_powers,
           const PDNSim::GeneratedSourceSettings& generated_source_settings);
  // Defined in the source file, since the shorts are only forward declared
  ~IRSolver();

  odb::dbNet* getNet() const { return net_; };

  bool check(bool check_bterms, bool check_placed);

  void solve(sta::Scene* corner,
             GeneratedSourceType source_type,
             const std::string& source_file);

  void report(sta::Scene* corner) const;
  void reportEM(sta::Scene* corner) const;

  Results getSolution(sta::Scene* corner) const;
  EMResults getEMSolution(sta::Scene* corner) const;
  PDNSim::IRDropByPoint getIRDrop(odb::dbTechLayer* layer,
                                  sta::Scene* corner) const;
  ConnectivityResults getConnectivityResults() const;

  void enableGui(bool enable);

  void writeErrorFile(const std::string& error_file) const;
  void writeInstanceVoltageFile(const std::string& voltage_file,
                                sta::Scene* corner) const;
  void writeEMFile(const std::string& em_file, sta::Scene* corner) const;
  void writeSpiceFile(GeneratedSourceType source_type,
                      const std::string& spice_file,
                      sta::Scene* corner,
                      const std::string& voltage_source_file) const;

  bool belongsTo(Node* node) const;
  bool belongsTo(Connection* connection) const;

  std::vector<sta::Scene*> getCorners() const;
  bool hasSolution(sta::Scene* corner) const;
  // Report that no solve has produced data for this corner, and stop, so a
  // report derived from one fails with a usable message instead of an
  // out-of-range map lookup. Each writer tests the map it actually reads
  // rather than hasSolution(): a grid solved with no powered instances has
  // voltages and an empty current map, and voltages are still reportable.
  void reportNoSolution(sta::Scene* corner) const;
  Voltage getNetVoltage(sta::Scene* corner) const;
  std::optional<Voltage> getVoltage(sta::Scene* corner, Node* node) const;

  std::optional<Voltage> getSDCVoltage(sta::Scene* corner,
                                       odb::dbNet* net) const;
  std::optional<Voltage> getPVTVoltage(sta::Scene* corner) const;
  std::optional<Voltage> getUserVoltage(sta::Scene* corner,
                                        odb::dbNet* net) const;
  std::optional<Voltage> getSolutionVoltage(sta::Scene* corner) const;

  odb::dbNet* getPowerNet() const;

  Connection::ResistanceMap getResistanceMap(sta::Scene* corner) const;
  void assertResistanceMap(sta::Scene* corner) const;

  IRNetwork* getNetwork() const { return network_.get(); }

 private:
  template <typename T>
  using ValueNodeMap = std::map<const Node*, T>;
  using LayerPolygons
      = odb::PtrMap<odb::dbTechLayer, std::vector<odb::Polygon>>;

  odb::dbBlock* getBlock() const;
  odb::dbTech* getTech() const;

  bool checkOpen();
  bool checkBTerms() const;
  bool checkShort(bool check_placed);
  // Walks every object that could short the net. Returns false when the
  // walk stopped early because the entry limit was reached, so the shorts
  // it collected are only part of what the design holds.
  bool findShorts(bool check_placed);
  // Adds one short to the list. Returns false when the list is already at
  // the entry limit and the short was not added.
  bool addShort(std::unique_ptr<IRShort> short_entry);
  // Checks one object against the net. Each returns false when the entry
  // limit was reached, so the caller stops walking.
  bool checkShortBPinBox(odb::dbBPin* bpin, odb::dbBox* box);
  bool checkShortNetBox(odb::dbNet* net, odb::dbBox* box);
  bool checkShortNetShape(odb::dbNet* net, const odb::dbShape& shape);
  // The obstructions of a master, clipped to the cell and with the pins of
  // the master removed from them.
  LayerPolygons getMasterObstructions(odb::dbMaster* master) const;
  // The layers where the instance holds a pin of the net under test.
  odb::PtrSet<odb::dbTechLayer> getNetPinLayers(odb::dbInst* inst) const;
  std::vector<odb::Polygon> determineShortShapes(odb::dbTechLayer* layer,
                                                 const odb::Polygon& polygon,
                                                 bool require_overlap
                                                 = false) const;
  std::vector<odb::Polygon> determineShortShapes(odb::dbTechLayer* layer,
                                                 const odb::Rect& rect) const;
  // The shape tree of a layer, or null when the net has no shape on it.
  // The trees are built once by checkShort, building one is linear in the
  // shapes of the layer so it cannot be done per object checked against it.
  const IRNetwork::ShapeTree* getShortCheckTree(odb::dbTechLayer* layer) const;

  odb::PtrMap<odb::dbInst, Power> getInstancePower(sta::Scene* corner) const;
  Voltage getPowerNetVoltage(sta::Scene* corner) const;

  Connection::ConnectionMap<Current> generateCurrentMap(
      sta::Scene* corner) const;

  Connection::ConnectionMap<Connection::Conductance> generateConductanceMap(
      sta::Scene* corner,
      const Connections& connections) const;
  Voltage generateSourceNodes(GeneratedSourceType source_type,
                              const std::string& source_file,
                              sta::Scene* corner,
                              SourceNodes& sources) const;
  SourceNodes generateSourceNodesFromBTerms() const;
  SourceNodes generateSourceNodesGenericFull() const;
  SourceNodes generateSourceNodesGenericStraps() const;
  SourceNodes generateSourceNodesGenericBumps() const;
  SourceNodes generateSourceNodesFromShapes(
      const std::set<odb::Rect>& shapes) const;
  Voltage generateSourceNodesFromSourceFile(const std::string& source_file,
                                            sta::Scene* corner,
                                            SourceNodes& sources) const;

  void reportUnconnectedNodes() const;
  void reportMissingBTerm() const;
  void reportShortedNodes() const;

  std::map<Node*, Connection::ConnectionSet> getNodeConnectionMap(
      const Connection::ConnectionMap<Connection::Conductance>& conductance)
      const;
  IRSolver::Power buildNodeCurrentMap(sta::Scene* corner,
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

  std::string getMetricKey(const std::string& key, sta::Scene* corner) const;

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
  std::map<sta::Scene*, Voltage> solution_voltages_;
  std::map<sta::Scene*, Power> solution_power_;

  const PDNSim::GeneratedSourceSettings& generated_source_settings_;

  std::optional<bool> connected_;

  std::map<sta::Scene*, ValueNodeMap<Voltage>> voltages_;
  std::map<sta::Scene*, ValueNodeMap<Current>> currents_;

  std::vector<std::unique_ptr<IRShort>> shorts_;
  odb::PtrMap<odb::dbTechLayer, IRNetwork::ShapeTree> short_check_trees_;

  static constexpr Current kSpiceFileMinCurrent = 1e-18;
  static constexpr size_t kMaxShortEntries = 10000;
};

}  // namespace psm
