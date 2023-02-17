///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
//
///////////////////////////////////////////////////////////////////////////////
// High-level description
// This is the interfaces for TritonPart
// It works in two ways:
// 1) default mode :  read a verilog netlist and extract the hypergraph based on
// netlist 2) classical mode : read the hypergraph file in hmetis format
///////////////////////////////////////////////////////////////////////////////
#include "TritonPart.h"

#include <set>
#include <string>

#include "KPMRefinement.h"
#include "TPCoarsener.h"
#include "TPHypergraph.h"
#include "TPMultilevel.h"
#include "TPPartitioner.h"
#include "TPRefiner.h"
#include "Utilities.h"
#include "odb/db.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/Bfs.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/ExceptionPath.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/Sequential.hh"
#include "sta/Sta.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

using utl::PAR;

namespace par {

// Read hypergraph from input files
void TritonPart::ReadHypergraph(std::string hypergraph_file,
                                std::string fixed_file)
{
  std::ifstream hypergraph_file_input(hypergraph_file);
  if (!hypergraph_file_input.is_open()) {
    logger_->error(PAR,
                   2500,
                   "Can not open the input hypergraph file : {}",
                   hypergraph_file);
  }

  // Set the flag variables
  // vertex_dimensions_ = 1;
  // hyperedge_dimensions_ = 1;
  // placement_dimensions_ = 0;
  timing_aware_flag_ = false;

  // Check the number of vertices, number of hyperedges, weight flag
  std::string cur_line;
  std::getline(hypergraph_file_input, cur_line);
  std::istringstream cur_line_buf(cur_line);
  std::vector<int> stats{std::istream_iterator<int>(cur_line_buf),
                         std::istream_iterator<int>()};
  num_hyperedges_ = stats[0];
  num_vertices_ = stats[1];
  bool hyperedge_weight_flag = false;
  bool vertex_weight_flag = false;
  if (stats.size() == 3) {
    if ((stats[2] % 10) == 1)
      hyperedge_weight_flag = true;

    if (stats[2] >= 10)
      vertex_weight_flag = true;
  }

  // clear vectors
  hyperedge_weights_.clear();
  nonscaled_hyperedge_weights_.clear();
  hyperedges_.clear();
  vertex_weights_.clear();

  // Read hyperedge information
  for (int i = 0; i < num_hyperedges_; i++) {
    std::getline(hypergraph_file_input, cur_line);
    if (hyperedge_weight_flag == true) {
      std::istringstream cur_line_buf(cur_line);
      std::vector<float> hvec{std::istream_iterator<float>(cur_line_buf),
                              std::istream_iterator<float>()};
      std::vector<float>::iterator breakpoint{hvec.begin()
                                              + hyperedge_dimensions_};
      // read first hyperedge_dimensions_ elements as hyperege weights
      // std::vector<float> hwts(hvec.begin(), std::advance(hvec.begin(),
      // hyperedge_dimensions_));
      std::vector<float> hwts(hvec.begin(), breakpoint);
      // read remaining elements as hyperedge
      // std::vector<int> hyperedge(std::advance(hvec.begin(),
      // hyperedge_dimensions_), hvec.end());
      std::vector<int> hyperedge(breakpoint, hvec.end());
      for (auto& value : hyperedge)
        value--;
      if (hyperedge.size() > global_net_threshold_)
        continue;

      hyperedge_weights_.push_back(hwts);
      if (timing_attr_.size() > 0) {
        nonscaled_hyperedge_weights_.push_back(hwts);
      }
      hyperedges_.push_back(hyperedge);
    } else {
      std::istringstream cur_line_buf(cur_line);
      std::vector<int> hyperedge{std::istream_iterator<int>(cur_line_buf),
                                 std::istream_iterator<int>()};
      for (auto& value : hyperedge)
        value--;
      std::vector<float> hwts(hyperedge_dimensions_, 1.0);
      if (hyperedge.size() > global_net_threshold_)
        continue;
      hyperedge_weights_.push_back(hwts);
      if (timing_attr_.size() > 0) {
        nonscaled_hyperedge_weights_.push_back(hwts);
      }
      hyperedges_.push_back(hyperedge);
    }
  }

  // Read weight for vertices
  for (int i = 0; i < num_vertices_; i++) {
    if (vertex_weight_flag == true) {
      std::getline(hypergraph_file_input, cur_line);
      std::istringstream cur_line_buf(cur_line);
      std::vector<float> vwts{std::istream_iterator<float>(cur_line_buf),
                              std::istream_iterator<float>()};
      vertex_weights_.push_back(vwts);
    } else {
      std::vector<float> vwts(vertex_dimensions_, 1.0);
      vertex_weights_.push_back(vwts);
    }
  }

  // Read fixed vertices
  if (fixed_file.size() > 0) {
    int part_id = -1;
    fixed_vertex_flag_ = true;
    std::ifstream fixed_file_input(fixed_file);
    if (!fixed_file_input.is_open()) {
      logger_->error(PAR, 2501, "Can not open the fixed file : {}", fixed_file);
    }
    for (int i = 0; i < num_vertices_; i++) {
      fixed_file_input >> part_id;
      fixed_attr_.push_back(part_id);
    }
    fixed_file_input.close();
  }
  num_vertices_ = vertex_weights_.size();
  num_hyperedges_ = hyperedge_weights_.size();
}

// Convert the netlist into hypergraphs
void TritonPart::ReadNetlist()
{
  // initialize other parameters
  // Set the flag variables
  vertex_dimensions_ = 1;
  hyperedge_dimensions_ = 1;
  placement_dimensions_ = 0;
  timing_aware_flag_ = true;

  // assign vertex_id property of each instance and each IO port
  int vertex_id = 0;
  for (auto term : block_->getBTerms()) {
    odb::dbIntProperty::create(term, "vertex_id", vertex_id++);
    std::vector<float> vwts(vertex_dimensions_, 0.0);
    vertex_weights_.push_back(vwts);
  }
  const float dbu = db_->getTech()->getDbUnitsPerMicron();
  for (auto inst : block_->getInsts()) {
    odb::dbIntProperty::create(inst, "vertex_id", vertex_id++);
    const odb::dbMaster* master = inst->getMaster();
    const float area = master->getWidth() * master->getHeight() / dbu / dbu;
    std::vector<float> vwts(vertex_dimensions_, area);
    vertex_weights_.push_back(vwts);
  }
  num_vertices_ = vertex_id;

  // Each net correponds to an hyperedge
  // Traverse the hyperedge and assign hyperedge id
  int hyperedge_id = 0;
  for (auto net : block_->getNets()) {
    odb::dbIntProperty::create(net, "hyperedge_id", -1);
    // ignore all the power net
    if (net->getSigType().isSupply())
      continue;
    // check the hyperedge
    int driver_id = -1;      // vertex id of the driver instance
    std::set<int> loads_id;  // vertex id of sink instances
    // check the connected instances
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      const int vertex_id
          = odb::dbIntProperty::find(inst, "vertex_id")->getValue();
      if (iterm->getIoType() == odb::dbIoType::OUTPUT)
        driver_id = vertex_id;
      else
        loads_id.insert(vertex_id);
    }
    // check the connected IO pins
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int vertex_id
          = odb::dbIntProperty::find(bterm, "vertex_id")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT)
        driver_id = vertex_id;
      else
        loads_id.insert(vertex_id);
    }
    // check the hyperedges
    std::vector<int> hyperedge;
    if (driver_id != -1 && loads_id.size() > 0) {
      hyperedge.push_back(driver_id);
      for (auto& load_id : loads_id)
        if (load_id != driver_id)
          hyperedge.push_back(load_id);
    }
    // Ignore all the single-vertex hyperedge
    if (hyperedge.size() > 1 && hyperedge.size() <= global_net_threshold_) {
      hyperedges_.push_back(hyperedge);
      hyperedge_weights_.push_back(
          std::vector<float>(hyperedge_dimensions_, 1.0));
      odb::dbIntProperty::find(net, "hyperedge_id")->setValue(hyperedge_id++);
    }
  }  // finish hyperedge

  num_hyperedges_ = static_cast<int>(hyperedges_.size());

  // add timing feature
  if (timing_aware_flag_ == true) {
    logger_->report("[STATUS] Extracting timing paths**** ");
    BuildTimingPaths();  // create timing paths
  }

  nonscaled_hyperedge_weights_ = hyperedge_weights_;
}

