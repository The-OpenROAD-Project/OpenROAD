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

#include "HypergraphDecomposition.h"

#include <random>
#include <set>

namespace odb {
class dbDatabase;
class dbChip;
class dbBlock;
}  // namespace odb

namespace sta {
class Instance;
class NetworkReader;
class Library;
class Port;
class Net;
}  // namespace sta

namespace utl {
class Logger;
}

using utl::Logger;

namespace par {

class PartOptions
{
 public:
  PartOptions() = default;

  void setNumStarts(unsigned numStarts) { _numStarts = numStarts; }
  unsigned getNumStarts() const { return _numStarts; }
  void setTargetPartitions(unsigned target) { _targetPartitions = target; }
  unsigned getTargetPartitions() const { return _targetPartitions; }
  void setWeightedVertices(bool enable) { _weightedVertices = enable; }
  bool getWeightedVertices() const { return _weightedVertices; }
  void setCoarRatio(double cratio) { _coarRatio = cratio; }
  double getCoarRatio() { return _coarRatio; }
  void setCoarVertices(unsigned coarVertices) { _coarVertices = coarVertices; }
  unsigned getCoarVertices() const { return _coarVertices; }
  void setTermProp(bool enable) { _termProp = enable; }
  bool getTermProp() const { return _termProp; }
  void setCutHopRatio(double ratio) { _cutHopRatio = ratio; }
  double getCutHopRatio() { return _cutHopRatio; }
  void setArchTopology(const std::vector<int>& arch) { _archTopology = arch; }
  const std::vector<int>& getArchTopology() const { return _archTopology; }
  void setTool(const std::string& tool) { _tool = tool; }
  std::string getTool() const { return _tool; }
  void setGraphModel(const std::string& graphModel)
  {
    if (graphModel == "clique") {
      _graphModel = CLIQUE;
    } else if (graphModel == "star") {
      _graphModel = STAR;
    } else
      _graphModel = HYBRID;
  }
  GraphType getGraphModel() const { return _graphModel; }
  void setCliqueThreshold(unsigned threshold) { _cliqueThreshold = threshold; }
  unsigned getCliqueThreshold() const { return _cliqueThreshold; }
  void setWeightModel(unsigned model) { _weightModel = model; }
  unsigned getWeightModel() const { return _weightModel; }
  void setMaxEdgeWeight(unsigned weight) { _maxEdgeWeight = weight; }
  unsigned getMaxEdgeWeight() const { return _maxEdgeWeight; }
  void setMaxVertexWeight(unsigned weight) { _maxVertexWeight = weight; }
  unsigned getMaxVertexWeight() const { return _maxVertexWeight; }
  void setBalanceConstraint(unsigned constraint)
  {
    _balanceConstraint = constraint;
  }
  unsigned getBalanceConstraint() const { return _balanceConstraint; }
  void setRefinement(unsigned number) { _refinement = number; }
  unsigned getRefinement() const { return _refinement; }
  void setRandomSeed(int seed);
  void generateSeeds(int seeds);
  int getNewSeed() { return seedGenerator_(); }
  void setSeeds(const std::set<int>& seeds) { _seeds = seeds; }
  const std::set<int>& getSeeds() const { return _seeds; }
  void setExistingID(int id) { _existingId = id; }
  int getExistingID() const { return _existingId; }
  void setPartitionsToTest(const std::vector<int>& partIds)
  {
    _partitionsToTest = partIds;
  }
  const std::vector<int>& getPartitionsToTest() const
  {
    return _partitionsToTest;
  }
  void setEvaluationFunction(const std::string& function)
  {
    _evaluationFunction = function;
  }
  std::string getEvaluationFunction() const { return _evaluationFunction; }
  void setLevel(unsigned level) { _level = level; }
  unsigned getLevel() const { return _level; }
  void setFinalPartitions(unsigned target) { _finalPartitions = target; }
  unsigned getFinalPartitions() { return _finalPartitions; }
  void setForceGraph(bool force) { _forceGraph = force; }
  bool getForceGraph() { return _forceGraph; }
  void setClusteringScheme(const std::string& scheme)
  {
    _clusteringScheme = scheme;
  }
  std::string getClusteringScheme() { return _clusteringScheme; }

