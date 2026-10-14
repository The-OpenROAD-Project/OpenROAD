// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Eigen/Core"
#include "Eigen/Sparse"
#include "connection.h"
#include "ir_network_3d.h"

namespace utl {
class Logger;
}

namespace est {
class EstimateParasitics;
}
namespace sta {
class Scene;
}

namespace psm {

class IRSolver3D
{
 public:
  using ConductanceMatrix = Eigen::SparseMatrix<Connection::Conductance>;
  using Current = double;
  using CurrentVector = Eigen::VectorXd;
  using Voltage = double;
  using VoltageVector = Eigen::VectorXd;

  IRSolver3D(odb::dbChipNet* chip_net,
             utl::Logger* logger,
             est::EstimateParasitics* estimate_parasitics,
             sta::Scene* corner);

  // Build G and J after constructing the network and adding the loads.
  void buildCondMatrixAndVoltages();
  bool check() const;
  void addCurrentLoad(odb::dbChipInst* chip_inst,
                      const std::string& terminal,
                      Current current);
  // Called after network construction, using power from the active OpenSTA
  // scene.
  void addStaLoads(odb::dbNet* net,
                   const odb::PtrMap<odb::dbInst, float>& powers,
                   Voltage power_voltage);
  bool checkCurrentVector() const;
  void addVoltageSource(odb::dbChipInst* chip_inst,
                        const std::string& terminal,
                        Voltage voltage);
  bool solve();
  bool checkSolution() const;
  void report() const;
  void reportEM() const;
  void writeInstanceVoltageFile(const std::string& voltage_file) const;
  bool hasValidSources() const;

  IRNetwork3D* getNetwork() const { return network_.get(); }
  std::size_t getNodeCount() const { return node_index_.size(); }
  std::size_t getNonZeroCount() const
  {
    return static_cast<std::size_t>(g_matrix_.nonZeros());
  }
  std::size_t getCurrentLoadCount() const
  {
    return current_loads_.size() + sta_currents_.size();
  }
  Current getTotalCurrent() const { return j_vector_.sum(); }
  std::size_t getVoltageSourceCount() const { return voltage_sources_.size(); }
  bool hasSolution() const
  {
    return getNodeCount() > 0
           && voltage_vector_.size()
                  == static_cast<Eigen::Index>(getNodeCount())
           && network_->hasSameBonds();
  }
  const VoltageVector& getVoltageVector() const { return voltage_vector_; }
  std::optional<Voltage> getVoltage(odb::dbChipInst* chip_inst,
                                    const std::string& terminal) const;

 private:
  struct CurrentLoad
  {
    odb::dbChipInst* chip_inst;
    std::string terminal;
    Current current;
  };

  struct VoltageSource
  {
    odb::dbChipInst* chip_inst;
    std::string terminal;
    Voltage voltage;
  };

  Connection::ResistanceMap getResistanceMap() const;
  bool buildSourceMap(std::map<std::size_t, Voltage>& sources) const;

  utl::Logger* logger_;
  est::EstimateParasitics* estimate_parasitics_;
  sta::Scene* corner_;
  std::unique_ptr<IRNetwork3D> network_;
  std::map<Node*, std::size_t> node_index_;
  ConductanceMatrix g_matrix_;
  std::vector<CurrentLoad> current_loads_;
  std::map<Node*, Current> sta_currents_;
  double total_power_ = 0.0;
  odb::PtrMap<odb::dbNet, Voltage> nominal_voltages_;
  CurrentVector j_vector_;
  std::vector<VoltageSource> voltage_sources_;
  VoltageVector voltage_vector_;
};

}  // namespace psm