// Find all the critical timing paths
// The codes below similar to gui/src/staGui.cpp
// Please refer to sta/Search/ReportPath.cc for how to check the timing path
void TritonPart::BuildTimingPaths()
{
  if (timing_aware_flag_ == false || top_n_ <= 0)
    return;

  sta_->ensureGraph();     // Ensure that the timing graph has been built
  sta_->searchPreamble();  // Make graph and find delays

  sta::ExceptionFrom* e_from = nullptr;
  sta::ExceptionThruSeq* e_thrus = nullptr;
  sta::ExceptionTo* e_to = nullptr;
  bool include_unconstrained = false;
  bool get_max = true;  // max for setup check, min for hold check
  // Timing paths are grouped into path groups according to the clock
  // associated with the endpoint of the path, for example, path group for clk
  int group_count = top_n_;
  int endpoint_count = 1;  // The number of paths to report for each endpoint.
  // Definition for findPathEnds function in Search.hh
  // PathEndSeq *findPathEnds(ExceptionFrom *from,
  //              ExceptionThruSeq *thrus,
  //              ExceptionTo *to,
  //              bool unconstrained,
  //              const Corner *corner,
  //              const MinMaxAll *min_max,
  //              int group_count,
  //              int endpoint_count,
  //              bool unique_pins,
  //              float slack_min,
  //              float slack_max,
  //              bool sort_by_slack,
  //              PathGroupNameSet *group_names,
  //              bool setup,
  //              bool hold,
  //              bool recovery,
  //              bool removal,
  //              bool clk_gating_setup,
  //              bool clk_gating_hold);
  // PathEnds represent search endpoints that are either unconstrained or
  // constrained by a timing check, output delay, data check, or path delay.
  sta::PathEndSeq* path_ends
      = sta_->search()->findPathEnds(  // from, thrus, to, unconstrained
          e_from,                      // return paths from a list of
                   // clocks/instances/ports/register clock pins or latch data
                   // pins
          e_thrus,  // return paths through a list of instances/ports/nets
          e_to,     // return paths to a list of clocks/instances/ports or pins
          include_unconstrained,  // return unconstrained paths
          // corner, min_max,
          sta_->cmdCorner(),  // return paths for a process corner
          get_max ? sta::MinMaxAll::max()
                  : sta::MinMaxAll::min(),  // return max/min paths checks
          // group_count, endpoint_count, unique_pins
          group_count,     // path_count used by GUI, not sure what happened
          endpoint_count,  // path_count used by GUI, not sure what happened
          true,
          -sta::INF,
          sta::INF,  // slack_min, slack_max,
          true,      // sort_by_slack
          nullptr,   // group_names
          // setup, hold, recovery, removal,
          get_max,
          !get_max,
          false,
          false,
          // clk_gating_setup, clk_gating_hold
          false,
          false);

  logger_->report("[INFO] Timing scale : {} second",
                  sta_->units()->timeUnit()->scale());

  // check all the timing paths
  for (auto& path_end : *path_ends) {
    // Printing timing paths to logger
    // sta_->reportPathEnd(path_end);
    auto* path = path_end->path();
    TimingPath timing_path;               // create the timing path
    float slack = path_end->slack(sta_);  // slack information
    // hack to force all paths to be timing critical, this is only for testing
    if (slack > 0) {
      slack = slack * -1.0;
    }
    // normalize the slack according to the clock period
    const float clock_period = path_end->targetClk(sta_)->period();
    slack = 1.0 - (slack / clock_period);
    // slack = slack / sta_->search()->units()->timeUnit()->scale();
    timing_path.slack = slack;
    sta::PathExpanded expand(path, sta_);
    expand.path(expand.size() - 1);
    for (size_t i = 0; i < expand.size(); i++) {
      // PathRef is reference to a path vertex
      sta::PathRef* ref = expand.path(i);
      sta::Pin* pin = ref->vertex(sta_)->pin();
      // Nets connect pins at a level of the hierarchy
      auto net = network_->net(pin);  // sta::Net*
      // Check if the pin is connected to a net
      if (net == nullptr)
        continue;  // check if the net exists
      if (network_->isTopLevelPort(pin) == true) {
        auto bterm = block_->findBTerm(network_->pathName(pin));
        int vertex_id
            = odb::dbIntProperty::find(bterm, "vertex_id")->getValue();
        if (std::find(
                timing_path.path.begin(), timing_path.path.end(), vertex_id)
            == timing_path.path.end())
          timing_path.path.push_back(vertex_id);
      } else {
        auto inst = network_->instance(pin);
        auto dbinst = block_->findInst(network_->pathName(inst));
        int vertex_id
            = odb::dbIntProperty::find(dbinst, "vertex_id")->getValue();
        if (std::find(
                timing_path.path.begin(), timing_path.path.end(), vertex_id)
            == timing_path.path.end())
          timing_path.path.push_back(vertex_id);
      }

      auto dbnet = block_->findNet(
          network_->pathName(net));  // convert sta::Net* to dbNet*
      int hyperedge_id
          = odb::dbIntProperty::find(dbnet, "hyperedge_id")->getValue();
      if (hyperedge_id == -1) {
        continue;
      }
      if (std::find(
              timing_path.arcs.begin(), timing_path.arcs.end(), hyperedge_id)
          == timing_path.arcs.end())
        timing_path.arcs.push_back(hyperedge_id);
    }
    // add timing path
    timing_paths_.push_back(timing_path);
    timing_attr_.push_back(slack);
  }

  // release memory
  delete path_ends;
}

void TritonPart::GenerateTimingReport(std::vector<int>& partition, bool design)
{
  std::vector<int> critical_path_cuts;
  for (int i = 0; i < timing_paths_.size(); ++i) {
    auto timing_path = timing_paths_[i].path;
    std::vector<int> path_block;
    for (int j = 0; j < timing_path.size(); ++j) {
      int block_id = partition[timing_path[j]];
      if (path_block.size() == 0 || path_block.back() != block_id) {
        path_block.push_back(block_id);
      }
    }
    critical_path_cuts.push_back(path_block.size() - 1);
  }

  std::string timing_report
      = design == true ? "design.timing.rpt" : "hypergraph.timing.rpt";
  std::ofstream timing_report_output;
  timing_report_output.open(timing_report);
  for (int i = 0; i < critical_path_cuts.size(); ++i) {
    timing_report_output << i << "," << critical_path_cuts[i] << std::endl;
  }
  timing_report_output.close();
}

