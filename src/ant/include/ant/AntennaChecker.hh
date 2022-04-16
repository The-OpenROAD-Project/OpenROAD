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
  int antenna_cell_nums;
};

class AntennaChecker
{
 public:
  AntennaChecker();
  ~AntennaChecker();

  void init(odb::dbDatabase* db,
            utl::Logger *logger);

  void load_antenna_rules();

  int check_antennas(std::string report_filename, bool simple_report);

  void check_max_length(const char *net_name,
                        int layer);

  void find_max_wire_length();

  std::vector<ViolationInfo> get_net_antenna_violations(dbNet* net,
                                                std::string antenna_cell_name
                                                = "",
                                                std::string cell_pin = "");

private:
  dbNet* get_net(std::string net_name);

  template <class valueType>
  double defdist(valueType value);

  dbWireGraph::Node* find_segment_root(dbWireGraph::Node* node_info,
                                       int wire_level);
  dbWireGraph::Node* find_segment_start(dbWireGraph::Node* node);
  bool if_segment_root(dbWireGraph::Node* node, int wire_level);

  void find_wire_below_iterms(dbWireGraph::Node* node,
                              double iterm_areas[2],
                              int wire_level,
                              std::set<dbITerm*>& iv,
                              std::set<dbWireGraph::Node*>& nv);
  std::pair<double, double> calculate_wire_area(
      dbWireGraph::Node* node,
      int wire_level,
      std::set<dbWireGraph::Node*>& nv,
      std::set<dbWireGraph::Node*>& level_nodes);

  double get_via_area(dbWireGraph::Edge* edge);
  dbTechLayer* get_via_layer(dbWireGraph::Edge* edge);
  std::string get_via_name(dbWireGraph::Edge* edge);
  double calculate_via_area(dbWireGraph::Node* node, int wire_level);
  dbWireGraph::Edge* find_via(dbWireGraph::Node* node, int wire_level);

  void find_car_path(dbWireGraph::Node* node,
                     int wire_level,
                     dbWireGraph::Node* goal,
                     std::vector<dbWireGraph::Node*>& current_path,
                     std::vector<dbWireGraph::Node*>& path_found);

  void print_graph_info(dbWireGraph graph);
  void calculate_PAR_info(PARinfo& PARtable);
  bool check_iterm(dbWireGraph::Node* node, double iterm_areas[2]);
  double get_pwl_factor(odb::dbTechLayerAntennaRule::pwl_pair pwl_info,
                        double ref_val,
                        double def);

  void build_wire_PAR_table(std::vector<PARinfo>& PARtable,
                            std::vector<dbWireGraph::Node*> wireroots_info);
  void build_wire_CAR_table(std::vector<ARinfo>& CARtable,
                            std::vector<PARinfo> PARtable,
                            std::vector<PARinfo> VIA_PARtable,
                            std::vector<dbWireGraph::Node*> gate_iterms);
  void build_VIA_PAR_table(std::vector<PARinfo>& VIA_PARtable,
                           std::vector<dbWireGraph::Node*> wireroots_info);
  void build_VIA_CAR_table(std::vector<ARinfo>& VIA_CARtable,
                           std::vector<PARinfo> PARtable,
                           std::vector<PARinfo> VIA_PARtable,
                           std::vector<dbWireGraph::Node*> gate_iterms);

  std::vector<dbWireGraph::Node*> get_wireroots(dbWireGraph graph);

  std::pair<bool, bool> check_wire_PAR(ARinfo AntennaRatio, bool simple_report, bool print);
  std::pair<bool, bool> check_wire_CAR(ARinfo AntennaRatio, bool par_checked, bool simple_report, bool print);
  bool check_VIA_PAR(ARinfo AntennaRatio, bool simple_report, bool print);
  bool check_VIA_CAR(ARinfo AntennaRatio, bool simple_report, bool print);

  std::vector<int> GetAntennaRatio(std::string path, bool simple_report);

  void check_antenna_cell();

  bool check_violation(PARinfo par_info, dbTechLayer* layer);

  void find_wireroot_iterms(dbWireGraph::Node* node,
                            int wire_level,
                            std::vector<dbITerm*>& gates);
  std::vector<std::pair<double, std::vector<dbITerm*>>> PAR_max_wire_length(
      dbNet* net,
      int layer);
  std::vector<std::pair<double, std::vector<dbITerm*>>>
  get_violated_wire_length(dbNet* net, int routing_level);

  odb::dbDatabase* db_;
  utl::Logger *logger_;
  FILE* _out;
  std::map<odb::dbTechLayer*, ANTENNAmodel> layer_info;
};

}  // namespace ant
