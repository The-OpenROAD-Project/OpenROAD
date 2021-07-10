/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

%{
#include "par/PartitionMgr.h"
#include "ord/OpenRoad.hh"
#include <set>

namespace ord {
// Defined in OpenRoad.i
        par::PartitionMgr* getPartitionMgr();
}  // namespace ord

using ord::getPartitionMgr;
%}

%include "../../Exception.i"

%import <std_vector.i>
namespace std {
  %template(IntVector) vector<int>;
}

%inline %{

void set_tool(const char* name) {
        getPartitionMgr()->getOptions().setTool(name);
}

void set_target_partitions(unsigned value) {
        getPartitionMgr()->getOptions().setTargetPartitions(value);
}

void set_graph_model(const char* name) {
        getPartitionMgr()->getOptions().setGraphModel(name);
}

void set_clique_threshold(unsigned value) {
        getPartitionMgr()->getOptions().setCliqueThreshold(value);
}

void set_weight_model(unsigned value) {
        getPartitionMgr()->getOptions().setWeightModel(value);
}

void set_max_edge_weight(unsigned value) {
        getPartitionMgr()->getOptions().setMaxEdgeWeight(value);
}

void set_max_vertex_weight(unsigned value) {
        getPartitionMgr()->getOptions().setMaxVertexWeight(value);
}

void set_num_starts(unsigned value) {
        getPartitionMgr()->getOptions().setNumStarts(value);
}

void set_balance_constraints(unsigned value) {
        getPartitionMgr()->getOptions().setBalanceConstraint(value);
}

void set_coarsening_ratio(float value) {
        getPartitionMgr()->getOptions().setCoarRatio(value);
}

void set_coarsening_vertices(unsigned value) {
        getPartitionMgr()->getOptions().setCoarVertices(value);
}

void set_enable_term_prop(unsigned value) {
        if (value > 0){
                getPartitionMgr()->getOptions().setTermProp(true);
        } else {
                getPartitionMgr()->getOptions().setTermProp(false);
        }
}

void set_cut_hop_ratio(float value) {
        getPartitionMgr()->getOptions().setCutHopRatio(value);
}

void set_architecture(const std::vector<int>& topology) {
        getPartitionMgr()->getOptions().setArchTopology(topology);
}

void clear_architecture() {
        std::vector<int> archTopology;
        getPartitionMgr()->getOptions().setArchTopology(archTopology);
}

void set_refinement(unsigned value) {
        getPartitionMgr()->getOptions().setRefinement(value);
}

void set_seeds(const std::vector<int>& seeds) {
  std::set<int> seedSet(seeds.begin(), seeds.end());
  getPartitionMgr()->getOptions().setSeeds(seedSet);
}

void set_existing_id(int value) {
        getPartitionMgr()->getOptions().setExistingID(value);
}

void set_random_seed(int value) {
        getPartitionMgr()->getOptions().setRandomSeed(value);
}

void generate_seeds(unsigned value) {
        getPartitionMgr()->getOptions().generateSeeds(value);
}

void set_partition_ids_to_test(const std::vector<int>& ids) {
        getPartitionMgr()->getOptions().setPartitionsToTest(ids);
}

void set_evaluation_function(const char* function) {
        getPartitionMgr()->getOptions().setEvaluationFunction(function);
}

unsigned run_partitioning() {
        getPartitionMgr()->runPartitioning();
        unsigned id = getPartitionMgr()->getCurrentId();
        return id;
}


unsigned evaluate_partitioning() {
        getPartitionMgr()->evaluatePartitioning();
        unsigned id = getPartitionMgr()->getCurrentBestId();
        return id;
}

void write_partitioning_to_db(unsigned id) {
        getPartitionMgr()->writePartitioningToDb(id);
}

void dump_part_id_to_file(const char *name) {
        getPartitionMgr()->dumpPartIdToFile(name);
}

unsigned run_3party_clustering() {
        getPartitionMgr()->run3PClustering();
        unsigned id = getPartitionMgr()->getCurrentClusId();
        return id;
}

void set_level(unsigned value) {
        getPartitionMgr()->getOptions().setLevel(value);
}

void write_clustering_to_db(unsigned id) {
        getPartitionMgr()->writeClusteringToDb(id);
}

void dump_clus_id_to_file(const char* name) {
        getPartitionMgr()->dumpClusIdToFile(name);
}

void report_netlist_partitions(unsigned id) {
        getPartitionMgr()->reportNetlistPartitions(id);
}

void read_file(const char* filename) {
        getPartitionMgr()->readPartitioningFile(filename);
}

void set_final_partitions(unsigned value) {
        getPartitionMgr()->getOptions().setFinalPartitions(value);
}

void set_force_graph(bool value) {
        getPartitionMgr()->getOptions().setForceGraph(value);
}

void set_clustering_scheme(const char* name) {
        getPartitionMgr()->getOptions().setClusteringScheme(name);
}

void run_clustering() {
        getPartitionMgr()->runClustering();
}

void report_graph() {
        getPartitionMgr()->reportGraph();
}

void write_partition_verilog(int id, const char* portprefix, const char* module_suffix, const char* path) {
  write_partitioning_to_db(id);
  getPartitionMgr()->writePartitionVerilog(path, portprefix, module_suffix);
}

void
partition_design_cmd(unsigned int max_num_macro, unsigned int min_num_macro,
                     unsigned int max_num_inst,  unsigned int min_num_inst,
                     unsigned int net_threshold, unsigned int virtual_weight,
                     unsigned int ignore_net_threshold,
                     const char* report_directory, const char* file_name)
{
  getPartitionMgr()->partitionDesign(max_num_macro, min_num_macro,
                                     max_num_inst, min_num_inst,
                                     net_threshold, virtual_weight,
                                     ignore_net_threshold, report_directory,
                                     file_name);
}

%}
