///////////////////////////////////////////////////////////////////////////
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

#pragma once

#include <map>
#include <memory>
#include <random>
#include <set>

namespace ord {
class dbVerilogNetwork;
}

namespace odb {
class dbDatabase;
class dbChip;
class dbBlock;
}  // namespace odb

namespace sta {
class dbNetwork;
class Instance;
class NetworkReader;
class Library;
class Port;
class Net;
class dbSta;
}  // namespace sta

namespace utl {
class Logger;
}

using utl::Logger;

namespace par {

class Graph;
class Hypergraph;

enum GraphType : uint8_t
{
  CLIQUE,
  HYBRID,
  STAR
};

class PartOptions
{
 public:
  PartOptions() = default;

  void setNumStarts(unsigned numStarts) { numStarts_ = numStarts; }
  unsigned getNumStarts() const { return numStarts_; }
  void setTargetPartitions(unsigned target) { targetPartitions_ = target; }
  unsigned getTargetPartitions() const { return targetPartitions_; }
  void setWeightedVertices(bool enable) { weightedVertices_ = enable; }
  bool getWeightedVertices() const { return weightedVertices_; }
  void setCoarRatio(double cratio) { coarRatio_ = cratio; }
  double getCoarRatio() { return coarRatio_; }
  void setCoarVertices(unsigned coarVertices) { coarVertices_ = coarVertices; }
  unsigned getCoarVertices() const { return coarVertices_; }
  void setTermProp(bool enable) { termProp_ = enable; }
  bool getTermProp() const { return termProp_; }
  void setCutHopRatio(double ratio) { cutHopRatio_ = ratio; }
  double getCutHopRatio() { return cutHopRatio_; }
  void setArchTopology(const std::vector<int>& arch) { archTopology_ = arch; }
  const std::vector<int>& getArchTopology() const { return archTopology_; }
  void setTool(const std::string& tool) { tool_ = tool; }
  std::string getTool() const { return tool_; }
  void setGraphModel(const std::string& graphModel)
  {
    if (graphModel == "clique") {
      graphModel_ = CLIQUE;
    } else if (graphModel == "star") {
      graphModel_ = STAR;
    } else
      graphModel_ = HYBRID;
  }
  GraphType getGraphModel() const { return graphModel_; }
  void setCliqueThreshold(unsigned threshold) { cliqueThreshold_ = threshold; }
  unsigned getCliqueThreshold() const { return cliqueThreshold_; }
  void setWeightModel(unsigned model) { weightModel_ = model; }
  unsigned getWeightModel() const { return weightModel_; }
  void setMaxEdgeWeight(unsigned weight) { maxEdgeWeight_ = weight; }
  unsigned getMaxEdgeWeight() const { return maxEdgeWeight_; }
  void setMaxVertexWeight(unsigned weight) { maxVertexWeight_ = weight; }
  unsigned getMaxVertexWeight() const { return maxVertexWeight_; }
  void setBalanceConstraint(unsigned constraint)
  {
    balanceConstraint_ = constraint;
  }
  unsigned getBalanceConstraint() const { return balanceConstraint_; }
  void setRefinement(unsigned number) { refinement_ = number; }
  unsigned getRefinement() const { return refinement_; }
  void setRandomSeed(int seed);
  void generateSeeds(int seeds);
  int getNewSeed() { return seedGenerator_(); }
  void setSeeds(const std::set<int>& seeds) { seeds_ = seeds; }
  const std::set<int>& getSeeds() const { return seeds_; }
  void setExistingID(int id) { existingId_ = id; }
  int getExistingID() const { return existingId_; }
  void setPartitionsToTest(const std::vector<int>& partIds)
  {
    partitionsToTest_ = partIds;
  }
  const std::vector<int>& getPartitionsToTest() const
  {
    return partitionsToTest_;
  }
  void setEvaluationFunction(const std::string& function)
  {
    evaluationFunction_ = function;
  }
  std::string getEvaluationFunction() const { return evaluationFunction_; }
  void setLevel(unsigned level) { level_ = level; }
  unsigned getLevel() const { return level_; }
  void setFinalPartitions(unsigned target) { finalPartitions_ = target; }
  unsigned getFinalPartitions() { return finalPartitions_; }
  void setForceGraph(bool force) { forceGraph_ = force; }
  bool getForceGraph() { return forceGraph_; }