// Create the hypergraph object hypergraph_
void TritonPart::BuildHypergraph()
{
  // add hyperedge
  std::vector<int> eind;
  std::vector<int> eptr;  // hyperedges
  eptr.push_back(static_cast<int>(eind.size()));
  for (auto hyperedge : hyperedges_) {
    eind.insert(eind.end(), hyperedge.begin(), hyperedge.end());
    eptr.push_back(static_cast<int>(eind.size()));
  }
  // add vertex
  // create vertices from hyperedges
  std::vector<std::vector<int>> vertices(num_vertices_);
  for (int i = 0; i < num_hyperedges_; i++)
    for (auto v : hyperedges_[i])
      vertices[v].push_back(i);  // i is the hyperedge id
  std::vector<int> vind;
  std::vector<int> vptr;  // vertices
  vptr.push_back(static_cast<int>(vind.size()));
  for (auto& vertex : vertices) {
    vind.insert(vind.end(), vertex.begin(), vertex.end());
    vptr.push_back(static_cast<int>(vind.size()));
  }

  // Convert the timing information
  std::vector<int> vind_p;  // each timing path is a sequences of vertices
  std::vector<int> vptr_p;
  std::vector<int>
      pind_v;  // store all the timing paths connected to the vertex
  std::vector<int> pptr_v;
  matrix<int> incident_paths(num_vertices_);
  vptr_p.push_back(static_cast<int>(vind_p.size()));
  for (int i = 0; i < timing_paths_.size(); ++i) {
    auto timing_path = timing_paths_[i].path;
    vind_p.insert(vind_p.end(), timing_path.begin(), timing_path.end());
    vptr_p.push_back(static_cast<int>(vind_p.size()));
    for (int j = 0; j < timing_path.size(); ++j) {
      int v = timing_path[j];
      incident_paths[v].push_back(i);
    }
  }
  pptr_v.push_back(pind_v.size());
  for (auto& paths : incident_paths) {
    pind_v.insert(pind_v.end(), paths.begin(), paths.end());
    pptr_v.push_back(static_cast<int>(pind_v.size()));
  }

  // Update hyperedge weights according to slack
  std::vector<std::set<float>> net_based_slacks(num_hyperedges_);
  std::vector<float> total_net_based_slacks(num_hyperedges_, 0.0);
  for (int i = 0; i < timing_paths_.size(); ++i) {
    auto timing_arcs = timing_paths_[i].arcs;
    float slack = timing_paths_[i].slack;
    for (int j = 0; j < timing_arcs.size(); ++j) {
      int he = timing_arcs[j];
      net_based_slacks[he].insert(slack);
      total_net_based_slacks[he] += slack;
    }
  }

  for (int i = 0; i < num_hyperedges_; ++i) {
    if (net_based_slacks[i].size() == 0) {
      continue;
    }
    float total_wt = total_net_based_slacks[i];
    auto hwt = hyperedge_weights_[i];
    hyperedge_weights_[i] = MultiplyFactor(hwt, total_wt);
  }

  // create TPHypergraph
  hypergraph_ = std::make_shared<TPHypergraph>(num_vertices_,
                                               num_hyperedges_,
                                               vertex_dimensions_,
                                               hyperedge_dimensions_,
                                               eind,
                                               eptr,
                                               vind,
                                               vptr,
                                               vertex_weights_,
                                               hyperedge_weights_,
                                               hyperedge_weights_,
                                               fixed_attr_,
                                               community_attr_,
                                               placement_dimensions_,
                                               placement_attr_,
                                               vind_p,
                                               vptr_p,
                                               pind_v,
                                               pptr_v,
                                               timing_attr_,
                                               logger_);
}

// Write the hypergraph to file
void TritonPart::WriteHypergraph(const std::string& hypergraph_filename)
{
  float max_vtx_wt = -std::numeric_limits<float>::max();
  float max_he_wt = max_vtx_wt;

  // Scaling hyperedge weights so that hmetis/TritonPart is aware of these
  // weights
  auto hwts = hyperedge_weights_;

  for (int i = 0; i < num_hyperedges_; ++i) {
    auto hwt = hyperedge_weights_[i];
    if (hwt.front() < 1.0) {
      hwts[i].front() = 1.0;
    }
  }

  for (int i = 0; i < num_vertices_; ++i) {
    auto vwt = vertex_weights_[i];
    if (vwt.front() > max_vtx_wt) {
      max_vtx_wt = vwt.front();
    }
  }
  for (int i = 0; i < num_hyperedges_; ++i) {
    // auto hwt = hyperedge_weights_[i];
    auto hwt = hwts[i];
    if (hwt.front() > max_he_wt) {
      max_he_wt = hwt.front();
    }
  }
  int wt_type;
  if (max_vtx_wt > 1.0 && max_he_wt > 1.0) {
    wt_type = 11;
  } else if (max_vtx_wt > 1.0 && max_he_wt == 1.0) {
    wt_type = 10;
  } else if (max_vtx_wt == 1.0 && max_he_wt > 1.0) {
    wt_type = 1;
  } else {
    wt_type = 0;
  }

  // hack to print hypergraph without timing-scaled weights on hypergraph edges
  std::ofstream hypergraph_file(hypergraph_filename);
  if (wt_type == 0) {
    hypergraph_file << num_hyperedges_ << " " << num_vertices_ << std::endl;
  } else {
    hypergraph_file << num_hyperedges_ << " " << num_vertices_ << " " << wt_type
                    << std::endl;
  }
  for (int i = 0; i < num_hyperedges_; ++i) {
    if (wt_type == 1 || wt_type == 11) {
      // hypergraph_file << hyperedge_weights_[i].front() << " ";
      auto he_wt = static_cast<int>(ceil(hwts[i].front()));
      if (he_wt == 0) {
        he_wt = 1;
      }
      hypergraph_file << he_wt << " ";
    }
    for (auto& v : hyperedges_[i]) {
      hypergraph_file << v + 1 << " ";
    }
    hypergraph_file << std::endl;
  }
  if (wt_type == 10 || wt_type == 11) {
    for (int i = 0; i < num_vertices_; ++i) {
      hypergraph_file << vertex_weights_[i].front() << std::endl;
    }
  }
}

// Write timing paths to file for other applications
void TritonPart::WritePathsToFile(const std::string& paths_filename)
{
  std::ofstream paths_file(paths_filename);

  for (int i = 0; i < timing_paths_.size(); ++i) {
    auto timing_path = timing_paths_[i].path;
    for (auto& v : timing_path) {
      paths_file << v << " ";
    }
    paths_file << std::endl;
  }
}

