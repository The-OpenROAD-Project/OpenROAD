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
#include "KPMRefinement.h"

#include "TPHypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

using utl::PAR;

namespace par {
KPMpartition KPMRefinement::KPMevaluator(const HGraph hgraph,
                                         std::vector<int>& solution,
                                         bool print_flag)
{
  std::vector<std::vector<float>> block_balance
      = KPMgetBlockBalance(hgraph, solution);
  float cost = 0.0;
  // check the cutsize
  for (int e = 0; e < hgraph->num_hyperedges_; e++) {
    for (int idx = hgraph->eptr_[e] + 1; idx < hgraph->eptr_[e + 1]; idx++) {
      if (solution[hgraph->eind_[idx]] != solution[hgraph->eind_[idx - 1]]) {
        cost += std::inner_product(hgraph->hyperedge_weights_[e].begin(),
                                   hgraph->hyperedge_weights_[e].end(),
                                   e_wt_factors_.begin(),
                                   0.0);
        break;  // this net has been cut
      }
    }  // finish hyperedge e
  }
  // check timing paths
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++)
    cost += KPMcalculatePathCost(path_id, hgraph, solution);

  // check if the solution is valid
  for (auto value : solution)
    if (value < 0 || value >= num_parts_)
      logger_->report("[ERROR] The solution is invalid!! ");

  if (print_flag == true) {
    // print cost
    logger_->report("[EVAL] Cutsize: {}", cost);
    // print block balance
    std::vector<float> tot_vertex_weights = hgraph->GetTotalVertexWeights();
    for (auto block_id = 0; block_id < block_balance.size(); block_id++) {
      std::string line
          = "Vertex balance of block_" + std::to_string(block_id) + " : ";
      for (auto dim = 0; dim < tot_vertex_weights.size(); dim++) {
        std::stringstream ss;  // for converting float to string
        ss << std::fixed << std::setprecision(5)
           << block_balance[block_id][dim] / tot_vertex_weights[dim] << "  ( "
           << block_balance[block_id][dim] << " )  ";
        line += ss.str() + "  ";
      }
      logger_->info(PAR, 2911, line);
      logger_->report("cutsize : {}", cost);
      logger_->report("balance : {}", line);
    }  // finish block balance
  }
  return KPMpartition(cost, block_balance);
}

inline void kpm_heap::HeapifyUp(int index)
{
  while (index > 0
         && vertices_[Parent(index)]->GetGain() < vertices_[index]->GetGain()) {
    // Update the map
    auto& parent_heap_element = vertices_[Parent(index)];
    auto& child_heap_element = vertices_[index];
    vertices_map_[child_heap_element->GetVertex()] = Parent(index);
    vertices_map_[parent_heap_element->GetVertex()] = index;
    // Swap the elements
    std::swap(parent_heap_element, child_heap_element);
    // Next iteration
    index = Parent(index);
  }
}

void kpm_heap::HeapifyDown(int index)
{
  int max_index = index;
  int left_child = LeftChild(index);
  if (left_child < total_elements_
      && vertices_[left_child]->GetGain() > vertices_[max_index]->GetGain()) {
    max_index = left_child;
  }
  int right_child = RightChild(index);
  if (right_child < total_elements_
      && vertices_[right_child]->GetGain() > vertices_[max_index]->GetGain()) {
    max_index = right_child;
  } else if (right_child < total_elements_
             && vertices_[right_child]->GetGain()
                    == vertices_[max_index]->GetGain()) {
    // If the gains are equal then pick the vertex with the smaller weight
    // The hope is doing this will incentivize in preventing corking effect
    // The alternative option is keep the ordering of insertion by the PQ
    if (hypergraph_->vertex_weights_[vertices_[right_child]->GetVertex()]
        < hypergraph_->vertex_weights_[vertices_[max_index]->GetVertex()]) {
      max_index = right_child;
    }
  }
  if (index != max_index) {
    auto& left_heap_element = vertices_[index];
    auto& right_heap_element = vertices_[max_index];
    // Update the map
    vertices_map_[left_heap_element->GetVertex()] = max_index;
    vertices_map_[right_heap_element->GetVertex()] = index;
    // Swap the elements
    std::swap(left_heap_element, right_heap_element);
    // Next recursive iteration
    HeapifyDown(max_index);
  }
}

void kpm_heap::InsertIntoPQ(std::shared_ptr<vertex> vertex)
{
  total_elements_++;
  vertices_.push_back(vertex);
  vertices_map_[vertex->GetVertex()] = total_elements_ - 1;
  HeapifyUp(total_elements_ - 1);
}

std::shared_ptr<vertex> kpm_heap::ExtractMax()
{
  auto max_element = vertices_.front();
  vertices_[0] = vertices_[total_elements_ - 1];
  vertices_map_[vertices_[total_elements_ - 1]->GetVertex()] = 0;
  total_elements_--;
  vertices_.pop_back();
  HeapifyDown(0);
  // Set location of this vertex to -1 in the map
  vertices_map_[max_element->GetVertex()] = -1;
  return max_element;
}

void kpm_heap::ChangePriority(int index, float priority)
{
  float old_priority = vertices_[index]->GetGain();
  vertices_[index]->SetGain(priority);
  if (priority > old_priority) {
    HeapifyUp(index);
  } else {
    HeapifyDown(index);
  }
}

void kpm_heap::RemoveAt(int index)
{
  vertices_[index]->SetGain(GetMax()->GetGain() + 1.0);
  // Shift the element to top of the heap
  HeapifyUp(index);
  // Extract the element from the heap
  ExtractMax();
}

