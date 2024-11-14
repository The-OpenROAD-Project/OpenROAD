///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////
#include "leidenClustering.h"

#include "ModularityVertexPartition.h"
#include "Optimizer.h"
#include "db_sta/dbNetwork.hh"
#include "leidenInterface.h"
#include "par/PartitionMgr.h"
#include "sta/Liberty.hh"

namespace odb {
class dbBlock;
class dbMaster;
}  // namespace odb

namespace sta {
class dbNetwork;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace mpl2 {
/**
 * @brief Constructor for the Leiden Clustering class.
 *
 * @param db Pointer to the database.
 * @param block Pointer to the block.
 * @param logger Pointer to the logger.
 */
leidenClustering::leidenClustering(odb::dbDatabase* db,
                                   odb::dbBlock* block,
                                   utl::Logger* logger)
    : db_(db),
      block_(block),
      logger_(logger),
      graph_(nullptr),
      hypergraph_(nullptr)
{
}

leidenClustering::~leidenClustering()
{
  delete graph_;
  delete hypergraph_;
}

/**
 * @brief Initializes the Leiden Clustering algorithm.
 *
 * This function is a placeholder for any initialization steps required
 * before running the clustering algorithm.
 */
void leidenClustering::init()
{
}

/**
 * @brief Runs the Leiden Clustering algorithm.
 *
 * This function initializes the clustering process, creates the hypergraph,
 * and then creates the graph from the hypergraph.
 */
void leidenClustering::run()
{
  logger_->report("\tRunning Leiden Clustering, initializing...");
  init();
  createHypergraph();
  createGraph();
  runLeidenClustering();
}

/**
 * @brief Creates a graph from the hypergraph.
 *
 * This function constructs a graph representation from the hypergraph. The
 * hypergraph is traversed to create a vertex for each hyperedge and an edge
 * between each pair of vertices that share a hyperedge. The function performs
 * the following steps:
 */
void leidenClustering::createGraph()
{
  /**
   * @brief Creates a graph from the hypergraph.
   * 1. Initializes an empty graph using GraphForLeidenAlgorithm.
   * 2. Iterates over each hyperedge in the hypergraph.
   * 3. For each hyperedge, iterates over each pair of vertices.
   * 4. Adds an edge between each pair of vertices with the calculated weight.
   */

  // Initialize the graph with the number of vertices
  graph_ = new GraphForLeidenAlgorithm(hypergraph_->num_vertices_);

  // Vertex weights are directly copied from the hypergraph
  graph_->vertex_weights_ = hypergraph_->vertex_weights_;

  logger_->report(
      "Creating Graph for Leiden algorithm. hypergraph_->numvertices_: {}",
      hypergraph_->num_vertices_);

  // Iterate over all hyperedges
  for (size_t i = 0; i < hypergraph_->hyperedges_.size(); ++i) {
    const std::vector<int>& hyperedge = hypergraph_->hyperedges_[i];
    float hyperedge_weight = hypergraph_->hyperedge_weights_[i];

    // Compute the weight of each pair of vertices in the hyperedge
    float edge_weight = hyperedge_weight * (hyperedge.size() - 1);

    // For each pair of vertices (j, k) in the hyperedge, add an edge
    for (size_t j = 0; j < hyperedge.size(); ++j) {
      for (size_t k = j + 1; k < hyperedge.size(); ++k) {
        // Add an edge between vertex j and vertex k with the calculated weight
        graph_->addEdge(hyperedge[j], hyperedge[k], edge_weight);
      }
    }
  }
  logger_->report("Creating Graph Finished.");
  graph_->calculateTotalweight();
  logger_->report("Calculate total edge weight finished.");
}

bool leidenClustering::isIgnoredMaster(odb::dbMaster* master)
{
  // IO corners are sometimes marked as end caps
  return master->isPad() || master->isCover() || master->isEndCap();
}

/**
 * @brief Creates a hypergraph from the block.
 *
 * This function iterates over the nets in the block and constructs a hypergraph
 * representation. Each net is examined to determine its driver and load
 * instances. Instances that are ignored based on their master type are skipped.
 * For each net, a hyperedge is created connecting the driver and load vertices,
 * and added to the hypergraph if it meets the criteria.
 *
 * The function performs the following steps:
 * 1. Initializes an empty list of hyperedges and a vertex counter.
 * 2. Iterates over each net in the block.
 * 3. Skips nets that are of supply type.
 * 4. For each net, iterates over its terminals to determine the driver and load
 * vertices.
 * 5. Skips instances that should be ignored based on their master type.
 * 6. Assigns a unique vertex ID to each instance if it doesn't already have
 * one.
 * 7. Identifies the driver and load vertices based on the terminal I/O type.
 * 8. Creates a hyperedge connecting the driver and load vertices if valid.
 * 9. Adds the hyperedge to the list of hyperedges.
 */
void leidenClustering::createHypergraph()
{
  std::vector<std::vector<int>> hyperedges;
  size_t num_vertices = 0;
  logger_->report(
      "Creating Hypergraph for leiden algorithm. block_->getNets().size(): {}",
      block_->getNets().size());

  for (odb::dbNet* net : block_->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }

    int driver_id = -1;
    std::set<int> loads_id;
    bool ignore = false;
    // logger_->report("Processing net: {}", net->getName());

    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      odb::dbMaster* master = inst->getMaster();

      if (isIgnoredMaster(master)) {
        ignore = true;
        break;
      }

      int vertex_id = -1;
      odb::dbIntProperty* int_prop
          = odb::dbIntProperty::find(inst, "vertex_id");
      if (int_prop) {
        vertex_id = int_prop->getValue();
      } else {
        vertex_id = num_vertices++;
        odb::dbIntProperty::create(inst, "vertex_id", vertex_id);
      }

      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }

    if (ignore) {
      continue;
    }

    loads_id.insert(driver_id);

    if (driver_id != -1 && loads_id.size() > 1) {
      std::vector<int> hyperedge;
      hyperedge.insert(hyperedge.end(), loads_id.begin(), loads_id.end());
      hyperedges.push_back(hyperedge);
      // logger_->report("Added hyperedge for net: {} with vertices: {}",
      // net->getName(), fmt::join(hyperedge, ", "));
    }
  }
  hypergraph_ = new HyperGraphForLeidenAlgorithm();
  hypergraph_->hyperedges_ = std::move(hyperedges);
  hypergraph_->hyperedge_weights_.resize(hypergraph_->hyperedges_.size(), 1.0);
  hypergraph_->num_vertices_ = num_vertices;
  hypergraph_->vertex_weights_.resize(num_vertices, 1.0);
  logger_->report(
      "Finished creating hypergraph with {} hyperedges, {} vertices number",
      hypergraph_->hyperedges_.size(),
      num_vertices);
}

void leidenClustering::runLeidenClustering()
{
  // Initialize the partition using the Modularity partition strategy
  ModularityVertexPartition part(graph_);

  // Initialize the optimizer
  Optimiser optimiser;
  logger_->report("Start Running Leiden Clustering");
  // Perform optimization on the partition
  optimiser.optimise_partition(&part);

  logger_->report("Finished running Leiden Clustering");
}

}  // namespace mpl2