 private:
  unsigned numStarts_ = 1;
  unsigned targetPartitions_ = 0;
  bool weightedVertices_ = false;
  double coarRatio_ = 0.7;
  unsigned coarVertices_ = 2500;
  bool termProp_ = false;
  double cutHopRatio_ = 1.0;
  std::string tool_ = "chaco";
  GraphType graphModel_ = HYBRID;
  std::string evaluationFunction_ = "hyperedges";
  unsigned cliqueThreshold_ = 50;
  unsigned weightModel_ = 1;
  unsigned maxEdgeWeight_ = 100;
  unsigned maxVertexWeight_ = 100;
  unsigned balanceConstraint_ = 2;
  unsigned refinement_ = 0;
  int level_ = -1;
  int existingId_ = -1;
  unsigned finalPartitions_ = 2;
  bool forceGraph_ = false;
  std::vector<int> archTopology_;
  std::set<int> seeds_;
  std::vector<int> partitionsToTest_;
  std::mt19937 seedGenerator_ = std::mt19937();
};

class PartSolutions
{
 public:
  void addAssignment(const std::vector<unsigned long>& currentAssignment,
                     const unsigned long runtime,
                     const int seed);
  void clearAssignments();
  const std::vector<unsigned long>& getAssignment(unsigned idx) const
  {
    return assignmentResults_[idx];
  }
  unsigned long getRuntime(unsigned idx) const { return runtimeResults_[idx]; }
  int getSeed(unsigned idx) const { return seeds_[idx]; }
  void setToolName(const std::string& name) { toolName_ = name; }
  std::string getToolName() const { return toolName_; }
  void setPartitionId(unsigned id) { partitionId_ = id; }
  unsigned getPartitionId() const { return partitionId_; }
  void setClusterId(unsigned id) { clusterId_ = id; }
  unsigned getClusterId() const { return clusterId_; }
  void setBestSolutionIdx(unsigned idx) { bestSolutionIdx_ = idx; }
  unsigned getBestSolutionIdx() const { return bestSolutionIdx_; }
  unsigned getNumOfRuns() const { return seeds_.size(); }
  void setBestSetSize(double result) { bestSetSizeSD_ = result; }
  double getBestSetSize() const { return bestSetSizeSD_; }
  void setBestSetArea(double result) { bestSetAreaSD_ = result; }
  double getBestSetArea() const { return bestSetAreaSD_; }
  void setBestNumTerminals(unsigned long result) { bestNumTerminals_ = result; }
  unsigned long getBestNumTerminals() const { return bestNumTerminals_; }
  void setBestNumHyperedgeCuts(unsigned long result)
  {
    bestNumHyperedgeCuts_ = result;
  }
  unsigned long getBestNumHyperedgeCuts() const
  {
    return bestNumHyperedgeCuts_;
  }
  void setBestRuntime(unsigned long result) { bestRuntime_ = result; }
  unsigned long getBestRuntime() const { return bestRuntime_; }
  void setBestHopWeigth(unsigned long result) { bestHopWeigth_ = result; }
  unsigned long getBestHopWeigth() const { return bestHopWeigth_; }

  void resetEvaluation();

