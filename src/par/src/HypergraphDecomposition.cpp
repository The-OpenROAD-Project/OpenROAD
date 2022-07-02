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

#include "HypergraphDecomposition.h"

#include <fstream>

#include "odb/db.h"

namespace par {

HypergraphDecomposition::HypergraphDecomposition()
    : block_(nullptr), logger_(nullptr), weightingOption_(0)
{
}

void HypergraphDecomposition::init(odb::dbBlock* block, Logger* logger)
{
  block_ = block;
  logger_ = logger;
}

void HypergraphDecomposition::addMapping(Hypergraph& hypergraph,
                                         std::string instName,
                                         const odb::Rect& rect)
{
  const int64_t length = rect.dy();
  const int64_t width = rect.dx();
  const int64_t area = length * width;
  const int nextIdx = hypergraph.computeNextVertexIdx();
  hypergraph.addMapping(instName, nextIdx);
  hypergraph.addVertexWeight(area);
}

void HypergraphDecomposition::constructMap(Hypergraph& hypergraph,
                                           const unsigned maxVertexWeight)
{
  for (odb::dbNet* net : block_->getNets()) {
    const int nITerms = (net->getITerms()).size();
    const int nBTerms = (net->getBTerms()).size();
    if (nITerms + nBTerms >= 2) {
      for (odb::dbBTerm* bterm : net->getBTerms()) {
        for (odb::dbBPin* pin : bterm->getBPins()) {
          if (!hypergraph.isInMap(bterm->getName())) {
            const odb::Rect box = pin->getBBox();
            addMapping(hypergraph, bterm->getName(), box);
          }
        }
      }

      for (odb::dbITerm* iterm : net->getITerms()) {
        odb::dbInst* inst = iterm->getInst();
        if (!hypergraph.isInMap(inst->getName())) {
          odb::dbBox* bbox = inst->getBBox();
          odb::Rect rect = bbox->getBox();
          addMapping(hypergraph, inst->getName(), rect);
        }
      }
    }
  }
  hypergraph.computeVertexWeightRange(maxVertexWeight, logger_);
}

void HypergraphDecomposition::createHypergraph(
    Hypergraph& hypergraph,
    const std::vector<unsigned long>& clusters,
    const short currentCluster)
{
  for (odb::dbNet* net : block_->getNets()) {
    const int nITerms = (net->getITerms()).size();
    const int nBTerms = (net->getBTerms()).size();
    if (nITerms + nBTerms >= 2) {
      int driveIdx = -1;
      std::vector<int> netVertices;
      const int nextPtr = hypergraph.computeNextRowPtr();
      hypergraph.addRowPtr(nextPtr);
      hypergraph.addEdgeWeightNormalized(1);
      for (odb::dbBTerm* bterm : net->getBTerms()) {
        for (odb::dbBPin* pin : bterm->getBPins()) {
          (void) pin;  // unused?
          const int mapping = hypergraph.getMapping(bterm->getName());
          if (clusters[mapping] == currentCluster) {
            if (driveIdx == -1 && bterm->getIoType() == odb::dbIoType::INPUT)
              driveIdx = mapping;
            else
              netVertices.push_back(mapping);
          }
        }
      }

      for (odb::dbITerm* iterm : net->getITerms()) {
        odb::dbInst* inst = iterm->getInst();
        const int mapping = hypergraph.getMapping(inst->getName());
        if (clusters[mapping] == currentCluster) {
          if (driveIdx == -1 && iterm->isOutputSignal())
            driveIdx = mapping;
          else
            netVertices.push_back(mapping);
        }
      }
      if (driveIdx != -1)
        hypergraph.addColIdx(driveIdx);
      for (const int vertex : netVertices) {
        hypergraph.addColIdx(vertex);
      }
    }
  }
  const int nextPtr = hypergraph.computeNextRowPtr();
  hypergraph.addRowPtr(nextPtr);
}

/*
updateHypergraph: Given a hypergraph and a cluster solution,
this function creates a new hypergraph containing only nodes/connections
that are inside a specific cluster
*/

void HypergraphDecomposition::updateHypergraph(
    const Hypergraph& hypergraph,
    Hypergraph& newHypergraph,
    const std::vector<unsigned long>& clusters,
    const short currentCluster)
{
  const std::vector<int>& colIdx = hypergraph.getColIdx();
  const std::vector<int>& rowPtr = hypergraph.getRowPtr();
  const std::vector<int>& vertexWeights = hypergraph.getVertexWeight();
  const std::vector<int>& edgeWeights = hypergraph.getEdgeWeight();

  for (int i = 0; i < rowPtr.size() - 1; i++) {
    int instInNet = 0;
    const int start = rowPtr[i];
    const int end = rowPtr[i + 1];

    std::vector<int> netVertices;
    const int nextPtr = newHypergraph.computeNextRowPtr();

    for (int j = start; j < end; j++) {
      const int mapping = colIdx[j];
      if (clusters[mapping] == currentCluster) {
        instInNet++;
        if (!newHypergraph.isInClusterMap(mapping)) {
          const int nextIdx = newHypergraph.computeNextVertexIdx(true);
          newHypergraph.addClusterMapping(mapping, nextIdx);
          newHypergraph.addVertexWeightNormalized(vertexWeights[mapping]);
        }
        const int idx = newHypergraph.getClusterMapping(mapping);
        netVertices.push_back(idx);
      }
    }
    if (instInNet >= 2) {
      for (const int j : netVertices)
        newHypergraph.addColIdx(j);

      newHypergraph.addRowPtr(nextPtr);
      newHypergraph.addEdgeWeightNormalized(edgeWeights[i]);
    }
  }

  const int nextPtr = newHypergraph.computeNextRowPtr();
  newHypergraph.addRowPtr(nextPtr);
}

void HypergraphDecomposition::toGraph(const Hypergraph& hypergraph,
                                      Graph& graph,
                                      const GraphType graphModel,
                                      const unsigned weightingOption,
                                      const unsigned maxEdgeWeight,
                                      const unsigned threshold)
{
  weightingOption_ = weightingOption;
  const std::vector<int>& colIdx = hypergraph.getColIdx();
  adjMatrix_.resize(hypergraph.getNumVertex());
  for (int i = 0; i < hypergraph.getNumRowPtr() - 1; i++) {
    const auto begin = colIdx.begin() + hypergraph.getRowPtr(i);
    const auto end = colIdx.begin() + hypergraph.getRowPtr(i + 1);
    const std::vector<int> net(begin, end);

    switch (graphModel) {
      case CLIQUE:
        if (net.size() <= threshold)
          createCliqueGraph(net);
        break;
      case STAR:
        createStarGraph(net);
        break;
      case HYBRID:
        if (net.size() > threshold) {
          createStarGraph(net);
        } else {
          createCliqueGraph(net);
        }
        break;
    }
  }
  createCompressedMatrix(graph);
  graph.assignVertexWeight(hypergraph.getVertexWeight());
  graph.computeEdgeWeightRange(maxEdgeWeight, logger_);
}

/*
The following formulas were selected due to the work presented in
C. J. Alpert and A. B. Kahng, "Recent Directions in Netlist Partitioning: A
Survey"
 */

float HypergraphDecomposition::computeWeight(const int nPins)
{
  switch (weightingOption_) {
    case 1:
      return 1.0 / (nPins - 1);
    case 2:
      return 4.0 / (nPins * (nPins - 1));
    case 3:
      return 4.0 / (nPins * nPins - (nPins % 2));
    case 4:
      return 6.0 / (nPins * (nPins + 1));
    case 5:
      return pow((2.0 / nPins), 1.5);
    case 6:
      return pow((2.0 / nPins), 3);
    case 7:
      return 2.0 / nPins;
  }
  return 1.0;
}

void HypergraphDecomposition::connectStarPins(const int firstPin,
                                              const int secondPin,
                                              const float weight)
{
  if (firstPin != secondPin) {
    if (adjMatrix_[firstPin].find(secondPin) == adjMatrix_[firstPin].end())
      adjMatrix_[firstPin][secondPin] = weight;
  }
}

void HypergraphDecomposition::connectPins(const int firstPin,
                                          const int secondPin,
                                          const float weight)
{
  if (firstPin != secondPin) {
    if (adjMatrix_[firstPin].find(secondPin) == adjMatrix_[firstPin].end())
      adjMatrix_[firstPin][secondPin] = weight;
    else
      adjMatrix_[firstPin][secondPin] += weight;
  }
}

void HypergraphDecomposition::createStarGraph(const std::vector<int>& net)
{
  const float weight = 1;
  const int driveIdx = 0;
  for (int i = 1; i < net.size(); i++) {
    connectStarPins(net[i], net[driveIdx], weight);
    connectStarPins(net[driveIdx], net[i], weight);
  }
}

void HypergraphDecomposition::createCliqueGraph(const std::vector<int>& net)
{
  const float weight = computeWeight(net.size());
  for (int i = 0; i < net.size(); i++) {
    for (int j = i + 1; j < net.size(); j++) {
      connectPins(net[i], net[j], weight);
      connectPins(net[j], net[i], weight);
    }
  }
}

void HypergraphDecomposition::createCompressedMatrix(Graph& graph)
{
  for (const std::map<int, float>& node : adjMatrix_) {
    const int nextPtr = graph.computeNextRowPtr();
    graph.addRowPtr(nextPtr);
    for (const std::pair<const int, float>& connection : node) {
      graph.addEdgeWeight(connection.second);
      graph.addColIdx(connection.first);
    }
  }
}

void HypergraphDecomposition::toHypergraph(Hypergraph& hypergraph,
                                           const Graph* graph)
{
  const std::vector<int>& colIdx = graph->getColIdx();
  adjMatrix_.resize(graph->getNumVertex());
  int countEdges = 0;
  for (int i = 0; i < graph->getNumRowPtr() - 1; i++) {
    std::vector<int> connections;
    const auto begin = colIdx.begin() + graph->getRowPtr(i);
    const auto end = colIdx.begin() + graph->getRowPtr(i + 1);
    connections.assign(begin, end);
    for (const int j : connections) {
      if (j > i) {
        adjMatrix_[i][j] = graph->getEdgeWeight(countEdges);
      }
      countEdges++;
    }
  }
  int countVertex = 0;
  for (const std::map<int, float>& node : adjMatrix_) {
    for (const std::pair<const int, float>& connection : node) {
      const int nextPtr = hypergraph.computeNextRowPtr();
      hypergraph.addRowPtr(nextPtr);
      hypergraph.addColIdx(countVertex);
      hypergraph.addColIdx(connection.first);
      hypergraph.addEdgeWeightNormalized(connection.second);
    }
    countVertex++;
  }
  const int nextPtr = hypergraph.computeNextRowPtr();
  hypergraph.addRowPtr(nextPtr);

  hypergraph.assignVertexWeight(graph->getVertexWeight());
}

}  // namespace par