// Get block balance
inline matrix<float> KPMRefinement::KPMgetBlockBalance(
    const HGraph hgraph,
    std::vector<int>& solution)
{
  matrix<float> block_balance(
      num_parts_, std::vector<float>(hgraph->vertex_dimensions_, 0.0));
  if (hgraph->num_vertices_ != static_cast<int>(solution.size()))
    return block_balance;
  // check if the solution is valid
  for (auto block_id : solution)
    if (block_id < 0 || block_id >= num_parts_)
      return block_balance;
  // update the block_balance
  for (int v = 0; v < hgraph->num_vertices_; v++)
    block_balance[solution[v]]
        = block_balance[solution[v]] + hgraph->vertex_weights_[v];
  return block_balance;
}

// update the net degree for existing solution
// for each hyperedge, calculate the number of vertices in each part
matrix<int> KPMRefinement::KPMgetNetDegrees(const HGraph hgraph,
                                            std::vector<int>& solution)
{
  matrix<int> net_degs(hgraph->num_hyperedges_,
                       std::vector<int>(num_parts_, 0));
  for (int e = 0; e < hgraph->num_hyperedges_; ++e) {
    for (int idx = hgraph->eptr_[e]; idx < hgraph->eptr_[e + 1]; ++idx) {
      net_degs[e][solution[hgraph->eind_[idx]]]++;
    }
  }
  return net_degs;
}

// Calculate the cost for each timing path
// In the default mode (v = -1, to_pid = -1),
// we just calculate the cost for the timing path path_id
// In the replacement mode, we replace the block id of v to to_pid
float KPMRefinement::KPMcalculatePathCost(int path_id,
                                          const HGraph hgraph,
                                          const std::vector<int>& solution,
                                          int v,
                                          int to_pid)
{
  float cost = 0.0;  // cost for current path
  if (hgraph->num_timing_paths_ == 0)
    return cost;  // no timing paths

  std::vector<int> path;             // represent the path in terms of block_id
  std::map<int, int> block_counter;  // block_id counter
  for (auto idx = hgraph->vptr_p_[path_id]; idx < hgraph->vptr_p_[path_id];
       idx++) {
    const int u = hgraph->vind_p_[idx];  // current vertex
    const int block_id = (u == v) ? to_pid : solution[u];
    if (path.size() == 0 || path.back() != block_id) {
      path.push_back(block_id);
      if (block_counter.find(block_id) != block_counter.end())
        block_counter[block_id] += 1;
      else
        block_counter[block_id] = 1;
    }
  }

  if (path.size() <= 1)
    return cost;

  // num_cut = path.size() - 1
  cost = path_wt_factor_ * static_cast<float>(path.size() - 1);
  // get the snaking factor of the path (maximum repetition of block_id - 1)
  int snaking_factor = 0;
  for (auto [block_id, count] : block_counter)
    if (count > snaking_factor)
      snaking_factor = count;
  cost += snaking_wt_factor_ * static_cast<float>(snaking_factor - 1);
  return cost;
}

// Calculate the gain for a vertex v
// from_pid is the id of current block
// to_pid is the id of destination block
std::shared_ptr<vertex> KPMRefinement::KPMcalculateGain(
    int v,
    int from_pid,
    int to_pid,
    const HGraph hgraph,
    const std::vector<int>& solution,
    const std::vector<float>& cur_path_cost,
    const std::vector<std::vector<int>>& net_degs)
{
  float score = 0.0;
  std::map<int, float> path_cost;  // map path_id to latest score
  if (from_pid == to_pid)
    return std::shared_ptr<vertex>(new vertex(v, score, path_cost));
  // define two lamda function
  // 1) for checking connectivity
  // 2) for calculating the score of a hyperedge
  // function : check the connectivity for the hyperedge
  auto GetConnectivity = [&](int e) {
    int connectivity = 0;
    for (auto& num_v : net_degs[e])
      if (num_v > 0)
        connectivity++;
    return connectivity;
  };
  // function : calculate the score for the hyperedge
  auto GetHyperedgeScore = [&](int e) {
    return std::inner_product(hgraph->hyperedge_weights_[e].begin(),
                              hgraph->hyperedge_weights_[e].end(),
                              e_wt_factors_.begin(),
                              0.0);
  };
  // traverse all the hyperedges connected to v
  const int first_valid_entry = hgraph->vptr_[v];
  const int first_invalid_entry = hgraph->vptr_[v + 1];
  for (auto e_idx = first_valid_entry; e_idx < first_invalid_entry; e_idx++) {
    const int e = hgraph->vind_[e_idx];  // hyperedge id
    const int connectivity = GetConnectivity(e);
    const float e_score = GetHyperedgeScore(e);
    if (connectivity == 1
        && net_degs[e][from_pid]
               > 1) {  // move from_pid to to_pid will have negative socre
      score -= e_score;
    } else if (connectivity == 2 && net_degs[e][from_pid] == 1
               && net_degs[e][to_pid] > 0) {
      score += e_score;  // after move, all the vertices in to_pid
    }
  }
  // check the timing path
  if (hgraph->num_timing_paths_ > 0) {
    for (auto p_idx = hgraph->pptr_v_[v]; p_idx < hgraph->pptr_v_[v + 1];
         p_idx++) {
      const int path_id = hgraph->pind_v_[p_idx];
      const float cost
          = KPMcalculatePathCost(path_id, hgraph, solution, v, to_pid);
      path_cost[path_id] = cost;
      score += cur_path_cost[path_id] - cost;
    }
  }
  return std::shared_ptr<vertex>(new vertex(v, score, solution[v], path_cost));
}