// Partition the design
// The first step is to convert the netlist into a hypergraph
// The second step is to get all the features such as timing paths
void TritonPart::tritonPartDesign(unsigned int num_parts_arg,
                                  float balance_constraint_arg,
                                  unsigned int seed_arg,
                                  const std::string& solution_filename,
                                  const std::string& paths_filename,
                                  const std::string& hypergraph_filename)
{
  logger_->report("========================================");
  logger_->report("[STATUS] Starting TritonPart Partitioner");
  logger_->report("========================================");
  logger_->report("[INFO] Partitioning parameters**** ");

  // Parameters
  num_parts_ = num_parts_arg;
  ub_factor_ = balance_constraint_arg;
  seed_ = seed_arg;
  srand(seed_);  // set the random seed
  logger_->report("[PARAM] Number of partitions = {}", num_parts_);
  logger_->report("[PARAM] UBfactor = {}", ub_factor_);
  logger_->report("[PARAM] Vertex dimensions = {}", vertex_dimensions_);
  logger_->report("[PARAM] Hyperedge dimensions = {}", hyperedge_dimensions_);
  // only for TritonPartDesign, we need the block_ information
  block_ = db_->getChip()->getBlock();
  // build hypergraph
  // for IO port and insts (std cells and macros),
  // there is an attribute for vertex_id
  logger_->report("========================================");
  logger_->report("[STATUS] Reading netlist**** ");
  ReadNetlist();
  // Check how many unique endpoints are extracted by STA
  std::set<int> endpoints;
  for (int i = 0; i < timing_paths_.size(); ++i) {
    auto path = timing_paths_[i].path;
    endpoints.insert(path.back());
  }
  if (!paths_filename.empty()) {
    WritePathsToFile(paths_filename);
  }
  BuildHypergraph();
  logger_->report("[STATUS] Building hypergraph**** ");
  logger_->report("[STATUS] Writing hypergraph**** ");
  logger_->report("========================================");
  if (!hypergraph_filename.empty()) {
    WriteHypergraph(hypergraph_filename);
  }

  logger_->report("[INFO] Hypergraph Information**");
  logger_->report("[INFO] Vertices = {}", num_vertices_);
  logger_->report("[INFO] Hyperedges = {}", num_hyperedges_);
  logger_->report("[INFO] Timing paths = {}", hypergraph_->GetNumTimingPaths());
  logger_->report("[INFO] Unique endpoints extracted = {}", endpoints.size());
  if (!timing_attr_.empty()) {
    logger_->report(
        "[INFO] Worst negative slack = {}",
        *std::max_element(timing_attr_.begin(), timing_attr_.end()));
  }
  std::vector<int> partition;
  if (num_parts_ == 2) {
    partition = TritonPart_design_PartTwoWay(num_parts_,
                                             ub_factor_,
                                             vertex_dimensions_,
                                             hyperedge_dimensions_,
                                             seed_);
  } else {
    partition = TritonPart_design_PartKWay(num_parts_,
                                           ub_factor_,
                                           vertex_dimensions_,
                                           hyperedge_dimensions_,
                                           seed_);
  }
  // AnalyzeTimingCuts();
  if (!solution_filename.empty()) {
    WriteSolution(solution_filename.c_str(), partition);
  }
  std::vector<std::vector<int>> timing_paths;
  for (auto& tpath : timing_paths_) {
    timing_paths.push_back(tpath.path);
  }
  auto timing_cuts = AnalyzeTimingOfPartition(timing_paths, partition);
  logger_->report("===============================================");
  logger_->report("[STATUS] Displaying timing path cuts statistics");
  logger_->report("[INFO] Total timing critical paths = {}",
                  timing_cuts->GetTotalPaths());
  logger_->report("[INFO] Total paths cut = {}",
                  timing_cuts->GetTotalCriticalPathsCut());
  logger_->report("[INFO] Worst cut on a path = {}",
                  timing_cuts->GetWorstCut());
  logger_->report("[INFO] Average cuts on critical paths = {}",
                  timing_cuts->GetAvereageCriticalPathsCut());
  logger_->report("===============================================");
  logger_->report("Exiting TritonPart");
}

HGraph TritonPart::preProcessHypergraph()
{
  logger_->report(
      "Pre-processing hypergraph by temporarily removing hyperedges of size {}",
      he_size_threshold_);
  std::vector<std::vector<int>> hyperedges_p;
  std::vector<std::vector<float>> hyperedge_weights_p;
  int num_hyperedges_p = 0;
  for (int i = 0; i < num_hyperedges_; ++i) {
    const int he_size = hyperedges_[i].size();
    if (he_size <= he_size_threshold_) {
      std::vector<int> he = hyperedges_[i];
      hyperedges_p.push_back(he);
      std::vector<float> hwt = hyperedge_weights_[i];
      hyperedge_weights_p.push_back(hwt);
      ++num_hyperedges_p;
    }
  }

  // add hyperedge
  std::vector<int> eind_p;
  std::vector<int> eptr_p;  // hyperedges
  eptr_p.push_back(static_cast<int>(eind_p.size()));
  for (const auto& hyperedge : hyperedges_p) {
    eind_p.insert(eind_p.end(), hyperedge.begin(), hyperedge.end());
    eptr_p.push_back(static_cast<int>(eind_p.size()));
  }
  // add vertex
  // create vertices from hyperedges
  std::vector<std::vector<int>> vertices_p(num_vertices_);
  for (int i = 0; i < num_hyperedges_p; i++)
    for (auto v : hyperedges_p[i])
      vertices_p[v].push_back(i);  // i is the hyperedge id
  std::vector<int> vind_p;
  std::vector<int> vptr_p;  // vertices
  vptr_p.push_back(static_cast<int>(vind_p.size()));
  for (auto& vertex : vertices_p) {
    vind_p.insert(vind_p.end(), vertex.begin(), vertex.end());
    vptr_p.push_back(static_cast<int>(vind_p.size()));
  }

  // Convert the timing information
  std::vector<int> vind_p_p;  // each timing path is a sequences of vertices
  std::vector<int> vptr_p_p;
  std::vector<int>
      pind_v_p;  // store all the timing paths connected to the vertex
  std::vector<int> pptr_v_p;
  std::vector<float> timing_attr_p;

  // create TPHypergraph
  HGraph hypergraph_p
      = std::make_shared<TPHypergraph>(num_vertices_,
                                       num_hyperedges_p,
                                       vertex_dimensions_,
                                       hyperedge_dimensions_,
                                       eind_p,
                                       eptr_p,
                                       vind_p,
                                       vptr_p,
                                       vertex_weights_,
                                       hyperedge_weights_p,
                                       fixed_attr_,
                                       community_attr_,
                                       placement_dimensions_,
                                       placement_attr_,
                                       hypergraph_->vind_p_,
                                       hypergraph_->vptr_p_,
                                       hypergraph_->pind_v_,
                                       hypergraph_->pptr_v_,
                                       hypergraph_->timing_attr_,
                                       logger_);
  return hypergraph_p;
}

void TritonPart::tritonPartHypergraph(const char* hypergraph_file_arg,
                                      const char* fixed_file_arg,
                                      unsigned int num_parts_arg,
                                      float balance_constraint_arg,
                                      int vertex_dimension_arg,
                                      int hyperedge_dimension_arg,
                                      unsigned int seed_arg)
{
  std::vector<int> partition;
  if (num_parts_arg == 2) {
    partition = TritonPart_hypergraph_PartTwoWay(hypergraph_file_arg,
                                                 fixed_file_arg,
                                                 num_parts_arg,
                                                 balance_constraint_arg,
                                                 vertex_dimension_arg,
                                                 hyperedge_dimension_arg,
                                                 seed_arg);
  } else {
    partition = TritonPart_hypergraph_PartKWay(hypergraph_file_arg,
                                               fixed_file_arg,
                                               num_parts_arg,
                                               balance_constraint_arg,
                                               vertex_dimension_arg,
                                               hyperedge_dimension_arg,
                                               seed_arg);
  }
  std::string solution_file = std::string(hypergraph_file_arg)
                              + std::string(".part.")
                              + std::to_string(num_parts_);
  WriteSolution(solution_file.c_str(), partition);
}

