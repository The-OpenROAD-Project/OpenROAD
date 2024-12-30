///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <map>
#include <queue>
#include <set>
#include <vector>
#include <igraph/igraph.h>
#include <algorithm>
#include "clusterEngine.h"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "sta/Liberty.hh"
namespace utl {
class Logger;
}

namespace sta {
class dbNetwork;
}

namespace mpl3 {

class leidenClustering
{
 public:
  leidenClustering(sta::dbNetwork* network, odb::dbBlock* block, utl::Logger* logger);
  void init(bool);
  void run();
  void write_graph_csv(const std::string& edge_file, const std::string& node_file);
  void write_placement_csv(const std::string& file_name);
  void write_partition_csv(const std::string& file_name);
  size_t get_vertex_count() { return igraph_vcount(graph_); }
  std::vector<int> &get_partition() { return partition_; }
  void set_iteration(int iteration) { iteration_ = iteration; }
  void set_resolution(double resolution) { resolution_ = resolution; }
  void set_large_net_threshold(int threshold) { large_net_threshold_ = threshold; }
  size_t get_cluster_count() { return max_cluster_id_ + 1; }
 private:
  sta::dbNetwork* network_{nullptr};
  odb::dbBlock* block_{nullptr};
  utl::Logger* logger_{nullptr};
  igraph_t* graph_;
  std::vector<double> edge_weights_;
  std::map<int, std::string> vertex_names_{};
  std::vector<int> partition_{};
  int large_net_threshold_{50};
  int iteration_{10};
  double resolution_{0.05};
  size_t max_cluster_id_{0};
  void create_vertex_id();
  bool get_vertex_id();
  /**
   * @brief Runs the Leiden clustering algorithm.
   */
  void runLeidenClustering();

  bool isIgnoredMaster(odb::dbMaster* master);

  /**
   * @brief Extracts net information including the source, sinks, and fan-out.
   *
   * This lambda function processes a given net to determine its driver (source),
   * load vertices (sinks), and the fan-out (number of sinks). It skips supply nets,
   * nets with ignored masters, and nets with high fan-out or invalid drivers/loads.
   *
   * @param net Pointer to the net to be processed.
   * @param source Reference to an integer where the driver vertex ID will be stored.
   * @param sinks Reference to a set of integers where the load vertex IDs will be stored.
   * @param fan_out Reference to a size_t where the fan-out (number of sinks) will be stored.
   * @return True if the net information was successfully extracted, false otherwise.
   */
  bool get_net_info(odb::dbNet* net, int &source, std::set<int> &sinks, size_t &fan_out) {
    if (net->getSigType().isSupply()) {
      return false;
    }
    int driver_id = -1;
    std::set<int> loads_id;
    bool ignore = false;
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      odb::dbMaster* master = inst->getMaster();
      if (isIgnoredMaster(master)) {
        ignore = true;
        break;
      }
      int vertex_id = -1;
      if (master->isBlock()) {
        vertex_id = odb::dbIntProperty::find(iterm, "leiden_vertex_id")->getValue();
      } else {
        odb::dbIntProperty* int_prop
            = odb::dbIntProperty::find(inst, "leiden_vertex_id");

        // Std cells without liberty data are not marked as vertices
        if (int_prop) {
          vertex_id = int_prop->getValue();
        } else {
          ignore = true;
          break;
        }
      }

      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }

    if (ignore) {
      return false;
    }

    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int vertex_id
          = odb::dbIntProperty::find(bterm, "leiden_vertex_id")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }

    // Skip high fanout nets or nets that do not have valid driver or loads
    if (driver_id < 0 || loads_id.empty()
        || loads_id.size() > large_net_threshold_) {
      return false;
    }
    source = std::move(driver_id);
    sinks = std::move(loads_id);
    fan_out = sinks.size();
    return true;
  };
};

}  // namespace mpl3