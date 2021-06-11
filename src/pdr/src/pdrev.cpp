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

#include "pdr/pdrev.h"

#include "graph.h"
#include "utl/Logger.h"

namespace pdr {

class Graph;

using stt::Tree;
using stt::Branch;

class PdRev
{
public:
  PdRev(std::vector<int> x,
        std::vector<int> y,
        Logger* logger);
  ~PdRev();
  void runPD(float alpha);
  void runPDII(float alpha);
  Tree translateTree();
  void graphLines(std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> &lines);

private:
  void replaceNode(Graph* graph, int originalNode);
  void transferChildren(int originalNode);
  void printTree(Tree fluteTree);

  Graph* graph_;
  Logger* logger_;
};

Tree
primDikstra(std::vector<int> x,
            std::vector<int> y,
            float alpha,
            Logger* logger)
{
  pdr::PdRev pd(x, y, logger);
  pd.runPD(alpha);
  Tree tree = pd.translateTree();
  return tree;
}

Tree
primDikstraRevII(std::vector<int> x,
                 std::vector<int> y,
                 float alpha,
                 Logger* logger)
{
  pdr::PdRev pd(x, y, logger);
  pd.runPDII(alpha);
  Tree tree = pd.translateTree();
  return tree;
}

PdRev::PdRev(std::vector<int> x,
             std::vector<int> y,
             Logger* logger) :
  logger_(logger)
{
  graph_ = new Graph(x, y, logger_);
}

PdRev::~PdRev()
{
  delete graph_;
}

void PdRev::runPD(float alpha)
{
  graph_->buildNearestNeighborsForSPT();
  graph_->run_PD_brute_force(alpha);
  graph_->doSteiner_HoVW();
}

void PdRev::runPDII(float alpha)
{
  graph_->buildNearestNeighborsForSPT();
  graph_->PDBU_new_NN(alpha);
  graph_->doSteiner_HoVW();
  graph_->fix_max_dc();
}

void PdRev::replaceNode(Graph* tree, int originalNode)
{
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

void PdRev::transferChildren(int originalNode)
{
  std::vector<Node>& nodes = graph_->nodes;
  Node& node = nodes[originalNode];
  std::vector<int> nodeChildren = node.children;

  int newNode = nodes.size();
  Node newSP(newNode, node.x, node.y);

  // Replace parent in old node children
  // Add children to new node
  int count = 0;
  node.children.clear();
  for (int child : nodeChildren) {
    if (count < 2) {
      graph_->replaceParent(nodes[child], originalNode, newNode);
      graph_->addChild(newSP, child);
    } else {
      graph_->addChild(node, child);
    }
    count++;
  }
  newSP.parent = originalNode;

  graph_->addChild(node, newNode);
  nodes.push_back(newSP);
}

Tree PdRev::translateTree()
{
  if (graph_->orig_num_terminals > 2) {
    for (int i = 0; i < graph_->orig_num_terminals; ++i) {
      Node& child = graph_->nodes[i];
      if (child.children.size() == 0
          || (child.parent == i && child.children.size() == 1
              && child.children[0] >= graph_->orig_num_terminals))
        continue;
      replaceNode(graph_, i);
    }
    int nNodes = graph_->nodes.size();
    for (int i = graph_->orig_num_terminals; i < nNodes; ++i) {
      Node& child = graph_->nodes[i];
      while (graph_->nodes[i].children.size() > 3
             || (graph_->nodes[i].parent != i
                 && graph_->nodes[i].children.size() == 3)) {
        transferChildren(i);
      }
    }
    graph_->RemoveSTNodes();
  }

  Tree tree;
  tree.deg = graph_->orig_num_terminals;
  int branch_count = tree.deg * 2 - 2;
  if (graph_->nodes.size() != branch_count)
    logger_->error(utl::GRT, 666, "steiner branch count inconsistent");
  tree.branch = (Branch*) malloc(branch_count * sizeof(Branch));
  tree.length = graph_->calc_tree_wl_pd();
  for (int i = 0; i < graph_->nodes.size(); ++i) {
    Node& child = graph_->nodes[i];
    int parent = child.parent;
    if (parent >= graph_->nodes.size())
      logger_->error(utl::GRT, 666, "steiner branch node out of bounds");
    Branch& newBranch = tree.branch[i];
    newBranch.x = child.x;
    newBranch.y = child.y;
    newBranch.n = parent;
  }
  return tree;
}

void
PdRev::graphLines(std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> &lines)
{
  vector<Node> &nodes = graph_->nodes;
  for (Node &node : nodes) {
    Node &parent = nodes[node.parent];
    std::pair<int, int> node_xy(node.x, node.y);
    std::pair<int, int> parent_xy(parent.x, parent.y);
    std::pair<std::pair<int, int>, std::pair<int, int>> line(node_xy, parent_xy);
    lines.push_back(line);
  }
}

}  // namespace PD