std::vector<int> TritonPart::TritonPart_hypergraph_PartTwoWay(
    const char* hypergraph_file_arg,
    const char* fixed_file_arg,
    unsigned int num_parts_arg,
    float balance_constraint_arg,
    int vertex_dimension_arg,
    int hyperedge_dimension_arg,
    unsigned int seed_arg)
{
  logger_->report("Starting TritonPart Partitioner");
  auto start_time_stamp_global = std::chrono::high_resolution_clock::now();
  // Parameters
  num_parts_ = num_parts_arg;
  ub_factor_ = balance_constraint_arg;
  seed_ = seed_arg;
  vertex_dimensions_ = vertex_dimension_arg;
  hyperedge_dimensions_ = hyperedge_dimension_arg;
  // local parameters
  std::string hypergraph_file = hypergraph_file_arg;
  std::string fixed_file = fixed_file_arg;
  logger_->report("Partition Parameters**");
  logger_->report("Number of partitions = {}", num_parts_);
  logger_->report("UBfactor = {}", ub_factor_);
  logger_->report("Vertex dimensions = {}", vertex_dimensions_);
  logger_->report("Hyperedge dimensions = {}", hyperedge_dimensions_);
  // build hypergraph
  ReadHypergraph(hypergraph_file, fixed_file);
  BuildHypergraph();
  logger_->report("Hypergraph Information**");
  logger_->report("#Vertices = {}", num_vertices_);
  logger_->report("#Hyperedges = {}", num_hyperedges_);
  // process hypergraph
  HGraph hypergraph_processed = preProcessHypergraph();
  logger_->report("Post processing hypergraph information**");
  logger_->report("#Vertices = {}", hypergraph_processed->GetNumVertices());
  logger_->report("#Hyperedges = {}", hypergraph_processed->GetNumHyperedges());
  // create coarsening class
  const std::vector<float> e_wt_factors(hyperedge_dimensions_, 1.0);
  const std::vector<float> v_wt_factors(vertex_dimensions_, 1.0);
  const std::vector<float> p_wt_factors(placement_dimensions_ + 100, 1.0);
  const float timing_factor = 1.0;
  const int path_traverse_step = 2;
  const std::vector<float> tot_vertex_weights
      = hypergraph_->GetTotalVertexWeights();
  const int alpha = 4;
  const std::vector<float> max_vertex_weights
      = DivideFactor(hypergraph_->GetTotalVertexWeights(), alpha * num_parts_);
  const int thr_coarsen_hyperedge_size = 50;
  global_net_threshold_ = 500;
  const int thr_coarsen_vertices = 200;
  const int thr_coarsen_hyperedges = 50;
  const float coarsening_ratio = 1.5;
  const int max_coarsen_iters = 20;
  const float adj_diff_ratio = 0.0001;
  TP_coarsening_ptr tritonpart_coarsener
      = std::make_shared<TPcoarsener>(e_wt_factors,
                                      v_wt_factors,
                                      p_wt_factors,
                                      timing_factor,
                                      path_traverse_step,
                                      max_vertex_weights,
                                      thr_coarsen_hyperedge_size,
                                      global_net_threshold_,
                                      thr_coarsen_vertices,
                                      thr_coarsen_hyperedges,
                                      coarsening_ratio,
                                      max_coarsen_iters,
                                      adj_diff_ratio,
                                      seed_,
                                      logger_);
  float path_wt_factor = 1.0;
  float snaking_wt_factor = 1.0;
  const int refiner_iters = 2;
  const int max_moves = 50;
  int refiner_choice = TWO_WAY_FM;
  TP_two_way_refining_ptr tritonpart_twoway_refiner
      = std::make_shared<TPtwoWayFM>(num_parts_,
                                     refiner_iters,
                                     max_moves,
                                     refiner_choice,
                                     seed_,
                                     e_wt_factors,
                                     path_wt_factor,
                                     snaking_wt_factor,
                                     logger_);
  const int greedy_refiner_iters = 2;
  const int greedy_max_moves = 10;
  refiner_choice = GREEDY;
  TP_greedy_refiner_ptr tritonpart_greedy_refiner
      = std::make_shared<TPgreedyRefine>(num_parts_,
                                         greedy_refiner_iters,
                                         greedy_max_moves,
                                         refiner_choice,
                                         seed_,
                                         e_wt_factors,
                                         path_wt_factor,
                                         snaking_wt_factor,
                                         logger_);
  int wavefront = 50;
  TP_ilp_refiner_ptr tritonpart_ilp_refiner
      = std::make_shared<TPilpRefine>(num_parts_,
                                      greedy_refiner_iters,
                                      greedy_max_moves,
                                      refiner_choice,
                                      seed_,
                                      e_wt_factors,
                                      path_wt_factor,
                                      snaking_wt_factor,
                                      logger_,
                                      wavefront);
  float early_stop_ratio = 0.5;
  int max_num_fm_pass = 10;
  TP_partitioning_ptr tritonpart_partitioner
      = std::make_shared<TPpartitioner>(num_parts_,
                                        e_wt_factors,
                                        path_wt_factor,
                                        snaking_wt_factor,
                                        early_stop_ratio,
                                        max_num_fm_pass,
                                        seed_,
                                        tritonpart_twoway_refiner,
                                        logger_);
  bool v_cycle_flag = true;
  RefinerType refine_type = KPM_REFINEMENT;
  int num_initial_solutions = 50;       // number of initial random solutions
  int num_best_initial_solutions = 10;  // number of best initial solutions
  int num_ubfactor_delta = 5;  // allowing marginal imbalance to improve QoR
  int max_num_vcycle = 5;      // maximum number of vcycles
  TP_mlevel_partitioning_ptr tritonpart_mlevel_partitioner
      = std::make_shared<TPmultilevelPartitioner>(tritonpart_coarsener,
                                                  tritonpart_partitioner,
                                                  tritonpart_twoway_refiner,
                                                  tritonpart_greedy_refiner,
                                                  tritonpart_ilp_refiner,
                                                  num_parts_,
                                                  v_cycle_flag,
                                                  num_initial_solutions,
                                                  num_best_initial_solutions,
                                                  num_ubfactor_delta,
                                                  max_num_vcycle,
                                                  seed_,
                                                  ub_factor_,
                                                  refine_type,
                                                  logger_);
  bool vcycle = true;
  matrix<float> vertex_balance
      = hypergraph_->GetVertexBalance(num_parts_, ub_factor_);
  auto solution = tritonpart_mlevel_partitioner->PartitionTwoWay(
      hypergraph_, hypergraph_processed, vertex_balance, vcycle);
  auto cut_pair
      = tritonpart_partitioner->GoldenEvaluator(hypergraph_, solution, true);
  auto end_timestamp_global = std::chrono::high_resolution_clock::now();
  double total_global_time
      = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_timestamp_global - start_time_stamp_global)
            .count();
  total_global_time *= 1e-9;
  logger_->info(PAR, 6, "Total runtime {} seconds", total_global_time);
  return solution;
}

