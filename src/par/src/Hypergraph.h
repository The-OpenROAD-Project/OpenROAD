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

#pragma once

#include <map>
#include <string>
#include <vector>

namespace utl {
class Logger;
}
using utl::Logger;

namespace par {

class Hypergraph
{
 public:
  Hypergraph() {}
  virtual ~Hypergraph() = default;
  int getEdgeWeight(int idx) const { return edgeWeightsNormalized_.at(idx); }
  int getVertexWeight(int idx) const
  {
    return vertexWeightsNormalized_.at(idx);
  }
  int getColIdx(int idx) const { return colIdx_.at(idx); }
  int getRowPtr(int idx) const { return rowPtr_.at(idx); }
  int getMapping(const std::string& inst) const { return instToIdx_.at(inst); }
  int getClusterMapping(int idx) const { return idxToClusterIdx_.at(idx); }
  const std::vector<float>& getDefaultEdgeWeight() const
  {
    return edgeWeights_;
  };
  const std::vector<int>& getEdgeWeight() const
  {
    return edgeWeightsNormalized_;
  };
  const std::vector<int>& getVertexWeight() const
  {
    return vertexWeightsNormalized_;
  };
  const std::vector<int>& getColIdx() const { return colIdx_; };
  const std::vector<int>& getRowPtr() const { return rowPtr_; };

  void addEdgeWeight(float weight) { edgeWeights_.push_back(weight); }
  void addEdgeWeightNormalized(int weight)
  {
    edgeWeightsNormalized_.push_back(weight);
  }
  void addVertexWeight(int64_t weight) { vertexWeights_.push_back(weight); }
  void addVertexWeightNormalized(int64_t weight)
  {
    vertexWeightsNormalized_.push_back(weight);
  }
  void addColIdx(int idx) { colIdx_.push_back(idx); }
  void addRowPtr(int idx) { rowPtr_.push_back(idx); }
  void addMapping(std::string inst, int idx) { instToIdx_[inst] = idx; }
  void addClusterMapping(int idx, int clusterIdx)
  {
    idxToClusterIdx_[idx] = clusterIdx;
  }
  const std::map<std::string, int>& getMap() { return instToIdx_; }

  bool isInMap(std::string pinName) const
  {
    return (instToIdx_.find(pinName) != instToIdx_.end());
  }

  bool isInClusterMap(int idx) const
  {
    return (idxToClusterIdx_.find(idx) != idxToClusterIdx_.end());
  }

  void clearHypergraph()
  {
    edgeWeightsNormalized_.clear();
    edgeWeights_.clear();
    vertexWeightsNormalized_.clear();
    colIdx_.clear();
    rowPtr_.clear();
    instToIdx_.clear();
  }

  void computeWeightRange(int maxEdgeWeight, int maxVertexWeight);
  void computeVertexWeightRange(int maxVertexWeight, Logger* logger);
  void computeEdgeWeightRange(int maxEdgeWeight, Logger* logger);
  int computeNextVertexIdx(bool cluster = false) const
  {
    return cluster ? vertexWeightsNormalized_.size() : vertexWeights_.size();
  }
  int computeNextRowPtr() const { return colIdx_.size(); }
  int getNumEdges() const { return edgeWeightsNormalized_.size(); }
  int getNumVertex() const { return vertexWeightsNormalized_.size(); }
  int getNumColIdx() const { return colIdx_.size(); }
  int getNumRowPtr() const { return rowPtr_.size(); }
  void assignVertexWeight(const std::vector<int>& vertex)
  {
    vertexWeightsNormalized_ = vertex;
  }

 protected:
  std::vector<float> edgeWeights_;
  std::vector<int> edgeWeightsNormalized_;
  std::vector<int64_t> vertexWeights_;
  std::vector<int> vertexWeightsNormalized_;
  std::vector<int> colIdx_;
  std::vector<int> rowPtr_;
  std::map<std::string, int> instToIdx_;
  std::map<int, int> idxToClusterIdx_;
};

/*
Although the "Graph" class does not add new functionalities
to the "Hypergraph" class, edges and hyperedges are stored
using different formats. Therefore, the different classes
are important to indicate how to read the edges/hyperedges
connections.

Example:

Adjacency matrix of a 3-node clique graph:

                |0 1 1|
                |1 0 1|
                |1 1 0|

colidx 1 2 0 2 0 1
rowptr 0 2 4 6

colidx lists all adjacent nodes for each node on the graph
rowptr indicates where the list for each node starts/ends
e.g.,
the nodes adjacent to node 0 begin to be described at colidx[rowptr[0]]
and end at colidx[rowptr[1] - 1]
i.e., nodes 1 and 2

------------------------------------------------------------------------

The same 3 nodes, now represented as a hypergraph with a
single hyperedge:

colidx 0 1 2
rowptr 0 3

colidx lists the nodes connected to each hyperedge
rowptr indicates where the list for each hyperedge starts/ends
e.g.,
the nodes connected to hyperedge 0 begin to be described at colidx[rowptr[0]]
and end at colidx[rowptr[1] - 1]
i.e., nodes 0, 1 and 2

*/
class Graph : public Hypergraph
{
 public:
  Graph() {}
  void clearGraph()
  {
    edgeWeightsNormalized_.clear();
    edgeWeights_.clear();
    vertexWeightsNormalized_.clear();
    colIdx_.clear();
    rowPtr_.clear();
    instToIdx_.clear();
  }
  void assignVertexWeight(const std::vector<int>& vertex)
  {
    vertexWeightsNormalized_ = vertex;
  }
};

}  // namespace par
