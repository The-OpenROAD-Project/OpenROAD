/*
BSD 3-Clause License

Copyright (c) 2020, The Regents of the University of Minnesota

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include <optional>

#include "gmat.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace sta {
class dbSta;
class Corner;
}  // namespace sta

namespace rsz {
class Resizer;
}

namespace psm {

//! Class for IR solver
/*
 * Builds the equations GV=J and uses SuperLU
 * to solve the matrix equations
 */
struct ViaCut
{
  Point loc = Point(0, 0);
  NodeEnclosure bot_encl{0, 0, 0, 0};
  NodeEnclosure top_encl{0, 0, 0, 0};
};

class IRSolver
{
 public:
  struct SourceData
  {
    int x;
    int y;
    int size;
    double voltage;
    int layer;
    bool user_specified;
  };

  //! Constructor for IRSolver class
  /*
   * This constructor creates an instance of the class using
   * the given inputs.
   */
  IRSolver(odb::dbDatabase* db,
           sta::dbSta* sta,
           rsz::Resizer* resizer,
           utl::Logger* logger,
           odb::dbNet* net,
           const std::optional<float>& voltage,
           const std::string& vsrc_loc,
           bool em_analyze,
           int bump_pitch_x,
           int bump_pitch_y,
           float node_density_um,
           int node_density_factor_user,
           sta::Corner* corner);
  //! IRSolver destructor
  ~IRSolver();
  //! Returns the created G matrix for the design
  GMat* getGMat();
  //! Returns current map represented as a 1D vector
  const std::vector<double>& getJ() const;
  //! Function to solve for IR drop
  void solveIR();
  //! Function to get the power value from OpenSTA
  std::vector<std::pair<odb::dbInst*, float>> getPower();

  bool getConnectionTest() const;
  int getMinimumResolution() const;

  void writeSpiceFile(const std::string& file) const;
  void writeVoltageFile(const std::string& file) const;
  void writeEMFile(const std::string& file) const;
  void writeErrorFile(const std::string& file) const;

  bool build(const std::string& error_file = "",
             bool connectivity_only = false);

  const std::vector<SourceData>& getSources() const { return sources_; }

  double getWorstCaseVoltage() const { return wc_voltage_; }
  double getMaxCurrent() const { return max_cur_; }
  double getAvgCurrent() const { return avg_cur_; }
  int getNumResistors() const { return num_res_; }
  double getAvgVoltage() const { return avg_voltage_; }
  float getSupplyVoltageSrc() const
  {
    if (supply_voltage_src_.has_value()) {
      return supply_voltage_src_.value();
    } else {
      return 0.0;
    }
  }

 private:
  //! Function to add sources to the G matrix
  bool addSources();
  //! Function that parses the Vsrc file
  void readSourceData(bool require_voltage);
  void createSourcesFromVsrc(const std::string& file);
  bool createSourcesFromBTerms();
  bool createSourcesFromPads();
  void createDefaultSources();
  //! Function to create a J vector from the current map
  bool createJ(size_t gmat_nodes);
  //! Function to create a G matrix using the nodes
  bool createGmat(bool connection_only = false);
  //! Function to find and store the upper and lower PDN layers and return a
  //! list
  // of wires for all PDN tasks
  void findPdnWires();
  //! Function to create the nodes of vias in the G matrix
  void createGmatViaNodes();
  //! Function to create the nodes of wires in the G matrix
  void createGmatWireNodes();

  NodeEnclosure getViaEnclosure(int layer, odb::dbSet<odb::dbBox> via_boxes);

  std::map<Point, ViaCut> getViaCuts(Point loc,
                                     odb::dbSet<odb::dbBox> via_boxes,
                                     int lb,
                                     int lt,
                                     bool has_params,
                                     const odb::dbViaParams& params);

  //! Function to create the nodes for the sources
  int createSourceNodes(bool connection_only, int unit_micron);
  //! Function to create the connections of the G matrix
  void createGmatConnections(bool connection_only);
  bool checkConnectivity(const std::string& error_file = "",
                         bool connection_only = false);
  bool checkValidR(double R) const;

  double getResistance(odb::dbTechLayer* layer) const;

  std::pair<bool, std::set<Node*>> getInstNodes(odb::dbInst* inst) const;

  struct InstCompare
  {
    bool operator()(odb::dbInst* lhs, odb::dbInst* rhs) const
    {
      return lhs->getId() < rhs->getId();
    }
  };
  using ITermMap
      = std::map<odb::dbInst*, std::vector<odb::dbITerm*>, InstCompare>;
  void findUnconnectedInstances();
  void findUnconnectedInstancesByStdCells(
      ITermMap& iterms,
      std::set<odb::dbInst*>& connected_insts);
  void findUnconnectedInstancesByITerms(
      ITermMap& iterms,
      std::set<odb::dbInst*>& connected_insts);
  void findUnconnectedInstancesByAbutment(
      ITermMap& iterms,
      std::set<odb::dbInst*>& connected_insts);

  bool isStdCell(odb::dbInst* inst) const;
  bool isConnected(odb::dbITerm* iterm) const;

  std::optional<float> supply_voltage_src_;
  //! Worst case voltage at the lowest layer nodes
  double wc_voltage_{0};
  //! Worst case current at the lowest layer nodes
  double max_cur_{0};
  //! Average current at the lowest layer nodes
  double avg_cur_{0};
  //! number of resistances
  int num_res_{0};
  //! Average voltage at lowest layer nodes
  double avg_voltage_{0};
  //! Pointer to the Db
  odb::dbDatabase* db_;
  //! Pointer to STA
  sta::dbSta* sta_;
  //! Pointer to Resizer for parastics
  rsz::Resizer* resizer_;
  //! Pointer to Logger
  utl::Logger* logger_;

  odb::dbNet* net_;

  //! Voltage source file
  std::string vsrc_file_;
  bool em_flag_;
  //! G matrix for voltage
  std::unique_ptr<GMat> Gmat_;
  //! Node density in the lower most layer to append the current sources
  int node_density_{0};              // Initialize to zero
  int node_density_factor_{5};       // Default value
  int node_density_factor_user_{0};  // User defined value
  float node_density_um_{-1};  // Initialize to negative unless set by user
  //! Routing Level of the top layer
  int top_layer_{0};
  int bump_pitch_x_{0};
  int bump_pitch_y_{0};
  int bump_pitch_default_{140};
  int bump_size_{10};

  int bottom_layer_{10};

  bool connection_{false};

  sta::Corner* corner_;
  //! Current vector 1D
  std::vector<double> J_;
  //! source locations and values
  std::vector<SourceData> sources_;
  //! Locations of the source in the G matrix
  std::map<NodeIdx, double> source_nodes_;

  std::vector<odb::dbSBox*> power_wires_;
  std::vector<odb::dbInst*> unconnected_insts_;
  std::map<odb::dbITerm*, odb::dbITerm*> iterms_connected_by_abutment_;
};
}  // namespace psm