std::vector<int> TritonPart::TritonPart_design_PartTwoWay(
    unsigned int num_parts_,
    float ub_factor_,
    int vertex_dimensions_,
    int hyperedge_dimensions_,
    unsigned int seed_)
{
  auto start_time_stamp_global = std::chrono::high_resolution_clock::now();
  // create coarsening class
  const std::vector<float> e_wt_factors(hyperedge_dimensions_, 1.0);
  const std::vector<float> v_wt_factors(vertex_dimensions_, 1.0);
  const std::vector<float> p_wt_factors(placement_dimensions_ + 100, 1.0);
  const float timing_factor = 1.0;
  const int path_traverse_step = 2;
  const std::vector<float> tot_vertex_weights
      = hypergraph_->GetTotalVertexWeights();
  const int alpha = 4;
  const std::vector<float> max_vertex_weights
      = DivideFactor(hypergraph_->GetTotalVertexWeights(), alpha * num_parts_);
  const int thr_coarsen_hyperedge_size = 50;
  global_net_threshold_ = 500;
  const int thr_coarsen_vertices = 200;
  const int thr_coarsen_hyperedges = 50;
  const float coarsening_ratio = 1.5;
  const int max_coarsen_iters = 20;
  const float adj_diff_ratio = 0.0001;
  TP_coarsening_ptr tritonpart_coarsener
      = std::make_shared<TPcoarsener>(e_wt_factors,
                                      v_wt_factors,
                                      p_wt_factors,
                                      timing_factor,
                                      path_traverse_step,
                                      max_vertex_weights,
                                      thr_coarsen_hyperedge_size,
                                      global_net_threshold_,
                                      thr_coarsen_vertices,
                                      thr_coarsen_hyperedges,
                                      coarsening_ratio,
                                      max_coarsen_iters,
                                      adj_diff_ratio,
                                      seed_,
                                      logger_);
  // how much weight on path cuts
  float path_wt_factor = 1.0;
  // how much weight on snaking paths
  float snaking_wt_factor = 1.0;
  const int refiner_iters = 2;
  const int max_moves = 50;
  int refiner_choice = TWO_WAY_FM;
  TP_two_way_refining_ptr tritonpart_twoway_refiner
      = std::make_shared<TPtwoWayFM>(num_parts_,
                                     refiner_iters,
                                     max_moves,
                                     refiner_choice,
                                     seed_,
                                     e_wt_factors,
                                     path_wt_factor,
                                     snaking_wt_factor,
                                     logger_);
  const int greedy_refiner_iters = 2;
  const int greedy_max_moves = 10;
  refiner_choice = GREEDY;
  TP_greedy_refiner_ptr tritonpart_greedy_refiner
      = std::make_shared<TPgreedyRefine>(num_parts_,
                                         greedy_refiner_iters,
                                         greedy_max_moves,
                                         refiner_choice,
                                         seed_,
                                         e_wt_factors,
                                         path_wt_factor,
                                         snaking_wt_factor,
                                         logger_);
  int wavefront = 50;
  TP_ilp_refiner_ptr tritonpart_ilp_refiner
      = std::make_shared<TPilpRefine>(num_parts_,
                                      greedy_refiner_iters,
                                      greedy_max_moves,
                                      refiner_choice,
                                      seed_,
                                      e_wt_factors,
                                      path_wt_factor,
                                      snaking_wt_factor,
                                      logger_,
                                      wavefront);
  float early_stop_ratio = 0.5;
  int max_num_fm_pass = 10;
  TP_partitioning_ptr tritonpart_partitioner
      = std::make_shared<TPpartitioner>(num_parts_,
                                        e_wt_factors,
                                        path_wt_factor,
                                        snaking_wt_factor,
                                        early_stop_ratio,
                                        max_num_fm_pass,
                                        seed_,
                                        tritonpart_twoway_refiner,
                                        logger_);
  bool v_cycle_flag = true;
  RefinerType refine_type = KPM_REFINEMENT;
  int num_initial_solutions = 50;       // number of initial random solutions
  int num_best_initial_solutions = 10;  // number of best initial solutions
  int num_ubfactor_delta = 5;  // allowing marginal imbalance to improve QoR
  int max_num_vcycle = 5;      // maximum number of vcycles
  TP_mlevel_partitioning_ptr tritonpart_mlevel_partitioner
      = std::make_shared<TPmultilevelPartitioner>(tritonpart_coarsener,
                                                  tritonpart_partitioner,
                                                  tritonpart_twoway_refiner,
                                                  tritonpart_greedy_refiner,
                                                  tritonpart_ilp_refiner,
                                                  num_parts_,
                                                  v_cycle_flag,
                                                  num_initial_solutions,
                                                  num_best_initial_solutions,
                                                  num_ubfactor_delta,
                                                  max_num_vcycle,
                                                  seed_,
                                                  ub_factor_,
                                                  refine_type,
                                                  logger_);
  bool vcycle = true;
  matrix<float> vertex_balance
      = hypergraph_->GetVertexBalance(num_parts_, ub_factor_);
  auto solution = tritonpart_mlevel_partitioner->PartitionTwoWay(
      hypergraph_, hypergraph_, vertex_balance, vcycle);
  auto cut_pair
      = tritonpart_partitioner->GoldenEvaluator(hypergraph_, solution, true);
  auto end_timestamp_global = std::chrono::high_resolution_clock::now();
  double total_global_time
      = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_timestamp_global - start_time_stamp_global)
            .count();
  total_global_time *= 1e-9;
  return solution;
}

std::vector<int> TritonPart::TritonPart_hypergraph_PartKWay(
    const char* hypergraph_file_arg,
    const char* fixed_file_arg,
    unsigned int num_parts_arg,
    float balance_constraint_arg,
    int vertex_dimension_arg,
    int hyperedge_dimension_arg,
    unsigned int seed_arg)
{
  logger_->report("Starting TritonPart Partitioner");
  auto start_time_stamp_global = std::chrono::high_resolution_clock::now();
  // Parameters
  num_parts_ = num_parts_arg;
  ub_factor_ = balance_constraint_arg;
  seed_ = seed_arg;
  vertex_dimensions_ = vertex_dimension_arg;
  hyperedge_dimensions_ = hyperedge_dimension_arg;
  // local parameters
  std::string hypergraph_file = hypergraph_file_arg;
  std::string fixed_file = fixed_file_arg;
  logger_->report("Partition Parameters**");
  logger_->report("Number of partitions = {}", num_parts_);
  logger_->report("UBfactor = {}", ub_factor_);
  logger_->report("Vertex dimensions = {}", vertex_dimensions_);
  logger_->report("Hyperedge dimensions = {}", hyperedge_dimensions_);
  // build hypergraph
  ReadHypergraph(hypergraph_file, fixed_file);
  BuildHypergraph();
  logger_->report("Hypergraph Information**");
  logger_->report("#Vertices = {}", num_vertices_);
  logger_->report("#Hyperedges = {}", num_hyperedges_);
  // process hypergraph
  HGraph hypergraph_processed = preProcessHypergraph();
  logger_->report("Post processing hypergraph information**");
  logger_->report("#Vertices = {}", hypergraph_processed->GetNumVertices());
  logger_->report("#Hyperedges = {}", hypergraph_processed->GetNumHyperedges());
  // create coarsening class
  const std::vector<float> e_wt_factors(hyperedge_dimensions_, 1.0);
  const std::vector<float> v_wt_factors(vertex_dimensions_, 1.0);
  const std::vector<float> p_wt_factors(placement_dimensions_ + 100, 1.0);
  const float timing_factor = 1.0;
  const int path_traverse_step = 2;
  const std::vector<float> tot_vertex_weights
      = hypergraph_->GetTotalVertexWeights();
  const int alpha = 4;
  const std::vector<float> max_vertex_weights
      = DivideFactor(hypergraph_->GetTotalVertexWeights(), alpha * num_parts_);
  const int thr_coarsen_hyperedge_size = 50;
  global_net_threshold_ = 500;
  const int thr_coarsen_vertices = 200;
  const int thr_coarsen_hyperedges = 50;
  const float coarsening_ratio = 1.5;
  const int max_coarsen_iters = 20;
  const float adj_diff_ratio = 0.0001;
  TP_coarsening_ptr tritonpart_coarsener
      = std::make_shared<TPcoarsener>(e_wt_factors,
                                      v_wt_factors,
                                      p_wt_factors,
                                      timing_factor,
                                      path_traverse_step,
                                      max_vertex_weights,
                                      thr_coarsen_hyperedge_size,
                                      global_net_threshold_,
                                      thr_coarsen_vertices,
                                      thr_coarsen_hyperedges,
                                      coarsening_ratio,
                                      max_coarsen_iters,
                                      adj_diff_ratio,
                                      seed_,
                                      logger_);
  float path_wt_factor = 1.0;
  float snaking_wt_factor = 1.0;
  const int refiner_iters = 2;
  const int max_moves = 50;
  int refiner_choice = FLAT_K_WAY_FM;
  TP_k_way_refining_ptr tritonpart_kway_refiner
      = std::make_shared<TPkWayFM>(num_parts_,
                                   refiner_iters,
                                   max_moves,
                                   refiner_choice,
                                   seed_,
                                   e_wt_factors,
                                   path_wt_factor,
                                   snaking_wt_factor,
                                   logger_);
  float early_stop_ratio = 0.5;
  int max_num_fm_pass = 10;
  TP_partitioning_ptr tritonpart_partitioner
      = std::make_shared<TPpartitioner>(num_parts_,
                                        e_wt_factors,
                                        path_wt_factor,
                                        snaking_wt_factor,
                                        early_stop_ratio,
                                        max_num_fm_pass,
                                        seed_,
                                        tritonpart_kway_refiner,
                                        logger_);
  bool v_cycle_flag = true;
  RefinerType refine_type = KFM_REFINEMENT;
  int num_initial_solutions = 50;       // number of initial random solutions
  int num_best_initial_solutions = 10;  // number of best initial solutions
  int num_ubfactor_delta = 5;  // allowing marginal imbalance to improve QoR
  int max_num_vcycle = 5;      // maximum number of vcycles
  TP_mlevel_partitioning_ptr tritonpart_mlevel_partitioner
      = std::make_shared<TPmultilevelPartitioner>(tritonpart_coarsener,
                                                  tritonpart_partitioner,
                                                  tritonpart_kway_refiner,
                                                  num_parts_,
                                                  v_cycle_flag,
                                                  num_initial_solutions,
                                                  num_best_initial_solutions,
                                                  num_ubfactor_delta,
                                                  max_num_vcycle,
                                                  seed_,
                                                  ub_factor_,
                                                  refine_type,
                                                  logger_);
  bool vcycle = true;
  matrix<float> vertex_balance
      = hypergraph_->GetVertexBalance(num_parts_, ub_factor_);
  auto solution = tritonpart_mlevel_partitioner->PartitionKWay(
      hypergraph_, hypergraph_, vertex_balance, vcycle);
  auto cut_pair
      = tritonpart_partitioner->GoldenEvaluator(hypergraph_, solution, true);
  auto end_timestamp_global = std::chrono::high_resolution_clock::now();
  double total_global_time
      = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_timestamp_global - start_time_stamp_global)
            .count();
  total_global_time *= 1e-9;
  logger_->info(PAR, 4, "Total runtime {} seconds", total_global_time);
  return solution;
}