// Calculate the span of hyperedges between two partitions
float KPMRefinement::KPMcalculateSpan(const HGraph hgraph,
                                      int& from_pid,
                                      int& to_pid,
                                      std::vector<int>& solution)
{
  float span = 0.0;
  for (int i = 0; i < hgraph->num_hyperedges_; ++i) {
    const int first_valid_entry = hgraph->eptr_[i];
    const int first_invalid_entry = hgraph->eptr_[i + 1];
    bool flag_partition_from = false;
    bool flag_partition_to = false;
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      const int v_id = hgraph->eind_[j];
      if (solution[v_id] == from_pid) {
        flag_partition_from = true;
      } else if (solution[v_id] == to_pid) {
        flag_partition_to = true;
      }
    }
    if (flag_partition_from == true && flag_partition_to == true) {
      span += std::inner_product(hgraph->hyperedge_weights_[i].begin(),
                                 hgraph->hyperedge_weights_[i].end(),
                                 e_wt_factors_.begin(),
                                 0.0);
    }
  }
  return span;
}

// Find vertices lying on the boundary of the partition
std::vector<int> KPMRefinement::KPMfindBoundaryVertices(
    const HGraph hgraph,
    std::pair<int, int>& partition_pair,
    matrix<int>& net_degs)
{
  std::set<int> boundary_set;
  std::vector<bool> boundary_net_flag(hgraph->num_hyperedges_, false);
  for (int i = 0; i < hgraph->num_hyperedges_; ++i) {
    if (net_degs[i][partition_pair.first] > 0
        && net_degs[i][partition_pair.second] > 0) {
      boundary_net_flag[i] = true;
    }
  }
  for (int i = 0; i < hgraph->num_vertices_; ++i) {
    const int first_valid_entry = hgraph->vptr_[i];
    const int first_invalid_entry = hgraph->vptr_[i + 1];
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      const int h_id = hgraph->vind_[j];
      if (boundary_net_flag[h_id] == true) {
        boundary_set.insert(i);
        break;
      }
    }
  }
  std::vector<int> boundary_vertices(boundary_set.begin(), boundary_set.end());
  return boundary_vertices;
}

// Check if a vertex is on the boundary of a bipartition
inline bool KPMRefinement::KPMcheckBoundaryVertex(
    const HGraph hgraph,
    const int& v,
    const std::pair<int, int> partition_pair,
    const matrix<int>& net_degs) const
{
  const int first_valid_entry = hgraph->vptr_[v];
  const int first_invalid_entry = hgraph->vptr_[v + 1];
  for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
    const int he = hgraph->vind_[i];
    if (net_degs[he][partition_pair.first] > 0
        && net_degs[he][partition_pair.second] > 0)
      return true;
  }
  return false;
}

// Check connectivity of a hyperedge
inline int KPMRefinement::KPMgetConnectivity(const int& he,
                                             const matrix<int>& net_degs) const
{
  int connectivity = 0;
  for (int i = 0; i < num_parts_; ++i) {
    if (net_degs[he][i] > 0) {
      ++connectivity;
    }
  }
  return connectivity;
}

// Function to return the pairwise combination of partitions that are tightly
// connected
std::vector<std::pair<int, int>> KPMRefinement::KPMfindPairs(
    const HGraph hgraph,
    std::vector<int>& solution,
    std::vector<float>& prev_scores)
{
  // calculate the pairwise combination of the different partition blocks
  int total_pairs = static_cast<int>(num_parts_ * (num_parts_ - 1) / 2);
  //
  if (prev_scores.empty() == true) {
    prev_scores.resize(total_pairs);
    std::fill(prev_scores.begin(), prev_scores.end(), 0.0);
  }
  // define a vector of pairwise partition scores
  std::vector<float> pair_scores(total_pairs, -1.0);
  // define a vector of partition pairs
  std::vector<std::pair<int, int>> pairs;
  std::vector<std::pair<int, int>> ppairs;
  std::vector<float> pair_scores_diff = prev_scores;
  // lambda function for building pqs of pairs
  auto comp = [&](const int x, const int y) {
    return pair_scores_diff[x] > pair_scores_diff[y];
  };
  // define priority queues with lambda function
  std::priority_queue<int, std::vector<int>, decltype(comp)> pair_queues(comp);
  int idx = -1;
  // For each pair calculate the difference between spanned hyperedges before
  // and after prev pass of FM
  for (int i = 0; i < num_parts_; ++i) {
    for (int j = i + 1; j < num_parts_; ++j) {
      ++idx;
      pairs.push_back(std::make_pair(i, j));
      pair_scores[idx] = KPMcalculateSpan(hgraph, i, j, solution);
      // Calculate the difference between the spans before and after FM from
      // prev pass
      pair_scores_diff[idx] = pair_scores[idx] - prev_scores[idx];
      pair_queues.push(idx);
    }
  }

  return pairs;
}

void KPMRefinement::KPMrefinement(const HGraph hgraph,
                                  const matrix<float>& max_block_balance,
                                  std::vector<int>& solution)
{
  int same_gain_passes = 0;
  std::vector<float> prev_scores;
  for (int num_pass = 0; num_pass < max_num_fm_pass_; num_pass++) {
    KPMevaluator(hgraph, solution, false);
    // Find the pairs that are tightly connected with each other
    auto partition_pairs = KPMfindPairs(hgraph, solution, prev_scores);
    const float tot_gain
        = KPMpass(hgraph, partition_pairs, max_block_balance, solution);
    if (tot_gain < 0.0) {
      break;  // stop pass loop if there is no improvement
    } else if (tot_gain == 0.0) {
      ++same_gain_passes;
      if (same_gain_passes == max_stagnation_) {
        break;
      }
    } else {
      same_gain_passes = 0;
    }
  }
  logger_->report("[K-PM-FM] Hypergraph ( {} , {} ): Refined cut {}",
                  hgraph->num_vertices_,
                  hgraph->num_hyperedges_,
                  KPMevaluator(hgraph, solution, false).first);
}

