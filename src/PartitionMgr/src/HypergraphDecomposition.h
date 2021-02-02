///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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
#include "string"

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

enum GraphType : uint8_t
{
  CLIQUE,
  HYBRID,
  STAR,
  HYPERGRAPH
};

class HypergraphDecomposition
{
 public:
  HypergraphDecomposition() {}
  void init(int dbId, Logger * logger);
  void constructMap(Hypergraph& hypergraph, unsigned maxVertexWeight);
  void createHypergraph(Hypergraph& hypergraph,
                        std::vector<unsigned long> clusters,
                        short currentCluster);
  void toGraph(Hypergraph& hypergraph,
               Graph& graph,
               GraphType graphModel,
               unsigned weightingOption,
               unsigned maxEdgeWeight,
               unsigned threshold);
  void toHypergraph(Hypergraph& hypergraph, Graph& graph);
  void updateHypergraph(Hypergraph& hypergraph,
                        Hypergraph& newHypergraph,
                        std::vector<unsigned long> clusters,
                        short currentCluster);

 private:
  odb::dbBlock* _block;
  odb::dbDatabase* _db;
  odb::dbChip* _chip;
  Logger * _logger;

  int _weightingOption;
  std::vector<std::map<int, float>> adjMatrix;
  void addMapping(Hypergraph& hypergraph,
                  std::string instName,
                  const odb::Rect& rect);
  void createCliqueGraph(Graph& graph, std::vector<int> net);
  void createStarGraph(Graph& graph, std::vector<int> net);
  void connectPins(int firstPin, int secondPin, float weight);
  void connectStarPins(int firstPin, int secondPin, float weight);
  float computeWeight(int nPins);
  void createCompressedMatrix(Graph& graph);
};

}  // namespace par
