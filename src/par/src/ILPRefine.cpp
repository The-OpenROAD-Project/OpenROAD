///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
#include "Hypergraph.h"
#include "Refiner.h"
#include "Utilities.h"
#include "utl/Logger.h"

// ------------------------------------------------------------------------------
// ILP-based refinement
// ILP-based hypergraph partitioning cannot optimize path cost
// Please try to avoid using ILP-based refinement for k-way partitioning
// given that ILP-based refinement is very time-consuming for k-way partitioning
// ------------------------------------------------------------------------------

namespace par {
TPilpRefine::TPilpRefine(const int num_parts,
                         const int refiner_iters,
                         const float path_wt_factor,
                         const float snaking_wt_factor,
                         const int max_move,
                         TP_evaluator_ptr evaluator,
                         utl::Logger* logger)
    : TPrefiner(num_parts,
                refiner_iters,
                path_wt_factor,
                snaking_wt_factor,
                max_move,
                std::move(evaluator),
                logger)
{
}

// Implement the ILP-based refinement pass
float TPilpRefine::Pass(
    const HGraphPtr& hgraph,
    const MATRIX<float>& upper_block_balance,
    const MATRIX<float>& lower_block_balance,
    MATRIX<float>& block_balance,        // the current block balance
    MATRIX<int>& net_degs,               // the current net degree
    std::vector<float>& cur_paths_cost,  // the current path cost
    TP_partition& solution,
    std::vector<bool>& visited_vertices_flag)
{
  // Step 1: identify all the boundary vertices (boundary vertices will not
  // include fixed vertices)
  std::vector<int> boundary_vertices
      = FindBoundaryVertices(hgraph, net_degs, visited_vertices_flag);
  // Step 2: extract the related information
  // vertices, hyperedges
  // In our implementation, try to avoid traversing the entire hypergraph
  // multiple times
  const int num_extracted_vertices
      = num_parts_ + static_cast<int>(boundary_vertices.size());
  std::vector<int> vertices_extracted;
  vertices_extracted.reserve(num_extracted_vertices);
  std::map<int, int> fixed_vertices_extracted;  // vertex_id, block_id
  std::map<int, int> vertices_extracted_map;    // map the boundary vertices to
                                                // the extracted vertices
  MATRIX<float> vertices_weight_extracted;      // extracted vertex weight
  MATRIX<int> hyperedges_extracted;
  std::vector<float> hyperedges_weight_extracted;
  vertices_weight_extracted.reserve(num_extracted_vertices);
  int vertex_id = 0;
  for (const auto& v : boundary_vertices) {
    vertices_extracted.push_back(v);
    vertices_extracted_map[v] = vertex_id++;
    vertices_weight_extracted.push_back(hgraph->vertex_weights_[v]);
    const int block_id = solution[v];
    block_balance[block_id]
        = block_balance[block_id] - hgraph->vertex_weights_[v];
  }
  const int part_vertex_id_base = vertex_id;
  // the remaining vertices in each block are modeled as a fixed vertex
  for (int block_id = 0; block_id < num_parts_; block_id++) {
    vertices_extracted.push_back(-1);  // map to nothing
    fixed_vertices_extracted[vertex_id++] = block_id;
    vertices_weight_extracted.push_back(block_balance[block_id]);
  }

  // get the hyperedge information
  std::set<int> boundary_hyperedges;
  for (const auto& v : boundary_vertices) {
    for (int idx = hgraph->vptr_[v]; idx < hgraph->vptr_[v + 1]; idx++) {
      boundary_hyperedges.insert(hgraph->vind_[idx]);
    }
  }
  // convert boundary hyperegdes into extracted hyperedges
  for (const auto& e : boundary_hyperedges) {
    std::set<int> hyperedge;
    for (int idx = hgraph->eptr_[e]; idx < hgraph->eptr_[e + 1]; idx++) {
      const int vertex_id = hgraph->eind_[idx];
      if (vertices_extracted_map.find(vertex_id)
          == vertices_extracted_map.end()) {
        hyperedge.insert(part_vertex_id_base + solution[vertex_id]);
      } else {
        hyperedge.insert(vertices_extracted_map[vertex_id]);
      }
    }
    if (hyperedge.size() > 1) {
      hyperedges_extracted.push_back(
          std::vector<int>(hyperedge.begin(), hyperedge.end()));
      hyperedges_weight_extracted.push_back(
          evaluator_->CalculateHyperedgeCost(e, hgraph));
    }
  }
  // get vertex weight dimension
  const int vertex_weight_dimension = hgraph->vertex_dimensions_;
  // call the ILP solver
  std::vector<int> solution_extracted;
  if (ILPPartitionInst(num_parts_,
                       vertex_weight_dimension,
                       solution_extracted,
                       fixed_vertices_extracted,
                       hyperedges_extracted,
                       hyperedges_weight_extracted,
                       vertices_weight_extracted,
                       upper_block_balance,
                       lower_block_balance)
      == false) {
    logger_->report(
        "[WARNING] ILP-based partitioning cannot find a valid solution.");
    return 0.0;  // no valid solution
  }
  // try to update the solution
  // We have this extra step, because the ILP-based partitioning cannot handle
  // path related cost
  std::vector<TP_gain_cell> moves_trace;
  float best_gain = 0.0;
  float total_gain = 0.0;
  int best_vertex_id = -1;
  for (int i = 0; i < part_vertex_id_base; i++) {
    const int vertex_id = vertices_extracted[i];
    const int to_pid = solution_extracted[i];
    // calculate the gain
    TP_gain_cell gain_cell = CalculateVertexGain(vertex_id,
                                                 solution[vertex_id],
                                                 to_pid,
                                                 hgraph,
                                                 solution,
                                                 cur_paths_cost,
                                                 net_degs);
    moves_trace.push_back(gain_cell);
    // accept the gain
    AcceptVertexGain(gain_cell,
                     hgraph,
                     total_gain,
                     visited_vertices_flag,
                     solution,
                     cur_paths_cost,
                     block_balance,
                     net_degs);
    if (total_gain >= best_gain) {
      best_gain = total_gain;
      best_vertex_id = vertex_id;
    }
  }

  // update the solution to the status with best_gain
  // traverse the moves_trace in the reversing order
  for (auto move_iter = moves_trace.rbegin(); move_iter != moves_trace.rend();
       move_iter++) {
    // stop when we encounter the best_vertex_id
    auto& vertex_move = *move_iter;
    if (vertex_move->GetVertex() == best_vertex_id) {
      break;  // stop here
    }
    RollBackVertexGain(vertex_move,
                       hgraph,
                       visited_vertices_flag,
                       solution,
                       cur_paths_cost,
                       block_balance,
                       net_degs);
  }
  return best_gain;
}

}  // namespace par
