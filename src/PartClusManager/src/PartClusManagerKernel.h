////////////////////////////////////////////////////////////////////////////////
// Authors: Mateus Fogaça, Isadora Oliveira and Marcelo Danigno
// 
//          (Advisor: Ricardo Reis and Paulo Butzen)
//
// BSD 3-Clause License
//
// Copyright (c) 2020, Federal University of Rio Grande do Sul (UFRGS)
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
////////////////////////////////////////////////////////////////////////////////

#include "GraphDecomposition.h"
#include <iostream>

namespace odb{
class dbDatabase;
class dbChip;
class dbBlock;
class dbNet;
}

namespace PartClusManager {

class PartOptions {
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
        std::vector<int> getArchTopology() const { return _archTopology; } 
        void setTool(const std::string& tool) { _tool = tool; }
        std::string getTool() const { return _tool; } 
        void setGraphModel(const std::string& graphModel) { _graphModel = graphModel; }
        std::string getGraphModel() const { return _graphModel; } 
        void setCliqueThreshold(unsigned threshold) { _cliqueThreshold = threshold; }
        unsigned getCliqueThreshold() const { return _cliqueThreshold; } 
        void setWeightModel(unsigned model) { _weightModel = model; }
        unsigned getWeightModel() const { return _weightModel; } 
        void setMaxEdgeWeight(unsigned weight) { _maxEdgeWeight = weight; }
        unsigned getMaxEdgeWeight() const { return _maxEdgeWeight; }
        void setMaxVertexWeight(unsigned weight) { _maxVertexWeight = weight; }
        unsigned getMaxVertexWeight() const { return _maxVertexWeight; }
        void setBalanceConstraint(unsigned constraint) { _balanceConstraint = constraint; }
        unsigned getBalanceConstraint() const { return _balanceConstraint; }
        void setSeeds(const std::vector<int>& seeds) { _seeds = seeds; }
        std::vector<int> getSeeds() const { return _seeds; } 

private:
        unsigned                _numStarts              = 1;
        unsigned                _targetPartitions       = 0;
        bool                    _weightedVertices       = false;
        double                  _coarRatio              = 0.8;
        unsigned                _coarVertices           = 2500;
        bool                    _termProp               = true;
        double                  _cutHopRatio            = 1.0;
        std::string             _tool                   = "chaco";
        std::string             _graphModel             = "clique";
        unsigned                _cliqueThreshold        = 50;
        unsigned                _weightModel            = 1;
        unsigned                _maxEdgeWeight          = 100; 
        unsigned                _maxVertexWeight        = 100; 
        unsigned                _balanceConstraint      = 5; 
        std::vector<int>        _archTopology;
        std::vector<int>        _seeds;
}; 

class PartResults {
public:
        void addResult(std::vector<short> currentAssignment, unsigned long runtime, int seed) {
                _assignmentResults.push_back(currentAssignment);
                _runtimeResults.push_back(runtime);
                _seeds.push_back(seed);
        }
        std::vector<short> getResult(unsigned idx){ return _assignmentResults[idx]; }
        unsigned long getResultRuntime(unsigned idx){ return _runtimeResults[idx]; }
        int getResultSeed(unsigned idx){ return _seeds[idx]; }
        void setToolName(std::string name) { _toolName = name; }
        std::string getToolName() { return _toolName; }
        void setPartitionId(unsigned id) { _partitionId = id; }
        unsigned getPartitionId() { return _partitionId; }
        void setBestSolutionIdx(unsigned idx) { _bestSolutionIdx = idx; }
        unsigned getBestSolutionIdx() { return _bestSolutionIdx; }
        void setNumOfRuns(unsigned runs) { _numOfRuns = runs; }
        unsigned getNumOfRuns() { return _numOfRuns; }
        void setBestSetSize(double result) { _bestSetSizeSD = result; }
        double getBestSetSize() { return _bestSetSizeSD; }
        void setBestSetArea(double result) { _bestSetAreaSD = result; }
        double getBestSetArea() { return _bestSetAreaSD; }
        void setBestNumTerminals(unsigned long result) { _bestNumTerminals = result; }
        unsigned long getBestNumTerminals() { return _bestNumTerminals; }
        void setBestNumHyperedgeCuts(unsigned long result) { _bestNumHyperedgeCuts = result; }
        unsigned long getBestNumHyperedgeCuts() { return _bestNumHyperedgeCuts; }
        void setBestRuntime(unsigned long result) { _bestRuntime = result; }
        unsigned long getBestRuntime() { return _bestRuntime; }
        void setBestHopWeigth(unsigned long result) { _bestHopWeigth = result; }
        unsigned long getBestHopWeigth() { return _bestHopWeigth; }

private:
        std::vector<std::vector<short>> _assignmentResults;
        std::vector<unsigned long>      _runtimeResults;
        std::vector<int>                _seeds;
        std::string                     _toolName               = "";
        unsigned                        _partitionId            = 0;
        unsigned                        _bestSolutionIdx        = 0;
        unsigned                        _numOfRuns              = 0;
        double                          _bestSetSizeSD          = 0;
        double                          _bestSetAreaSD          = 0;
        unsigned long                   _bestNumTerminals       = 0;
        unsigned long                   _bestNumHyperedgeCuts   = 0;
        unsigned long                   _bestRuntime            = 0;
        unsigned long                   _bestHopWeigth          = 0;

};

class PartClusManagerKernel {
protected:

private:
        PartOptions _options;
	unsigned _dbId;
	Graph _graph;
        std::vector<PartResults> _results;

public:
        PartClusManagerKernel() = default;
        void runPartitioning();
        void runChaco();
        void runChaco(const Graph& graph, const PartOptions& options);
        void runGpMetis();
        void runGpMetis(const Graph& graph, const PartOptions& options);
        void runMlPart();
        void runMlPart(const Graph& graph, const PartOptions& options);
        PartOptions& getOptions() { return _options; }
	void setDbId(unsigned id) {_dbId = id;}
	void graph();
        unsigned generatePartitionId();
        void computePartitionResult(unsigned partitionId);
        void reportPartitionResult(unsigned partitionId);
};

}