 private:
  std::vector<std::vector<unsigned long>> assignmentResults_;
  std::vector<unsigned long> runtimeResults_;
  std::vector<int> seeds_;
  std::string toolName_ = "";
  unsigned partitionId_ = 0;
  unsigned clusterId_ = 0;
  unsigned bestSolutionIdx_ = 0;
  double bestSetSizeSD_ = 0;
  double bestSetAreaSD_ = 0;
  unsigned long bestNumTerminals_ = 0;
  unsigned long bestNumHyperedgeCuts_ = 0;
  unsigned long bestRuntime_ = 0;
  unsigned long bestHopWeigth_ = 0;
};

class PartitionMgr
{
 private:
  odb::dbBlock* getDbBlock() const;
  unsigned getNumPartitioningResults() const { return results_.size(); }
  unsigned getNumClusteringResults() const { return clusResults_.size(); }
  const PartSolutions& getPartitioningResult(unsigned id) const
  {
    return results_[id];
  }
  const PartSolutions& getClusteringResult(unsigned id) const
  {
    return clusResults_[id];
  }

  PartOptions options_;
  odb::dbDatabase* db_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  sta::dbSta* _sta = nullptr;
  unsigned bestId_ = 0;
  Logger* logger_;
  std::unique_ptr<Graph> graph_;
  std::unique_ptr<Hypergraph> hypergraph_;
  std::vector<PartSolutions> results_;
  std::vector<PartSolutions> clusResults_;

 public:
  PartitionMgr();
  ~PartitionMgr();
  void init(odb::dbDatabase* db,
            sta::dbNetwork* db_network,
            sta::dbSta* sta,
            Logger* logger);
  void runPartitioning();
  void runClustering();
  void evaluatePartitioning();
  unsigned getCurrentBestId() const { return bestId_; }
  void setCurrentBestId(unsigned id) { bestId_ = id; }
  void runChaco();
  void runGpMetis();
  void runMlPart();
  void runChacoClustering();
  void runGpMetisClustering();
  void runMlPartClustering();
  PartOptions& getOptions() { return options_; }
  unsigned getCurrentId() const { return (results_.size() - 1); }
  unsigned getCurrentClusId() const { return (clusResults_.size() - 1); }
  void toGraph();
  void toHypergraph();
  void hypergraph(bool buildGraph = false);
  unsigned generatePartitionId() const;
  unsigned generateClusterId() const;
  void computePartitionResult(unsigned partitionId, std::string function);
  bool comparePartitionings(const PartSolutions& oldPartition,
                            const PartSolutions& newPartition,
                            const std::string& function);
  void reportPartitionResult(const unsigned partitionId);
  void writePartitioningToDb(const unsigned partitionId);
  void dumpPartIdToFile(std::string name);
  void writeClusteringToDb(unsigned clusteringId);
  void dumpClusIdToFile(std::string name) const;
  void reportNetlistPartitions(unsigned partitionId);
  unsigned readPartitioningFile(const std::string& filename,
                                const std::string& instance_map_file);
  void reportGraph();

  void writePartitionVerilog(const char* path,
                             const char* port_prefix,
                             const char* module_suffix);

  void partitionDesign(unsigned int max_num_macro,
                       unsigned int min_num_macro,
                       unsigned int max_num_inst,
                       unsigned int min_num_inst,
                       unsigned int net_threshold,
                       unsigned int ignore_net_threshold,
                       unsigned int virtual_weight,
                       unsigned int num_hops,
                       unsigned int timing_weight,
                       bool std_cell_timing_flag_,
                       const char* report_directory,
                       const char* file_name,
                       float keepin_lx,
                       float keepin_ly,
                       float keepin_ux,
                       float keepin_uy);

 private:
  sta::Instance* buildPartitionedInstance(
      const char* name,
      const char* port_prefix,
      sta::Library* library,
      sta::NetworkReader* network,
      sta::Instance* parent,
      const std::set<sta::Instance*>* insts,
      std::map<sta::Net*, sta::Port*>* port_map);
  sta::Instance* buildPartitionedTopInstance(const char* name,
                                             sta::Library* library,
                                             sta::NetworkReader* network);
};

}  // namespace par
