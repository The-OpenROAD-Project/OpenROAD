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
#pragma once
#include "leidenInterface.h"
#include <cfloat>
#include <deque>
#include <exception>
#include <limits>
#include <map>
#include <vector>


namespace mpl2 {
class Optimizer
{
 public:
  Optimizer();
  double optimise_partition(ModularityVertexPartition* partition);
  double optimise_partition(ModularityVertexPartition* partition,
                            std::vector<bool> const& is_membership_fixed);
  double optimise_partition(ModularityVertexPartition* partition,
                            std::vector<bool> const& is_membership_fixed,
                            size_t max_comm_size);

  // The multiplex functions that simultaneously optimise multiple graphs and
  // partitions (i.e. methods) Each node will be in the same community in all
  // graphs, and the graphs are expected to have identical nodes Optionally we
  // can loop over all possible communities instead of only the neighbours. In
  // the case of negative layer weights this may be necessary.
  double optimise_partition(std::vector<ModularityVertexPartition*> partitions,
                            std::vector<double> layer_weights,
                            std::vector<bool> const& is_membership_fixed);
  double optimise_partition(std::vector<ModularityVertexPartition*> partitions,
                            std::vector<double> layer_weights,
                            std::vector<bool> const& is_membership_fixed,
                            size_t max_comm_size);

  double move_nodes(ModularityVertexPartition* partition);
  double move_nodes(ModularityVertexPartition* partition,
                    std::vector<bool> const& is_membership_fixed,
                    bool renumber_fixed_nodes);
  double move_nodes(ModularityVertexPartition* partition,
                    std::vector<bool> const& is_membership_fixed,
                    bool renumber_fixed_nodes,
                    size_t max_comm_size);
  double move_nodes(std::vector<ModularityVertexPartition*> partitions,
                    std::vector<double> layer_weights,
                    std::vector<bool> const& is_membership_fixed,
                    bool renumber_fixed_nodes);
  double move_nodes(std::vector<ModularityVertexPartition*> partitions,
                    std::vector<double> layer_weights,
                    std::vector<bool> const& is_membership_fixed,
                    int consider_empty_community);
  double move_nodes(std::vector<ModularityVertexPartition*> partitions,
                    std::vector<double> layer_weights,
                    std::vector<bool> const& is_membership_fixed,
                    int consider_empty_community,
                    bool renumber_fixed_nodes);
  double move_nodes(std::vector<ModularityVertexPartition*> partitions,
                    std::vector<double> layer_weights,
                    std::vector<bool> const& is_membership_fixed,
                    int consider_empty_community,
                    bool renumber_fixed_nodes,
                    size_t max_comm_size);

  double merge_nodes(ModularityVertexPartition* partition);
  double merge_nodes(ModularityVertexPartition* partition,
                     std::vector<bool> const& is_membership_fixed,
                     bool renumber_fixed_nodes);
  double merge_nodes(ModularityVertexPartition* partition,
                     std::vector<bool> const& is_membership_fixed,
                     bool renumber_fixed_nodes,
                     size_t max_comm_size);
  double merge_nodes(std::vector<ModularityVertexPartition*> partitions,
                     std::vector<double> layer_weights,
                     std::vector<bool> const& is_membership_fixed,
                     bool renumber_fixed_nodes);
  double merge_nodes(std::vector<ModularityVertexPartition*> partitions,
                     std::vector<double> layer_weights,
                     std::vector<bool> const& is_membership_fixed,
                     bool renumber_fixed_nodes,
                     size_t max_comm_size);

  ~Optimizer();

  int refine_partition;          // Refine partition before aggregating
  int optimise_routine;          // What routine to use for optimisation
  int refine_routine;            // What routine to use for optimisation
  int consider_empty_community;  // Determine whether to consider moving nodes
                                 // to an empty community
  size_t max_comm_size;          // Constrain the maximal community size.

  static const int MOVE_NODES = 10;   // Use move node routine
  static const int MERGE_NODES = 11;  // Use merge node routine

 protected:
 private:
  void print_settings();
};;

}  // namespace mpl2