void KPMRefinement::KPMinitialGainsBetweenPairs(
    const HGraph hgraph,
    const std::pair<int, int>& partition_pair,
    const std::vector<int>& boundary_vertices,
    const matrix<int>& net_degs,
    const std::vector<int>& solution,
    const std::vector<float>& cur_path_cost,
    kpm_heaps& gain_buckets)
{
  // Unwrap the partition blocks of the pair
  const int partition_left = partition_pair.first;
  const int partition_right = partition_pair.second;
  // Loop through the boundary vertices
  for (const int& v : boundary_vertices) {
    if (solution[v] != partition_pair.first && solution[v] != partition_right) {
      continue;
    }
    assert(gain_buckets[partition_pair.first]->GetStatus() == true);
    assert(gain_buckets[partition_pair.second]->GetStatus() == true);
    // Set vertex v as boundary vertex
    if (GetBoundaryStatus(v) == false) {
      MarkBoundary(v);
    }
    // Reset visit flag
    if (GetVisitStatus(v) == true) {
      ResetVisited(v);
    }
    // Initialize gain
    const int from_pid
        = solution[v] == partition_left ? partition_left : partition_right;
    const int to_pid
        = from_pid == partition_left ? partition_right : partition_left;
    auto gain_cell = KPMcalculateGain(
        v, from_pid, to_pid, hgraph, solution, cur_path_cost, net_degs);
    // If the vertex was inserted into the PQ by previous iterations dont
    // reinsert again
    if (gain_buckets[to_pid]->CheckIfVertexExists(v) == false) {
      gain_buckets[to_pid]->InsertIntoPQ(gain_cell);
    }
  }
}

// Legality checker for a vertex move
// Check if the move of the vertex violates the max balance constraint
bool KPMRefinement::KPMcheckLegality(HGraph hgraph,
                                     int to,
                                     std::shared_ptr<vertex> v,
                                     matrix<float>& curr_block_balance,
                                     const matrix<float>& max_block_balance)
{
  if (GetVisitStatus(v->GetVertex()) == true) {
    return false;
  }
  std::vector<float> total_wt
      = curr_block_balance[to] + hgraph->vertex_weights_[v->GetVertex()];
  if (total_wt <= max_block_balance[to]) {
    return true;
  }
  return false;
}

// Pick the best vertex to move across different pairs
std::shared_ptr<vertex> KPMRefinement::KPMpickVertexToMove(
    HGraph hgraph,
    pqs& gain_buckets,
    matrix<float>& curr_block_balance,
    const matrix<float>& max_block_balance)
{
  int part_to = -1;
  // auto best_vertex = new vertex;
  std::shared_ptr<vertex> best_vertex(new vertex);
  float max_gain = -std::numeric_limits<float>::max();
  for (int i = 0; i < num_parts_; ++i) {
    if (gain_buckets[i]->GetStatus() == true
        && gain_buckets[i]->CheckIfEmpty() == false) {
      auto ele = gain_buckets[i]->GetMax();
      // If the vertex has laready been visited, then we dont want to move it
      // again This is more of a secure check to prevent thrashing of vertices
      // This situation is highly likely not to happen
      if (GetVisitStatus(ele->GetVertex()) == true) {
        continue;
      }
      float gain = ele->GetGain();
      // If the gain of the vertex is higher than the max gain and the move is
      // legal
      bool legality_of_move = KPMcheckLegality(
          hgraph, i, ele, curr_block_balance, max_block_balance);
      if (gain > max_gain && legality_of_move == true) {
        max_gain = gain;
        best_vertex = ele;
        part_to = i;
      }
    }
  }
  // Fix for corking effect
  if (best_vertex->GetStatus() == false) {
    std::vector<float> min_area_part(num_parts_,
                                     std::numeric_limits<float>::max());
    int min_part = -1;
    for (int i = 0; i < num_parts_; ++i) {
      if (gain_buckets[i]->GetStatus() == true
          && gain_buckets[i]->CheckIfEmpty() == false
          && curr_block_balance[i] < min_area_part) {
        min_area_part = curr_block_balance[i];
        min_part = i;
      }
    }
    // Check if gain bucket is empty
    // If so stop
    if (min_part == -1) {
      return best_vertex;
    }
    max_gain = -std::numeric_limits<float>::max();
    best_vertex = gain_buckets[min_part]->GetMax();
    int corking_passes = static_cast<int>(
        ceil(gain_buckets[min_part]->GetTotalElements()) * 0.5);
    for (int j = 1; j < corking_passes; ++j) {
      auto ele = gain_buckets[min_part]->GetHeapVertex(j);
      if (KPMcheckLegality(
              hgraph, min_part, ele, curr_block_balance, max_block_balance)
              == true
          && ele->GetGain() > max_gain) {
        max_gain = ele->GetGain();
        best_vertex = ele;
      }
    }
    best_vertex->SetPotentialMove(min_part);
  } else {
    best_vertex->SetPotentialMove(part_to);
  }
  return best_vertex;
}