std::vector<int> TritonPart::TritonPart_design_PartKWay(
    unsigned int num_parts_,
    float ub_factor_,
    int vertex_dimensions_,
    int hyperedge_dimensions_,
    unsigned int seed_)
{
  logger_->report("TritonPart_design_PartKWay starts !!!");
  auto start_time_stamp_global = std::chrono::high_resolution_clock::now();
  // create coarsening class
  const std::vector<float> e_wt_factors(hyperedge_dimensions_, 1.0);
  const std::vector<float> v_wt_factors(vertex_dimensions_, 1.0);
  const std::vector<float> p_wt_factors(placement_dimensions_ + 100, 1.0);
  // Increasing the timing factor will prioritize clustering vertices that share
  // multiple timing paths between them
  const float timing_factor = 1.0;
  const int path_traverse_step = 2;
  const std::vector<float> tot_vertex_weights
      = hypergraph_->GetTotalVertexWeights();
  const int alpha = 4;
  const std::vector<float> max_vertex_weights
      = DivideFactor(hypergraph_->GetTotalVertexWeights(), alpha * num_parts_);
  const int thr_coarsen_hyperedge_size = 50;
  global_net_threshold_ = 500;
  const int thr_coarsen_vertices = 200;
  const int thr_coarsen_hyperedges = 50;
  const float coarsening_ratio = 1.5;
  const int max_coarsen_iters = 20;
  const float adj_diff_ratio = 0.0001;
  TP_coarsening_ptr tritonpart_coarsener
      = std::make_shared<TPcoarsener>(e_wt_factors,
                                      v_wt_factors,
                                      p_wt_factors,
                                      timing_factor,
                                      path_traverse_step,
                                      max_vertex_weights,
                                      thr_coarsen_hyperedge_size,
                                      global_net_threshold_,
                                      thr_coarsen_vertices,
                                      thr_coarsen_hyperedges,
                                      coarsening_ratio,
                                      max_coarsen_iters,
                                      adj_diff_ratio,
                                      seed_,
                                      logger_);
  // Increasing the path weight factor will add more weight to cost incurred
  // by moving a vertex to a different partition with respect to a path graph
  float path_wt_factor = 1.0;
  // Increasing the snaking weight factor will add more weight to snaking cost
  // incurred by moving a vertex to a different partition with respect to a path
  // graph
  float snaking_wt_factor = 1.0;
  // Setting the number of refiner iterations and maximum number of moves in a
  // refiner pass
  const int refiner_iters = 2;
  const int max_moves = 50;
  int refiner_choice = FLAT_K_WAY_FM;
  TP_k_way_refining_ptr tritonpart_kway_refiner
      = std::make_shared<TPkWayFM>(num_parts_,
                                   refiner_iters,
                                   max_moves,
                                   refiner_choice,
                                   seed_,
                                   e_wt_factors,
                                   path_wt_factor,
                                   snaking_wt_factor,
                                   logger_);
  float early_stop_ratio = 0.5;
  int max_num_fm_pass = 10;
  TP_partitioning_ptr tritonpart_partitioner
      = std::make_shared<TPpartitioner>(num_parts_,
                                        e_wt_factors,
                                        path_wt_factor,
                                        snaking_wt_factor,
                                        early_stop_ratio,
                                        max_num_fm_pass,
                                        seed_,
                                        tritonpart_kway_refiner,
                                        logger_);
  bool v_cycle_flag = true;
  RefinerType refine_type = KFM_REFINEMENT;
  int num_initial_solutions = 50;       // number of initial random solutions
  int num_best_initial_solutions = 10;  // number of best initial solutions
  int num_ubfactor_delta = 5;  // allowing marginal imbalance to improve QoR
  int max_num_vcycle = 5;      // maximum number of vcycles
  TP_mlevel_partitioning_ptr tritonpart_mlevel_partitioner
      = std::make_shared<TPmultilevelPartitioner>(tritonpart_coarsener,
                                                  tritonpart_partitioner,
                                                  tritonpart_kway_refiner,
                                                  num_parts_,
                                                  v_cycle_flag,
                                                  num_initial_solutions,
                                                  num_best_initial_solutions,
                                                  num_ubfactor_delta,
                                                  max_num_vcycle,
                                                  seed_,
                                                  ub_factor_,
                                                  refine_type,
                                                  logger_);
  bool vcycle = true;
  matrix<float> vertex_balance
      = hypergraph_->GetVertexBalance(num_parts_, ub_factor_);
  auto solution = tritonpart_mlevel_partitioner->PartitionKWay(
      hypergraph_, hypergraph_, vertex_balance, vcycle);
  auto cut_pair
      = tritonpart_partitioner->GoldenEvaluator(hypergraph_, solution, true);
  auto end_timestamp_global = std::chrono::high_resolution_clock::now();
  double total_global_time
      = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_timestamp_global - start_time_stamp_global)
            .count();
  total_global_time *= 1e-9;
  return solution;
}

