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
#pragma once

#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <vector>

#include "Hypergraph.h"
#include "Utilities.h"

namespace par {

// Vertex Gain is the basic elements of FM
// We do not use the classical gain-bucket data structure
// We design our own priority-queue based gain-bucket data structure
// to support float gain
class VertexGain
{
 public:
  // constructors
  VertexGain() = default;

  VertexGain(int vertex,
             int src_block_id,
             int destination_block_id,
             float gain,
             const std::map<int, float>& path_cost);

  // accessor functions
  int GetVertex() const { return vertex_; }
  void SetVertex(int vertex) { vertex_ = vertex; }

  float GetGain() const { return gain_; }
  void SetGain(float gain) { gain_ = gain; }

  // get the delta path cost
  const std::map<int, float>& GetPathCost() const { return path_cost_; }

  int GetSourcePart() const { return source_part_; }
  int GetDestinationPart() const { return destination_part_; }

 private:
  int vertex_ = -1;            // vertex id
  int source_part_ = -1;       // the source block id
  int destination_part_ = -1;  // the destination block id
  float gain_
      = -std::numeric_limits<float>::max();  // gain value of moving this vertex
  std::map<int, float>
      path_cost_;  // the updated DELTA path cost after moving vertex
                   // the path_cost will change because we will dynamically
                   // update the the weight of the path based on the number of
                   // the cut on the path
};

// ------------------------------------------------------------
// Priority-queue based gain bucket (Only for VertexGain)
// Actually we implement the priority queue with Max Heap
// We did not use the STL priority queue becuase we need
// to record the location of each element (vertex gain)
// -------------------------------------------------------------
class PriorityQueue
{
 public:
  // constructors
  PriorityQueue(int total_elements,
                int maximum_traverse_level,
                HGraphPtr hypergraph);

  // insert one element (std::shared_ptr<VertexGain>) into the priority queue
  void InsertIntoPQ(const std::shared_ptr<VertexGain>& element);

  // extract the largest element, i.e.,
  // get the largest element and remove it from the heap
  std::shared_ptr<VertexGain> ExtractMax();

  // get the largest element without removing it from the heap
  std::shared_ptr<VertexGain> GetMax() { return vertices_.front(); }

  // find the vertex gain which can satisfy the balance constraint
  std::shared_ptr<VertexGain> GetBestCandidate(
      const Matrix<float>& curr_block_balance,
      const Matrix<float>& upper_block_balance,
      const Matrix<float>& lower_block_balance,
      const HGraphPtr& hgraph);

  // update the priority (gain) for the specified vertex
  void ChangePriority(int vertex_id,
                      const std::shared_ptr<VertexGain>& new_element);

  // Remove the specified vertex
  void Remove(int vertex_id);

  // Basic accessors
  bool CheckIfEmpty() const { return vertices_.empty(); }
  int GetTotalElements() const { return total_elements_; }
  // the size of the max heap
  int GetSizeOfMap() const { return vertices_map_.size(); }
  // check if the vertex exists
  bool CheckIfVertexExists(int v) const { return vertices_map_[v] > -1; }
  // check the status of the heap
  void SetActive() { active_ = true; }
  void SetDeactive() { active_ = false; }
  bool GetStatus() const { return active_; }
  // clear the heap
  void Clear();

 private:
  // The max heap (priority queue) is organized as a binary tree
  // Get parent, left child and right child index
  // Generic functions of max heap
  int Parent(int element) const { return std::floor((element - 1) / 2); }

  int LeftChild(int& element) const { return 2 * element + 1; }

  int RightChild(int& element) const { return 2 * element + 2; }

  // This is called when we add a new element
  void HeapifyUp(int index);

  // This is called when we remove an existing element
  void HeapifyDown(int index);

  // Compare the two elements
  // If the gains are equal then pick the vertex with the smaller weight
  // The hope is doing this will incentivize in preventing corking effect
  bool CompareElementLargeThan(int index_a, int index_b);

  // private variables
  bool active_;
  HGraphPtr hypergraph_;
  std::vector<std::shared_ptr<VertexGain>> vertices_;  // elements
  std::vector<int> vertices_map_;  // store the location of vertex_gain for each
                                   // vertex vertices_map_ always has the size
                                   // of hypergraph_->num_vertices_
  int total_elements_;             // number of elements in the priority queue
  int maximum_traverse_level_ = 25;  // the maximum level of traversing the
                                     // buckets to solve the "corking effect"
};

}  // namespace par