// Accept the requested move and update partition
void KPMRefinement::KPMAcceptMove(std::shared_ptr<vertex> vertex_to_move,
                                  HGraph hgraph,
                                  std::vector<vertex>& moves_trace,
                                  float& total_gain,
                                  float& total_delta_gain,
                                  std::pair<int, int>& partition_pair,
                                  std::vector<int>& solution,
                                  std::vector<float>& paths_cost,
                                  matrix<float>& curr_block_balance,
                                  kpm_heaps& gain_buckets,
                                  matrix<int>& net_degs)
{
  int vertex_id = vertex_to_move->GetVertex();
  // Push the vertex into the moves_trace
  moves_trace.push_back(*vertex_to_move);
  // Add gain of the candidate vertex to the total gain
  total_gain -= vertex_to_move->GetGain();
  total_delta_gain += vertex_to_move->GetGain();
  // Deactivate given vertex, meaning it will be never be moved again
  MarkVisited(vertex_id);
  vertex_to_move->SetDeactive();
  // Update the path cost first
  for (int i = 0; i < vertex_to_move->GetTotalPaths(); ++i) {
    paths_cost[i] = vertex_to_move->GetPathCost(i);
  }
  // Fetch old and new partitions
  const int prev_part_id = partition_pair.first;
  const int new_part_id = partition_pair.second;
  // Update the partition balance
  curr_block_balance[prev_part_id]
      = curr_block_balance[prev_part_id] - hgraph->vertex_weights_[vertex_id];
  curr_block_balance[new_part_id]
      = curr_block_balance[new_part_id] + hgraph->vertex_weights_[vertex_id];
  // Update the partition
  solution[vertex_id] = new_part_id;
  // update net_degs
  const int first_valid_entry = hgraph->vptr_[vertex_id];
  const int first_invalid_entry = hgraph->vptr_[vertex_id + 1];
  for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
    const int he = hgraph->vind_[i];
    --net_degs[he][prev_part_id];
    ++net_degs[he][new_part_id];
  }
  // Remove vertex from the gain bucket
  int heap_loc = gain_buckets[new_part_id]->GetLocationOfVertex(vertex_id);
  gain_buckets[new_part_id]->RemoveAt(heap_loc);
}

// Function to find neighbors of a vertex
void KPMRefinement::KPMfindNeighbors(const HGraph hgraph,
                                     int& vertex,
                                     std::pair<int, int>& partition_pair,
                                     std::vector<int>& solution,
                                     std::set<int>& neighbors)
{
  const int first_valid_entry = hgraph->vptr_[vertex];
  const int first_invalid_entry = hgraph->vptr_[vertex + 1];
  for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
    const int he = hgraph->vind_[i];
    const int first_valid_entry_he = hgraph->eptr_[he];
    const int first_invalid_entry_he = hgraph->eptr_[he + 1];
    for (int j = first_valid_entry_he; j < first_invalid_entry_he; ++j) {
      const int v = hgraph->eind_[j];
      if (v == vertex || GetVisitStatus(v) == true
          || (solution[v] != partition_pair.first
              && solution[v] != partition_pair.second)) {
        continue;
      }
      neighbors.insert(v);
    }
  }
}

// Function update the gains of neighboring vertices of a candidate vertex
void KPMRefinement::KPMupdateNeighbors(
    const HGraph hgraph,
    const std::pair<int, int>& partition_pair,
    const std::set<int>& neighbors,
    const std::vector<int>& solution,
    const std::vector<float>* cur_path_cost,
    const matrix<int>& net_degs,
    kpm_heaps& gain_buckets)
{
  // Unwrap pair
  int from_pid = partition_pair.first;
  int to_pid = partition_pair.second;
  // After moving a vertex, loop through all neighbors of that vertex
  for (const int& v : neighbors) {
    int nbr_part = solution[v];
    if (nbr_part != partition_pair.first && nbr_part != partition_pair.second) {
      continue;
    }
    if (GetVisitStatus(v) == true) {
      continue;
    }
    int nbr_from_part = nbr_part == from_pid ? from_pid : to_pid;
    int nbr_to_part = nbr_from_part == from_pid ? to_pid : from_pid;
    // If the vertex is in the gain bucket then simply update the priority
    if (gain_buckets[nbr_to_part]->CheckIfVertexExists(v) == true) {
      auto new_gain_cell = KPMcalculateGain(v,
                                            nbr_from_part,
                                            nbr_to_part,
                                            hgraph,
                                            solution,
                                            *cur_path_cost,
                                            net_degs);
      float new_gain = new_gain_cell->GetGain();
      // Now update the heap
      // assert(gain_buckets[nbr_to_part]->GetStatus() == true);
      int nbr_index_in_heap = gain_buckets[nbr_to_part]->GetLocationOfVertex(v);
      gain_buckets[nbr_to_part]->ChangePriority(nbr_index_in_heap, new_gain);
    }  // If the vertex is a boundary vertex and is not in the gain bucket then
       // add it to the bucket
    else if (KPMcheckBoundaryVertex(hgraph, v, partition_pair, net_degs) == true
             && gain_buckets[nbr_to_part]->CheckIfVertexExists(v) == false) {
      MarkBoundary(v);
      auto gain_cell = KPMcalculateGain(v,
                                        nbr_from_part,
                                        nbr_to_part,
                                        hgraph,
                                        solution,
                                        *cur_path_cost,
                                        net_degs);
      // Add this new cell to the gain bucket
      gain_buckets[nbr_to_part]->InsertIntoPQ(gain_cell);
    }
    /*
    // Check if this vertex used to be a nonboundary vertex and recently became
    // a boundary vertex
    if (KPMcheckBoundaryVertex(hgraph, v, partition_pair, net_degs) == true
        && gain_buckets[nbr_to_part]->CheckIfVertexExists(v) == false) {
      // if (gain_buckets[nbr_to_part]->CheckIfVertexExists(v) == false) {
      // Calculate the gain of this vertex if it is a newly added boundary
      // vertex. Set the boundary status of the neighbor vertex
      // std::cout << "[DEBUG] Adding vertex begins " << std::endl;
      MarkBoundary(v);
      vertex gain_cell = KPMcalculateGain(v,
                                          nbr_from_part,
                                          nbr_to_part,
                                          hgraph,
                                          solution,
                                          *cur_path_cost,
                                          net_degs);
      // Add this new cell to the gain bucket
      std::cout << "[DEBUG] Adding vertex " << v << " with gain "
                << gain_cell.GetGain() << std::endl;
      gain_buckets[nbr_to_part]->InsertIntoPQ(gain_cell);
      continue;
    } else if (KPMcheckBoundaryVertex(hgraph, v, partition_pair, net_degs)
                   == true
               && gain_buckets[nbr_to_part]->CheckIfVertexExists(v) == true) {
      // Fetch neighbor vertex from heap
      // Delta gain update
      vertex new_gain_cell = KPMcalculateGain(v,
                                              nbr_from_part,
                                              nbr_to_part,
                                              hgraph,
                                              solution,
                                              *cur_path_cost,
                                              net_degs);
      float new_gain = new_gain_cell.GetGain();
      // Now update the heap
      // std::cout << "[DEBUG] Updating the gain bucket " << std::endl;
      assert(gain_buckets[nbr_to_part]->GetStatus() == true);
      int nbr_index_in_heap = gain_buckets[nbr_to_part]->GetLocationOfVertex(v);
      // std::cout << "[DEBUG] Location of vertex " << v << " is "
      //            << nbr_index_in_heap << std::endl;
      gain_buckets[nbr_to_part]->ChangePriority(nbr_index_in_heap, new_gain);
    }*/
  }
}
// Rollback function to reverse the moves made during a pass
void KPMRefinement::KPMrollBackMoves(std::vector<vertex>& trace,
                                     matrix<int>& net_degs,
                                     int& best_move,
                                     float& total_delta_gain,
                                     kpm_heaps& gain_buckets,
                                     HGraph hgraph,
                                     matrix<float>& curr_block_balance,
                                     std::vector<float>& cur_path_cost,
                                     std::vector<int>& solution)
{
  int idx = trace.size() - 1;
  while (true) {
    // Grab vertex cell from back of the move trace
    auto vertex_cell = trace.back();
    const int vertex_id = vertex_cell.GetVertex();
    const int source_part = vertex_cell.GetSourcePart();
    const int dest_part = vertex_cell.GetPotentialMove();
    const float gain = vertex_cell.GetGain();
    // Deduct gain from tot_delta_gain
    total_delta_gain -= gain;
    assert(dest_part == solution[vertex_id]);
    // Flip the partition of the vertex to its previous part id
    solution[vertex_id] = source_part;
    // Add the vertex back into the PQ
    assert(gain_buckets[source_part]->CheckIfVertexExists(vertex_id) == false);
    // gain_buckets[source_part]->InsertIntoPQ(vertex_cell);
    // Update the balance
    curr_block_balance[dest_part]
        = curr_block_balance[dest_part] - hgraph->vertex_weights_[vertex_id];
    curr_block_balance[source_part]
        = curr_block_balance[source_part] + hgraph->vertex_weights_[vertex_id];
    // Update the net degs and collect neighbors
    std::set<int> neighbors;
    const int first_valid_entry_he = hgraph->vptr_[vertex_id];
    const int first_invalid_entry_he = hgraph->vptr_[vertex_id + 1];
    for (int i = first_valid_entry_he; i < first_invalid_entry_he; ++i) {
      const int he = hgraph->vind_[i];
      // Updating the net degs here
      ++net_degs[he][source_part];
      --net_degs[he][dest_part];
      // Looping to collect neighbors
    }
    // Update the trace
    if (idx == best_move) {
      break;
    }
    --idx;
    trace.pop_back();
  }
}