// 2way TritonPart interface for Hier-RTLMP
std::vector<int> TritonPart::TritonPart2Way(
    int num_vertices,
    int num_hyperedges,
    const std::vector<std::vector<int>>& hyperedges,
    const std::vector<float>& vertex_weights,
    float balance_constraints,
    int seed)
{
  logger_->report("Starting TritonPart Partitioner");
  auto start_time_stamp_global = std::chrono::high_resolution_clock::now();
  // Parameters
  num_parts_ = 2;
  ub_factor_ = balance_constraints;
  seed_ = seed;
  vertex_dimensions_ = 1;
  hyperedge_dimensions_ = 1;
  placement_dimensions_
      = 0;  // no placememnt information is provided for this function
  num_vertices_ = num_vertices;
  num_hyperedges_ = num_hyperedges;
  // hyperedges_ = hyperedges;
  //  local parameters
  logger_->report("Partition Parameters**");
  logger_->report("Number of partitions = {}", num_parts_);
  logger_->report("UBfactor = {}", ub_factor_);
  logger_->report("Vertex dimensions = {}", vertex_dimensions_);
  logger_->report("Hyperedge dimensions = {}", hyperedge_dimensions_);

  // build hypergraph
  // add hyperedge
  std::vector<int> eind;
  std::vector<int> eptr;  // hyperedges
  eptr.push_back(static_cast<int>(eind.size()));
  for (auto hyperedge : hyperedges) {
    eind.insert(eind.end(), hyperedge.begin(), hyperedge.end());
    eptr.push_back(static_cast<int>(eind.size()));
    std::vector<float> temp_weight{1.0};
    hyperedge_weights_.push_back(temp_weight);
  }
  // add vertex
  // create vertices from hyperedges
  std::vector<std::vector<int>> vertices(num_vertices_);
  for (int i = 0; i < num_hyperedges_; i++)
    for (auto v : hyperedges[i])
      vertices[v].push_back(i);  // i is the hyperedge id
  std::vector<int> vind;
  std::vector<int> vptr;  // vertices
  vptr.push_back(static_cast<int>(vind.size()));
  for (auto& vertex : vertices) {
    vind.insert(vind.end(), vertex.begin(), vertex.end());
    vptr.push_back(static_cast<int>(vind.size()));
  }
  vertex_weights_.clear();
  for (auto& weight : vertex_weights) {
    std::vector<float> temp_weight{weight};
    vertex_weights_.push_back(temp_weight);
  }

  // Convert the timing information (no timing information)
  std::vector<int> vind_p;  // each timing path is a sequences of vertices
  std::vector<int> vptr_p;
  std::vector<int>
      pind_v;  // store all the timing paths connected to the vertex
  std::vector<int> pptr_v;
  std::vector<float> timing_attr;
  // create TPHypergraph
  hypergraph_ = std::make_shared<TPHypergraph>(num_vertices_,
                                               num_hyperedges_,
                                               vertex_dimensions_,
                                               hyperedge_dimensions_,
                                               eind,
                                               eptr,
                                               vind,
                                               vptr,
                                               vertex_weights_,
                                               hyperedge_weights_,
                                               fixed_attr_,
                                               community_attr_,
                                               placement_dimensions_,
                                               placement_attr_,
                                               vind_p,
                                               vptr_p,
                                               pind_v,
                                               pptr_v,
                                               timing_attr,
                                               logger_);
  logger_->report("Hypergraph Information**");
  logger_->report("#Vertices = {}", num_vertices_);
  logger_->report("#Hyperedges = {}", num_hyperedges_);

  // create coarsening class
  const std::vector<float> e_wt_factors(hyperedge_dimensions_, 1.0);
  const std::vector<float> v_wt_factors(vertex_dimensions_, 1.0);
  const std::vector<float> p_wt_factors(placement_dimensions_ + 100, 1.0);
  const float timing_factor = 1.0;
  const int path_traverse_step = 2;
  const std::vector<float> tot_vertex_weights
      = hypergraph_->GetTotalVertexWeights();
  const int alpha = 4;
  const std::vector<float> max_vertex_weights
      = DivideFactor(hypergraph_->GetTotalVertexWeights(), alpha * num_parts_);
  const int thr_coarsen_hyperedge_size = 50;
  global_net_threshold_ = 500;
  const int thr_coarsen_vertices = 200;
  const int thr_coarsen_hyperedges = 50;
  const float coarsening_ratio = 1.5;
  const int max_coarsen_iters = 20;
  const float adj_diff_ratio = 0.0001;
  TP_coarsening_ptr tritonpart_coarsener
      = std::make_shared<TPcoarsener>(e_wt_factors,
                                      v_wt_factors,
                                      p_wt_factors,
                                      timing_factor,
                                      path_traverse_step,
                                      max_vertex_weights,
                                      thr_coarsen_hyperedge_size,
                                      global_net_threshold_,
                                      thr_coarsen_vertices,
                                      thr_coarsen_hyperedges,
                                      coarsening_ratio,
                                      max_coarsen_iters,
                                      adj_diff_ratio,
                                      seed_,
                                      logger_);
  float path_wt_factor = 1.0;
  float snaking_wt_factor = 1.0;
  const int refiner_iters = 2;
  const int max_moves = 50;
  int refiner_choice = TWO_WAY_FM;
  TP_two_way_refining_ptr tritonpart_twoway_refiner
      = std::make_shared<TPtwoWayFM>(num_parts_,
                                     refiner_iters,
                                     max_moves,
                                     refiner_choice,
                                     seed_,
                                     e_wt_factors,
                                     path_wt_factor,
                                     snaking_wt_factor,
                                     logger_);
  const int greedy_refiner_iters = 2;
  const int greedy_max_moves = 10;
  refiner_choice = GREEDY;
  TP_greedy_refiner_ptr tritonpart_greedy_refiner
      = std::make_shared<TPgreedyRefine>(num_parts_,
                                         greedy_refiner_iters,
                                         greedy_max_moves,
                                         refiner_choice,
                                         seed_,
                                         e_wt_factors,
                                         path_wt_factor,
                                         snaking_wt_factor,
                                         logger_);
  int wavefront = 50;
  TP_ilp_refiner_ptr tritonpart_ilp_refiner
      = std::make_shared<TPilpRefine>(num_parts_,
                                      greedy_refiner_iters,
                                      greedy_max_moves,
                                      refiner_choice,
                                      seed_,
                                      e_wt_factors,
                                      path_wt_factor,
                                      snaking_wt_factor,
                                      logger_,
                                      wavefront);
  float early_stop_ratio = 0.5;
  int max_num_fm_pass = 10;
  TP_partitioning_ptr tritonpart_partitioner
      = std::make_shared<TPpartitioner>(num_parts_,
                                        e_wt_factors,
                                        path_wt_factor,
                                        snaking_wt_factor,
                                        early_stop_ratio,
                                        max_num_fm_pass,
                                        seed_,
                                        tritonpart_twoway_refiner,
                                        logger_);
  bool v_cycle_flag = true;
  // RefinerType refine_type = 2_WAY_FM;
  RefinerType refine_type = KPM_REFINEMENT;
  int num_initial_solutions = 50;       // number of initial random solutions
  int num_best_initial_solutions = 10;  // number of best initial solutions
  int num_ubfactor_delta = 5;  // allowing marginal imbalance to improve QoR
  int max_num_vcycle = 5;      // maximum number of vcycles
  TP_mlevel_partitioning_ptr tritonpart_mlevel_partitioner
      = std::make_shared<TPmultilevelPartitioner>(tritonpart_coarsener,
                                                  tritonpart_partitioner,
                                                  tritonpart_twoway_refiner,
                                                  tritonpart_greedy_refiner,
                                                  tritonpart_ilp_refiner,
                                                  num_parts_,
                                                  v_cycle_flag,
                                                  num_initial_solutions,
                                                  num_best_initial_solutions,
                                                  num_ubfactor_delta,
                                                  max_num_vcycle,
                                                  seed_,
                                                  ub_factor_,
                                                  refine_type,
                                                  logger_);
  bool vcycle = true;
  matrix<float> vertex_balance
      = hypergraph_->GetVertexBalance(num_parts_, ub_factor_);
  std::vector<int> solution = tritonpart_mlevel_partitioner->PartitionTwoWay(
      hypergraph_, hypergraph_, vertex_balance, vcycle);
  auto end_timestamp_global = std::chrono::high_resolution_clock::now();
  double total_global_time
      = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_timestamp_global - start_time_stamp_global)
            .count();
  total_global_time *= 1e-9;
  logger_->info(PAR, 5, "Total runtime {} seconds", total_global_time);
  return solution;
}

}  // namespace par
