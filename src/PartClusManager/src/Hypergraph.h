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

#include <map>
#include <string>
#include <vector>

namespace PartClusManager {

class Hypergraph
{
 public:
  Hypergraph() {}
  int getEdgeWeight(int idx) const { return _edgeWeightsNormalized[idx]; }
  int getVertexWeight(int idx) const { return _vertexWeightsNormalized[idx]; }
  int getColIdx(int idx) const { return _colIdx[idx]; }
  int getRowPtr(int idx) const { return _rowPtr[idx]; }
  int getMapping(std::string inst) { return _instToIdx[inst]; }
  int getClusterMapping(int idx) { return _idxToClusterIdx[idx]; }
  std::vector<float> getDefaultEdgeWeight() const { return _edgeWeights; };
  std::vector<int> getEdgeWeight() const { return _edgeWeightsNormalized; };
  std::vector<int> getVertexWeight() const { return _vertexWeightsNormalized; };
  std::vector<int> getColIdx() const { return _colIdx; };
  std::vector<int> getRowPtr() const { return _rowPtr; };

  void addEdgeWeight(float weight) { _edgeWeights.push_back(weight); }
  void addEdgeWeightNormalized(int weight)
  {
    _edgeWeightsNormalized.push_back(weight);
  }
  void addVertexWeight(long long int weight)
  {
    _vertexWeights.push_back(weight);
  }
  void addVertexWeightNormalized(long long int weight)
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
    if (_instToIdx.find(pinName) != _instToIdx.end())
      return true;
    else
      return false;
  }
  inline bool isInClusterMap(int idx) const
  {
    if (_idxToClusterIdx.find(idx) != _idxToClusterIdx.end())
      return true;
    else
      return false;
  }

  void fullClearHypergraph()
  {
    _edgeWeightsNormalized.clear();
    _edgeWeights.clear();
    _vertexWeightsNormalized.clear();
    _colIdx.clear();
    _rowPtr.clear();
    _instToIdx.clear();
  }

  void clearHypergraph()
  {
    _colIdx.clear();
    _rowPtr.clear();
    _edgeWeightsNormalized.clear();
    _vertexWeightsNormalized.clear();
  }

  void computeWeightRange(int maxEdgeWeight, int maxVertexWeight);
  void computeVertexWeightRange(int maxVertexWeight);
  void computeEdgeWeightRange(int maxEdgeWeight);
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
  std::vector<long long int> _vertexWeights;
  std::vector<int> _vertexWeightsNormalized;
  std::vector<int> _colIdx;
  std::vector<int> _rowPtr;
  std::map<std::string, int> _instToIdx;
  std::map<int, int> _idxToClusterIdx;
};

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

}  // namespace PartClusManager