float KPMRefinement::KPMpass(const HGraph hgraph,
                             std::vector<std::pair<int, int>>& partition_pairs,
                             const matrix<float>& max_block_balance,
                             std::vector<int>& solution)
{
  // Initialize cost for timing paths
  std::vector<float> paths_cost;
  paths_cost.resize(hgraph->num_timing_paths_);
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    paths_cost[path_id] = KPMcalculatePathCost(path_id, hgraph, solution);
  }
  // Do an initial calculation of net degrees only for hyperedges lying on the
  // cut for each boundary hyperedge, calculate the number of vertices in each
  // part
  matrix<int> net_degs = KPMgetNetDegrees(hgraph, solution);
  // calculate the current balance for each block
  matrix<float> block_balance = KPMgetBlockBalance(hgraph, solution);
  // Initialize gain buckets for FM
  kpm_heaps gain_buckets;
  for (int i = 0; i < num_parts_; ++i) {
    // Initialize individual gain bucket for each partition
    std::shared_ptr<kpm_heap> gain_bucket
        = std::make_shared<kpm_heap>(hgraph->num_vertices_, hgraph);
    gain_buckets.push_back(gain_bucket);
  }
  // Initialize boundary flag
  InitBoundaryFlags(hgraph->num_vertices_);
  // Initialize the visit flags to false meaning no vertex has been visited
  InitVisitFlags(hgraph->num_vertices_);
  // std::cout << "[DEBUG] Init done " << std::endl;
  for (int i = 0; i < partition_pairs.size(); ++i) {
    auto partition_pair = partition_pairs[i];
    // Set the bucket to active
    gain_buckets[partition_pair.first]->SetActive();
    gain_buckets[partition_pair.second]->SetActive();
    // Evaluate what are the boundary vertices lying between the two partition
    // blocks
    // std::cout << "[DEBUG] Finding b-vertices " << std::endl;
    std::vector<int> boundary_vertices
        = KPMfindBoundaryVertices(hgraph, partition_pair, net_degs);
    // Initialize gain data structures for those selected boundary vertices
    // std::cout << "[DEBUG] Initialize gains between pairs " << std::endl;
    KPMinitialGainsBetweenPairs(hgraph,
                                partition_pair,
                                boundary_vertices,
                                net_degs,
                                solution,
                                paths_cost,
                                gain_buckets);
  }
  // Get current balance of partition
  matrix<float> curr_block_balance = KPMgetBlockBalance(hgraph, solution);
  // Start with main body of KPM here
  std::vector<vertex> moves_trace;
  float tot_cutsize = KPMevaluator(hgraph, solution, false).first;
  float min_cutsize = tot_cutsize;
  // Keep a track of the cutsize and solution vector pre pass of FM
  // If cutsize increases post FM pass then revert back to this recorded
  // solution Doing this saves unncecessary function calls
  float pre_pass_cutsize = tot_cutsize;
  float total_delta_gain = 0.0;
  std::vector<int> pre_pass_solution = solution;
  // std::cout << "[DEBUG] Begin pass " << std::endl;
  std::set<int> neighbors;
  for (int mini_pass = 0; mini_pass < 1; ++mini_pass) {
    int best_move = -1;
    for (int i = 0; i < max_num_moves_; ++i) {
      auto ele = KPMpickVertexToMove(
          hgraph, gain_buckets, curr_block_balance, max_block_balance);
      // Check if no candidate vertex was returned
      if (ele->GetStatus() == false) {
        break;
      }
      int vertex_id = ele->GetVertex();

      std::pair<int, int> partition_pair
          = std::make_pair(solution[vertex_id], ele->GetPotentialMove());
      // Accept the proposed move and make changes post movement
      KPMAcceptMove(ele,
                    hgraph,
                    moves_trace,
                    tot_cutsize,
                    total_delta_gain,
                    partition_pair,
                    solution,
                    paths_cost,
                    curr_block_balance,
                    gain_buckets,
                    net_degs);
      // Find the neighbors of the candidate vertex
      KPMfindNeighbors(hgraph, vertex_id, partition_pair, solution, neighbors);
      assert(solution[vertex_id] == ele->GetPotentialMove());
      // Update the gains of the neighbors
      KPMupdateNeighbors(hgraph,
                         partition_pair,
                         neighbors,
                         solution,
                         &paths_cost,
                         net_degs,
                         gain_buckets);
      /*std::cout << "[DEBUG] MOVE #" << i << " cutsize " << tot_cutsize << "["
                << KPMevaluator(hgraph, solution, false).first << "]"
                << std::endl;*/
      if (tot_cutsize <= min_cutsize) {
        min_cutsize = tot_cutsize;
        best_move = i;
      }
    }
    if (min_cutsize > pre_pass_cutsize) {
      solution = pre_pass_solution;
      total_delta_gain = 0;
      return total_delta_gain;
    }
    // Roll back bad moves
    int revert_till = best_move + 1;
    if (revert_till < moves_trace.size()) {
      KPMrollBackMoves(moves_trace,
                       net_degs,
                       revert_till,  // revert moves till best move position + 1
                       total_delta_gain,
                       gain_buckets,
                       hgraph,
                       curr_block_balance,
                       paths_cost,
                       solution);
    }
    // Clearing the gain buckets for the next pass
    for (int i = 0; i < gain_buckets.size(); ++i) {
      if (gain_buckets[i]->GetStatus() == false) {
        continue;
      }
      gain_buckets[i]->Clear();
    }
    // Resetting the flags for next pass
    ResetBoundaryFlags(hgraph->num_vertices_);
    ResetVisitFlags(hgraph->num_vertices_);
    // Clearing neighbors set
    neighbors.clear();
    for (int i = 0; i < partition_pairs.size(); ++i) {
      auto partiton_pair = partition_pairs[i];
      // Activating the gain buckets as required
      gain_buckets[partiton_pair.first]->SetActive();
      gain_buckets[partiton_pair.second]->SetActive();
      // Finding new set of boundary vertices
      std::vector<int> boundary_vertices
          = KPMfindBoundaryVertices(hgraph, partiton_pair, net_degs);
      // Initialize gain data structures for those selected boundary vertices
      KPMinitialGainsBetweenPairs(hgraph,
                                  partiton_pair,
                                  boundary_vertices,
                                  net_degs,
                                  solution,
                                  paths_cost,
                                  gain_buckets);
    }
    tot_cutsize = min_cutsize;
    total_delta_gain = pre_pass_cutsize - tot_cutsize;
    pre_pass_cutsize = tot_cutsize;
    pre_pass_solution = solution;
    moves_trace.clear();
  }
  return total_delta_gain;
}

