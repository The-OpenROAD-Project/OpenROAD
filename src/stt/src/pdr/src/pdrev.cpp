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

#include "stt/pdrev.h"

#include "graph.h"
#include "stt/LinesRenderer.h"
#include "utl/Logger.h"

namespace pdr {

class Graph;

using stt::Branch;
using stt::Tree;
using utl::PDR;

class PdRev
{
 public:
  PdRev(std::vector<int>& x,
        std::vector<int>& y,
        int root_index,
        Logger* logger);
  ~PdRev();
  Tree primDijkstra(float alpha, int root_idx);
  Tree primDijkstraRevII(float alpha, int root_idx);
  void graphLines(
      std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>>& lines);
  void highlightGraph();

 private:
  Tree translateTree();
  void replaceNode(Graph* graph, int originalNode);
  void transferChildren(int originalNode);
  void printTree(Tree fluteTree);
  void RemoveSTNodes();

  Graph* graph_;
  Logger* logger_;
};

Tree primDijkstra(std::vector<int>& x,
                  std::vector<int>& y,
                  int drvr_index,
                  float alpha,
                  Logger* logger)
{
  pdr::PdRev pd(x, y, drvr_index, logger);
  return pd.primDijkstra(alpha, drvr_index);
}

Tree primDijkstraRevII(std::vector<int>& x,
                       std::vector<int>& y,
                       int drvr_index,
                       float alpha,
                       Logger* logger)
{
  // pdrev fails with non-zero root index despite showing signs of supporting
  // it.
  std::vector<int> x1(x);
  std::vector<int> y1(y);
  // Move driver to pole position until drvr_index arg works.
  std::swap(x1[0], x1[drvr_index]);
  std::swap(y1[0], y1[drvr_index]);
  drvr_index = 0;

  pdr::PdRev pd(x1, y1, drvr_index, logger);
  return pd.primDijkstraRevII(alpha, drvr_index);
}

PdRev::PdRev(std::vector<int>& x,
             std::vector<int>& y,
             int root_index,
             Logger* logger)
    : logger_(logger)
{
  graph_ = new Graph(x, y, root_index, logger_);
}

PdRev::~PdRev()
{
  delete graph_;
}

Tree PdRev::primDijkstra(float alpha, int root_idx)
{
  graph_->buildNearestNeighborsForSPT();
  graph_->run_PD_brute_force(alpha);
  graph_->doSteiner_HoVW();
  // The following slightly improves wire length but the cost is the use
  // of absolutely horrid unreliable code.
  // graph_->fix_max_dc();
  return translateTree();
}

Tree PdRev::primDijkstraRevII(float alpha, int root_idx)
{
#ifdef PDREVII
  graph_->buildNearestNeighborsForSPT();
  graph_->PDBU_new_NN(alpha);
  graph_->doSteiner_HoVW();
  graph_->fix_max_dc();
#endif
  return translateTree();
}

////////////////////////////////////////////////////////////////

// Translate pdrev graph to flute steiner tree representation.
// Apparently this simple mindedly clones non-terminal nodes along with their
// location so the number of branches are num_terminals * 2 - 2 like flute.
Tree PdRev::translateTree()
{
  if (graph_->num_terminals > 2) {
    for (int i = 0; i < graph_->num_terminals; ++i) {
      Node& node = graph_->nodes[i];
      if (!(node.children.empty()
            || (node.parent == i  // is root node
                && node.children.size() == 1
                && node.children[0] >= graph_->num_terminals))) {
        replaceNode(graph_, i);
      }
    }

    int nNodes = graph_->nodes.size();
    for (int i = graph_->num_terminals; i < nNodes; ++i) {
      while (graph_->nodes[i].children.size() > 3
             || (graph_->nodes[i].parent != i
                 && graph_->nodes[i].children.size() == 3)) {
        transferChildren(i);
      }
    }
    RemoveSTNodes();
  }

  Tree tree;
  int num_terminals = graph_->num_terminals;
  tree.deg = num_terminals;
  if (num_terminals < 2) {
    // No branches.
    tree.length = 0;
  } else {
    int branch_count = tree.branchCount();
    tree.branch.resize(branch_count);
    tree.length = graph_->calc_tree_wl_pd();
    if (graph_->nodes.size() != branch_count)
      logger_->error(PDR, 666, "steiner branch count inconsistent");
    for (int i = 0; i < graph_->nodes.size(); ++i) {
      Node& node = graph_->nodes[i];
      int parent = node.parent;
      if (parent >= graph_->nodes.size())
        logger_->error(PDR, 667, "steiner branch node out of bounds");
      Branch& newBranch = tree.branch[i];
      newBranch.x = node.x;
      newBranch.y = node.y;
      newBranch.n = parent;
    }
  }
  return tree;
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

// Remove spanning tree nodes?
// Remove steiner tree nodes?
// Who knows -cherry 08/16/2021
void PdRev::RemoveSTNodes()
{
  vector<int> toBeRemoved;
  vector<Node>& nodes = graph_->nodes;

  for (int i = graph_->num_terminals; i < nodes.size(); ++i) {
    if (nodes[i].children.size() < 2
        || (nodes[i].parent == i && nodes[i].children.size() == 2)) {
      toBeRemoved.push_back(i);
    }
  }
  for (int i = toBeRemoved.size() - 1; i >= 0; --i) {
    Node& cN = nodes[toBeRemoved[i]];
    graph_->removeChild(nodes[cN.parent], cN.idx);
    for (int j = 0; j < cN.children.size(); ++j) {
      graph_->replaceParent(
          nodes[cN.children[j]],
          cN.idx,
          // Note that the root node's parent is itself,
          // so the removed node's parent cannot be used
          // for the children. This fact seems to have escaped
          // the original author. Use the original root node
          // as the parent.
          // Note well that the root_idx passed to the top level
          // function cannot be used here because it may have
          // been if duplicate x/y locations were removed.
          // -cherry 08/09/2021
          cN.parent == cN.idx ? graph_->root_idx_ : cN.parent);
      graph_->addChild(nodes[cN.parent], cN.children[j]);
    }
  }
  for (int i = toBeRemoved.size() - 1; i >= 0; --i) {
    nodes.erase(nodes.begin() + toBeRemoved[i]);
  }

  std::map<int, int> idxMap;
  for (int i = 0; i < nodes.size(); ++i) {
    idxMap[nodes[i].idx] = i;
  }
  for (int i = 0; i < nodes.size(); ++i) {
    Node& cN = nodes[i];
    for (int j = 0; j < toBeRemoved.size(); ++j) {
      graph_->removeChild(nodes[i], toBeRemoved[j]);
    }

    sort(cN.children.begin(), cN.children.end());
    for (int j = 0; j < cN.children.size(); ++j) {
      if (cN.children[j] != idxMap[cN.children[j]])
        graph_->replaceChild(cN, cN.children[j], idxMap[cN.children[j]]);
    }
    cN.idx = i;
    cN.parent = idxMap[cN.parent];
  }
}

////////////////////////////////////////////////////////////////

void PdRev::graphLines(
    std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>>& lines)
{
  vector<Node>& nodes = graph_->nodes;
  for (Node& node : nodes) {
    Node& parent = nodes[node.parent];
    std::pair<int, int> node_xy(node.x, node.y);
    std::pair<int, int> parent_xy(parent.x, parent.y);
    std::pair<std::pair<int, int>, std::pair<int, int>> line(node_xy,
                                                             parent_xy);
    lines.push_back(line);
  }
}

// Useful for generating regression data.
void reportXY(std::vector<int> x, std::vector<int> y, Logger* logger)
{
  for (int i = 0; i < x.size(); i++)
    logger->report("\\{p{} {} {}\\}", i, x[i], y[i]);
}

void PdRev::highlightGraph()
{
  if (gui::Gui::enabled()) {
    gui::Gui* gui = gui::Gui::get();
    if (stt::LinesRenderer::lines_renderer == nullptr) {
      stt::LinesRenderer::lines_renderer = new stt::LinesRenderer();
      gui->registerRenderer(stt::LinesRenderer::lines_renderer);
    }
    std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> xy_lines;
    graphLines(xy_lines);
    std::vector<std::pair<odb::Point, odb::Point>> lines;
    for (int i = 0; i < xy_lines.size(); i++) {
      std::pair<int, int> xy1 = xy_lines[i].first;
      std::pair<int, int> xy2 = xy_lines[i].second;
      lines.push_back(std::pair(odb::Point(xy1.first, xy1.second),
                                odb::Point(xy2.first, xy2.second)));
    }
    stt::LinesRenderer::lines_renderer->highlight(lines, gui::Painter::red);
  }
}

}  // namespace pdr
