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

#pragma once

#include <string>

#include "Hypergraph.h"
#include "par/PartitionMgr.h"

namespace odb {
class dbDatabase;
class dbChip;
class dbBlock;
class dbNet;
class Rect;
}  // namespace odb

namespace utl {
class Logger;
}
using utl::Logger;

namespace par {

class HypergraphDecomposition
{
 public:
  HypergraphDecomposition();
  void init(odb::dbBlock* block, Logger* logger);
  void constructMap(Hypergraph& hypergraph, const unsigned maxVertexWeight);
  void createHypergraph(Hypergraph& hypergraph,
                        const std::vector<unsigned long>& clusters,
                        const short currentCluster);
  void toGraph(const Hypergraph& hypergraph,
               Graph& graph,
               const GraphType graphModel,
               const unsigned weightingOption,
               const unsigned maxEdgeWeight,
               const unsigned threshold);
  void toHypergraph(Hypergraph& hypergraph, const Graph* graph);
  void updateHypergraph(const Hypergraph& hypergraph,
                        Hypergraph& newHypergraph,
                        const std::vector<unsigned long>& clusters,
                        const short currentCluster);

 private:
  odb::dbBlock* block_;
  Logger* logger_;

  int weightingOption_;
  std::vector<std::map<int, float>> adjMatrix_;
  void addMapping(Hypergraph& hypergraph,
                  std::string instName,
                  const odb::Rect& rect);
  void createCliqueGraph(const std::vector<int>& net);
  void createStarGraph(const std::vector<int>& net);
  void connectPins(const int firstPin, const int secondPin, const float weight);
  void connectStarPins(const int firstPin,
                       const int secondPin,
                       const float weight);
  float computeWeight(const int nPins);
  void createCompressedMatrix(Graph& graph);
};

}  // namespace par