 private:
  unsigned _numStarts = 1;
  unsigned _targetPartitions = 0;
  bool _weightedVertices = false;
  double _coarRatio = 0.7;
  unsigned _coarVertices = 2500;
  bool _termProp = false;
  double _cutHopRatio = 1.0;
  std::string _tool = "chaco";
  GraphType _graphModel = HYPERGRAPH;
  std::string _evaluationFunction = "hyperedges";
  unsigned _cliqueThreshold = 50;
  unsigned _weightModel = 1;
  unsigned _maxEdgeWeight = 100;
  unsigned _maxVertexWeight = 100;
  unsigned _balanceConstraint = 2;
  unsigned _refinement = 0;
  int _level = -1;
  int _existingId = -1;
  unsigned _finalPartitions = 2;
  bool _forceGraph = false;
  std::vector<int> _archTopology;
  std::set<int> _seeds;
  std::vector<int> _partitionsToTest;
  std::string _clusteringScheme = "scheme1";
  std::mt19937 seedGenerator_ = std::mt19937();
};

class PartSolutions
{
 public:
  void addAssignment(std::vector<unsigned long> currentAssignment,
                     unsigned long runtime,
                     int seed);
  void clearAssignments();
  const std::vector<unsigned long>& getAssignment(unsigned idx) const
  {
    return _assignmentResults[idx];
  }
  unsigned long getRuntime(unsigned idx) const { return _runtimeResults[idx]; }
  int getSeed(unsigned idx) const { return _seeds[idx]; }
  void setToolName(std::string name) { _toolName = name; }
  std::string getToolName() const { return _toolName; }
  void setPartitionId(unsigned id) { _partitionId = id; }
  unsigned getPartitionId() const { return _partitionId; }
  void setClusterId(unsigned id) { _clusterId = id; }
  unsigned getClusterId() const { return _clusterId; }
  void setBestSolutionIdx(unsigned idx) { _bestSolutionIdx = idx; }
  unsigned getBestSolutionIdx() const { return _bestSolutionIdx; }
  unsigned getNumOfRuns() const { return _seeds.size(); }
  void setBestSetSize(double result) { _bestSetSizeSD = result; }
  double getBestSetSize() const { return _bestSetSizeSD; }
  void setBestSetArea(double result) { _bestSetAreaSD = result; }
  double getBestSetArea() const { return _bestSetAreaSD; }
  void setBestNumTerminals(unsigned long result) { _bestNumTerminals = result; }
  unsigned long getBestNumTerminals() const { return _bestNumTerminals; }
  void setBestNumHyperedgeCuts(unsigned long result)
  {
    _bestNumHyperedgeCuts = result;
  }
  unsigned long getBestNumHyperedgeCuts() const
  {
    return _bestNumHyperedgeCuts;
  }
  void setBestRuntime(unsigned long result) { _bestRuntime = result; }
  unsigned long getBestRuntime() const { return _bestRuntime; }
  void setBestHopWeigth(unsigned long result) { _bestHopWeigth = result; }
  unsigned long getBestHopWeigth() const { return _bestHopWeigth; }

  void resetEvaluation();

 private:
  std::vector<std::vector<unsigned long>> _assignmentResults;
  std::vector<unsigned long> _runtimeResults;
  std::vector<int> _seeds;
  std::string _toolName = "";
  unsigned _partitionId = 0;
  unsigned _clusterId = 0;
  unsigned _bestSolutionIdx = 0;
  unsigned _numOfRuns = 0;
  double _bestSetSizeSD = 0;
  double _bestSetAreaSD = 0;
  unsigned long _bestNumTerminals = 0;
  unsigned long _bestNumHyperedgeCuts = 0;
  unsigned long _bestRuntime = 0;
  unsigned long _bestHopWeigth = 0;
};

class PartitionMgr
{
 protected:
  odb::dbBlock* getDbBlock() const;
  unsigned getNumPartitioningResults() const { return _results.size(); }
  unsigned getNumClusteringResults() const { return _clusResults.size(); }
  PartSolutions& getPartitioningResult(unsigned id) { return _results[id]; }
  PartSolutions& getClusteringResult(unsigned id) { return _clusResults[id]; }

  PartOptions _options;
  unsigned _dbId = 0;
  unsigned _bestId = 0;
  Logger * _logger;
  Graph _graph;
  Hypergraph _hypergraph;
  std::vector<PartSolutions> _results;
  std::vector<PartSolutions> _clusResults;

 public:
  PartitionMgr() = default;
  void init(unsigned dbId, Logger* logger);
  void runPartitioning();
  void runClustering();
  void run3PClustering();
  void evaluatePartitioning();
  unsigned getCurrentBestId() { return _bestId; }
  void setCurrentBestId(unsigned id) { _bestId = id; }
  void runChaco();
  void runGpMetis();
  void runMlPart();
  void runChacoClustering();
  void runGpMetisClustering();
  void runMlPartClustering();
  PartOptions& getOptions() { return _options; }
  unsigned getCurrentId() { return (_results.size() - 1); }
  unsigned getCurrentClusId() { return (_clusResults.size() - 1); }
  void toGraph();
  void toHypergraph();
  void hypergraph(bool buildGraph = false);
  unsigned generatePartitionId();
  unsigned generateClusterId();
  void computePartitionResult(unsigned partitionId, std::string function);
  bool comparePartitionings(const PartSolutions oldPartition,
                            const PartSolutions newPartition,
                            std::string function);
  void reportPartitionResult(unsigned partitionId);
  void writePartitioningToDb(unsigned partitionId);
  void dumpPartIdToFile(std::string name);
  void writeClusteringToDb(unsigned clusteringId);
  void dumpClusIdToFile(std::string name);
  void reportNetlistPartitions(unsigned partitionId);
  void readPartitioningFile(std::string filename);
  void reportGraph();

  void writePartitionVerilog(const char* path, const char* port_prefix, const char* module_suffix);

 private:
  sta::Instance* buildPartitionedInstance(const char* name,
                                          const char* port_prefix,
                                          sta::Library* library,
                                          sta::NetworkReader* network,
                                          sta::Instance* parent,
                                          std::set<sta::Instance*>* insts,
                                          std::map<sta::Net*, sta::Port*>* port_map);
};

}  // namespace par
