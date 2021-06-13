///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
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
///////////////////////////////////////////////////////////////////////////////

#include <vector>

namespace utl {
class Logger;
}  // namespace utl

namespace PD {

using utl::Logger;

class Graph;

typedef int DTYPE;

typedef struct
{
  DTYPE x, y;
  int n;
} Branch;

typedef struct
{
  int deg;
  DTYPE length;
  Branch* branch;
} Tree;

class PdRev
{
 public:
  PdRev(Logger* logger) : _logger(logger){};
  void setAlphaPDII(float alpha);
  void addNet(int numPins, std::vector<unsigned> x, std::vector<unsigned> y);
  void runPD(float alpha);
  void runPDII();
  Tree translateTree(int nTree);

 private:
  void runDAS();
  void config();
  void replaceNode(int graph, int originalNode);
  void transferChildren(int graph, int originalNode);
  void printTree(Tree fluteTree);

  unsigned num_nets = 1000;
  unsigned num_terminals = 64;
  unsigned verbose = 0;
  float alpha1 = 1;
  float alpha2 = 0.45;
  float alpha3 = 0;
  float alpha4 = 0;
  float margin = 1.1;
  unsigned seed = 0;
  unsigned root_idx = 0;
  unsigned dist = 2;
  float beta = 1.4;
  bool runOneNet = false;
  unsigned net_num = 0;
  std::vector<Graph*> my_graphs;
  Logger* _logger;
};

}  // namespace PD
