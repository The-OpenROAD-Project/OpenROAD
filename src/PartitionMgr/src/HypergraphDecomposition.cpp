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

#include "opendb/db.h"

namespace par {

void HypergraphDecomposition::init(int dbId, Logger* logger)
{
  _db = odb::dbDatabase::getDatabase(dbId);
  _chip = _db->getChip();
  _block = _chip->getBlock();
  _logger = logger;
}

void HypergraphDecomposition::addMapping(Hypergraph& hypergraph,
                                         std::string instName,
                                         const odb::Rect& rect)
{
  int length = rect.dy();
  int width = rect.dx();
  int64_t area = length * width;
  int nextIdx = hypergraph.computeNextVertexIdx();
  hypergraph.addMapping(instName, nextIdx);
  hypergraph.addVertexWeight(area);
}

void HypergraphDecomposition::constructMap(Hypergraph& hypergraph,
                                           unsigned maxVertexWeight)
{
  for (odb::dbNet* net : _block->getNets()) {
    int nITerms = (net->getITerms()).size();
    int nBTerms = (net->getBTerms()).size();
    if (nITerms + nBTerms >= 2) {
      for (odb::dbBTerm* bterm : net->getBTerms()) {
        for (odb::dbBPin* pin : bterm->getBPins()) {
          if (!hypergraph.isInMap(bterm->getName())) {
            odb::Rect box = pin->getBBox();
            addMapping(hypergraph, bterm->getName(), box);
          }
        }
      }

      for (odb::dbITerm* iterm : net->getITerms()) {
        odb::dbInst* inst = iterm->getInst();
        if (!hypergraph.isInMap(inst->getName())) {
          odb::dbBox* bbox = inst->getBBox();
          odb::Rect rect;
          bbox->getBox(rect);
          addMapping(hypergraph, inst->getName(), rect);
        }
      }
    }
  }
  hypergraph.computeVertexWeightRange(maxVertexWeight, _logger);
}

void HypergraphDecomposition::createHypergraph(
    Hypergraph& hypergraph,
    std::vector<unsigned long> clusters,
    short currentCluster)
{
  for (odb::dbNet* net : _block->getNets()) {
    int nITerms = (net->getITerms()).size();
    int nBTerms = (net->getBTerms()).size();
    if (nITerms + nBTerms >= 2) {
      int driveIdx = -1;
      std::vector<int> netVertices;
      int nextPtr = hypergraph.computeNextRowPtr();
      hypergraph.addRowPtr(nextPtr);
      hypergraph.addEdgeWeightNormalized(1);
      for (odb::dbBTerm* bterm : net->getBTerms()) {
        for (odb::dbBPin* pin : bterm->getBPins()) {
          int mapping = hypergraph.getMapping(bterm->getName());
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
        int mapping = hypergraph.getMapping(inst->getName());
        if (clusters[mapping] == currentCluster) {
          if (driveIdx == -1 && iterm->isOutputSignal())
            driveIdx = mapping;
          else
            netVertices.push_back(mapping);
        }
      }
      if (driveIdx != -1)
        hypergraph.addColIdx(driveIdx);
      for (int vertex : netVertices) {
        hypergraph.addColIdx(vertex);
      }
    }
  }
  int nextPtr = hypergraph.computeNextRowPtr();
  hypergraph.addRowPtr(nextPtr);
}

/*
updateHypergraph: Given a hypergraph and a cluster solution,
this function creates a new hypergraph containing only nodes/connections
that are inside a specific cluster
*/

void HypergraphDecomposition::updateHypergraph(
    Hypergraph& hypergraph,
    Hypergraph& newHypergraph,
    std::vector<unsigned long> clusters,
    short currentCluster)
{
  std::vector<int> colIdx = hypergraph.getColIdx();
  std::vector<int> rowPtr = hypergraph.getRowPtr();
  std::vector<int> vertexWeights = hypergraph.getVertexWeight();
  std::vector<int> edgeWeights = hypergraph.getEdgeWeight();

  for (int i = 0; i < rowPtr.size() - 1; i++) {
    int instInNet = 0;
    int start = rowPtr[i];
    int end = rowPtr[i + 1];

    std::vector<int> netVertices;
    int nextPtr = newHypergraph.computeNextRowPtr();

    for (int j = start; j < end; j++) {
      int mapping = colIdx[j];
      if (clusters[mapping] == currentCluster) {
        instInNet++;
        if (!newHypergraph.isInClusterMap(mapping)) {
          int nextIdx = newHypergraph.computeNextVertexIdx(true);
          newHypergraph.addClusterMapping(mapping, nextIdx);
          newHypergraph.addVertexWeightNormalized(vertexWeights[mapping]);
        }
        int idx = newHypergraph.getClusterMapping(mapping);
        netVertices.push_back(idx);
      }
    }
    if (instInNet >= 2) {
      for (int j : netVertices)
        newHypergraph.addColIdx(j);

      newHypergraph.addRowPtr(nextPtr);
      newHypergraph.addEdgeWeightNormalized(edgeWeights[i]);
    }
  }

  int nextPtr = newHypergraph.computeNextRowPtr();
  newHypergraph.addRowPtr(nextPtr);
}

void HypergraphDecomposition::toGraph(Hypergraph& hypergraph,
                                      Graph& graph,
                                      GraphType graphModel,
                                      unsigned weightingOption,
                                      unsigned maxEdgeWeight,
                                      unsigned threshold)
{
  _weightingOption = weightingOption;
  std::vector<int> colIdx = hypergraph.getColIdx();
  adjMatrix.resize(hypergraph.getNumVertex());
  for (int i = 0; i < hypergraph.getNumRowPtr() - 1; i++) {
    std::vector<int> net;
    std::vector<int>::iterator begin = colIdx.begin() + hypergraph.getRowPtr(i);
    std::vector<int>::iterator end = colIdx.end();
    end = colIdx.begin() + hypergraph.getRowPtr(i + 1);
    net.assign(begin, end);

    switch (graphModel) {
      case CLIQUE:
        if (net.size() <= threshold)
          createCliqueGraph(graph, net);
        break;
      case STAR:
        createStarGraph(graph, net);
        break;
      case HYBRID:
        if (net.size() > threshold) {
          createStarGraph(graph, net);
        } else {
          createCliqueGraph(graph, net);
        }
        break;
      case HYPERGRAPH:
        break;
    }
  }
  createCompressedMatrix(graph);
  graph.assignVertexWeight(hypergraph.getVertexWeight());
  graph.computeEdgeWeightRange(maxEdgeWeight, _logger);
}

/*
The following formulas were selected due to the work presented in
C. J. Alpert and A. B. Kahng, “Recent Directions in Netlist Partitioning: A
Survey“
 */

float HypergraphDecomposition::computeWeight(int nPins)
{
  switch (_weightingOption) {
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

void HypergraphDecomposition::connectStarPins(int firstPin,
                                              int secondPin,
                                              float weight)
{
  if (firstPin != secondPin) {
    if (adjMatrix[firstPin].find(secondPin) == adjMatrix[firstPin].end())
      adjMatrix[firstPin][secondPin] = weight;
  }
}

void HypergraphDecomposition::connectPins(int firstPin,
                                          int secondPin,
                                          float weight)
{
  if (firstPin != secondPin) {
    if (adjMatrix[firstPin].find(secondPin) == adjMatrix[firstPin].end())
      adjMatrix[firstPin][secondPin] = weight;
    else
      adjMatrix[firstPin][secondPin] += weight;
  }
}

void HypergraphDecomposition::createStarGraph(Graph& graph,
                                              std::vector<int> net)
{
  float weight = 1;
  int driveIdx = 0;
  for (int i = 1; i < net.size(); i++) {
    connectStarPins(net[i], net[driveIdx], weight);
    connectStarPins(net[driveIdx], net[i], weight);
  }
}

void HypergraphDecomposition::createCliqueGraph(Graph& graph,
                                                std::vector<int> net)
{
  float weight = computeWeight(net.size());
  for (int i = 0; i < net.size(); i++) {
    for (int j = i + 1; j < net.size(); j++) {
      connectPins(net[i], net[j], weight);
      connectPins(net[j], net[i], weight);
    }
  }
}

void HypergraphDecomposition::createCompressedMatrix(Graph& graph)
{
  for (std::map<int, float>& node : adjMatrix) {
    int nextPtr = graph.computeNextRowPtr();
    graph.addRowPtr(nextPtr);
    for (std::pair<const int, float>& connection : node) {
      graph.addEdgeWeight(connection.second);
      graph.addColIdx(connection.first);
    }
  }
}

void HypergraphDecomposition::toHypergraph(Hypergraph& hypergraph, Graph& graph)
{
  std::vector<int> colIdx = graph.getColIdx();
  adjMatrix.resize(graph.getNumVertex());
  int countEdges = 0;
  for (int i = 0; i < graph.getNumRowPtr() - 1; i++) {
    std::vector<int> connections;
    std::vector<int>::iterator begin = colIdx.begin() + graph.getRowPtr(i);
    std::vector<int>::iterator end = colIdx.end();
    end = colIdx.begin() + graph.getRowPtr(i + 1);
    connections.assign(begin, end);
    for (int j : connections) {
      if (j > i) {
        adjMatrix[i][j] = graph.getEdgeWeight(countEdges);
      }
      countEdges++;
    }
  }
  int countVertex = 0;
  for (std::map<int, float>& node : adjMatrix) {
    for (std::pair<const int, float>& connection : node) {
      int nextPtr = hypergraph.computeNextRowPtr();
      hypergraph.addRowPtr(nextPtr);
      hypergraph.addColIdx(countVertex);
      hypergraph.addColIdx(connection.first);
      hypergraph.addEdgeWeightNormalized(connection.second);
    }
    countVertex++;
  }
  int nextPtr = hypergraph.computeNextRowPtr();
  hypergraph.addRowPtr(nextPtr);

  hypergraph.assignVertexWeight(graph.getVertexWeight());
}

}  // namespace par