/*float KPMRefinement::KPMpass(const HGraph hgraph,
                             const matrix<float>& max_block_balance,
                             std::vector<int>& solution)
{
  // Initialize cost for timing paths
  std::vector<float> paths_cost;
  paths_cost.resize(hgraph->num_timing_paths_);
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    paths_cost[path_id] = KPMcalculatePathCost(path_id, hgraph, solution);
  }
  // Do an initial calculation of net degrees only for hyperedges lying on the
  // cut for each boundary hyperedge, calculate the number of vertices in each
  // part
  matrix<int> net_degs = KPMgetNetDegrees(hgraph, solution);
  // calculate the current balance for each block
  matrix<float> block_balance = KPMgetBlockBalance(hgraph, solution);
  // calculate the pairwise combination of the different partition blocks
  int total_pairs = static_cast<int>(num_parts_ * (num_parts_ - 1) / 2);
  int max_pairs = static_cast<int>(floor(num_parts_ / 2));
  // define a vector of pairwise partition scores
  std::vector<float> pair_scores(total_pairs, -1.0);
  // define a vector of partition pairs
  std::vector<std::pair<int, int>> pairs;
  // lambda function for building pqs of pairs
  auto comp = [&](const int x, const int y) {
    return pair_scores[x] < pair_scores[y];
  };
  // define priority queues with lambda function
  std::priority_queue<int, std::vector<int>, decltype(comp)> pair_queues(comp);
  int idx = -1;
  // Initialize gain buckets for FM
  kpm_heaps gain_buckets;
  for (int i = 0; i < num_parts_; ++i) {
    // Initialize individual gain bucket for each partition
    std::shared_ptr<kpm_heap> gain_bucket
        = std::make_shared<kpm_heap>(hgraph->num_vertices_, hgraph);
    gain_buckets.push_back(gain_bucket);
    for (int j = i + 1; j < num_parts_; ++j) {
      ++idx;
      pairs.push_back(std::make_pair(i, j));
      pair_scores[idx] = KPMcalculateSpan(hgraph, i, j, solution);
      pair_queues.push(idx);
    }
  }
  // Initialize boundary flag
  InitBoundaryFlags(hgraph->num_vertices_);
  // Initialize the visit flags to false meaning no vertex has been visited
  InitVisitFlags(hgraph->num_vertices_);
  std::vector<std::pair<int, int>> ppairs;
  // Pick best pairs and initialize gains
  std::vector<bool> pair_mask(num_parts_, false);
  for (int i = 0; i < max_pairs; ++i) {
    auto partition_pair = pairs[i];
    if (pair_mask[partition_pair.first] == true
        || pair_mask[partition_pair.second] == true) {
      continue;
    }
    ppairs.push_back(partition_pair);
    // Set the gain buckets to active
    gain_buckets[partition_pair.first]->SetActive();
    gain_buckets[partition_pair.second]->SetActive();
    // Evaluate what are the boundary vertices lying between the two partition
    // blocks
    std::vector<int> boundary_vertices
        = KPMfindBoundaryVertices(hgraph, partition_pair, net_degs);
    // Initialize gain data structures for those selected boundary vertices
    KPMinitialGainsBetweenPairs(hgraph,
                                partition_pair,
                                boundary_vertices,
                                net_degs,
                                solution,
                                paths_cost,
                                gain_buckets);
    pair_mask[partition_pair.first] = true;
    pair_mask[partition_pair.second] = true;
  }
  // Get current balance of partition
  matrix<float> curr_block_balance = KPMgetBlockBalance(hgraph, solution);
  // Start with main body of KPM here
  std::vector<vertex> moves_trace;
  float tot_cutsize = KPMevaluator(hgraph, solution, false).first;
  float min_cutsize = tot_cutsize;
  // Keep a track of the cutsize and solution vector pre pass of FM
  // If cutsize increases post FM pass then revert back to this recorded
  // solution Doing this saves unncecessary function calls
  float pre_pass_cutsize = tot_cutsize;
  std::vector<int> pre_pass_solution = solution;
  for (int mini_pass = 0; mini_pass < 2; ++mini_pass) {
    int best_move = -1;
    for (int i = 0; i < max_num_moves_; ++i) {
      // std::cout << "[DEBUG] Starting move " << i << std::endl;
      auto ele = KPMpickVertexToMove(
          hgraph, gain_buckets, curr_block_balance, max_block_balance);
      // Check if no candidate vertex was returned
      if (ele->GetStatus() == false) {
        break;
      }
      int vertex_id = ele->GetVertex();
      std::set<int> neighbors;
      std::pair<int, int> partition_pair
          = std::make_pair(solution[vertex_id], ele->GetPotentialMove());
      // std::cout << "[DEBUG] Move picked " << vertex_id << std::endl;
      // std::cout << "[DEBUG] Old part " << ele->GetSourcePart() << " "
      //          << ele->GetPotentialMove() << std::endl;
      // Accept the proposed move and make changes post movement
      KPMAcceptMove(ele,
                    hgraph,
                    moves_trace,
                    tot_cutsize,
                    partition_pair,
                    solution,
                    paths_cost,
                    curr_block_balance,
                    gain_buckets,
                    net_degs);
      // std::cout << "[DEBUG] Move accepted " << std::endl;
      // Find the neighbors of the candidate vertex
      KPMfindNeighbors(hgraph, vertex_id, partition_pair, solution, neighbors);
      // std::cout << "[DEBUG] Neighbors calculated " << std::endl;
      assert(solution[vertex_id] == ele->GetPotentialMove());
      // Update the gains of the neighbors
      KPMupdateNeighbors(hgraph,
                         partition_pair,
                         neighbors,
                         solution,
                         &paths_cost,
                         net_degs,
                         gain_buckets);
      std::cout << "[DEBUG] MOVE #" << i << " cutsize " << tot_cutsize << "["
                << KPMevaluator(hgraph, solution, false).first << "]"
                << std::endl;
      if (tot_cutsize <= min_cutsize) {
        min_cutsize = tot_cutsize;
        best_move = i;
      }
    }
    std::cout << "[DEBUG] Min cutsize recorded " << min_cutsize << std::endl;
    std::cout << "[DEBUG] Best move detected " << best_move << std::endl;
    if (min_cutsize >= pre_pass_cutsize) {
      solution = pre_pass_solution;
      exit(1);
      return pre_pass_cutsize;
    }
    // Roll back bad moves
    int revert_till = best_move+1;
    KPMrollBackMoves(moves_trace,
                     net_degs,
                     revert_till,  // revert moves till best move position + 1
                     gain_buckets,
                     hgraph,
                     curr_block_balance,
                     paths_cost,
                     solution);
    // Clearing the gain buckets for the next pass
    for (int i = 0; i < gain_buckets.size(); ++i) {
      if (gain_buckets[i]->GetStatus() == false) {
        continue;
      }
      gain_buckets[i]->Clear();
    }
    for (int i = 0; i < ppairs.size(); ++i) {
      auto partiton_pair = ppairs[i];
      gain_buckets[partiton_pair.first]->SetActive();
      gain_buckets[partiton_pair.second]->SetActive();
      std::vector<int> boundary_vertices
          = KPMfindBoundaryVertices(hgraph, partiton_pair, net_degs);
      // Initialize gain data structures for those selected boundary vertices
      KPMinitialGainsBetweenPairs(hgraph,
                                  partiton_pair,
                                  boundary_vertices,
                                  net_degs,
                                  solution,
                                  paths_cost,
                                  gain_buckets);
    }
    tot_cutsize = min_cutsize;
    pre_pass_cutsize = tot_cutsize;
    pre_pass_solution = solution;
  }
  exit(1);
}
*/
}  // namespace par
