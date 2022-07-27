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

#include "par/PartitionMgr.h"

#include "MLPart.h"
#ifdef PARTITIONERS
extern "C" {
#include "main/ChacoWrapper.h"
}
#include "metis.h"
#endif
#include <time.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>

#include "HypergraphDecomposition.h"
#include "autocluster.h"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

using utl::PAR;

namespace par {

PartitionMgr::PartitionMgr()
    : logger_(nullptr),
      graph_(std::make_unique<Graph>()),
      hypergraph_(std::make_unique<Hypergraph>())
{
}

PartitionMgr::~PartitionMgr()
{
}

void PartitionMgr::init(odb::dbDatabase* db,
                        sta::dbNetwork* db_network,
                        sta::dbSta* sta,
                        Logger* logger)
{
  db_ = db;
  db_network_ = db_network;
  _sta = sta;
  logger_ = logger;
}

// Partition Netlist

void PartitionMgr::runPartitioning()
{
  hypergraph(true);
  if (options_.getTool() == "mlpart") {
    runMlPart();
  } else if (options_.getTool() == "gpmetis") {
    runGpMetis();
  } else {
    runChaco();
  }
}

void PartitionMgr::runChaco()
{
#ifdef PARTITIONERS
  logger_->report("Running Chaco.");

  PartSolutions currentResults;
  currentResults.setToolName(options_.getTool());
  unsigned partitionId = generatePartitionId();
  currentResults.setPartitionId(partitionId);
  std::string evaluationFunction = options_.getEvaluationFunction();

  const std::vector<int>& edgeWeights = graph_->getEdgeWeight();
  const std::vector<int>& vertexWeights = graph_->getVertexWeight();
  const std::vector<int>& colIdx = graph_->getColIdx();
  const std::vector<int>& rowPtr = graph_->getRowPtr();
  const int numVertices = vertexWeights.size();
  const int numVerticesTotal = vertexWeights.size();

  const int architecture = options_.getArchTopology().size();
  int architectureDims = 1;
  int* mesh_dims = (int*) malloc((unsigned) 3 * sizeof(int));
  if (architecture > 0) {
    std::vector<int> archTopology = options_.getArchTopology();
    for (int i = 0; ((i < architecture) && (i < 3)); i++) {
      mesh_dims[i] = archTopology[i];
      architectureDims = architectureDims * archTopology[i];
    }
  }

  int hypercubeDims
      = (int) (std::log2(((double) (options_.getTargetPartitions()))));

  const int numVertCoar = options_.getCoarVertices();

  const int refinement = options_.getRefinement();

  const int termPropagation = options_.getTermProp() ? 1 : 0;

  const double inbalance = (double) options_.getBalanceConstraint() / 100;

  const double coarRatio = options_.getCoarRatio();

  const double cutCost = options_.getCutHopRatio();

  const int level = options_.getLevel();

  int partitioningMethod = 1;  // Multi-level KL

  int kWay = 1;  // recursive 2-way

  for (long seed : options_.getSeeds()) {
    const auto start = std::chrono::system_clock::now();

    int* starts = (int*) malloc((unsigned) (numVertices + 1) * sizeof(int));
    int* currentIndex = starts;
    for (int pointer : rowPtr) {
      *currentIndex = (pointer);
      currentIndex++;
    }
    *currentIndex = colIdx.size();  // Needed so Chaco can find the end of the
                                    // interval of the last vertex

    int* vweights = (int*) malloc((unsigned) numVertices * sizeof(int));
    currentIndex = vweights;
    for (int weigth : vertexWeights) {
      *currentIndex = weigth;
      currentIndex++;
    }

    int* adjacency = (int*) malloc((unsigned) colIdx.size() * sizeof(int));
    currentIndex = adjacency;
    for (int pointer : colIdx) {
      *currentIndex = (pointer + 1);
      currentIndex++;
    }

    float* eweights = (float*) malloc((unsigned) colIdx.size() * sizeof(float));
    float* currentIndexFloat = eweights;
    for (int weigth : edgeWeights) {
      *currentIndexFloat = weigth;
      currentIndexFloat++;
    }

    short* assigment
        = (short*) malloc((unsigned) numVerticesTotal * sizeof(short));

    int oldTargetPartitions = 0;

    if (options_.getExistingID() > -1) {
      // If a previous solution ID already exists.
      const PartSolutions existingResult = results_[options_.getExistingID()];
      const unsigned existingBestIdx = existingResult.getBestSolutionIdx();
      const std::vector<unsigned long>& vertexResult
          = existingResult.getAssignment(existingBestIdx);
      // Gets the vertex assignments results from the last ID.
      short* currentIndexShort = assigment;
      for (unsigned long existingPartId : vertexResult) {
        // Apply the Partition IDs to the current assignment vector.
        if (existingPartId > oldTargetPartitions) {
          oldTargetPartitions = existingPartId;
        }
        *currentIndexShort = existingPartId;
        currentIndexShort++;
      }

      partitioningMethod = 7;
      kWay = hypercubeDims;
      oldTargetPartitions++;

      if (architecture) {
        hypercubeDims = (int) (std::log2(((double) (architectureDims))));
        kWay = hypercubeDims;
        if (kWay > 3 || architectureDims < oldTargetPartitions
            || architectureDims % 2 == 1) {
          logger_->error(PAR,
                         1,
                         "Graph has too many sets (>8), the number of target "
                         "partitions changed or the architecture is invalid.");
        }
      } else {
        if (kWay > 3 || options_.getTargetPartitions() < oldTargetPartitions) {
          logger_->error(PAR,
                         2,
                         "Graph has too many sets (>8) or the number of target "
                         "partitions changed.");
        }
      }
    }

    interface_wrap(
        numVertices, /* number of vertices */
        starts,
        adjacency,
        vweights,
        eweights, /* graph definition for chaco */
        NULL,
        NULL,
        NULL, /* x y z positions for the inertial method, not needed for
                 multi-level KL */
        NULL,
        NULL, /* output assigment name and file, isn't needed because internal
                 methods of PartitionMgr are used instead */
        assigment, /* vertex assigment vector. Contains the set that each vector
                      is present on.*/
        architecture,
        hypercubeDims,
        mesh_dims, /* architecture, architecture topology and the hypercube
                      dimensions (number of 2-way divisions) */
        NULL, /* desired set sizes for each set, computed automatically, so it
                 isn't needed */
        partitioningMethod,
        1, /* constants that define the methods used by the partitioner ->
              multi-level KL, KL refinement */
        0,
        numVertCoar,
        kWay, /* disables the eigensolver, number of vertices to coarsen down to
                 and bisection/quadrisection/octasection */
        0.001,
        seed, /* tolerance on eigenvectors (hard-coded, not used) and the seed
               */
        termPropagation,
        inbalance, /* terminal propagation enable and inbalance */
        coarRatio,
        cutCost, /* coarsening ratio and cut to hop cost */
        0,
        refinement,
        level); /* debug text enable, refinement and clustering level to
                   export*/

    std::vector<unsigned long> chacoResult;

    for (int i = 0; i < numVertices; i++) {
      const short* currentpointer = assigment + i;
      chacoResult.push_back(*currentpointer);
    }

    const auto end = std::chrono::system_clock::now();
    const unsigned long runtime
        = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();

    currentResults.addAssignment(chacoResult, runtime, seed);
    free(assigment);

    logger_->info(PAR,
                  4,
                  "[Chaco] Partitioned graph for seed {} in {} ms.",
                  seed,
                  runtime);
  }

  results_.push_back(currentResults);
  free(mesh_dims);

  logger_->info(PAR,
                5,
                "[Chaco] Run completed. Partition ID = {}. Total runs = {}.",
                partitionId,
                options_.getSeeds().size());
#endif
}

void PartitionMgr::runGpMetis()
{
#ifdef PARTITIONERS
  logger_->report("Running GPMetis.");
  PartSolutions currentResults;
  currentResults.setToolName(options_.getTool());
  const unsigned partitionId = generatePartitionId();
  currentResults.setPartitionId(partitionId);
  const std::string evaluationFunction = options_.getEvaluationFunction();

  idx_t edgeCut;
  idx_t nPartitions = options_.getTargetPartitions();
  int numVertices = graph_->getNumVertex();
  const int numEdges = graph_->getNumEdges();
  idx_t constraints = 1;
  idx_t options[METIS_NOPTIONS];
  METIS_SetDefaultOptions(options);

  options[METIS_OPTION_PTYPE] = METIS_PTYPE_RB;
  options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
  options[METIS_OPTION_NUMBERING] = 0;
  options[METIS_OPTION_UFACTOR] = options_.getBalanceConstraint() * 10;

  idx_t* vertexWeights
      = (idx_t*) malloc((unsigned) numVertices * sizeof(idx_t));
  idx_t* rowPtr = (idx_t*) malloc((unsigned) (numVertices + 1) * sizeof(idx_t));
  idx_t* colIdx = (idx_t*) malloc((unsigned) numEdges * sizeof(idx_t));
  idx_t* edgeWeights = (idx_t*) malloc((unsigned) numEdges * sizeof(idx_t));
  for (int i = 0; i < numVertices; i++) {
    vertexWeights[i] = graph_->getVertexWeight(i);
    edgeWeights[i] = graph_->getEdgeWeight(i);
    rowPtr[i] = graph_->getRowPtr(i);
    colIdx[i] = graph_->getColIdx(i);
  }
  rowPtr[numVertices] = numEdges;

  for (int i = numVertices; i < numEdges; i++) {
    edgeWeights[i] = graph_->getEdgeWeight(i);
    colIdx[i] = graph_->getColIdx(i);
  }
  for (int seed : options_.getSeeds()) {
    const auto start = std::chrono::system_clock::now();
    options[METIS_OPTION_SEED] = seed;
    idx_t* parts = (idx_t*) malloc((unsigned) numVertices * sizeof(idx_t));

    METIS_PartGraphRecursive(&numVertices,
                             &constraints,
                             rowPtr,
                             colIdx,
                             vertexWeights,
                             NULL,
                             edgeWeights,
                             &nPartitions,
                             NULL,
                             NULL,
                             options,
                             &edgeCut,
                             parts);

    std::vector<unsigned long> gpmetisResults;
    for (int i = 0; i < numVertices; i++)
      gpmetisResults.push_back(parts[i]);
    const auto end = std::chrono::system_clock::now();
    const unsigned long runtime
        = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();

    currentResults.addAssignment(gpmetisResults, runtime, seed);
    free(parts);

    logger_->info(PAR,
                  56,
                  "[GPMetis] Partitioned graph for seed {} in {} ms.",
                  seed,
                  runtime);
  }
  free(vertexWeights);
  free(rowPtr);
  free(colIdx);
  free(edgeWeights);

  results_.push_back(currentResults);

  logger_->info(PAR,
                57,
                "[GPMetis] Run completed. Partition ID = {}. Total runs = {}.",
                partitionId,
                options_.getSeeds().size());
#endif
}

void PartitionMgr::runMlPart()
{
  logger_->report("Running MLPart.");
  HypergraphDecomposition hypergraphDecomp;
  hypergraphDecomp.init(getDbBlock(), logger_);
  Hypergraph hypergraph;
  if (options_.getForceGraph()) {
    hypergraphDecomp.toHypergraph(hypergraph, graph_.get());
  } else {
    hypergraph = *hypergraph_;
  }

  PartSolutions currentResults;
  currentResults.setToolName(options_.getTool());
  const unsigned partitionId = generatePartitionId();
  currentResults.setPartitionId(partitionId);
  const std::string evaluationFunction = options_.getEvaluationFunction();

  PartSolutions bestResult;
  bestResult.setToolName(options_.getTool());
  bestResult.setPartitionId(partitionId);

  const int numOriginalVertices = hypergraph.getNumVertex();
  std::vector<unsigned long> clusters(numOriginalVertices, 0);

  const double tolerance = options_.getBalanceConstraint() / 100.0;
  double balanceArray[2] = {0.5, 0.5};

  for (long seed : options_.getSeeds()) {
    std::vector<short> partitions;
    int countPartitions = 0;
    partitions.push_back(0);

    const auto start = std::chrono::system_clock::now();
    while (partitions.size() < options_.getTargetPartitions()) {
      std::vector<short> auxPartitions;
      for (const int p : partitions) {
        Hypergraph newHypergraph;
        countPartitions++;
        hypergraphDecomp.updateHypergraph(
            hypergraph, newHypergraph, clusters, p);
        const int numEdges = newHypergraph.getNumEdges();
        const int numColIdx = newHypergraph.getNumColIdx();
        const int numVertices = newHypergraph.getNumVertex();

        double* vertexWeights
            = (double*) malloc((unsigned) numVertices * sizeof(double));
        int* rowPtr = (int*) malloc((unsigned) (numEdges + 1) * sizeof(int));
        int* colIdx = (int*) malloc((unsigned) numColIdx * sizeof(int));
        double* edgeWeights
            = (double*) malloc((unsigned) numEdges * sizeof(double));
        int* part = (int*) malloc((unsigned) numVertices * sizeof(int));

        for (int j = 0; j < numVertices; j++)
          part[j] = -1;

        for (int i = 0; i < numVertices; i++) {
          vertexWeights[i] = newHypergraph.getVertexWeight(i)
                             / options_.getMaxVertexWeight();
        }
        for (int i = 0; i < numColIdx; i++) {
          colIdx[i] = newHypergraph.getColIdx(i);
        }
        for (int i = 0; i < numEdges; i++) {
          rowPtr[i] = newHypergraph.getRowPtr(i);
          edgeWeights[i] = newHypergraph.getEdgeWeight(i);
        }
        rowPtr[numEdges] = newHypergraph.getRowPtr(numEdges);

        UMpack_mlpart(numVertices,
                      numEdges,
                      vertexWeights,
                      rowPtr,
                      colIdx,
                      edgeWeights,
                      2,  // Number of Partitions
                      balanceArray,
                      tolerance,
                      part,
                      1,  // Starts Per Run #TODO: add a tcl command
                      1,  // Number of Runs
                      0,  // Debug Level
                      seed);
        for (int i = 0; i < numOriginalVertices; i++) {
          if (clusters[i] == p) {
            if (part[newHypergraph.getClusterMapping(i)] == 0)
              clusters[i] = p;
            else
              clusters[i] = countPartitions;
          }
        }
        free(vertexWeights);
        free(rowPtr);
        free(colIdx);
        free(edgeWeights);
        free(part);

        auxPartitions.push_back(countPartitions);
      }

      partitions.insert(
          partitions.end(), auxPartitions.begin(), auxPartitions.end());
    }
    const auto end = std::chrono::system_clock::now();
    const unsigned long runtime
        = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();

    currentResults.addAssignment(clusters, runtime, seed);
    logger_->info(PAR, 59, "[MLPart] Partitioned graph for seed {}.", seed);

    std::fill(clusters.begin(), clusters.end(), 0);
  }

  results_.push_back(currentResults);

  logger_->info(PAR,
                60,
                "[MLPart] Run completed. Partition ID = {}. Total runs = {}.",
                partitionId,
                options_.getSeeds().size());
}

void PartitionMgr::toHypergraph()
{
  HypergraphDecomposition hypergraphDecomp;
  hypergraphDecomp.init(getDbBlock(), logger_);
  Hypergraph hype;
  hypergraphDecomp.toHypergraph(hype, graph_.get());
}

void PartitionMgr::hypergraph(bool buildGraph)
{
  hypergraph_->clearHypergraph();

  HypergraphDecomposition hypergraphDecomp;
  hypergraphDecomp.init(getDbBlock(), logger_);
  hypergraphDecomp.constructMap(*hypergraph_, options_.getMaxVertexWeight());

  const int numVertices = hypergraph_->getNumVertex();
  std::vector<unsigned long> clusters(numVertices, 0);
  hypergraphDecomp.createHypergraph(*hypergraph_, clusters, 0);

  if (buildGraph)
    toGraph();
}

void PartitionMgr::toGraph()
{
  HypergraphDecomposition hypergraphDecomp;
  hypergraphDecomp.init(getDbBlock(), logger_);
  graph_->clearGraph();
  hypergraphDecomp.toGraph(*hypergraph_,
                           *graph_,
                           options_.getGraphModel(),
                           options_.getWeightModel(),
                           options_.getMaxEdgeWeight(),
                           options_.getCliqueThreshold());
}

unsigned PartitionMgr::generatePartitionId() const
{
  return results_.size();
}

// Evaluate Partitioning

void PartitionMgr::evaluatePartitioning()
{
  const std::vector<int> partVector = options_.getPartitionsToTest();
  const std::string evaluationFunction = options_.getEvaluationFunction();
  // Checks if IDs are valid
  for (const int partId : partVector) {
    if (partId >= results_.size() || partId < 0) {
      logger_->error(PAR, 100, "Invalid partitioning id: {}", partId);
    }
  }
  for (const int partId : partVector) {
    computePartitionResult(partId, evaluationFunction);
  }
  int bestId = -1;
  for (const int partId : partVector) {
    // Compares the results for the current ID with the best one (if it exists).
    if (bestId == -1) {
      bestId = partId;
    } else {
      // If the new ID presents better results than the last one, update the
      // bestId.
      const bool isNewIdBetter
          = comparePartitionings(getPartitioningResult(bestId),
                                 getPartitioningResult(partId),
                                 evaluationFunction);
      if (isNewIdBetter) {
        bestId = partId;
      }
    }
  }

  if (bestId == -1) {
    logger_->error(PAR, 103, "No partitioning result found.");
  }

  reportPartitionResult(bestId);
  setCurrentBestId(bestId);
}

void PartitionMgr::computePartitionResult(unsigned partitionId,
                                          std::string function)
{
  const int weightModel = options_.getWeightModel();
  const std::vector<float>& edgeWeight = graph_->getDefaultEdgeWeight();
  const int maxEdgeWeight = options_.getMaxEdgeWeight();

  const float maxEWeight
      = *std::max_element(edgeWeight.begin(), edgeWeight.end());
  const float minEWeight
      = *std::min_element(edgeWeight.begin(), edgeWeight.end());

  PartSolutions& currentResults = results_[partitionId];
  currentResults.resetEvaluation();
  for (unsigned idx = 0; idx < currentResults.getNumOfRuns(); idx++) {
    const std::vector<unsigned long>& currentAssignment
        = currentResults.getAssignment(idx);
    const unsigned long currentRuntime = currentResults.getRuntime(idx);

    unsigned long terminalCounter = 0;
    unsigned long cutCounter = 0;
    unsigned long edgeTotalWeigth = 0;
    std::vector<unsigned long> setSize(options_.getTargetPartitions(), 0);
    std::vector<unsigned long> setArea(options_.getTargetPartitions(), 0);

    const std::vector<int>& hyperedgesEnd = hypergraph_->getRowPtr();
    const std::vector<int>& hyperedgeNets = hypergraph_->getColIdx();
    std::set<unsigned> computedVertices;
    int startIndex = 0;
    // Iterate over each net in the hypergraph.
    for (const int endIndex : hyperedgesEnd) {
      // Contains the partitions the net is part of.
      std::set<unsigned long> netPartitions;
      // Contains the vertices that are in the net.
      std::vector<unsigned> netVertices;
      if (endIndex != 0) {
        for (int currentIndex = startIndex; currentIndex < endIndex;
             currentIndex++) {  // Iterate over all vertices in the net.
          const int currentVertex = hyperedgeNets[currentIndex];
          const unsigned long currentPartition
              = currentAssignment[currentVertex];
          netPartitions.insert(currentPartition);
          netVertices.push_back(currentVertex);
          // Update the partition size and area if needed.
          if (computedVertices.find(currentVertex) == computedVertices.end()) {
            setSize[currentPartition]++;
            setArea[currentPartition] += graph_->getVertexWeight(currentVertex);
            computedVertices.insert(currentVertex);
          }
        }
      }
      if (netPartitions.size() > 1) {  // Net was cut.
        cutCounter++;  // If the net was cut, a hyperedge cut happened.
        terminalCounter
            += netPartitions.size();  // The number of different partitions
                                      // present in the net is the number of
                                      // terminals. (Pathways to another set.)
        // Computations for hop weight:
        float currentNetWeight = 0;
        const int netSize = netVertices.size();
        switch (weightModel) {
          case 1:
            currentNetWeight = 1.0 / (netSize - 1);
            break;
          case 2:
            currentNetWeight = 4.0 / (netSize * (netSize - 1));
            break;
          case 3:
            currentNetWeight = 4.0 / (netSize * netSize - (netSize % 2));
            break;
          case 4:
            currentNetWeight = 6.0 / (netSize * (netSize + 1));
            break;
          case 5:
            currentNetWeight = pow((2.0 / netSize), 1.5);
            break;
          case 6:
            currentNetWeight = pow((2.0 / netSize), 3);
            break;
          case 7:
            currentNetWeight = 2.0 / netSize;
            break;
        }
        int auxWeight;

        currentNetWeight = std::min(currentNetWeight, maxEWeight);
        if (minEWeight == maxEWeight) {
          auxWeight = maxEdgeWeight;
        } else {
          auxWeight
              = (int) ((((currentNetWeight - minEWeight) * (maxEdgeWeight - 1))
                        / (maxEWeight - minEWeight))
                       + 1);
        }

        edgeTotalWeigth = edgeTotalWeigth + auxWeight;
      }
      startIndex = endIndex;
    }

    // Computation for the standard deviation of set size.
    double currentSum = 0;
    for (unsigned long clusterSize : setSize) {
      currentSum += clusterSize;
    }
    double currentMean = currentSum / setSize.size();
    double sizeSD = 0;
    for (unsigned long clusterSize : setSize) {
      sizeSD += std::pow(clusterSize - currentMean, 2);
    }
    sizeSD = std::sqrt(sizeSD / setSize.size());

    // Computation for the standard deviation of set area.
    currentSum = 0;
    for (unsigned long clusterArea : setArea) {
      currentSum += clusterArea;
    }
    currentMean = currentSum / setArea.size();
    double areaSD = 0;
    for (unsigned long clusterArea : setArea) {
      areaSD += std::pow(clusterArea - currentMean, 2);
    }
    areaSD = std::sqrt(areaSD / setArea.size());

    // Check if the current assignment is better than the last one. "terminals
    // hyperedges size area runtime hops"
    bool isBetter = false;
    if (function == "hyperedges") {
      isBetter = ((cutCounter < currentResults.getBestNumHyperedgeCuts())
                  || (currentResults.getBestNumHyperedgeCuts() == 0));
    } else if (function == "terminals") {
      isBetter = ((terminalCounter < currentResults.getBestNumTerminals())
                  || (currentResults.getBestNumTerminals() == 0));
    } else if (function == "size") {
      isBetter = ((sizeSD < currentResults.getBestSetSize())
                  || (currentResults.getBestSetSize() == 0));
    } else if (function == "area") {
      isBetter = ((areaSD < currentResults.getBestSetArea())
                  || (currentResults.getBestSetArea() == 0));
    } else if (function == "hops") {
      isBetter = ((edgeTotalWeigth < currentResults.getBestHopWeigth())
                  || (currentResults.getBestHopWeigth() == 0));
    } else {
      isBetter = ((currentRuntime < currentResults.getBestRuntime())
                  || (currentResults.getBestRuntime() == 0));
    }
    if (isBetter) {
      currentResults.setBestSolutionIdx(idx);
      currentResults.setBestRuntime(currentRuntime);
      currentResults.setBestNumHyperedgeCuts(cutCounter);
      currentResults.setBestNumTerminals(terminalCounter);
      currentResults.setBestHopWeigth(edgeTotalWeigth);
      currentResults.setBestSetSize(sizeSD);
      currentResults.setBestSetArea(areaSD);
    }
  }
}

bool PartitionMgr::comparePartitionings(const PartSolutions& oldPartition,
                                        const PartSolutions& newPartition,
                                        const std::string& function)
{
  bool isBetter = false;
  if (function == "hyperedges") {
    isBetter = newPartition.getBestNumHyperedgeCuts()
               < oldPartition.getBestNumHyperedgeCuts();
  } else if (function == "terminals") {
    isBetter = newPartition.getBestNumTerminals()
               < oldPartition.getBestNumTerminals();
  } else if (function == "size") {
    isBetter = newPartition.getBestSetSize() < oldPartition.getBestSetSize();
  } else if (function == "area") {
    isBetter = newPartition.getBestSetArea() < oldPartition.getBestSetArea();
  } else if (function == "hops") {
    isBetter
        = newPartition.getBestHopWeigth() < oldPartition.getBestHopWeigth();
  } else {
    isBetter = newPartition.getBestRuntime() < oldPartition.getBestRuntime();
  }
  return isBetter;
}

void PartitionMgr::reportPartitionResult(const unsigned partitionId)
{
  PartSolutions currentResults = results_[partitionId];
  logger_->info(PAR,
                6,
                "Partitioning Results for ID = {} and Tool = {}.",
                partitionId,
                currentResults.getToolName());
  const unsigned bestIdx = currentResults.getBestSolutionIdx();
  const int seed = currentResults.getSeed(bestIdx);
  logger_->info(PAR, 7, "Best results used seed {}.", seed);
  logger_->info(PAR,
                8,
                "Number of Hyperedge Cuts = {}.",
                currentResults.getBestNumHyperedgeCuts());
  logger_->info(PAR,
                9,
                "Number of Terminals = {}.",
                currentResults.getBestNumTerminals());
  logger_->info(
      PAR, 10, "Cluster Size SD = {}.", currentResults.getBestSetSize());
  logger_->info(
      PAR, 11, "Cluster Area SD = {}.", currentResults.getBestSetArea());
  logger_->info(
      PAR, 12, "Total Hop Weight = {}.", currentResults.getBestHopWeigth());
}

// Write Partitioning To DB

odb::dbBlock* PartitionMgr::getDbBlock() const
{
  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip->getBlock();
  return block;
}

void PartitionMgr::writePartitioningToDb(const unsigned partitioningId)
{
  logger_->report("Writing partition id's to DB.");
  if (partitioningId >= getNumPartitioningResults()) {
    logger_->error(PAR, 14, "Partition id out of range ({}).", partitioningId);
  }

  const PartSolutions& results = getPartitioningResult(partitioningId);
  const unsigned bestSolutionIdx = results.getBestSolutionIdx();
  const std::vector<unsigned long>& result
      = results.getAssignment(bestSolutionIdx);

  odb::dbBlock* block = getDbBlock();
  for (odb::dbInst* inst : block->getInsts()) {
    const std::string instName = inst->getName();
    const int instIdx = hypergraph_->getMapping(instName);
    const unsigned long partitionId = result[instIdx];

    odb::dbIntProperty* propId = odb::dbIntProperty::find(inst, "partition_id");
    if (!propId) {
      propId = odb::dbIntProperty::create(inst, "partition_id", partitionId);
    } else {
      propId->setValue(partitionId);
    }
  }
  logger_->report("Writing done.");
}

void PartitionMgr::dumpPartIdToFile(std::string name)
{
  std::ofstream file(name);

  odb::dbBlock* block = getDbBlock();
  for (odb::dbInst* inst : block->getInsts()) {
    const std::string instName = inst->getName();
    odb::dbIntProperty* propId = odb::dbIntProperty::find(inst, "partition_id");
    if (!propId) {
      logger_->warn(
          PAR, 101, "Property 'partition_id' not found for inst {}.", instName);
      continue;
    }
    file << instName << " " << propId->getValue() << "\n";
  }

  file.close();
}

// Cluster Netlist

void PartitionMgr::runClustering()
{
  hypergraph(true);
  if (options_.getTool() == "mlpart") {
    runMlPartClustering();
  } else if (options_.getTool() == "gpmetis") {
    runGpMetisClustering();
  } else {
    runChacoClustering();
  }
}

void PartitionMgr::runChacoClustering()
{
#ifdef PARTITIONERS
  logger_->report("Running Chaco.");

  PartSolutions currentResults;
  currentResults.setToolName(options_.getTool());
  unsigned clusterId = generateClusterId();
  currentResults.setPartitionId(clusterId);
  const std::string evaluationFunction = options_.getEvaluationFunction();

  const std::vector<int>& edgeWeights = graph_->getEdgeWeight();
  const std::vector<int>& vertexWeights = graph_->getVertexWeight();
  const std::vector<int>& colIdx = graph_->getColIdx();
  const std::vector<int>& rowPtr = graph_->getRowPtr();
  const int numVertices = vertexWeights.size();

  const int architecture = options_.getArchTopology().size();
  int* mesh_dims = (int*) malloc((unsigned) 3 * sizeof(int));

  const int numVertCoar = options_.getCoarVertices();

  const int refinement = options_.getRefinement();

  const double inbalance = (double) options_.getBalanceConstraint() / 100;

  const double coarRatio = options_.getCoarRatio();

  const double cutCost = options_.getCutHopRatio();

  const int level = options_.getLevel();

  const auto start = std::chrono::system_clock::now();

  int* starts = (int*) malloc((unsigned) (numVertices + 1) * sizeof(int));
  int* currentIndex = starts;
  for (const int pointer : rowPtr) {
    *currentIndex = (pointer);
    currentIndex++;
  }
  *currentIndex = colIdx.size();  // Needed so Chaco can find the end of the
                                  // interval of the last vertex

  int* vweights = (int*) malloc((unsigned) numVertices * sizeof(int));
  currentIndex = vweights;
  for (const int weigth : vertexWeights) {
    *currentIndex = weigth;
    currentIndex++;
  }

  int* adjacency = (int*) malloc((unsigned) colIdx.size() * sizeof(int));
  currentIndex = adjacency;
  for (const int pointer : colIdx) {
    *currentIndex = (pointer + 1);
    currentIndex++;
  }

  float* eweights = (float*) malloc((unsigned) colIdx.size() * sizeof(float));
  float* currentIndexFloat = eweights;
  for (const int weigth : edgeWeights) {
    *currentIndexFloat = weigth;
    currentIndexFloat++;
  }

  short* assigment = (short*) malloc((unsigned) numVertices * sizeof(short));

  interface_wrap(
      numVertices, /* number of vertices */
      starts,
      adjacency,
      vweights,
      eweights, /* graph definition for chaco */
      NULL,
      NULL,
      NULL, /* x y z positions for the inertial method, not needed for
               multi-level KL */
      NULL,
      NULL, /* output assigment name and file, isn't needed because internal
               methods of PartitionMgr are used instead */
      assigment, /* vertex assigment vector. Contains the set that each vector
                    is present on.*/
      architecture,
      1,
      mesh_dims, /* architecture, architecture topology and the hypercube
                    dimensions (number of 2-way divisions) */
      NULL, /* desired set sizes for each set, computed automatically, so it
               isn't needed */
      1,
      1, /* constants that define the methods used by the partitioner ->
            multi-level KL, KL refinement */
      0,
      numVertCoar,
      1, /* disables the eigensolver, number of vertices to coarsen down to and
            bisection/quadrisection/octasection */
      0.001,
      0, /* tolerance on eigenvectors (hard-coded, not used) and the seed */
      0,
      inbalance, /* terminal propagation enable and inbalance */
      coarRatio,
      cutCost, /* coarsening ratio and cut to hop cost */
      0,
      refinement,
      level); /* debug text enable, refinement and clustering level to export*/

  std::vector<unsigned long> chacoResult;

  int* clusteringResults = clustering_wrap();
  for (int i = 0; i < numVertices; i++) {
    int* currentpointer = (clusteringResults + 1) + i;
    chacoResult.push_back(*currentpointer);
  }
  free(clusteringResults);

  const auto end = std::chrono::system_clock::now();
  const unsigned long runtime
      = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();

  currentResults.addAssignment(chacoResult, runtime, 0);
  free(assigment);
  logger_->info(PAR, 16, "[Chaco] Clustered graph in {} ms.", runtime);

  clusResults_.push_back(currentResults);

  free(mesh_dims);

  logger_->info(PAR, 17, "[Chaco] Run completed. Cluster ID = {}.", clusterId);
#endif
}

void PartitionMgr::runGpMetisClustering()
{
#ifdef PARTITIONERS
  logger_->report("Running GPMetis.");
  PartSolutions currentResults;
  currentResults.setToolName(options_.getTool());
  const unsigned clusterId = generateClusterId();
  currentResults.setPartitionId(clusterId);
  const std::string evaluationFunction = options_.getEvaluationFunction();

  idx_t edgeCut;
  idx_t nPartitions = options_.getTargetPartitions();
  int numVertices = graph_->getNumVertex();
  const int numEdges = graph_->getNumEdges();
  idx_t constraints = 1;
  idx_t options[METIS_NOPTIONS];
  METIS_SetDefaultOptions(options);
  idx_t level = options_.getLevel();

  options[METIS_OPTION_PTYPE] = METIS_PTYPE_RB;
  options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
  options[METIS_OPTION_NUMBERING] = 0;
  options[METIS_OPTION_UFACTOR] = options_.getBalanceConstraint() * 10;

  idx_t* vertexWeights
      = (idx_t*) malloc((unsigned) numVertices * sizeof(idx_t));
  idx_t* rowPtr = (idx_t*) malloc((unsigned) (numVertices + 1) * sizeof(idx_t));
  idx_t* colIdx = (idx_t*) malloc((unsigned) numEdges * sizeof(idx_t));
  idx_t* edgeWeights = (idx_t*) malloc((unsigned) numEdges * sizeof(idx_t));
  for (int i = 0; i < numVertices; i++) {
    vertexWeights[i] = graph_->getVertexWeight(i);
    edgeWeights[i] = graph_->getEdgeWeight(i);
    rowPtr[i] = graph_->getRowPtr(i);
    colIdx[i] = graph_->getColIdx(i);
  }
  rowPtr[numVertices] = numEdges;

  for (int i = numVertices; i < numEdges; i++) {
    edgeWeights[i] = graph_->getEdgeWeight(i);
    colIdx[i] = graph_->getColIdx(i);
  }
  const auto start = std::chrono::system_clock::now();
  idx_t* parts = (idx_t*) malloc((unsigned) numVertices * sizeof(idx_t));

  METIS_CoarsenGraph(&numVertices,
                     &constraints,
                     rowPtr,
                     colIdx,
                     vertexWeights,
                     NULL,
                     edgeWeights,
                     &nPartitions,
                     NULL,
                     NULL,
                     options,
                     &edgeCut,
                     parts,
                     &level);

  std::vector<unsigned long> gpmetisResults;
  for (int i = 0; i < numVertices; i++)
    gpmetisResults.push_back(parts[i]);
  const auto end = std::chrono::system_clock::now();
  const unsigned long runtime
      = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();

  logger_->info(PAR, 61, "[GPMetis] Clustered graph in {} ms.", runtime);
  currentResults.addAssignment(gpmetisResults, runtime, 0);
  clusResults_.push_back(currentResults);
  free(parts);
  free(vertexWeights);
  free(rowPtr);
  free(colIdx);
  free(edgeWeights);

  logger_->info(
      PAR, 62, "[GPMetis] Run completed. Cluster ID = {}.", clusterId);
#endif
}

void PartitionMgr::runMlPartClustering()
{
  logger_->report("Running MLPart.");
  HypergraphDecomposition hypergraphDecomp;
  hypergraphDecomp.init(getDbBlock(), logger_);
  Hypergraph hypergraph;
  if (options_.getForceGraph()) {
    hypergraphDecomp.toHypergraph(hypergraph, graph_.get());
  } else {
    hypergraph = *hypergraph_;
  }

  PartSolutions currentResults;
  currentResults.setToolName(options_.getTool());
  const unsigned clusterId = generateClusterId();
  currentResults.setPartitionId(clusterId);
  const std::string evaluationFunction = options_.getEvaluationFunction();
  const unsigned level = options_.getLevel();

  std::vector<unsigned long> clusters;

  const double tolerance = options_.getBalanceConstraint() / 100.0;
  double balanceArray[2] = {0.5, 0.5};

  const auto start = std::chrono::system_clock::now();
  const int numEdges = hypergraph.getNumEdges();
  const int numColIdx = hypergraph.getNumColIdx();
  const int numVertices = hypergraph.getNumVertex();

  double* vertexWeights
      = (double*) malloc((unsigned) numVertices * sizeof(double));
  int* rowPtr = (int*) malloc((unsigned) (numEdges + 1) * sizeof(int));
  int* colIdx = (int*) malloc((unsigned) numColIdx * sizeof(int));
  double* edgeWeights = (double*) malloc((unsigned) numEdges * sizeof(double));
  int* part = (int*) malloc((unsigned) numVertices * sizeof(int));

  for (int j = 0; j < numVertices; j++)
    part[j] = -1;

  for (int i = 0; i < numVertices; i++) {
    vertexWeights[i]
        = hypergraph.getVertexWeight(i) / options_.getMaxVertexWeight();
  }
  for (int i = 0; i < numColIdx; i++) {
    colIdx[i] = hypergraph.getColIdx(i);
  }
  for (int i = 0; i < numEdges; i++) {
    rowPtr[i] = hypergraph.getRowPtr(i);
    edgeWeights[i] = hypergraph.getEdgeWeight(i);
  }
  rowPtr[numEdges] = hypergraph.getRowPtr(numEdges);

  UMpack_mlpart(numVertices,
                numEdges,
                vertexWeights,
                rowPtr,
                colIdx,
                edgeWeights,
                2,  // Number of Partitions
                balanceArray,
                tolerance,
                part,
                1,  // Starts Per Run #TODO: add a tcl command
                1,  // Number of Runs
                0,  // Debug Level
                123,
                level);
  for (int i = 0; i < numVertices; i++) {
    clusters.push_back(part[i]);
  }

  const auto end = std::chrono::system_clock::now();
  const unsigned long runtime
      = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
  free(vertexWeights);
  free(rowPtr);
  free(colIdx);
  free(edgeWeights);
  free(part);

  currentResults.addAssignment(clusters, runtime, 0);

  clusResults_.push_back(currentResults);

  logger_->info(PAR, 64, "[MLPart] Run completed. Cluster ID = {}.", clusterId);
}

unsigned PartitionMgr::generateClusterId() const
{
  return clusResults_.size();
}

// Write Clustering To DB

void PartitionMgr::writeClusteringToDb(unsigned clusteringId)
{
  logger_->report("Writing cluster id's to DB.");
  if (clusteringId >= getNumClusteringResults()) {
    logger_->error(PAR, 18, "Cluster id out of range ({}).", clusteringId);
  }

  const PartSolutions& results = getClusteringResult(clusteringId);
  const std::vector<unsigned long>& result
      = results.getAssignment(0);  // Clustering uses only 1 seed

  odb::dbBlock* block = getDbBlock();
  for (odb::dbInst* inst : block->getInsts()) {
    const std::string instName = inst->getName();
    const int instIdx = hypergraph_->getMapping(instName);
    const unsigned long clusterId = result[instIdx];

    odb::dbIntProperty* propId = odb::dbIntProperty::find(inst, "cluster_id");
    if (!propId) {
      propId = odb::dbIntProperty::create(inst, "cluster_id", clusterId);
    } else {
      propId->setValue(clusterId);
    }
  }

  logger_->report("Writing done.");
}

void PartitionMgr::dumpClusIdToFile(std::string name) const
{
  std::ofstream file(name);

  odb::dbBlock* block = getDbBlock();
  for (odb::dbInst* inst : block->getInsts()) {
    const std::string instName = inst->getName();
    odb::dbIntProperty* propId = odb::dbIntProperty::find(inst, "cluster_id");
    if (!propId) {
      logger_->warn(
          PAR, 65, "Property 'cluster_id' not found for inst {}.", instName);
      continue;
    }
    file << instName << " " << propId->getValue() << "\n";
  }

  file.close();
}

// Report Netlist Partitions

void PartitionMgr::reportNetlistPartitions(unsigned partitionId)
{
  std::map<unsigned long, unsigned long> setSizes;
  std::set<unsigned long> partitions;
  unsigned long numberOfPartitions = 0;
  const PartSolutions& results = getPartitioningResult(partitionId);
  const unsigned bestSolutionIdx = results.getBestSolutionIdx();
  const std::vector<unsigned long>& result
      = results.getAssignment(bestSolutionIdx);
  for (unsigned long currentPartition : result) {
    if (currentPartition > numberOfPartitions) {
      numberOfPartitions = currentPartition;
    }
    if (setSizes.find(currentPartition) == setSizes.end()) {
      setSizes[currentPartition] = 1;
    } else {
      setSizes[currentPartition]++;
    }
    partitions.insert(currentPartition);
  }
  logger_->info(
      PAR, 19, "The netlist has {} partitions.", (numberOfPartitions + 1));
  unsigned long totalVertices = 0;
  for (const unsigned long partIdx : partitions) {
    const unsigned long partSize = setSizes[partIdx];
    logger_->info(PAR, 20, "Partition {} has {} vertices.", partIdx, partSize);
    totalVertices += partSize;
  }
  logger_->info(PAR, 21, "The total number of vertices is {}.", totalVertices);
}

// Read partitioning input file

unsigned PartitionMgr::readPartitioningFile(
    const std::string& filename,
    const std::string& instance_map_file)
{
  hypergraph();
  PartSolutions currentResults;
  currentResults.setToolName(options_.getTool());
  const unsigned partitionId = generatePartitionId();
  currentResults.setPartitionId(partitionId);
  const std::string evaluationFunction = options_.getEvaluationFunction();
  options_.setTargetPartitions(options_.getFinalPartitions());

  const auto& inst_map = hypergraph_->getMap();

  // determine order the instances will be in the input file
  std::vector<std::string> instance_order;
  if (!instance_map_file.empty()) {
    std::ifstream map_file(instance_map_file);
    if (map_file.is_open()) {
      std::string line;
      while (getline(map_file, line)) {
        if (line.empty()) {
          continue;
        }
        instance_order.push_back(line);
      }
      map_file.close();
    } else {
      logger_->error(PAR, 73, "Unable to open file {}.", instance_map_file);
    }
  } else {
    instance_order.resize(inst_map.size());
    for (const auto& [inst, idx] : inst_map) {
      instance_order[idx] = inst;
    }
  }

  std::ifstream file(filename);
  std::vector<unsigned long> inst_partitions;
  if (file.is_open()) {
    std::string line;
    while (getline(file, line)) {
      if (line.empty()) {
        continue;
      }
      try {
        inst_partitions.push_back(std::stoi(line));
      } catch (const std::invalid_argument&) {
        logger_->error(
            PAR,
            71,
            "Unable to convert line \"{}\" to an integer in file: {}",
            line,
            filename);
      } catch (const std::out_of_range&) {
        logger_->error(
            PAR,
            72,
            "Unable to convert line \"{}\" to an integer in file: {}",
            line,
            filename);
      }
    }
    file.close();
  } else {
    logger_->error(PAR, 22, "Unable to open file {}.", filename);
  }

  if (inst_partitions.size() != instance_order.size()) {
    logger_->error(PAR,
                   74,
                   "Instances in partitioning ({}) does not match instances in "
                   "netlist ({}).",
                   inst_partitions.size(),
                   instance_order.size());
  }

  std::vector<unsigned long> partitions(inst_partitions.size());
  for (size_t i = 0; i < inst_partitions.size(); i++) {
    const auto& inst_name = instance_order[i];
    const auto& inst_part = inst_partitions[i];

    auto inst_find = inst_map.find(inst_name);
    if (inst_find == inst_map.end()) {
      logger_->error(PAR, 75, "Instance {} not in the netlist.", inst_name);
    }
    partitions[inst_find->second] = inst_part;
  }

  currentResults.addAssignment(partitions, 0, 1);
  results_.push_back(currentResults);

  return partitionId;
}

void PartSolutions::addAssignment(
    const std::vector<unsigned long>& currentAssignment,
    const unsigned long runtime,
    const int seed)
{
  assignmentResults_.push_back(currentAssignment);
  runtimeResults_.push_back(runtime);
  seeds_.push_back(seed);
}

void PartSolutions::clearAssignments()
{
  resetEvaluation();

  assignmentResults_.clear();
  runtimeResults_.clear();
  seeds_.clear();
}

void PartSolutions::resetEvaluation()
{
  bestSolutionIdx_ = 0;
  bestSetSizeSD_ = 0;
  bestSetAreaSD_ = 0;
  bestNumTerminals_ = 0;
  bestNumHyperedgeCuts_ = 0;
  bestRuntime_ = 0;
  bestHopWeigth_ = 0;
}

void PartitionMgr::reportGraph()
{
  hypergraph();
  toGraph();
  logger_->info(PAR, 67, "Number of Nodes: {}", graph_->getNumVertex());
  logger_->info(
      PAR, 68, "Number of Hyperedges/Edges: {}", graph_->getNumEdges());
}

void PartitionMgr::partitionDesign(unsigned int max_num_macro,
                                   unsigned int min_num_macro,
                                   unsigned int max_num_inst,
                                   unsigned int min_num_inst,
                                   unsigned int net_threshold,
                                   unsigned int ignore_net_threshold,
                                   unsigned int virtual_weight,
                                   unsigned int num_hops,
                                   unsigned int timing_weight,
                                   bool std_cell_timing_flag,
                                   const char* report_directory,
                                   const char* file_name,
                                   float keepin_lx,
                                   float keepin_ly,
                                   float keepin_ux,
                                   float keepin_uy)
{
  auto clusterer
      = std::make_unique<AutoClusterMgr>(db_network_, db_, _sta, logger_);
  clusterer->partitionDesign(max_num_macro,
                             min_num_macro,
                             max_num_inst,
                             min_num_inst,
                             net_threshold,
                             virtual_weight,
                             ignore_net_threshold,
                             num_hops,
                             timing_weight,
                             std_cell_timing_flag,
                             report_directory,
                             file_name,
                             keepin_lx,
                             keepin_ly,
                             keepin_ux,
                             keepin_uy);
}

}  // namespace par
