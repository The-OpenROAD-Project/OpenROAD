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

#include "PriorityQueue.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "Hypergraph.h"
#include "Utilities.h"

namespace par {

PriorityQueue::PriorityQueue(const int total_elements,
                             const int maximum_traverse_level,
                             HGraphPtr hypergraph)
    : maximum_traverse_level_(maximum_traverse_level)
{
  vertices_map_.resize(total_elements);
  std::fill(vertices_map_.begin(), vertices_map_.end(), -1);
  total_elements_ = 0;
  hypergraph_ = std::move(hypergraph);
  active_ = false;
}

void PriorityQueue::Clear()
{
  active_ = false;
  vertices_.clear();
  total_elements_ = 0;
  std::fill(vertices_map_.begin(), vertices_map_.end(), -1);
}

// insert one element into the priority queue
void PriorityQueue::InsertIntoPQ(const std::shared_ptr<VertexGain>& element)
{
  total_elements_++;
  vertices_.push_back(element);
  vertices_map_[element->GetVertex()] = total_elements_ - 1;
  HeapifyUp(total_elements_ - 1);
}

// get the largest element
std::shared_ptr<VertexGain> PriorityQueue::ExtractMax()
{
  auto max_element = vertices_.front();
  // replace the first element with the last element, then
  // call HeapifyDown to update the order of elements
  vertices_[0] = vertices_[total_elements_ - 1];
  vertices_map_[vertices_[total_elements_ - 1]->GetVertex()] = 0;
  total_elements_--;
  vertices_.pop_back();
  HeapifyDown(0);
  // Set location of this vertex to -1 in the map
  vertices_map_[max_element->GetVertex()] = -1;
  return max_element;
}

// find the vertex gain which can satisfy the balance constraint
std::shared_ptr<VertexGain> PriorityQueue::GetBestCandidate(
    const Matrix<float>& curr_block_balance,
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance,
    const HGraphPtr& hgraph)
{
  if (total_elements_ <= 0) {               // empty
    return std::make_shared<VertexGain>();  // return the dummy cell
  }
  int pass = 0;
  int candidate_index = -1;  // the index of the candidate vertex gain
  int index = 0;             // starting from the first index

  // define the lambda function to check the balance constraint
  auto CheckBalance = [&](int index) {
    const int vertex_id = vertices_[index]->GetVertex();
    const int to_pid = vertices_[index]->GetDestinationPart();
    const int from_pid = vertices_[index]->GetSourcePart();
    if ((curr_block_balance[to_pid] + hgraph->GetVertexWeights(vertex_id)
         < upper_block_balance[to_pid])
        && (curr_block_balance[from_pid] - hgraph->GetVertexWeights(vertex_id)
            > lower_block_balance[from_pid])) {
      return true;
    }
    return false;
  };

  // check the first index
  if (CheckBalance(index) == true) {
    return vertices_[index];
  }

  // traverse the max heap
  while (pass < maximum_traverse_level_) {
    pass++;
    // Step 1: check the left child first
    const int left_child = LeftChild(index);
    if (left_child < total_elements_ && CheckBalance(left_child) == true) {
      candidate_index = left_child;
    }

    // Step 2: check the right child second
    const int right_child = RightChild(index);
    if (right_child < total_elements_ && CheckBalance(right_child) == true
        && (candidate_index == -1
            || CompareElementLargeThan(right_child, candidate_index))) {
      candidate_index = right_child;  // use the right index
    }

    if (candidate_index > 0) {
      return vertices_[candidate_index];  // return the candidate gain cell
    }
    if (left_child >= total_elements_ || right_child >= total_elements_) {
      // no valid candidate
      return std::make_shared<VertexGain>();  // return the dummy cell
    }
    index = CompareElementLargeThan(right_child, left_child) == true
                ? right_child
                : left_child;
  }
  return std::make_shared<VertexGain>();  // return the dummy cell
}

// Remove the specifid vertex
void PriorityQueue::Remove(int vertex_id)
{
  const int index = vertices_map_[vertex_id];
  if (index == -1) {
    return;  // This vertex does not exists
  }
  // set the gain of this element to maximum + 1
  vertices_[index]->SetGain(GetMax()->GetGain() + 1.0);
  // Shift the element to top of the heap
  HeapifyUp(index);
  // Extract the element from the heap
  ExtractMax();
  if (total_elements_ <= 0) {
    active_ = false;
  }
}

// Update the priority (gain) for the specified vertex
void PriorityQueue::ChangePriority(
    int vertex_id,
    const std::shared_ptr<VertexGain>& new_element)
{
  const int index = vertices_map_[vertex_id];
  if (index == -1) {
    return;  // This vertex does not exists
  }
  const float old_priority = vertices_[index]->GetGain();
  vertices_[index] = new_element;
  if (new_element->GetGain() > old_priority) {
    HeapifyUp(index);
  } else {
    HeapifyDown(index);
  }
}

// ----------------------------------------------------------------
// Private functions
// ----------------------------------------------------------------

// Compare the two elements
// If the gains are equal then pick the vertex with the smaller weight
// The hope is doing this will incentivize in preventing corking effect
bool PriorityQueue::CompareElementLargeThan(int index_a, int index_b)
{
  if (vertices_[index_a]->GetGain() > vertices_[index_b]->GetGain()) {
    return true;
  }
  return (
      (vertices_[index_a]->GetGain() == vertices_[index_b]->GetGain())
      && (hypergraph_->GetVertexWeights(vertices_[index_a]->GetVertex())
          < hypergraph_->GetVertexWeights(vertices_[index_b]->GetVertex())));
}

// push the element at location index to its ordered location
// This function is called when we insert a new element
void PriorityQueue::HeapifyUp(int index)
{
  while (index > 0 && CompareElementLargeThan(index, Parent(index)) == true) {
    // Update the map (exchange parent and child)
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

// This function is called when we delete an existing element
void PriorityQueue::HeapifyDown(int index)
{
  int max_index = index;

  // Basically, we need order index, left child and right child
  // Step 1: check if current index is less than left child
  const int left_child = LeftChild(index);
  if (left_child < total_elements_
      && CompareElementLargeThan(left_child, max_index) == true) {
    max_index = left_child;
  }

  // Step 2: check if the max index is less than right child
  const int right_child = RightChild(index);
  if (right_child < total_elements_
      && CompareElementLargeThan(right_child, max_index) == true) {
    max_index = right_child;
  }

  if (index == max_index) {
    return;  // we do not need to further heapifydown
  }

  // swap index and max_index
  auto& cur_heap_element = vertices_[index];
  auto& large_heap_element = vertices_[max_index];
  // Update the map
  vertices_map_[cur_heap_element->GetVertex()] = max_index;
  vertices_map_[large_heap_element->GetVertex()] = index;
  // Swap the elements
  std::swap(cur_heap_element, large_heap_element);
  // Next recursive iteration
  HeapifyDown(max_index);
}

}  // namespace par
