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

#include "pdrev/pdrev.h"

#include "graph.h"
#include "utility/Logger.h"

namespace PD {

void PdRev::setAlphaPDII(float alpha)
{
  alpha2 = alpha;
}

void PdRev::addNet(int numPins,
                   std::vector<unsigned> x,
                   std::vector<unsigned> y)
{
  my_graphs.push_back(new Graph(numPins,
                                verbose,
                                alpha1,
                                alpha2,
                                alpha3,
                                alpha4,
                                root_idx,
                                beta,
                                margin,
                                seed,
                                dist,
                                x,
                                y,
                                _logger));
}

void PdRev::config()
{
  num_nets = my_graphs.size();
  for (unsigned i = 0; i < num_nets; ++i) {
    // Guibas-Stolfi algorithm for computing nearest NE (north-east) neighbors
    if (i == net_num || !runOneNet) {
      my_graphs[i]->buildNearestNeighborsForSPT(my_graphs[i]->num_terminals);
    }
  }
}

void PdRev::runPDII()
{
  config();
  for (unsigned i = 0; i < num_nets; ++i) {
    if (i == net_num || !runOneNet) {
      my_graphs[i]->PDBU_new_NN();
    }
  }
  runDAS();
}

void PdRev::runDAS()
{
  for (unsigned i = 0; i < num_nets; ++i) {
    if (i == net_num || !runOneNet) {
      my_graphs[i]->doSteiner_HoVW();
    }
  }

  for (unsigned i = 0; i < num_nets; ++i) {
    if (i == net_num || !runOneNet) {
      my_graphs[i]->fix_max_dc();
    }
  }
}

void PdRev::replaceNode(int graph, int originalNode)
{
  Graph* tree = my_graphs[graph];
  std::vector<Node>& nodes = tree->nodes;
  Node& node = nodes[originalNode];
  int nodeParent = node.parent;
  std::vector<int>& nodeChildren = node.children;

  int newNode = tree->nodes.size();
  Node newSP(newNode, node.x, node.y);

  // Replace parent in old node children
  // Add children to new node
  for (int child : nodeChildren) {
    tree->replaceParent(tree->nodes[child], originalNode, newNode);
    tree->addChild(newSP, child);
  }
  // Delete children from old node
  nodeChildren.clear();
  // Set new node as old node's parent
  node.parent = newNode;
  // Set new node parent
  if (nodeParent != originalNode) {
    newSP.parent = nodeParent;
    // Replace child in parent
    tree->replaceChild(tree->nodes[nodeParent], originalNode, newNode);
  } else
    newSP.parent = newNode;
  // Add old node as new node's child
  tree->addChild(newSP, originalNode);
  nodes.push_back(newSP);
}

void PdRev::transferChildren(int graph, int originalNode)
{
  Graph* tree = my_graphs[graph];
  std::vector<Node>& nodes = tree->nodes;
  Node& node = nodes[originalNode];
  std::vector<int> nodeChildren = node.children;

  int newNode = tree->nodes.size();
  Node newSP(newNode, node.x, node.y);

  // Replace parent in old node children
  // Add children to new node
  int count = 0;
  node.children.clear();
  for (int child : nodeChildren) {
    if (count < 2) {
      tree->replaceParent(tree->nodes[child], originalNode, newNode);
      tree->addChild(newSP, child);
    } else {
      tree->addChild(node, child);
    }
    count++;
  }
  newSP.parent = originalNode;

  tree->addChild(node, newNode);
  nodes.push_back(newSP);
}

Tree PdRev::translateTree(int nTree)
{
  Graph* pdTree = my_graphs[nTree];
  Tree fluteTree;
  fluteTree.deg = pdTree->orig_num_terminals;
  fluteTree.branch = (Branch*) malloc((2 * fluteTree.deg - 2) * sizeof(Branch));
  fluteTree.length = pdTree->daf_wl;
  if (pdTree->orig_num_terminals > 2) {
    for (int i = 0; i < pdTree->orig_num_terminals; ++i) {
      Node& child = pdTree->nodes[i];
      if (child.children.size() == 0
          || (child.parent == i && child.children.size() == 1
              && child.children[0] >= pdTree->orig_num_terminals))
        continue;
      replaceNode(nTree, i);
    }
    int nNodes = pdTree->nodes.size();
    for (int i = pdTree->orig_num_terminals; i < nNodes; ++i) {
      Node& child = pdTree->nodes[i];
      while (pdTree->nodes[i].children.size() > 3
             || (pdTree->nodes[i].parent != i
                 && pdTree->nodes[i].children.size() == 3)) {
        transferChildren(nTree, i);
      }
    }
    pdTree->RemoveSTNodes();
  }
  for (int i = 0; i < pdTree->nodes.size(); ++i) {
    Node& child = pdTree->nodes[i];
    int parent = child.parent;
    Branch& newBranch = fluteTree.branch[i];
    newBranch.x = (DTYPE) child.x;
    newBranch.y = (DTYPE) child.y;
    newBranch.n = parent;
  }
  my_graphs.clear();
  return fluteTree;
}

void PdRev::printTree(Tree fluteTree)
{
  int i;
  for (i = 0; i < 2 * fluteTree.deg - 2; i++) {
    _logger->report("{} ", i);
    _logger->report("{} {}", fluteTree.branch[i].x, fluteTree.branch[i].y);
    _logger->report("{} {}",
                    fluteTree.branch[fluteTree.branch[i].n].x,
                    fluteTree.branch[fluteTree.branch[i].n].y);
  }
}

}  // namespace PD
