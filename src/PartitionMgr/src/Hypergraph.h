/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
  int getEdgeWeight(int idx) const { return _edgeWeightsNormalized.at(idx); }
  int getVertexWeight(int idx) const
  {
    return _vertexWeightsNormalized.at(idx);
  }
  int getColIdx(int idx) const { return _colIdx.at(idx); }
  int getRowPtr(int idx) const { return _rowPtr.at(idx); }
  int getMapping(std::string inst) { return _instToIdx.at(inst); }
  int getClusterMapping(int idx) { return _idxToClusterIdx.at(idx); }
  std::vector<float> getDefaultEdgeWeight() const& { return _edgeWeights; };
  std::vector<int> getEdgeWeight() const& { return _edgeWeightsNormalized; };
  std::vector<int> getVertexWeight() const&
  {
    return _vertexWeightsNormalized;
  };
  std::vector<int> getColIdx() const& { return _colIdx; };
  std::vector<int> getRowPtr() const& { return _rowPtr; };

  void addEdgeWeight(float weight) { _edgeWeights.push_back(weight); }
  void addEdgeWeightNormalized(int weight)
  {
    _edgeWeightsNormalized.push_back(weight);
  }
  void addVertexWeight (int64_t weight)
  {
    _vertexWeights.push_back(weight);
  }
  void addVertexWeightNormalized(int64_t weight)
  {
    _vertexWeightsNormalized.push_back(weight);
  }
  void addColIdx(int idx) { _colIdx.push_back(idx); }
  void addRowPtr(int idx) { _rowPtr.push_back(idx); }
  void addMapping(std::string inst, int idx) { _instToIdx[inst] = idx; }
  void addClusterMapping(int idx, int clusterIdx)
  {
    _idxToClusterIdx[idx] = clusterIdx;
  }
  std::map<std::string, int>& getMap() { return _instToIdx; }

  inline bool isInMap(std::string pinName) const
  {
    return (_instToIdx.find(pinName) != _instToIdx.end());
  }

  inline bool isInClusterMap(int idx) const
  {
    return (_idxToClusterIdx.find(idx) != _idxToClusterIdx.end());
  }

  void clearHypergraph()
  {
    _edgeWeightsNormalized.clear();
    _edgeWeights.clear();
    _vertexWeightsNormalized.clear();
    _colIdx.clear();
    _rowPtr.clear();
    _instToIdx.clear();
  }

  void computeWeightRange(int maxEdgeWeight, int maxVertexWeight);
  void computeVertexWeightRange(int maxVertexWeight, Logger * logger);
  void computeEdgeWeightRange(int maxEdgeWeight, Logger * logger);
  int computeNextVertexIdx(bool cluster = false) const
  {
    if (cluster)
      return _vertexWeightsNormalized.size();
    return _vertexWeights.size();
  }
  int computeNextRowPtr() const { return _colIdx.size(); }
  int getNumEdges() const { return _edgeWeightsNormalized.size(); }
  int getNumVertex() const { return _vertexWeightsNormalized.size(); }
  int getNumColIdx() const { return _colIdx.size(); }
  int getNumRowPtr() const { return _rowPtr.size(); }
  void assignVertexWeight(std::vector<int> vertex)
  {
    _vertexWeightsNormalized = vertex;
  }

 protected:
  std::vector<float> _edgeWeights;
  std::vector<int> _edgeWeightsNormalized;
  std::vector<int64_t> _vertexWeights;
  std::vector<int> _vertexWeightsNormalized;
  std::vector<int> _colIdx;
  std::vector<int> _rowPtr;
  std::map<std::string, int> _instToIdx;
  std::map<int, int> _idxToClusterIdx;
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
    _edgeWeightsNormalized.clear();
    _edgeWeights.clear();
    _vertexWeightsNormalized.clear();
    _colIdx.clear();
    _rowPtr.clear();
    _instToIdx.clear();
  }
  void assignVertexWeight(std::vector<int> vertex)
  {
    _vertexWeightsNormalized = vertex;
  }
};

}  // namespace par
