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

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>

#include "graph.h"
#include "utl/Logger.h"

namespace pdr {

using std::map;
using std::max;
using std::min;
using std::pair;
using std::queue;
using std::swap;

using utl::PDR;

// Don't add odb::Point as a dependent here.
typedef pair<int, int> Pt;

class PtHash
{
 public:
  size_t operator()(const Pt& pt) const
  {
    size_t hash = 5381;
    // hash * 31 ^ add
    hash = ((hash << 5) + hash) ^ pt.first;
    hash = ((hash << 5) + hash) ^ pt.second;
    return hash;
  }
};

class PtEqual
{
 public:
  bool operator()(const Pt& pt1, const Pt& pt2) const
  {
    return pt1.first == pt2.first && pt1.second == pt2.second;
  }
};

typedef std::unordered_map<Pt, int, PtHash, PtEqual> PtMap;

Graph::Graph(vector<int>& x, vector<int>& y, int root_index, Logger* logger)
    : root_idx_(root_index), heap_size_(0), logger_(logger)
{
  PtMap pts;
  for (int i = 0; i < x.size(); ++i) {
    int x1 = x[i];
    int y1 = y[i];
    Pt pt(x1, y1);
    int idx = nodes.size();
    auto pt_itr = pts.find(pt);
    if (pt_itr == pts.end()) {
      pts[pt] = idx;
      nodes.push_back(Node(idx, x[i], y[i]));
      edges_.push_back(Edge(idx, 0, 0));
      sorted_.push_back(idx);
#ifdef Guibas_Stolfi
      sheared_.push_back(Node(idx, 0, 0));
#endif
    } else if (root_idx_ == idx)
      // Root is a duplicate location.
      root_idx_ = pt_itr->second;
    else if (idx < root_idx_)
      // Deleting duplicate before root.
      root_idx_--;
  }

  num_terminals = nodes.size();

  if (logger_->debugCheck(PDR, "pdrev", 3)) {
    debugPrint(logger_, PDR, "pdrev", 3, "-- Node locations --");
    for (int i = 0; i < nodes.size(); ++i)
      debugPrint(logger_, PDR, "pdrev", 3, "   {}", nodes[i]);
  }
}

/*********************************************************************/

// Return the Manhattan distance between two points
int dist(Node& p, Node& q)
{
  return abs(p.x - q.x) + abs(p.y - q.y);
}

void Graph::UpdateManhDist()
{
  for (int i = 0; i < nodes.size(); ++i) {
    vector<int> distV;
    for (int j = 0; j < nodes.size(); ++j) {
      if (i == j) {
        distV.push_back(0);
      } else {
        int cDist = dist(nodes[i], nodes[j]);
        distV.push_back(cDist);
      }
    }
    manh_dist_.push_back(distV);
  }
}

void Graph::UpdateMaxPLToChild(int cIdx)
{
  queue<int> q;
  q.push(cIdx);
  int maxPL = 0;
  while (!q.empty()) {
    int cIdx = q.front();
    q.pop();
    for (int i = 0; i < nodes[cIdx].children.size(); ++i) {
      int ccIdx = nodes[cIdx].children[i];
      if (nodes[ccIdx].src_to_sink_dist > maxPL) {
        maxPL = nodes[ccIdx].src_to_sink_dist;
      }
      q.push(ccIdx);
    }
  }
  nodes[cIdx].maxPLToChild = max(maxPL - nodes[cIdx].src_to_sink_dist, 0);
}

void Graph::addChild(Node& node, int idx)
{
  auto& children = node.children;
  if (std::find(children.begin(), children.end(), idx) == children.end())
    children.push_back(idx);
}

void Graph::replaceParent(Node& pNode, int idx, int tIdx)
{
  if (pNode.parent == idx)
    pNode.parent = tIdx;
}

void Graph::replaceChild(Node& pNode, int idx, int tIdx)
{
  vector<int> newC = pNode.children;
  pNode.children.clear();
  for (int i = 0; i < newC.size(); ++i) {
    if (newC[i] != idx) {
      pNode.children.push_back(newC[i]);
    } else {
      pNode.children.push_back(tIdx);
    }
  }
}

void Graph::removeChild(Node& pNode, int idx)
{
  vector<int> newC = pNode.children;
  pNode.children.clear();
  for (int i = 0; i < newC.size(); ++i) {
    if (newC[i] != idx) {
      pNode.children.push_back(newC[i]);
    }
  }
}

int Graph::IdentLoc(int pIdx, int cIdx)
{
  Node& pN = nodes[pIdx];
  Node& cN = nodes[cIdx];

  if (pN.y > cN.y) {
    if (pN.x > cN.x) {
      return 0;  // lower left of pN
    } else if (pN.x < cN.x) {
      return 1;  // lower right of pN
    } else {
      return 2;  // lower of pN
    }
  } else if (pN.y < cN.y) {
    if (pN.x > cN.x) {
      return 3;  // upper left of pN
    } else if (pN.x < cN.x) {
      return 4;  // upper right of pN
    } else {
      return 5;  // upper of pN
    }
  } else  // pN.y == cN.y
  {
    if (pN.x > cN.x) {
      return 6;  // left of pN
    } else if (pN.x < cN.x) {
      return 7;  // right of pN
    } else {
      logger_->error(PDR, 119, "pN == cN ({})", pN);
      return 10;
    }
  }
}

/***************************************************************************/

static void make_unique(vector<Node>& vec)
{
  for (int a = 0; a < vec.size(); a++) {
    for (int b = 1; b < vec.size() - 1; b++) {
      if (a != b) {
        if ((vec[a].x == vec[b].x) && (vec[a].y == vec[b].y)) {
          swap(vec[b], vec.back());
          vec.pop_back();
          b--;
        }
      }
    }
  }
}

/* comparison function for use in sort
 */
static bool comp_x(const Node& i, const Node& j)
{
  if (i.x == j.x)
    return (i.y < j.y);
  else
    return (i.x < j.x);
}

static bool comp_y(const Node& i, const Node& j)
{
  if (i.y == j.y)
    return (i.x < j.x);
  else
    return (i.y < j.y);
}

bool Graph::nodeLessY(int i, int j)
{
  int y1 = nodes[i].y;
  int y2 = nodes[j].y;
  if (y1 == y2)
    return nodes[i].x < nodes[j].x;
  else
    return y1 < y2;
}

// Guibas-Stolfi algorithm for computing nearest NE (north-east) neighbors
// This has no resemblance to any Guibas-Stolfi algorithm that I find. -cherry
// 06/18/2021 SPT is student double secret code for spanning tree.
void Graph::buildNearestNeighborsForSPT()
{
  nn_.clear();
  int node_count = nodes.size();
  nn_.resize(node_count);

  vector<int> urux;
  vector<int> urlx;
  vector<int> ulux;
  vector<int> ullx;
  vector<int> lrux;
  vector<int> lrlx;
  vector<int> llux;
  vector<int> lllx;

  sorted_.clear();
  for (int i = 0; i < node_count; ++i) {
    sorted_.push_back(nodes[i].idx);
    urux.push_back(std::numeric_limits<int>::max());
    urlx.push_back(nodes[i].x);
    ulux.push_back(nodes[i].x);
    ullx.push_back(std::numeric_limits<int>::min());
    lrux.push_back(std::numeric_limits<int>::max());
    lrlx.push_back(nodes[i].x);
    llux.push_back(nodes[i].x);
    lllx.push_back(std::numeric_limits<int>::min());
  }

  // sort in y-axis
  sort(sorted_.begin(), sorted_.end(), [=](int i, int j) {
    return nodeLessY(i, j);
  });
  // sorted now has indicies of nodes sorted by y

  // collect neighbor
  // Node uu/ur/ll/lr are named assbackward. -cherry 06/18/2021
  for (int idx = 0; idx < node_count; ++idx) {
    Node& node = nodes[sorted_[idx]];
    debugPrint(logger_,
               PDR,
               "pdrev",
               3,
               "sorted by y {} ({} {})",
               node.idx,
               node.x,
               node.y);
    // Consider all nodes below node
    for (int i = 0; i < idx; ++i) {
      Node& below = nodes[sorted_[i]];
      // below.y <= node.y
      debugPrint(logger_,
                 PDR,
                 "pdrev",
                 3,
                 " below {} ({} {})",
                 below.idx,
                 below.x,
                 below.y);
      if (node.x >= urlx[below.idx] && node.x < urux[below.idx]) {
        debugPrint(logger_,
                   PDR,
                   "pdrev",
                   3,
                   " node {} neighbor {} upper right",
                   below.idx,
                   node.idx);
        // right
        nn_[below.idx].push_back(node.idx);
        urux[below.idx] = node.x;
      } else if (node.x > ullx[below.idx] && node.x < ulux[below.idx]) {
        // left
        debugPrint(logger_,
                   PDR,
                   "pdrev",
                   3,
                   " node {} neighbor {} upper left",
                   below.idx,
                   node.idx);
        nn_[below.idx].push_back(node.idx);
        ullx[below.idx] = node.x;
      }
    }

    // update neighbor to idx
    // Note: below.y <= node.y
    for (int i = idx - 1; i >= 0; --i) {
      Node& below = nodes[sorted_[i]];
      debugPrint(logger_,
                 PDR,
                 "pdrev",
                 3,
                 " below {} ({} {})",
                 node.idx,
                 node.x,
                 node.y);
      if (below.x >= lrlx[node.idx] && below.x < lrux[node.idx]) {
        // right
        debugPrint(logger_,
                   PDR,
                   "pdrev",
                   3,
                   " node {} neighbor {} lower right",
                   node.idx,
                   below.idx);
        nn_[node.idx].push_back(below.idx);
        lrux[node.idx] = below.x;
      } else if (below.x > lllx[node.idx] && below.x < llux[node.idx]) {
        // left
        debugPrint(logger_,
                   PDR,
                   "pdrev",
                   3,
                   " node {} neighbor {} lower left",
                   node.idx,
                   below.idx);
        nn_[node.idx].push_back(below.idx);
        lllx[node.idx] = below.x;
      }
    }
  }
  // Print neighbors.
  if (logger_->debugCheck(PDR, "pdrev", 3)) {
    debugPrint(logger_, PDR, "pdrev", 3, "Neighbors");
    for (int i = 0; i < node_count; ++i) {
      debugPrint(logger_, PDR, "pdrev", 3, "node {}", nodes[i]);
      for (int j = 0; j < nn_[i].size(); ++j) {
        debugPrint(logger_, PDR, "pdrev", 3, " {}", nodes[nn_[i][j]]);
      }
    }
  }
}

void Graph::heap_insert(int p, float key)
{
  int k; /* hole in the heap     */
  int j; /* parent of the hole   */
  int q; /* heap_elt(j)          */

  heap_key_[p] = key;

  if (heap_size_ == 0) {
    heap_size_ = 1;
    heap_elt_[1] = p;
    heap_idx_[p] = 1;
    debugPrint(logger_,
               PDR,
               "pdrev",
               3,
               "    heap_elt[1]= {}  heap_idx[p]= {}",
               heap_elt_[1],
               heap_idx_[p]);
  } else {
    k = ++heap_size_;
    j = k >> 1; /* k/2 */

    debugPrint(logger_,
               PDR,
               "pdrev",
               3,
               "    k= {} j= {}  heap_elt[j]= {}  heap_key[q]= {}",
               k,
               j,
               heap_elt_[j],
               heap_key_[heap_elt_[j]]);

    q = heap_elt_[j];
    while ((j > 0) && (heap_key_[q] > key)) {
      heap_elt_[k] = q;
      heap_idx_[q] = k;
      k = j;
      j = k >> 1; /* k/2 */
      q = heap_elt_[j];

      debugPrint(logger_,
                 PDR,
                 "pdrev",
                 3,
                 "    k= {} j= {}  heap_elt[j]= {}  heap_key[q]= {}",
                 k,
                 j,
                 heap_elt_[j],
                 heap_key_[heap_elt_[j]]);
    }

    /* store p in the position of the hole */
    heap_elt_[k] = p;
    heap_idx_[p] = k;
  }
}

// Returns element index min key.
int Graph::heap_delete_min()
{
  int min, last;
  int k;       /* hole in the heap     */
  int j;       /* child of the hole    */
  float l_key; /* key of last point    */

  if (heap_size_ == 0) /* heap is empty */
    return (-1);

  min = heap_elt_[1];
  last = heap_elt_[heap_size_--];
  l_key = heap_key_[last];

  k = 1;
  j = 2;
  while (j <= heap_size_) {
    if (heap_key_[heap_elt_[j]] > heap_key_[heap_elt_[j + 1]])
      j++;

    if (heap_key_[heap_elt_[j]] >= l_key)
      break; /* found a position to insert 'last' */

    /* else, sift hole down */
    heap_elt_[k] = heap_elt_[j]; /* Note that j <= _heap_size */
    heap_idx_[heap_elt_[k]] = k;
    k = j;
    j = k << 1;

    debugPrint(logger_,
               PDR,
               "pdrev",
               3,
               "    k= {} j= {}  heap_size= {}",
               k,
               j,
               heap_size_);
  }

  debugPrint(
      logger_, PDR, "pdrev", 3, "    k= {} last= {}  min= {}", k, last, min);

  heap_elt_[k] = last;
  heap_idx_[last] = k;

  heap_idx_[min] = -1; /* mark the point visited */
  return min;
}

void Graph::heap_decrease_key(int p, float new_key)
{
  int k; /* hole in the heap     */
  int j; /* parent of the hole   */
  int q; /* heap_elt(j)          */

  heap_key_[p] = new_key;
  k = heap_idx_[p];
  j = k >> 1; /* k/2 */

  q = heap_elt_[j];
  if ((j > 0) && (heap_key_[q] > new_key)) { /* change is needed */
    do {
      heap_elt_[k] = q;
      heap_idx_[q] = k;
      k = j;
      j = k >> 1; /* k/2 */
      q = heap_elt_[j];
    } while ((j > 0) && (heap_key_[q] > new_key));

    /* store p in the position of the hole */
    heap_elt_[k] = p;
    heap_idx_[p] = k;
  }
}

void Graph::get_children_of_node()
{
  for (int j = 0; j < num_terminals; ++j) {
    nodes[j].children.clear();
    for (int k = 0; k < num_terminals; ++k) {
      if ((nodes[k].parent == j) && (k != nodes[root_idx_].idx) && (k != j)) {
        nodes[j].children.push_back(nodes[k].idx);
      }
    }
  }
}

void Graph::print_tree()
{
  for (size_t j = 0; j < nodes.size(); ++j) {
    Node& node = nodes[j];
    std::string children;
    for (int child : node.children)
      children += std::to_string(child) + " ";
    logger_->report("Node {} ({} {}) parent={} children={}",
                    j,
                    node.x,
                    node.y,
                    node.parent,
                    children);
  }
  for (auto i = 0; i < edges_.size(); i++) {
    Edge& edge = edges_[i];
    const char* shape = "?";
    if (edge.best_shape == 0)
      shape = "lower L";
    else if (edge.best_shape == 1)
      shape = "upper L";
    else if (edge.best_shape == 5)
      shape = "don't care";
    int head = edge.head;
    int tail = edge.tail;
    logger_->report("Edge ({} {}) {} -> ({} {}) {} shape={}",
                    nodes[head].x,
                    nodes[head].y,
                    head,
                    nodes[tail].x,
                    nodes[tail].y,
                    tail,
                    shape);
  }
}

int Graph::calc_tree_wl_pd()
{
  int wl = 0;
  for (int j = 0; j < nodes.size(); ++j) {
    int child = j;
    int par = nodes[j].parent;
    nodes[child].cost_edgeToP = dist(nodes[par], nodes[child]);
    wl += nodes[child].cost_edgeToP;
  }
  return wl;
}

int Graph::calc_tree_pl()
{
  int pl = 0;
  for (int j = 0; j < num_terminals; ++j) {
    int child = j;
    int par = nodes[j].parent;
    while (par != child) {
      debugPrint(
          logger_, PDR, "pdrev", 3, "Child = {} ; SV Par = {}", child, par);
      nodes[child].cost_edgeToP = dist(nodes[par], nodes[child]);
      pl += nodes[child].cost_edgeToP;
      child = par;
      par = nodes[par].parent;
    }
  }
  return pl;
}

float Graph::calc_tree_detour_cost()
{
  int detour_cost = 0;
  for (int j = 0; j < num_terminals; ++j)
    detour_cost += nodes[j].detour_cost_node;
  return detour_cost;
}

void Graph::run_PD_brute_force(float alpha)
{
  heap_size_ = 0;
  heap_key_.resize(num_terminals + 1);
  heap_idx_.resize(num_terminals + 1);
  heap_elt_.resize(num_terminals + 1);

  heap_insert(root_idx_, 0);
  nodes[root_idx_].parent = root_idx_;

  /* n points to be extracted from heap */
  for (int k = 0; k < num_terminals; k++) {
    int i = heap_delete_min();

    if (logger_->debugCheck(PDR, "pdrev", 3)) {
      std::string rpt = "\n############## k=" + std::to_string(k)
                        + " i=" + std::to_string(i) + "\n";
      rpt = rpt + "Heap_idx array: ";
      for (int ar = 0; ar < heap_idx_.size(); ar++)
        rpt = rpt + std::to_string(heap_idx_[ar]) + " ";
      rpt = rpt + "\n";
      rpt = rpt + "Heap_key array: ";
      for (int ar = 0; ar < heap_key_.size(); ar++)
        rpt = rpt + std::to_string(heap_key_[ar]) + " ";
      rpt = rpt + "\n";
      rpt = rpt + "Heap_elt array: ";
      for (int ar = 0; ar < heap_elt_.size(); ar++)
        rpt = rpt + std::to_string(heap_elt_[ar]) + " ";
      debugPrint(logger_, PDR, "pdrev", 3, "{}", rpt);
    }

    if (i >= 0) {
      // node[i] entered the tree, update heap keys for its neighbors
      int par = nodes[i].parent;
      int path_length = nodes[par].path_length + dist(nodes[i], nodes[par]);
      debugPrint(logger_,
                 PDR,
                 "pdrev",
                 3,
                 "path_length[{}] = path_length[{}] + dist = {} + {} = {}",
                 i,
                 par,
                 nodes[par].path_length,
                 dist(nodes[i], nodes[par]),
                 path_length);
      nodes[i].path_length = path_length;
      for (int oct = 0; oct < nn_[i].size(); oct++) {
        int nn1 = nn_[i][oct];
        if (heap_idx_[nn1] != -1) {
          int edge_length = (nn1 == root_idx_) ? 0 : dist(nodes[i], nodes[nn1]);
          float w = edge_length + path_length * alpha;
          debugPrint(logger_,
                     PDR,
                     "pdrev",
                     3,
                     "NN={} weight = edge_length + path_length * alpha = {} + "
                     "{} * {} = {}",
                     nn1,
                     edge_length,
                     path_length,
                     alpha,
                     w);
          debugPrint(logger_,
                     PDR,
                     "pdrev",
                     3,
                     " heap_idx[nn1]={} heap_key[nn1]={}",
                     heap_idx_[nn1],
                     heap_key_[nn1]);
          // FIXME : Tie-break
          if ((heap_idx_[nn1] > 0) && (w <= heap_key_[nn1])) {
            heap_decrease_key(nn1, w);
            nodes[nn1].parent = i;
          } else if (heap_idx_[nn1] == 0) {
            heap_insert(nn1, w);
            nodes[nn1].parent = i;
          }
        }
      }
    }
  }

  if (logger_->debugCheck(PDR, "pdrev", 1)) {
    debugPrint(logger_,
               PDR,
               "pdrev",
               1,
               "WL={} PL={}",
               calc_tree_wl_pd(),
               calc_tree_pl());
    print_tree();
  }
}

int Graph::IsAdded(Node& cN)
{
  for (int i = 1; i < nodes.size(); ++i) {
    if (nodes[i].x == cN.x && nodes[i].y == cN.y) {
      return i;
    }
  }
  return 0;
}

void Graph::FreeManhDist()
{
  for (int i = 0; i < manh_dist_.size(); ++i) {
    manh_dist_[i].clear();
  }
  manh_dist_.clear();
}

// Despite the name this does not appear to have much of anything in
// common with the referenced "HoVW" paper:
// "New Algorithms for the Rectilinear Steiner Tree Problem",
// JAN-MING HO, GOPALAKRISHNAN VIJAYAN, AND C. K. WONG
void Graph::doSteiner_HoVW()
{
  // Tree preparation
  for (int j = 0; j < num_terminals; ++j) /* For each terminal */ {
    int child = j;
    int par = nodes[j].parent;
    edges_[j].head = par;
    edges_[j].tail = child;
    update_edgecosts_to_parent(child, par);
  }

  get_children_of_node();
  get_level_in_tree();

  for (int j = 0; j < num_terminals; ++j) /* For each terminal */
    update_node_detcost_Kt(j);
  // End tree preparation

  debugPrint(logger_,
             PDR,
             "pdrev",
             3,
             "Wirelength before Steiner = {}",
             calc_tree_wl_pd());
  debugPrint(logger_,
             PDR,
             "pdrev",
             3,
             "Tree detour cost before Steiner = {}",
             calc_tree_detour_cost());
  debugPrint(logger_,
             PDR,
             "pdrev",
             3,
             "{} {}",
             calc_tree_wl_pd(),
             calc_tree_detour_cost());
  if (logger_->debugCheck(PDR, "pdrev", 3))
    print_tree();

  // Starting from the nodes in the second level from bottom
  for (int k = tree_struct_.size() - 3; k >= 0; k--) {
    for (int l = 0; l < tree_struct_[k].size(); l++) {
      int child = tree_struct_[k][l], par = nodes[child].parent;
      Node tmp_node = nodes[child];

      vector<Node> set_of_nodes;
      set_of_nodes.push_back(tmp_node);
      set_of_nodes.push_back(nodes[par]);
      for (int m = 0; m < tmp_node.children.size(); m++)
        set_of_nodes.push_back(nodes[tmp_node.children[m]]);

      if (set_of_nodes.size() > 2) {
        get_overlap_lshape(set_of_nodes, nodes[child].idx);
      }
    }
  }

  // Assigning best shapes top-down
  if (tree_struct_.size() >= 3) {
    for (int k = 0; k <= tree_struct_.size() - 3; k++) {
      for (int l = 0; l < tree_struct_[k].size(); l++) {
        int current_node = tree_struct_[k][l];
        if (nodes[current_node].children.size() > 0) {
          if (current_node == root_idx_) {
            for (int i = 0; i < edges_[current_node].lower_best_config.size();
                 i = i + 2)
              edges_[edges_[current_node].lower_best_config[i]].best_shape
                  = edges_[current_node].lower_best_config[i + 1];
          } else if (edges_[current_node].best_shape == 0) {
            for (int i = 0; i < edges_[current_node].lower_best_config.size();
                 i = i + 2)
              edges_[edges_[current_node].lower_best_config[i]].best_shape
                  = edges_[current_node].lower_best_config[i + 1];
          } else if (edges_[current_node].best_shape == 1) {
            for (int i = 0; i < edges_[current_node].upper_best_config.size();
                 i = i + 2)
              edges_[edges_[current_node].upper_best_config[i]].best_shape
                  = edges_[current_node].upper_best_config[i + 1];
          } else if (edges_[current_node].best_shape == 5) {
            for (int i = 0; i < edges_[current_node].upper_best_config.size();
                 i = i + 2)
              edges_[edges_[current_node].upper_best_config[i]].best_shape
                  = edges_[current_node].upper_best_config[i + 1];
          }
          // Condition for best_shape == 5? //SV: Not required since if current
          // edge shape is don't care, child config won't be set
        }
      }
    }
  }

  FreeManhDist();
  UpdateManhDist();
}

void Graph::generate_permutations(vector<vector<int>> lists,
                                  vector<vector<int>>& result,
                                  int depth,
                                  vector<int> current)
{
  if (depth == lists.size()) {
    result.push_back(current);  // result.add(current);
    return;
  }
  for (int i = 0; i < lists[depth].size(); ++i) {
    vector<int> tmp = current;
    tmp.push_back(lists[depth][i]);
    generate_permutations(lists, result, depth + 1, tmp);
  }
}

void Graph::get_overlap_lshape(vector<Node>& set_of_nodes, int index)
{
  vector<vector<int>> lists, result;
  vector<int> tmp1, tmp2;
  tmp1.push_back(0);
  tmp1.push_back(1);

  // Enumerate all possible options for children
  for (int i = 2; i < set_of_nodes.size(); i++)
    lists.push_back(tmp1);

  // This is horrifically inefficient in both memory and time.
  // It "counts" from 0 to list.size() using each index in the array as one bit,
  // so the result size is 2^lists.size() - exponential.  -cherry 06/18/2021
  generate_permutations(lists, result, 0, tmp2);
  // Lower of curr_edge
  // For each combination, calc overlap
  int max_overlap = 0;
  vector<int> best_config;
  vector<Node> best_sps_x, best_sps_y;
  int best_sps_current_node_idx_x = std::numeric_limits<int>::max();
  int best_sps_current_node_idx_y = std::numeric_limits<int>::max();

  for (int i = 0; i < result.size(); i++) {
    vector<vector<Node>> set_of_points;
    vector<Node> tmp3;
    tmp3.push_back(set_of_nodes[0]);
    tmp3.push_back(Node(0, set_of_nodes[1].x, set_of_nodes[0].y));
    tmp3.push_back(set_of_nodes[1]);
    set_of_points.push_back(tmp3);
    int lower_overlap = 0;
    vector<int> config;
    for (int j = 2; j < set_of_nodes.size(); j++) {
      if (result[i][j - 2] == 0) {
        vector<Node> tmp4;
        tmp4.push_back(set_of_nodes[0]);
        tmp4.push_back(Node(0, set_of_nodes[0].x, set_of_nodes[j].y));
        tmp4.push_back(set_of_nodes[j]);
        set_of_points.push_back(tmp4);
        tmp4.clear();
        lower_overlap += edges_[set_of_nodes[j].idx].lower_overlap;
        config.push_back(set_of_nodes[j].idx);
        config.push_back(0);
      } else if (result[i][j - 2] == 1) {
        vector<Node> tmp5;
        tmp5.push_back(set_of_nodes[0]);
        tmp5.push_back(Node(0, set_of_nodes[j].x, set_of_nodes[0].y));
        tmp5.push_back(set_of_nodes[j]);
        set_of_points.push_back(tmp5);
        tmp5.clear();
        lower_overlap += edges_[set_of_nodes[j].idx].upper_overlap;
        config.push_back(set_of_nodes[j].idx);
        config.push_back(1);
      }
    }

    // Calculation of overlaps
    int num_edges = set_of_points.size();
    int curr_level_overlap = calc_overlap(set_of_points);
    lower_overlap += curr_level_overlap;
    result[i].push_back(lower_overlap);
    if (lower_overlap >= max_overlap) {
      max_overlap = lower_overlap;
      best_config = config;
      best_sps_x.clear();
      best_sps_y.clear();
      best_sps_current_node_idx_x = std::numeric_limits<int>::max();
      best_sps_current_node_idx_y = std::numeric_limits<int>::max();
      if (set_of_points.size() != num_edges) {
        best_sps_x = set_of_points[num_edges];
        best_sps_current_node_idx_x = nodes[index].idx_of_current_node_x;
        best_sps_y = set_of_points[num_edges + 1];
        best_sps_current_node_idx_y = nodes[index].idx_of_current_node_y;
      }
    }
    for (int i = 0; i < set_of_points.size(); ++i)
      set_of_points[i].clear();
    set_of_points.clear();
    config.clear();
  }

  // New part added from here
  // Count of Max_overlap value appearing in the results combination
  int max_ap_cnt = 0;
  int res_size = result[0].size(), not_dont_care_flag = 0;
  vector<int> tmp_res, not_dont_care_child;
  for (int p = 0; p < result.size(); p++) {
    if (max_overlap == result[p][res_size - 1]) {
      max_ap_cnt++;
      // Set first row which matches as reference row
      if (max_ap_cnt == 1) {
        tmp_res = result[p];
      } else if (max_ap_cnt > 1) {
        for (int idk = 0; idk < res_size - 1; idk++) {
          if (res_size == 2) {
            not_dont_care_flag++;
          } else {
            if (result[p][idk] == tmp_res[idk]) {
              not_dont_care_flag++;
              not_dont_care_child.push_back(idk);
            }
          }
        }
      }
    }
  }
  int dont_care_flag = res_size - 1 - not_dont_care_flag;
  // Only if dont_care_flag = number of repeating rows - 1, then make the child
  // edge dont_care
  if ((dont_care_flag != 0) && (dont_care_flag == max_ap_cnt - 1)) {
    vector<int> List1;
    for (int mm = 0; mm < result[0].size() - 1; mm++)
      List1.push_back(mm);
    vector<int> dont_care_child;
    // Get dont care child index by removing the rest of the children indices
    copy_if(
        List1.begin(),
        List1.end(),
        back_inserter(dont_care_child),
        [&not_dont_care_child](const int& arg) {
          return (
              find(not_dont_care_child.begin(), not_dont_care_child.end(), arg)
              == not_dont_care_child.end());
        });

    if (best_config.size() != 0) {
      for (int mm = 0; mm < dont_care_child.size(); mm++) {
        best_config[dont_care_child[mm] * 2 + 1] = 5;
      }
    }
    dont_care_child.clear();
    List1.clear();
  }
  tmp_res.clear();
  not_dont_care_child.clear();

  edges_[index].lower_overlap = max_overlap;
  edges_[index].lower_best_config = best_config;
  edges_[index].lower_sps_to_be_added_x = best_sps_x;
  edges_[index].lower_sps_to_be_added_y = best_sps_y;

  edges_[index].lower_idx_of_current_node_x = best_sps_current_node_idx_x;
  edges_[index].lower_idx_of_current_node_y = best_sps_current_node_idx_y;
  best_config.clear();
  best_sps_x.clear();
  best_sps_y.clear();
  max_overlap = 0;
  for (int i = 0; i < result.size(); i++) {
    vector<vector<Node>> set_of_points;
    vector<Node> tmp3;
    tmp3.push_back(set_of_nodes[0]);
    tmp3.push_back(Node(0, set_of_nodes[0].x, set_of_nodes[1].y));
    tmp3.push_back(set_of_nodes[1]);
    set_of_points.push_back(tmp3);
    int upper_overlap = 0;
    vector<int> config;
    for (int j = 2; j < set_of_nodes.size(); j++) {
      if (result[i][j - 2] == 0) {
        vector<Node> tmp4;
        tmp4.push_back(set_of_nodes[0]);
        tmp4.push_back(Node(0, set_of_nodes[0].x, set_of_nodes[j].y));
        tmp4.push_back(set_of_nodes[j]);
        set_of_points.push_back(tmp4);
        upper_overlap += edges_[set_of_nodes[j].idx].lower_overlap;
        config.push_back(set_of_nodes[j].idx);
        config.push_back(0);
      } else if (result[i][j - 2] == 1) {
        vector<Node> tmp5;
        tmp5.push_back(set_of_nodes[0]);
        tmp5.push_back(Node(0, set_of_nodes[j].x, set_of_nodes[0].y));
        tmp5.push_back(set_of_nodes[j]);
        set_of_points.push_back(tmp5);
        upper_overlap += edges_[set_of_nodes[j].idx].upper_overlap;
        config.push_back(set_of_nodes[j].idx);
        config.push_back(1);
      }
    }

    // Calculation of overlaps
    int num_edges = set_of_points.size();
    int curr_level_overlap = calc_overlap(set_of_points);
    upper_overlap += curr_level_overlap;
    int last_res_idx = result[i].size() - 1;
    result[i][last_res_idx] = upper_overlap;
    if (upper_overlap >= max_overlap) {
      max_overlap = upper_overlap;
      best_config = config;
      best_sps_current_node_idx_x = std::numeric_limits<int>::max();
      best_sps_current_node_idx_y = std::numeric_limits<int>::max();
      best_sps_x.clear();
      best_sps_y.clear();
      if (set_of_points.size() != num_edges) {
        best_sps_x = set_of_points[num_edges];
        best_sps_current_node_idx_x = nodes[index].idx_of_current_node_x;
        best_sps_y = set_of_points[num_edges + 1];
        best_sps_current_node_idx_y = nodes[index].idx_of_current_node_y;
      }
    }
    for (int i = 0; i < set_of_points.size(); ++i)
      set_of_points[i].clear();
    set_of_points.clear();
    config.clear();
  }

  // New part added from here
  max_ap_cnt
      = 0;  // Count of Max_overlap value appearing in the results combination
  res_size = result[0].size();
  not_dont_care_flag = 0;
  for (int p = 0; p < result.size(); p++) {
    if (max_overlap == result[p][res_size - 1]) {
      max_ap_cnt++;
      if (max_ap_cnt == 1) {
        tmp_res = result[p];
      } else if (max_ap_cnt > 1) {
        for (int idk = 0; idk < res_size - 1; idk++) {
          if (res_size == 2) {
            not_dont_care_flag++;
          } else {
            if (result[p][idk] == tmp_res[idk]) {
              not_dont_care_flag++;
              not_dont_care_child.push_back(idk);
            }
          }
        }
      }
    }
  }
  dont_care_flag = res_size - 1 - not_dont_care_flag;

  // Only if dont_care_flag = number of repeating rows - 1, then make the child
  // edge dont_care
  if ((dont_care_flag != 0) && (dont_care_flag == max_ap_cnt - 1)
      && !best_config.empty()) {
    std::set<int> dont_care_children;
    for (int mm = 0; mm < result[0].size() - 1; mm++)
      dont_care_children.insert(mm);
    for (int dont_care_child : dont_care_children)
      best_config[dont_care_child * 2 + 1] = 5;
  }
  // New part added till here
  edges_[index].upper_overlap = max_overlap;
  edges_[index].upper_best_config = best_config;
  edges_[index].upper_sps_to_be_added_x = best_sps_x;
  edges_[index].upper_sps_to_be_added_y = best_sps_y;
  edges_[index].upper_idx_of_current_node_x = best_sps_current_node_idx_x;
  edges_[index].upper_idx_of_current_node_y = best_sps_current_node_idx_y;

  // Choosing the best
  if (edges_[index].lower_overlap > edges_[index].upper_overlap) {
    edges_[index].best_overlap = edges_[index].lower_overlap;
    edges_[index].best_shape = 0;
  } else if (edges_[index].lower_overlap < edges_[index].upper_overlap) {
    edges_[index].best_overlap = edges_[index].upper_overlap;
    edges_[index].best_shape = 1;
  } else {
    edges_[index].best_overlap = edges_[index].lower_overlap;
    edges_[index].best_shape = 5;
  }
}

bool Graph::segmentIntersection(pair<double, double> A,
                                pair<double, double> B,
                                pair<double, double> C,
                                pair<double, double> D,
                                pair<double, double>& out)
{
  double x, y;
  double a1 = B.second - A.second;
  double b1 = A.first - B.first;
  double c1 = a1 * (A.first) + b1 * (A.second);

  double a2 = D.second - C.second;
  double b2 = C.first - D.first;
  double c2 = a2 * (C.first) + b2 * (C.second);

  double determinant = a1 * b2 - a2 * b1;

  if (A == B && C == D && A != C) {
    return false;
  }

  if (determinant == 0) {
    if (A == B) {
      x = A.first;
      y = A.second;

      if (x <= max(C.first, D.first) && x >= min(C.first, D.first)
          && y <= max(C.second, D.second) && y >= min(C.second, D.second)) {
        std::pair<double, double> intersect(x, y);
        out = intersect;
        return true;
      }
    }

    if (C == D) {
      x = C.first;
      y = C.second;

      if (x <= max(A.first, B.first) && x >= min(A.first, B.first)
          && y <= max(A.second, B.second) && y >= min(A.second, B.second)) {
        std::pair<double, double> intersect(x, y);
        out = intersect;
        return true;
      }
    }

    if (A == C || A == D) {
      x = A.first;
      y = A.second;

      std::pair<double, double> intersect(x, y);
      out = intersect;
      return true;
    }

    if (B == C || B == D) {
      x = B.first;
      y = B.second;

      std::pair<double, double> intersect(x, y);
      out = intersect;
      return true;
    }

    return false;
  } else {
    x = (b2 * c1 - b1 * c2) / determinant;
    y = (a1 * c2 - a2 * c1) / determinant;

    if (x <= max(A.first, B.first) && x >= min(A.first, B.first)
        && y <= max(A.second, B.second) && y >= min(A.second, B.second)
        && x <= max(C.first, D.first) && x >= min(C.first, D.first)
        && y <= max(C.second, D.second) && y >= min(C.second, D.second)) {
      std::pair<double, double> intersect(x, y);
      out = intersect;
      return true;
    } else {
      return false;
    }
  }
}

void Graph::intersection(const std::vector<std::pair<double, double>>& l1,
                         const std::vector<std::pair<double, double>>& l2,
                         std::vector<std::pair<double, double>>& out)
{
  std::vector<std::pair<double, double>> tmpVec;
  for (int i = 0; i < l1.size() - 1; i++) {
    for (int j = 0; j < l2.size() - 1; j++) {
      std::pair<double, double> intersect;
      if (segmentIntersection(l1[i], l1[i + 1], l2[j], l2[j + 1], intersect)) {
        tmpVec.push_back(intersect);
      }
    }
  }

  out = tmpVec;
}

double Graph::length(const std::vector<std::pair<double, double>>& l)
{
  double totalLen = 0;

  if (l.size() <= 1) {
    return 0;
  }

  for (int i = 0; i < l.size() - 1; i++) {
    double tmpLen = std::sqrt((std::pow((l[i + 1].first - l[i].first), 2)
                               + std::pow((l[i + 1].second - l[i].second), 2)));
    totalLen += tmpLen;
  }

  return totalLen;
}

int Graph::calc_overlap(vector<vector<Node>>& set_of_nodes)
{
  int max_overlap = 0;
  typedef std::pair<double, double> s_point;
  Node current_node = set_of_nodes[0][0];
  vector<Node> all_pts, sorted_x, sorted_y;
  for (int i = 0; i < set_of_nodes.size(); i++) {
    const vector<Node>& n = set_of_nodes[i];

    const s_point pn0(n[0].x, n[0].y);
    const s_point pn1(n[1].x, n[1].y);
    const s_point pn2(n[2].x, n[2].y);

    const std::vector<s_point> line1{pn0, pn1, pn2};

    for (int j = i + 1; j < set_of_nodes.size(); j++) {
      const vector<Node>& m = set_of_nodes[j];

      const s_point pm0(m[0].x, m[0].y);
      const s_point pm1(m[1].x, m[1].y);
      const s_point pm2(m[2].x, m[2].y);

      const std::vector<s_point> line2{pm0, pm1, pm2};
      std::vector<s_point> output;

      intersection(line1, line2, output);

      //            Known Problem - output-double, when converting to int for
      //            output_nodes, 232 becomes 231 for some reason
      vector<Node> output_nodes;
      for (int g = 0; g < output.size(); g++)
        output_nodes.push_back(Node(0, output[g].first, output[g].second));

      make_unique(output_nodes);

      if (output_nodes.size() > 1) {
        for (int k = 0; k < output_nodes.size(); ++k) {
          // set flag if this node == current_node
          int current_node_flag = 0;
          if ((output_nodes[k].x == current_node.x)
              && (output_nodes[k].y == current_node.y)) {
            current_node_flag = 1;
          }
          // If not present in all_pts
          bool is_present = false;
          int all_pts_idx = std::numeric_limits<int>::max();
          for (int s = 0; s < all_pts.size(); s++) {
            if ((output_nodes[k].x == all_pts[s].x)
                && (output_nodes[k].y == all_pts[s].y)) {
              if (current_node_flag == 1) {
                is_present = true;
                all_pts_idx = s;
              }  // Add anyway if not current_node
            }
          }
          if (!is_present) {
            if (i == 0) {
              output_nodes[k].conn_to_parent = true;
              output_nodes[k].sp_chil.push_back(m[2].idx);
            } else {
              output_nodes[k].sp_chil.push_back(n[2].idx);
              output_nodes[k].sp_chil.push_back(m[2].idx);
            }
          } else {
            if (i == 0) {
              all_pts[all_pts_idx].conn_to_parent = true;
              all_pts[all_pts_idx].sp_chil.push_back(m[2].idx);
            } else {
              all_pts[all_pts_idx].sp_chil.push_back(n[2].idx);
              all_pts[all_pts_idx].sp_chil.push_back(m[2].idx);
            }
          }

          if (!is_present) {
            if (output_nodes.size() == 2) {
              if ((output_nodes[0].x == output_nodes[1].x)
                  || (output_nodes[0].y == output_nodes[1].y))
                all_pts.push_back(output_nodes[k]);
            } else {
              all_pts.push_back(output_nodes[k]);
            }
          }
        }
      }
    }
  }
  if (all_pts.size() > 1) {
    int position_of_current_node = std::numeric_limits<int>::max();
    for (int u = 0; u < all_pts.size(); u++)
      if ((all_pts[u].x == current_node.x)
          && (all_pts[u].y == current_node.y)) {
        position_of_current_node = u;
        break;
      }
    for (int u = 0; u < all_pts.size(); u++) {
      if (all_pts[u].x == all_pts[position_of_current_node].x)
        sorted_y.push_back(all_pts[u]);
      if (all_pts[u].y == all_pts[position_of_current_node].y)
        sorted_x.push_back(all_pts[u]);
    }
    sort(sorted_x.begin(), sorted_x.end(), comp_x);
    sort(sorted_y.begin(), sorted_y.end(), comp_y);
    for (int u = 0; u < sorted_x.size(); u++)
      if ((sorted_x[u].x == current_node.x)
          && (sorted_x[u].y == current_node.y)) {
        nodes[current_node.idx].idx_of_current_node_x = u;
        break;
      }
    for (int u = 0; u < sorted_y.size(); u++)
      if ((sorted_y[u].x == current_node.x)
          && (sorted_y[u].y == current_node.y)) {
        nodes[current_node.idx].idx_of_current_node_y = u;
        break;
      }
    set_of_nodes.push_back(sorted_x);
    set_of_nodes.push_back(sorted_y);
    int overlap_x = 0, overlap_y = 0;
    if (sorted_x.size() > 1) {
      overlap_x = calc_overlap_x_or_y(sorted_x, current_node, 'x');
    }
    if (sorted_y.size() > 1) {
      overlap_y = calc_overlap_x_or_y(sorted_y, current_node, 'y');
    }
    max_overlap = overlap_x + overlap_y;
  }
  all_pts.clear();
  return max_overlap;
}

int Graph::calc_overlap_x_or_y(const vector<Node>& sorted,
                               const Node& current_node,
                               const char tag)
{
  int overlap1 = 0, overlap2 = 0;
  vector<int> tmp_overlap, tmp;
  int index_of_current_node = 0;
  for (int i = 0; i < sorted.size(); i++) {
    // Getting position of "current_node"
    if ((sorted[i].x == current_node.x) && (sorted[i].y == current_node.y)) {
      index_of_current_node = i;
      break;
    }
  }
  if (index_of_current_node > 0) {
    int cnt = 0;
    for (int j = index_of_current_node - 1; j >= 0; j--) {
      tmp.push_back(0);
      if (tag == 'x')
        tmp[cnt] = sorted[j + 1].x - sorted[j].x;
      if (tag == 'y')
        tmp[cnt] = sorted[j + 1].y - sorted[j].y;
      cnt++;
    }
    cnt = 0;
    size_t s = tmp.size();
    for (int j = 0; j < s; j++) {
      tmp_overlap.push_back(0);
      tmp_overlap[j] = tmp[j] * (s - j);
    }
    for (int j = 0; j < s; j++)
      overlap1 += tmp_overlap[j];

    tmp_overlap.clear();
    tmp.clear();
  }
  if (index_of_current_node < (sorted.size() - 1)) {
    int cnt = 0;
    for (int j = index_of_current_node + 1; j <= sorted.size() - 1; j++) {
      tmp.push_back(0);
      if (tag == 'x')
        tmp[cnt] = sorted[j].x - sorted[j - 1].x;
      if (tag == 'y')
        tmp[cnt] = sorted[j].y - sorted[j - 1].y;
      cnt++;
    }
    cnt = 0;
    size_t s = tmp.size();
    for (int j = 0; j < s; j++) {
      tmp_overlap.push_back(0);
      tmp_overlap[j] = tmp[j] * (s - j);
    }
    for (int j = 0; j < s; j++)
      overlap2 += tmp_overlap[j];
    tmp_overlap.clear();
    tmp.clear();
  }
  tmp_overlap.clear();
  tmp.clear();
  return (overlap1 + overlap2);
}

void Graph::update_edgecosts_to_parent(int child, int par)
{
  nodes[child].cost_edgeToP = dist(nodes[par], nodes[child]);
  nodes[child].detcost_edgePToNode = nodes[par].min_dist
                                     + dist(nodes[par], nodes[child])
                                     - nodes[child].min_dist;
  nodes[child].detcost_edgeNodeToP = nodes[child].min_dist
                                     + dist(nodes[par], nodes[child])
                                     - nodes[par].min_dist;
}

void Graph::update_node_detcost_Kt(int j)
{
  int par = nodes[j].parent;
  int child = j;
  int count = 1;
  nodes[j].detour_cost_node = 0;
  while (par != child) {
    nodes[j].detour_cost_node += nodes[child].detcost_edgePToNode;
    nodes[par].K_t++;
    child = par;
    par = nodes[par].parent;
    count++;
    // WTF? -cherry 06/20/2021
    if (count > 1000)
      break;
  }
}

void Graph::get_level_in_tree()
{
  tree_struct_.clear();
  int level = 0;
  int j = root_idx_;
  tree_struct_.push_back({j});

  vector<int> children = nodes[j].children;
  tree_struct_.push_back(children);
  nodes[j].level = level;
  level++;

  while (!children.empty()) {
    vector<int> tmp = children;
    children.clear();
    for (int l = 0; l < tmp.size(); l++) {
      nodes[tmp[l]].level = level;
      children.insert(children.end(),
                      nodes[tmp[l]].children.begin(),
                      nodes[tmp[l]].children.end());
    }

    tree_struct_.push_back(children);
    level++;
  }
}

////////////////////////////////////////////////////////////////

// Segregated PDrevII code

#ifdef PDREVII

// increasing order
static bool comp_xi(const Node1& i, const Node1& j)
{
  if (i.x == j.x) {
    return (i.y < j.y);
  } else {
    return (i.x < j.x);
  }
}

// decreasing order
static bool comp_xd(const Node1& i, const Node1& j)
{
  if (i.x == j.x) {
    return (i.y < j.y);
  } else {
    return (i.x > j.x);
  }
}

// increasing order
static bool comp_yi(const Node1& i, const Node1& j)
{
  if (i.y == j.y) {
    return (i.x < j.x);
  } else {
    return (i.y < j.y);
  }
}

// decreasing order
static bool comp_yd(const Node1& i, const Node1& j)
{
  if (i.y == j.y) {
    return (i.x < j.x);
  } else {
    return (i.y > j.y);
  }
}

static bool comp_sw(const Node& i, const Node& j)
{
  if (i.y == j.y) {
    return (i.x > j.x);
  } else {
    return (i.y > j.y);
  }
}

static bool comp_se(const Node& i, const Node& j)
{
  if (i.y == j.y) {
    return (i.x < j.x);
  } else {
    return (i.y > j.y);
  }
}

static bool comp_nw(const Node& i, const Node& j)
{
  if (i.y == j.y) {
    return (i.x > j.x);
  } else {
    return (i.y < j.y);
  }
}

static bool comp_ne(const Node& i, const Node& j)
{
  if (i.y == j.y) {
    return (i.x < j.x);
  } else {
    return (i.y < j.y);
  }
}

static bool comp_en(const Node& i, const Node& j)
{
  if (i.x == j.x) {
    return (i.y < j.y);
  } else {
    return (i.x < j.x);
  }
}

static bool comp_es(const Node& i, const Node& j)
{
  if (i.x == j.x) {
    return (i.y > j.y);
  } else {
    return (i.x < j.x);
  }
}

static bool comp_wn(const Node& i, const Node& j)
{
  if (i.x == j.x) {
    return (i.y < j.y);
  } else {
    return (i.x > j.x);
  }
}

static bool comp_ws(const Node& i, const Node& j)
{
  if (i.x == j.x) {
    return (i.y > j.y);
  } else {
    return (i.x > j.x);
  }
}

void Graph::PDBU_new_NN(float alpha)
{
  alpha2_ = alpha;
  alpha3_ = 0;
  alpha4_ = 0;
  pl_margin_ = 1.1;
  beta_ = 1.4;
  distance_ = 2;
  max_pl_ = 0;
  aux_.resize(num_terminals);

  m_ = (1 + beta_ * (1 - alpha2_))
       / pow((num_terminals - 1), (beta_ * (1 - alpha2_)));

  buildNearestNeighborsForSPT();

  // Tree preparation
  for (int j = 0; j < num_terminals; ++j) /* For each terminal */ {
    int child = j;
    int par = nodes[j].parent;
    update_edgecosts_to_parent(child, par);
    debugPrint(logger_,
               PDR,
               "pdrev",
               3,
               "  Detour cost of edge to parent = {}",
               nodes[child].detcost_edgePToNode);
    debugPrint(
        logger_, PDR, "pdrev", 3, "  Nearest neighbors of node are {}", j);
    nodes[j].nn_edge_detcost.resize(nn_[j].size());

    // Calculating detour cost of all edges to nearest neighbours
    update_detourcosts_to_NNs(j);
  }
  get_children_of_node();

  // Calculating detour cost of each node and the K_t value
  for (int j = 0; j < num_terminals; ++j) /* For each terminal */
    update_node_detcost_Kt(j);
  // End tree preparation

  float initial_tree_cost = calc_tree_cost();
  float final_tree_cost = 0;
  float tree_cost_difference = final_tree_cost - initial_tree_cost;
  int count = 1;

  while ((tree_cost_difference > 0) || (tree_cost_difference < -1)) {
    debugPrint(logger_,
               PDR,
               "pdrev",
               3,
               "\n################# PDBU Swap Iteration {}"
               " #####################\n",
               count);

    count++;

    if (count >= num_terminals) {
      break;
    }

    if (logger_->debugCheck(PDR, "pdrev", 3)) {
      debugPrint(
          logger_, PDR, "pdrev", 3, "Tree before PDBU iteration {}", count - 1);
      print_tree();
      debugPrint(logger_,
                 PDR,
                 "pdrev",
                 3,
                 "Initial tree cost = {}",
                 initial_tree_cost);
    }

    // This is N^2 in the terminal count, and should probably be using
    // sets instead of 2D arrays. -cherry 05/03/2021
    // Generating the swap space
    for (int j = 0; j < num_terminals; ++j) /* For each terminal */ {
      nodes[j].swap_space.clear();
      vector<int> tmp_children = nodes[j].children;
      int iter = 0;
      vector<int> tmp;
      nodes[j].swap_space.push_back(tmp);
      nodes[j].swap_space[iter].insert(nodes[j].swap_space[iter].end(),
                                       tmp_children.begin(),
                                       tmp_children.end());
      iter++;
      while (iter <= num_terminals) {
        vector<int> tmp;
        nodes[j].swap_space.push_back(tmp);
        for (int k = 0; k < tmp_children.size(); k++) {
          int child = tmp_children[k];
          if (nodes[child].children.size() > 0) {
            nodes[j].swap_space[iter].insert(nodes[j].swap_space[iter].end(),
                                             nodes[child].children.begin(),
                                             nodes[child].children.end());
          }
        }
        tmp_children = nodes[j].swap_space[iter];
        iter++;
      }

      if (logger_->debugCheck(PDR, "pdrev", 3)) {
        debugPrint(logger_, PDR, "pdrev", 3, "j = {}  Swap space: ", j);
        std::string swapSpcRpt;
        for (int p = 0; p < nodes[j].swap_space.size(); p++) {
          for (int q = 0; q < nodes[j].swap_space[p].size(); q++) {
            swapSpcRpt
                = swapSpcRpt + std::to_string(nodes[j].swap_space[p][q]) + " ";
          }
          debugPrint(logger_, PDR, "pdrev", 3, "{}", swapSpcRpt);
        }
        debugPrint(logger_,
                   PDR,
                   "pdrev",
                   3,
                   "Swap space size = {}",
                   nodes[j].swap_space.size());
      }
    }

    // magic number alert -cherry 06/07/2021
    float overall_min_cost = 10000;
    int overall_min_node = -1;
    int overall_min_nn_idx = -1;
    int overall_swap_dist = -1;
    int overall_initial_i = 0;

    // Min node of the entire tree
    for (int j = 0; j < num_terminals; ++j) /* For each terminal */ {
      // For every row in the nodes[j].swap_space
      for (int iter = 0; iter <= distance_; iter++) {
        // For every element in the "iter"th row of nodes[j].swap_space
        for (int idx = 0; idx < nodes[j].swap_space[iter].size(); idx++) {
          float swap_cost = 10000;
          // Node to get new edge
          int node_e_new = nodes[j].swap_space[iter][idx];
          for (int oct = 0; oct < nn_[node_e_new].size(); oct++) {
            int nn_node = nn_[node_e_new][oct];
            bool is_found_in_swap_space = false;
            if (nn_node != -1) {
              int child = node_e_new, par = nodes[node_e_new].parent;
              for (int i = 0; i < nodes[child].swap_space.size(); i++) {
                for (int k = 0; k < nodes[child].swap_space[i].size(); k++) {
                  if (nn_node == nodes[child].swap_space[i][k]) {
                    is_found_in_swap_space = true;
                    break;
                  }
                }
                if (is_found_in_swap_space)
                  break;
              }
              if (!is_found_in_swap_space) {
                int count = 0;
                while (count < iter) {
                  for (int i = 0; i < nodes[par].swap_space.size(); i++) {
                    for (int k = 0; k < nodes[par].swap_space[i].size(); k++) {
                      if (nn_node == nodes[par].swap_space[i][k]) {
                        is_found_in_swap_space = true;
                        break;
                      }
                      if (nn_node == par) {
                        is_found_in_swap_space = true;
                        break;
                      }
                    }
                    if (is_found_in_swap_space)
                      break;
                  }
                  child = par;
                  par = nodes[par].parent;
                  count++;
                }
                if (nn_node == j)
                  is_found_in_swap_space = true;
              }
            }

            debugPrint(logger_,
                       PDR,
                       "pdrev",
                       3,
                       "j={} iter={} Node getting new edge={} NN_node={} Is "
                       "found in swap space? {}",
                       j,
                       iter,
                       node_e_new,
                       nn_node,
                       is_found_in_swap_space);

            if ((nn_node) > -1 && (nn_node != j) && !is_found_in_swap_space) {
              int child = node_e_new, par = nodes[node_e_new].parent;
              float sum_q_terms = nodes[node_e_new].nn_edge_detcost[oct];
              float cumul_detour_cost_term = 0;
              while (par != j) {
                sum_q_terms += nodes[child].detcost_edgeNodeToP;
                cumul_detour_cost_term
                    += (nodes[par].K_t - nodes[child].K_t)
                       * (nodes[nn_node].detour_cost_node + sum_q_terms
                          - nodes[par].detour_cost_node);
                child = par;
                par = nodes[par].parent;
                if ((child == 0) && (par == 0))
                  break;
              }
              // node_to_get_edge_removed
              int node_e_rem = child;
              int node_e_rem_par = j;
              float last_len_term
                  = (dist(nodes[nn_node], nodes[node_e_new])
                     - (dist(nodes[node_e_rem_par], nodes[node_e_rem])));
              float last_detour_cost_term
                  = nodes[node_e_new].K_t
                    * (nodes[nn_node].detour_cost_node
                       + nodes[node_e_new].nn_edge_detcost[oct]
                       - nodes[node_e_new].detour_cost_node);
              swap_cost = alpha2_ * m_
                              * (cumul_detour_cost_term + last_detour_cost_term)
                          + (1 - alpha2_) * (float) last_len_term;
              debugPrint(logger_,
                         PDR,
                         "pdrev",
                         3,
                         "j={} iter={} cost={}",
                         j,
                         iter,
                         swap_cost);
            }
            if (swap_cost < overall_min_cost) {
              overall_min_cost = swap_cost;
              overall_min_node = node_e_new;
              overall_min_nn_idx = oct;
              overall_swap_dist = iter;
              overall_initial_i = j;
            }
          }
        }
      }
      // End of the for loop which loops through each terminal
    }

    if (overall_min_cost < -0.05) {
      debugPrint(
          logger_,
          PDR,
          "pdrev",
          3,
          "Overall min = {}  for node {}  New parent (Nearest neighbour) is {}",
          overall_min_cost,
          overall_min_node,
          nn_[overall_min_node][overall_min_nn_idx]);

      debugPrint(logger_,
                 PDR,
                 "pdrev",
                 3,
                 "Swapping with a distance of {}",
                 overall_swap_dist);

      swap_and_update_tree(
          overall_min_node, overall_min_nn_idx, distance_, overall_initial_i);
    }

    final_tree_cost = calc_tree_cost();
    tree_cost_difference = final_tree_cost - initial_tree_cost;

    initial_tree_cost = final_tree_cost;
    debugPrint(
        logger_,
        PDR,
        "pdrev",
        3,
        "Final tree cost = {} \ntree_cost_difference = {}\n Tree after PDBU",
        final_tree_cost,
        tree_cost_difference);
    if (logger_->debugCheck(PDR, "pdrev", 3))
      print_tree();
  }
}

// The conjecture is that "dc" stands for "detour cost" -cherry 08/16/2021
void Graph::fix_max_dc()
{
  // This desparately needs to be factored -cherry 06/07/2020
  // DAG traversal
  BuildDAG();

  buildNearestNeighborsForSPT();

  UpdateAllEdgesNSEW();

  refineSteiner2();
  constructSteiner();

  FreeManhDist();
  UpdateManhDist();

  // DAG traversal
  BuildDAG();

  buildNearestNeighborsForSPT();
  UpdateAllEdgesNSEW();

  refineSteiner();

  constructSteiner();

  FreeManhDist();
  UpdateManhDist();

  // DAG traversal
  BuildDAG();

  buildNearestNeighborsForSPT();

  UpdateAllEdgesNSEW();

  refineSteiner();

  constructSteiner();

  RemoveUnneceSTNodes();

  FreeManhDist();
  UpdateManhDist();

  // DAG traversal
  BuildDAG();

  buildNearestNeighborsForSPT();

  UpdateAllEdgesNSEW();

  refineSteiner();

  constructSteiner();

  RemoveUnneceSTNodes();

  FreeManhDist();
  UpdateManhDist();

  // DAG traversal
  BuildDAG();

  buildNearestNeighborsForSPT();

  UpdateAllEdgesNSEW();

  refineSteiner();
  constructSteiner();

  float st_wl = calc_tree_wl_pd();

  int use_nn = 0;

  float init_wl = st_wl;
  for (int k = 1; k < tree_struct_1darr_.size(); k++) {
    int cnode = tree_struct_1darr_[k];
    float cnode_dc = (float) nodes[cnode].detour_cost_node;
    if (cnode_dc > 0) {
      if (use_nn == 1) {
        buildNearestNeighbors_single_node(cnode);
        update_detourcosts_to_NNs(cnode);
      }
      int cpar = nodes[cnode].parent;
      int edge_len_to_par = dist(nodes[cnode], nodes[cpar]);
      int new_edge_len, nn2, detour_cost_new_edge, size = 0, new_tree_wl = 0,
                                                   diff_in_wl, diff_in_dc = 0;
      float new_dc, min_dc = cnode_dc;
      int min_dc_nn = -1, min_dc_new_tree_wl = 0;

      if (use_nn == 1) {
        size = nn_[cnode].size();
      } else {
        size = k;
      }

      for (int ag = 0; ag < size; ag++) {
        if (use_nn == 1) {
          nn2 = nn_[cnode][ag];
        } else {
          nn2 = tree_struct_1darr_[ag];
        }
        if (nn2 != cpar) {
          new_edge_len = dist(nodes[cnode], nodes[nn2]);
          diff_in_wl = new_edge_len - edge_len_to_par;
          new_tree_wl = init_wl + diff_in_wl;
          if (((float) (new_tree_wl - st_wl) / (float) st_wl) <= alpha3_) {
            if (use_nn == 1) {
              new_dc = nodes[cnode].nn_edge_detcost[ag]
                       + nodes[nn2].detour_cost_node;
            } else {
              detour_cost_new_edge = nodes[nn2].min_dist
                                     + dist(nodes[nn2], nodes[cnode])
                                     - nodes[cnode].min_dist;
              new_dc = detour_cost_new_edge + nodes[nn2].detour_cost_node;
            }
            if (new_dc < cnode_dc) {
              if (new_dc < min_dc) {
                min_dc = new_dc;
                min_dc_nn = nn2;
                min_dc_new_tree_wl = new_tree_wl;
              }
            }
          }
        }
      }
      if (min_dc_nn != -1) {
        vector<int> chi = nodes[cnode].children, tmp;
        float init_dc = cnode_dc, final_dc = min_dc,
              change = init_dc - final_dc;
        int id = 0, count = 0;
        while (chi.size() != 0) {
          tmp = chi;
          chi.clear();
          for (id = 0; id < tmp.size(); id++) {
            init_dc += nodes[tmp[id]].detour_cost_node;
            count++;
            chi.insert(chi.end(),
                       nodes[tmp[id]].children.begin(),
                       nodes[tmp[id]].children.end());
          }
          tmp.clear();
        }
        final_dc = init_dc - count * change;
        chi.clear();
        tmp.clear();
        if ((init_dc - final_dc) / init_dc >= alpha4_) {
          nodes[cnode].parent = min_dc_nn;
          nodes[cnode].detour_cost_node = min_dc;
          diff_in_dc = cnode_dc - min_dc;
          init_wl = min_dc_new_tree_wl;
          // Update the DCs of the nodes in the subtree of k
          vector<int> chi = nodes[cnode].children, tmp;
          int id = 0;
          while (chi.size() != 0) {
            tmp = chi;
            chi.clear();
            for (id = 0; id < tmp.size(); id++) {
              nodes[tmp[id]].detour_cost_node -= diff_in_dc;
              chi.insert(chi.end(),
                         nodes[tmp[id]].children.begin(),
                         nodes[tmp[id]].children.end());
            }
            tmp.clear();
          }
          chi.clear();
          tmp.clear();
        }
      }
    }
  }
}

void Graph::update_detourcosts_to_NNs(int j)
{
  int child = j;
  for (int oct = 0; oct < nn_[child].size(); oct++) {
    int nn_node = nn_[child][oct];
    int detour_cost_nn_edge;
    if ((nn_node > -1) && (nodes[nn_node].parent != j)) {
      detour_cost_nn_edge = nodes[nn_node].min_dist
                            + dist(nodes[nn_node], nodes[child])
                            - nodes[child].min_dist;
    } else {
      detour_cost_nn_edge = 10000;
    }
    nodes[j].nn_edge_detcost[oct] = detour_cost_nn_edge;
  }
}

void Graph::swap_and_update_tree(int min_node,
                                 int nn_idx,
                                 int distance,
                                 int i_node)
{
  int child = min_node, par = nodes[child].parent;
  while ((par != i_node) && (par != 0)) {
    int tmp_par = nodes[par].parent;
    nodes[par].parent = child;
    update_edgecosts_to_parent(par, nodes[par].parent);
    child = par;
    par = tmp_par;
  }

  nodes[min_node].parent = nn_[min_node][nn_idx];

  update_edgecosts_to_parent(min_node, nodes[min_node].parent);

  for (int j = 0; j < num_terminals; ++j)
    nodes[j].K_t = 1;  // Resetting the K_t value for each node

  for (int j = 0; j < num_terminals; ++j) /* For each terminal */ {
    update_detourcosts_to_NNs(j);
    update_node_detcost_Kt(j);
  }
  get_children_of_node();
}

void Graph::RemoveUnneceSTNodes()
{
  vector<int> toBeRemoved;
  for (int i = num_terminals; i < nodes.size(); ++i) {
    if (nodes[i].children.size() < 2) {
      toBeRemoved.push_back(i);
    }
  }
  for (int i = toBeRemoved.size() - 1; i >= 0; --i) {
    Node& cN = nodes[toBeRemoved[i]];
    removeChild(nodes[cN.parent], cN.idx);
    for (int j = 0; j < cN.children.size(); ++j) {
      replaceParent(nodes[cN.children[j]], cN.idx, cN.parent);
      addChild(nodes[cN.parent], cN.children[j]);
      edges_[cN.children[j]].head = cN.parent;
    }
  }
  for (int i = toBeRemoved.size() - 1; i >= 0; --i) {
    nodes.erase(nodes.begin() + toBeRemoved[i]);
    edges_.erase(edges_.begin() + toBeRemoved[i]);
  }

  map<int, int> idxMap;
  for (int i = 0; i < nodes.size(); ++i) {
    debugPrint(logger_, PDR, "pdrev", 3, "idxMap {} {}", i, nodes[i].idx);
    idxMap[nodes[i].idx] = i;
  }
  for (int i = 0; i < nodes.size(); ++i) {
    Node& cN = nodes[i];
    for (int j = 0; j < toBeRemoved.size(); ++j) {
      removeChild(nodes[i], toBeRemoved[j]);
    }

    debugPrint(logger_, PDR, "pdrev", 3, "before cN: {}", cN);
    sort(cN.children.begin(), cN.children.end());
    for (int j = 0; j < cN.children.size(); ++j) {
      if (cN.children[j] != idxMap[cN.children[j]])
        replaceChild(cN, cN.children[j], idxMap[cN.children[j]]);
    }
    cN.idx = i;
    cN.parent = idxMap[cN.parent];
    edges_[i].tail = i;
    edges_[i].head = idxMap[edges_[i].head];
    edges_[i].idx = i;
    debugPrint(logger_, PDR, "pdrev", 3, "after cN: {}", cN);
  }
}

void Graph::refineSteiner2()
{
  debugPrint(logger_, PDR, "pdrev", 3, "Pre-refineSteiner() graph status: ");

  for (int i = dag_.size() - 1; i >= 0; --i) {
    Node& cN = nodes[dag_[i]];
    debugPrint(logger_, PDR, "pdrev", 3, "cN: {}", cN);
    if (cN.idx == 0)
      continue;

    // magic number alert -cherry
    int cGain = 1000000;
    int bestWLGain = cGain;
    int edgeShape = 1000;
    if (edges_[cN.idx].best_shape == 5) {
      cGain = ComputeWL(cN.idx, cN.parent, false, 0);
      if (cGain < bestWLGain) {
        bestWLGain = cGain;
        edgeShape = 0;
      }
      cGain = ComputeWL(cN.idx, cN.parent, true, 1);
      if (cGain < bestWLGain) {
        bestWLGain = cGain;
        edgeShape = 1;
      }
    } else {
      cGain = ComputeWL(cN.idx, cN.parent, true, edges_[cN.idx].best_shape);
      if (cGain < bestWLGain) {
        bestWLGain = cGain;
        edgeShape = edges_[cN.idx].best_shape;
      }
    }

    debugPrint(logger_, PDR, "pdrev", 3, "cGain: {}", cGain);
    int newParent = cN.parent;

    vector<int> neighbors = nn_[cN.idx];
    int newMaxPL = 0;
    int newPLToChildForParent = 0;

    if (cN.parent != newParent) {
      removeChild(nodes[cN.parent], cN.idx);
      addChild(nodes[newParent], cN.idx);
      cN.parent = newParent;
      nodes[newParent].maxPLToChild = newPLToChildForParent;
      edges_[cN.idx].head = newParent;
      cN.src_to_sink_dist = newMaxPL;
      if (newMaxPL > max_pl_)
        max_pl_ = newMaxPL;
    }
    AddNode(cN.idx, newParent, edgeShape);
    edges_[cN.idx].best_shape = edgeShape;
  }
  debugPrint(logger_, PDR, "pdrev", 3, "Post-refineSteiner() graph status: ");

  RemoveUnneceSTNodes();

  FreeManhDist();
  UpdateManhDist();

  // DAG traversal
  BuildDAG();

  buildNearestNeighborsForSPT();

  UpdateAllEdgesNSEW();

  debugPrint(
      logger_, PDR, "pdrev", 3, "Remove unnecessary Steiner nodes -- done");

  UpdateSteinerNodes();
  debugPrint(
      logger_, PDR, "pdrev", 3, "Post-updateSteinerNodes() graph status: ");
}

void Graph::refineSteiner()
{
  debugPrint(logger_, PDR, "pdrev", 3, "Pre-refineSteiner() graph status: ");

  for (int i = dag_.size() - 1; i >= 0; --i) {
    Node& cN = nodes[dag_[i]];
    debugPrint(logger_, PDR, "pdrev", 3, "cN: {}", cN);
    if (cN.idx == 0)
      continue;

    // magic number alert -cherry
    int cGain = 1000000;
    int bestWLGain = cGain;
    int edgeShape = 1000;
    if (edges_[cN.idx].best_shape == 5) {
      cGain = ComputeWL(cN.idx, cN.parent, false, 0);
      if (cGain < bestWLGain) {
        bestWLGain = cGain;
        edgeShape = 0;
      }
      cGain = ComputeWL(cN.idx, cN.parent, true, 1);
      if (cGain < bestWLGain) {
        bestWLGain = cGain;
        edgeShape = 1;
      }
    } else {
      cGain = ComputeWL(cN.idx, cN.parent, true, edges_[cN.idx].best_shape);
      if (cGain < bestWLGain) {
        bestWLGain = cGain;
        edgeShape = edges_[cN.idx].best_shape;
      }
    }

    debugPrint(logger_, PDR, "pdrev", 3, "cGain: {}", cGain);
    int newParent = cN.parent;

    vector<int> neighbors = nn_[cN.idx];
    int newMaxPL = 0;
    int newPLToChildForParent = 0;
    for (int j = 0; j < neighbors.size(); ++j) {
      Node& nNode = nodes[neighbors[j]];

      int newPLToChildForParentCandi = 0;
      int newMaxPLCandi = 0;
      debugPrint(logger_, PDR, "pdrev", 3, "nNode: {}", nNode);
      debugPrint(logger_,
                 PDR,
                 "pdrev",
                 3,
                 "IsSubTree: {}",
                 IsSubTree(cN.idx, nNode.idx));
      // is this neighbor node in sub-tree rooted by Node cN

      if (IsSubTree(cN.idx, nNode.idx) == false) {
        for (int i = 0; i < 2; ++i) {
          AddNode(cN.idx, nNode.idx, i);

          cGain = ComputeWL(cN.idx, nNode.idx, true, i);
          newMaxPLCandi = nNode.src_to_sink_dist + manh_dist_[cN.idx][nNode.idx]
                          + cN.maxPLToChild;
          if (IsSameDir(cN.idx, nNode.idx)) {
            newMaxPLCandi
                = newMaxPLCandi - 2 * (manh_dist_[cN.idx][nNode.idx] - cGain);
          }
          newPLToChildForParentCandi
              = max(nNode.maxPLToChild, newMaxPLCandi - nNode.src_to_sink_dist);
          debugPrint(logger_,
                     PDR,
                     "pdrev",
                     3,
                     "Computed gain: {} {}",
                     cGain,
                     bestWLGain);
          debugPrint(logger_,
                     PDR,
                     "pdrev",
                     3,
                     "newMaxPLCandi: {} {}",
                     newMaxPLCandi,
                     max_pl_ * pl_margin_);
          if (cGain < bestWLGain && newMaxPLCandi < max_pl_ * pl_margin_) {
            debugPrint(logger_,
                       PDR,
                       "pdrev",
                       3,
                       "new best pain: {} edgeShape: {} newParent: {} "
                       "newMaxPL: {} newPLToChild: {}",
                       cGain,
                       i,
                       nNode.idx,
                       newMaxPLCandi,
                       newPLToChildForParentCandi);
            bestWLGain = cGain;
            edgeShape = i;
            newParent = nNode.idx;
            newMaxPL = newMaxPLCandi;
            newPLToChildForParent = newPLToChildForParentCandi;
          }
        }
      }
    }

    if (cN.parent != newParent) {
      removeChild(nodes[cN.parent], cN.idx);
      addChild(nodes[newParent], cN.idx);
      cN.parent = newParent;
      nodes[newParent].maxPLToChild = newPLToChildForParent;
      edges_[cN.idx].head = newParent;
      cN.src_to_sink_dist = newMaxPL;
      if (newMaxPL > max_pl_)
        max_pl_ = newMaxPL;
    }
    AddNode(cN.idx, newParent, edgeShape);
    edges_[cN.idx].best_shape = edgeShape;
  }
  debugPrint(logger_, PDR, "pdrev", 3, "Post-refineSteiner() graph status: ");

  RemoveUnneceSTNodes();

  FreeManhDist();
  UpdateManhDist();

  // DAG traversal
  BuildDAG();

  buildNearestNeighborsForSPT();

  UpdateAllEdgesNSEW();

  debugPrint(
      logger_, PDR, "pdrev", 3, "Remove unnecessary Steiner nodes -- done");

  UpdateSteinerNodes();
  debugPrint(
      logger_, PDR, "pdrev", 3, "Post-updateSteinerNodes() graph status: ");
}

void Graph::UpdateSteinerNodes()
{
  BuildDAG();

  vector<Node> fSTNodes;
  for (int i = 0; i < dag_.size(); ++i) {
    int cIdx = dag_[i];
    Edge& e = edges_[cIdx];
    e.STNodes.clear();
    if (cIdx != 0) {
      debugPrint(logger_, PDR, "pdrev", 3, "cIdx = {}", cIdx);
      GetSteinerNodes(i, fSTNodes);
    }
  }

  UpdateEdges(fSTNodes);
}

void Graph::buildNearestNeighbors_single_node(int node_idx)
{
  vector<int> urux;
  vector<int> urlx;
  vector<int> ulux;
  vector<int> ullx;
  vector<int> lrux;
  vector<int> lrlx;
  vector<int> llux;
  vector<int> lllx;

  // sort in y-axis
  sort(sorted_.begin(), sorted_.end(), [=](int i, int j) {
    return nodeLessY(i, j);
  });

  int idx = 0;
  for (int abc = 0; abc < sorted_.size(); abc++)
    if (sorted_[abc] == node_idx) {
      idx = abc;
      break;
    }

  // collect neighbor
  Node& node = nodes[node_idx];
  // update idx to neighbors
  // Note: below.y <= node.y
  for (int i = 0; i < idx; ++i) {
    Node& below = nodes[sorted_[i]];
    if (node.x >= urlx[below.idx] && node.x < urux[below.idx]) {
      debugPrint(logger_,
                 PDR,
                 "pdrev",
                 3,
                 " node {} neighbor {} upper right",
                 below.idx,
                 node.idx);
      // right
      nn_[below.idx].push_back(node.idx);
      urux[below.idx] = node.x;
    } else if (node.x > ullx[below.idx] && node.x < ulux[below.idx]) {
      // left
      debugPrint(logger_,
                 PDR,
                 "pdrev",
                 3,
                 " node {} neighbor {} upper left",
                 below.idx,
                 node.idx);
      nn_[below.idx].push_back(node.idx);
      ullx[below.idx] = node.x;
    }
  }

  // update neighbor to idx
  // Note: below.y <= node.y
  for (int i = idx - 1; i >= 0; --i) {
    Node& below = nodes[sorted_[i]];
    if (below.x >= lrlx[node.idx] && below.x < lrux[node.idx]) {
      // right
      nn_[node.idx].push_back(below.idx);
      lrux[node.idx] = below.x;
    } else if (below.x > lllx[node.idx] && below.x < llux[node.idx]) {
      // left
      nn_[node.idx].push_back(below.idx);
      lllx[node.idx] = below.x;
    }
  }

  // Print neighbors.
  if (logger_->debugCheck(PDR, "pdrev", 3)) {
    debugPrint(logger_, PDR, "pdrev", 3, "Neighbors");
    debugPrint(logger_, PDR, "pdrev", 3, "node {}", nodes[node_idx]);
    for (int j = 0; j < nn_[node_idx].size(); ++j) {
      debugPrint(logger_, PDR, "pdrev", 3, " {}", nodes[nn_[node_idx][j]]);
    }
  }
}

int Graph::ComputeWL(int cIdx, int pIdx, bool isRemove, int eShape)
{
  int WL = 0;
  debugPrint(logger_, PDR, "pdrev", 3, "loc: {}", IdentLoc(pIdx, cIdx));
  debugPrint(logger_, PDR, "pdrev", 3, "pNode: {}", nodes[pIdx]);
  debugPrint(logger_, PDR, "pdrev", 3, "cNode: {}", nodes[cIdx]);

  switch (IdentLoc(pIdx, cIdx)) {
    case 0:
      WL += !eShape ? DeltaS(pIdx, cIdx, isRemove)
                    : DeltaW(pIdx, cIdx, isRemove);
      WL += !eShape ? DeltaE(cIdx, pIdx, isRemove)
                    : DeltaN(cIdx, pIdx, isRemove);
      break;
    case 1:
      WL += !eShape ? DeltaS(pIdx, cIdx, isRemove)
                    : DeltaE(pIdx, cIdx, isRemove);
      WL += !eShape ? DeltaW(cIdx, pIdx, isRemove)
                    : DeltaN(cIdx, pIdx, isRemove);
      break;
    case 2:
      WL += DeltaS(pIdx, cIdx, isRemove);
      DeltaN(cIdx, pIdx, isRemove);
      break;
    case 3:
      WL += !eShape ? DeltaN(pIdx, cIdx, isRemove)
                    : DeltaW(pIdx, cIdx, isRemove);
      WL += !eShape ? DeltaE(cIdx, pIdx, isRemove)
                    : DeltaS(cIdx, pIdx, isRemove);
      break;
    case 4:
      WL += !eShape ? DeltaN(pIdx, cIdx, isRemove)
                    : DeltaE(pIdx, cIdx, isRemove);
      WL += !eShape ? DeltaW(cIdx, pIdx, isRemove)
                    : DeltaS(cIdx, pIdx, isRemove);
      break;
    case 5:
      WL += DeltaS(cIdx, pIdx, isRemove);
      DeltaN(pIdx, cIdx, isRemove);
      break;
    case 6:
      WL += DeltaE(cIdx, pIdx, isRemove);
      DeltaW(pIdx, cIdx, isRemove);
      break;
    case 7:
      WL += DeltaW(cIdx, pIdx, isRemove);
      DeltaE(pIdx, cIdx, isRemove);
      break;
  }
  if (isRemove)
    removeNeighbor(pIdx, cIdx);

  return WL;
}

void Graph::UpdateNSEW(Edge& e)
{
  Node& pN = nodes[e.head];
  Node& cN = nodes[e.tail];

  // lower L --> update N, S for parent / E, W for child
  if (e.best_shape == 0) {
    switch (IdentLoc(pN.idx, cN.idx)) {
      case 0:
        pN.S.push_back(cN.idx);
        cN.E.push_back(pN.idx);
        break;
      case 1:
        pN.S.push_back(cN.idx);
        cN.W.push_back(pN.idx);
        break;
      case 2:
        pN.S.push_back(cN.idx);
        cN.N.push_back(pN.idx);
        break;
      case 3:
        pN.N.push_back(cN.idx);
        cN.E.push_back(pN.idx);
        break;
      case 4:
        pN.N.push_back(cN.idx);
        cN.W.push_back(pN.idx);
        break;
      case 5:
        pN.N.push_back(cN.idx);
        cN.S.push_back(pN.idx);
        break;
      case 6:
        pN.W.push_back(cN.idx);
        cN.E.push_back(pN.idx);
        break;
      case 7:
        pN.E.push_back(cN.idx);
        cN.W.push_back(pN.idx);
        break;
    }
  }  // upper L --> update E, W for parent / N, S for child
  else if (e.best_shape == 1) {
    switch (IdentLoc(pN.idx, cN.idx)) {
      case 0:
        pN.W.push_back(cN.idx);
        cN.N.push_back(pN.idx);
        break;
      case 1:
        pN.E.push_back(cN.idx);
        cN.N.push_back(pN.idx);
        break;
      case 2:
        pN.S.push_back(cN.idx);
        cN.N.push_back(pN.idx);
        break;
      case 3:
        pN.W.push_back(cN.idx);
        cN.S.push_back(pN.idx);
        break;
      case 4:
        pN.E.push_back(cN.idx);
        cN.S.push_back(pN.idx);
        break;
      case 5:
        pN.N.push_back(cN.idx);
        cN.S.push_back(pN.idx);
        break;
      case 6:
        pN.W.push_back(cN.idx);
        cN.E.push_back(pN.idx);
        break;
      case 7:
        pN.E.push_back(cN.idx);
        cN.W.push_back(pN.idx);
        break;
    }
  } else {
    switch (IdentLoc(pN.idx, cN.idx)) {
      case 0:
        pN.S.push_back(cN.idx);
        cN.E.push_back(pN.idx);
        pN.W.push_back(cN.idx);
        cN.N.push_back(pN.idx);
        break;
      case 1:
        pN.S.push_back(cN.idx);
        cN.W.push_back(pN.idx);
        pN.E.push_back(cN.idx);
        cN.N.push_back(pN.idx);
        break;
      case 2:
        pN.S.push_back(cN.idx);
        cN.N.push_back(pN.idx);
        break;
      case 3:
        pN.N.push_back(cN.idx);
        cN.E.push_back(pN.idx);
        pN.W.push_back(cN.idx);
        cN.S.push_back(pN.idx);
        break;
      case 4:
        pN.N.push_back(cN.idx);
        cN.W.push_back(pN.idx);
        pN.E.push_back(cN.idx);
        cN.S.push_back(pN.idx);
        break;
      case 5:
        pN.N.push_back(cN.idx);
        cN.S.push_back(pN.idx);
        break;
      case 6:
        pN.W.push_back(cN.idx);
        cN.E.push_back(pN.idx);
        break;
      case 7:
        pN.E.push_back(cN.idx);
        cN.W.push_back(pN.idx);
        break;
    }
  }
}

void Graph::SortE(Node& n)
{
  vector<Node1> tmpNode1;
  vector<int>& nList = n.E;
  for (int i = 0; i < nList.size(); ++i) {
    Node& cN = nodes[nList[i]];
    tmpNode1.push_back(Node1(cN.idx, cN.x, cN.y));
  }
  sort(tmpNode1.begin(), tmpNode1.end(), comp_xi);
  nList.clear();
  for (int i = 0; i < tmpNode1.size(); ++i) {
    nList.push_back(tmpNode1[i].idx);
  }
}

void Graph::SortW(Node& n)
{
  vector<Node1> tmpNode1;
  vector<int>& nList = n.W;
  for (int i = 0; i < nList.size(); ++i) {
    Node& cN = nodes[nList[i]];
    tmpNode1.push_back(Node1(cN.idx, cN.x, cN.y));
  }
  sort(tmpNode1.begin(), tmpNode1.end(), comp_xd);
  nList.clear();
  for (int i = 0; i < tmpNode1.size(); ++i) {
    nList.push_back(tmpNode1[i].idx);
  }
}

void Graph::SortS(Node& n)
{
  vector<Node1> tmpNode1;
  vector<int>& nList = n.S;
  for (int i = 0; i < nList.size(); ++i) {
    Node& cN = nodes[nList[i]];
    tmpNode1.push_back(Node1(cN.idx, cN.x, cN.y));
  }
  sort(tmpNode1.begin(), tmpNode1.end(), comp_yd);
  nList.clear();
  for (int i = 0; i < tmpNode1.size(); ++i) {
    nList.push_back(tmpNode1[i].idx);
  }
}

void Graph::SortN(Node& n)
{
  vector<Node1> tmpNode1;
  vector<int>& nList = n.N;
  for (int i = 0; i < nList.size(); ++i) {
    Node& cN = nodes[nList[i]];
    tmpNode1.push_back(Node1(cN.idx, cN.x, cN.y));
  }
  sort(tmpNode1.begin(), tmpNode1.end(), comp_yi);
  nList.clear();
  for (int i = 0; i < tmpNode1.size(); ++i) {
    nList.push_back(tmpNode1[i].idx);
  }
}

void Graph::SortAll(Node& n)
{
  SortN(n);
  SortS(n);
  SortE(n);
  SortW(n);
}

void Graph::UpdateAllEdgesNSEW()
{
  for (int i = 0; i < dag_.size(); ++i) {
    int cIdx = dag_[i];
    Node& cN = nodes[cIdx];
    cN.N.clear();
    cN.S.clear();
    cN.E.clear();
    cN.W.clear();
  }

  for (int i = 0; i < dag_.size(); ++i) {
    int cIdx = dag_[i];
    if (cIdx != 0) {
      UpdateNSEW(edges_[cIdx]);
    }
  }

  for (int i = 0; i < dag_.size(); ++i) {
    SortAll(nodes[dag_[i]]);
  }
}

int Graph::DeltaN(int idx, int rIdx, bool isRemove)
{
  Node& cN = nodes[idx];
  vector<int>& nList = cN.N;
  if (nList.size() != 0) {
    if (nList[nList.size() - 1] == rIdx) {
      Node& rN = nodes[rIdx];
      int ref = cN.y;
      if (nList.size() >= 2)
        ref = nodes[nList[nList.size() - 2]].y;
      int delta = abs(rN.y - ref);
      if (isRemove)
        nList.erase(nList.begin() + nList.size() - 1);
      return delta;
    }
    if (isRemove) {
      for (int i = 0; i < nList.size(); ++i) {
        if (nList[i] == rIdx) {
          nList.erase(nList.begin() + i);
          break;
        }
      }
    }
  }
  return 0;
}

int Graph::DeltaS(int idx, int rIdx, bool isRemove)
{
  Node& cN = nodes[idx];
  vector<int>& nList = cN.S;
  if (nList.size() != 0) {
    if (nList[nList.size() - 1] == rIdx) {
      Node& rN = nodes[rIdx];
      int ref = cN.y;
      if (nList.size() >= 2)
        ref = nodes[nList[nList.size() - 2]].y;
      int delta = abs(rN.y - ref);
      if (isRemove)
        nList.erase(nList.begin() + nList.size() - 1);
      return delta;
    }
    if (isRemove) {
      for (int i = 0; i < nList.size(); ++i) {
        if (nList[i] == rIdx) {
          nList.erase(nList.begin() + i);
          break;
        }
      }
    }
  }
  return 0;
}

int Graph::DeltaW(int idx, int rIdx, bool isRemove)
{
  Node& cN = nodes[idx];
  vector<int>& nList = cN.W;
  if (nList.size() != 0) {
    if (nList[nList.size() - 1] == rIdx) {
      Node& rN = nodes[rIdx];
      int ref = cN.x;
      if (nList.size() >= 2)
        ref = nodes[nList[nList.size() - 2]].x;
      int delta = abs(rN.x - ref);
      if (isRemove)
        nList.erase(nList.begin() + nList.size() - 1);
      return delta;
    }
    if (isRemove) {
      for (int i = 0; i < nList.size(); ++i) {
        if (nList[i] == rIdx) {
          nList.erase(nList.begin() + i);
          break;
        }
      }
    }
  }
  return 0;
}

int Graph::DeltaE(int idx, int rIdx, bool isRemove)
{
  Node& cN = nodes[idx];
  vector<int>& nList = cN.E;
  if (nList.size() != 0) {
    if (nList[nList.size() - 1] == rIdx) {
      Node& rN = nodes[rIdx];
      int ref = cN.x;
      if (nList.size() >= 2)
        ref = nodes[nList[nList.size() - 2]].x;
      int delta = abs(rN.x - ref);
      if (isRemove)
        nList.erase(nList.begin() + nList.size() - 1);
      return delta;
    }
    if (isRemove) {
      for (int i = 0; i < nList.size(); ++i) {
        if (nList[i] == rIdx) {
          nList.erase(nList.begin() + i);
          break;
        }
      }
    }
  }
  return 0;
}

void Graph::AddNode(int cIdx, int pIdx, int eShape)
{
  debugPrint(logger_,
             PDR,
             "pdrev",
             3,
             "loc: {} eShape: {}",
             IdentLoc(pIdx, cIdx),
             eShape);

  if (!eShape) {
    switch (IdentLoc(pIdx, cIdx)) {
      case 0:
        nodes[pIdx].S.push_back(cIdx);
        SortS(nodes[pIdx]);
        nodes[cIdx].E.push_back(pIdx);
        SortE(nodes[cIdx]);
        break;
      case 1:
        nodes[pIdx].S.push_back(cIdx);
        SortS(nodes[pIdx]);
        nodes[cIdx].W.push_back(pIdx);
        SortW(nodes[cIdx]);
        break;
      case 2:
        nodes[pIdx].S.push_back(cIdx);
        SortS(nodes[pIdx]);
        nodes[cIdx].N.push_back(pIdx);
        SortN(nodes[cIdx]);
        break;
      case 3:
        nodes[pIdx].N.push_back(cIdx);
        SortN(nodes[pIdx]);
        nodes[cIdx].E.push_back(pIdx);
        SortE(nodes[cIdx]);
        break;
      case 4:
        nodes[pIdx].N.push_back(cIdx);
        SortN(nodes[pIdx]);
        nodes[cIdx].W.push_back(pIdx);
        SortW(nodes[cIdx]);
        break;
      case 5:
        nodes[pIdx].N.push_back(cIdx);
        SortN(nodes[pIdx]);
        nodes[cIdx].S.push_back(pIdx);
        SortS(nodes[cIdx]);
        break;
      case 6:
        nodes[pIdx].W.push_back(cIdx);
        SortW(nodes[pIdx]);
        nodes[cIdx].E.push_back(pIdx);
        SortE(nodes[cIdx]);
        break;
      case 7:
        nodes[pIdx].E.push_back(cIdx);
        SortE(nodes[pIdx]);
        nodes[cIdx].W.push_back(pIdx);
        SortW(nodes[cIdx]);
        break;
    }
  } else {
    switch (IdentLoc(pIdx, cIdx)) {
      case 0:
        nodes[pIdx].W.push_back(cIdx);
        SortW(nodes[pIdx]);
        nodes[cIdx].N.push_back(pIdx);
        SortN(nodes[cIdx]);
        break;
      case 1:
        nodes[pIdx].E.push_back(cIdx);
        SortE(nodes[pIdx]);
        nodes[cIdx].N.push_back(pIdx);
        SortN(nodes[cIdx]);
        break;
      case 2:
        nodes[pIdx].S.push_back(cIdx);
        SortS(nodes[pIdx]);
        nodes[cIdx].N.push_back(pIdx);
        SortN(nodes[cIdx]);
        break;
      case 3:
        nodes[pIdx].W.push_back(cIdx);
        SortW(nodes[pIdx]);
        nodes[cIdx].S.push_back(pIdx);
        SortS(nodes[cIdx]);
        break;
      case 4:
        nodes[pIdx].E.push_back(cIdx);
        SortE(nodes[pIdx]);
        nodes[cIdx].S.push_back(pIdx);
        SortS(nodes[cIdx]);
        break;
      case 5:
        nodes[pIdx].N.push_back(cIdx);
        SortN(nodes[pIdx]);
        nodes[cIdx].S.push_back(pIdx);
        SortS(nodes[cIdx]);
        break;
      case 6:
        nodes[pIdx].W.push_back(cIdx);
        SortW(nodes[pIdx]);
        nodes[cIdx].E.push_back(pIdx);
        SortE(nodes[cIdx]);
        break;
      case 7:
        nodes[pIdx].E.push_back(cIdx);
        SortE(nodes[pIdx]);
        nodes[cIdx].W.push_back(pIdx);
        SortW(nodes[cIdx]);
        break;
    }
  }
}

void Graph::removeNeighbor(int pIdx, int cIdx)
{
  Node& pN = nodes[pIdx];
  Node& cN = nodes[cIdx];

  removeN(pN, cIdx);
  removeS(pN, cIdx);
  removeW(pN, cIdx);
  removeE(pN, cIdx);
  removeN(cN, pIdx);
  removeS(cN, pIdx);
  removeW(cN, pIdx);
  removeE(cN, pIdx);
}

void Graph::removeN(Node& pN, int idx)
{
  bool flag = false;
  int cIdx = 1000;
  vector<int>& nList = pN.N;
  for (int i = 0; i < nList.size(); ++i) {
    if (nList[i] == idx) {
      flag = true;
      cIdx = i;
      break;
    }
  }
  if (flag)
    nList.erase(nList.begin() + cIdx);
}

void Graph::removeS(Node& pN, int idx)
{
  bool flag = false;
  int cIdx = 1000;
  vector<int>& nList = pN.S;
  for (int i = 0; i < nList.size(); ++i) {
    if (nList[i] == idx) {
      flag = true;
      cIdx = i;
      break;
    }
  }
  if (flag)
    nList.erase(nList.begin() + cIdx);
}

void Graph::removeE(Node& pN, int idx)
{
  bool flag = false;
  int cIdx = 1000;
  vector<int>& nList = pN.E;
  for (int i = 0; i < nList.size(); ++i) {
    if (nList[i] == idx) {
      flag = true;
      cIdx = i;
      break;
    }
  }
  if (flag)
    nList.erase(nList.begin() + cIdx);
}

void Graph::removeW(Node& pN, int idx)
{
  bool flag = false;
  int cIdx = 1000;
  vector<int>& nList = pN.W;
  for (int i = 0; i < nList.size(); ++i) {
    if (nList[i] == idx) {
      flag = true;
      cIdx = i;
      break;
    }
  }
  if (flag)
    nList.erase(nList.begin() + cIdx);
}

void Graph::SortCNodes(vector<Node>& cNodes, int cIdx, int pIdx, int eShape)
{
  if (eShape == 0) {
    switch (IdentLoc(pIdx, cIdx)) {
      case 0:
        sort(cNodes.begin(), cNodes.end(), comp_sw);
        break;
      case 1:
        sort(cNodes.begin(), cNodes.end(), comp_se);
        break;
      case 2:
        sort(cNodes.begin(), cNodes.end(), comp_sw);
        break;
      case 3:
        sort(cNodes.begin(), cNodes.end(), comp_nw);
        break;
      case 4:
        sort(cNodes.begin(), cNodes.end(), comp_ne);
        break;
      case 5:
        sort(cNodes.begin(), cNodes.end(), comp_nw);
        break;
      case 6:
        sort(cNodes.begin(), cNodes.end(), comp_ws);
        break;
      case 7:
        sort(cNodes.begin(), cNodes.end(), comp_es);
        break;
    }
  } else {
    switch (IdentLoc(pIdx, cIdx)) {
      case 0:
        sort(cNodes.begin(), cNodes.end(), comp_ws);
        break;
      case 1:
        sort(cNodes.begin(), cNodes.end(), comp_es);
        break;
      case 2:
        sort(cNodes.begin(), cNodes.end(), comp_se);
        break;
      case 3:
        sort(cNodes.begin(), cNodes.end(), comp_wn);
        break;
      case 4:
        sort(cNodes.begin(), cNodes.end(), comp_en);
        break;
      case 5:
        sort(cNodes.begin(), cNodes.end(), comp_ne);
        break;
      case 6:
        sort(cNodes.begin(), cNodes.end(), comp_ws);
        break;
      case 7:
        sort(cNodes.begin(), cNodes.end(), comp_es);
        break;
    }
  }
}

void Graph::UpdateEdges(vector<Node>& STNodes)
{
  vector<int> idxs;
  for (int j = 0; j < STNodes.size(); ++j) {
    Node& cN = STNodes[j];
    for (int i = 0; i < dag_.size(); ++i) {
      Node& tN = nodes[dag_[i]];
      if (tN.x == cN.x && tN.y == cN.y) {
        idxs.push_back(j);
        break;
      }
    }
  }
  sort(idxs.begin(), idxs.end());
  reverse(idxs.begin(), idxs.end());
  for (int i = 0; i < idxs.size(); ++i) {
    STNodes.erase(STNodes.begin() + idxs[i]);
  }
  for (int i = 0; i < dag_.size(); ++i) {
    int cIdx = dag_[i];
    if (cIdx != 0) {
      vector<Node> cNodes;
      for (int j = 0; j < STNodes.size(); ++j) {
        bool flag = false;
        for (int k = 0; k < STNodes[j].sp_chil.size(); ++k) {
          if (cIdx == STNodes[j].sp_chil[k]) {
            flag = true;
            break;
          }
        }
        if (flag) {
          cNodes.push_back(STNodes[j]);
        }
      }

      Edge& e = edges_[cIdx];

      SortCNodes(cNodes, cIdx, nodes[cIdx].parent, e.best_shape);
      for (int i = 0; i < cNodes.size(); ++i) {
        e.STNodes.push_back(cNodes[i]);
      }
    }
  }
}

void Graph::constructSteiner()
{
  vector<int> newSP;
  for (int i = 1; i < dag_.size(); ++i) {
    int cIdx = dag_[i];
    Node child = nodes[cIdx];
    Node pN = nodes[child.parent];
    debugPrint(logger_, PDR, "pdrev", 3, "parent {} child: {}", pN, child);

    Edge e = edges_[cIdx];

    vector<int> toBeRemoved;
    for (int j = 0; j < e.STNodes.size(); ++j) {
      Node& cN = e.STNodes[j];
      if (!IsOnEdge(cN, cIdx)) {
        continue;
      }
      debugPrint(logger_, PDR, "pdrev", 3, "Before cN: {} pN: {}", cN, pN);
      int idx = IsAdded(cN);
      // check whether cN exists in the current nodes
      if (idx == 0) {
        // baffling magic number alert -cherry
        int pIdx = 10000;
        bool flagIdx = true;
        for (int k = 0; k < cN.sp_chil.size(); ++k) {
          if (pN.idx == cN.sp_chil[k]) {
            flagIdx = false;
            pIdx = nodes[pN.idx].parent;
          }
        }
        if (flagIdx) {
          pIdx = pN.idx;
        }

        // update current node cN
        nodes.push_back(Node(nodes.size(), cN.x, cN.y));
        edges_.push_back(Edge(edges_.size(), pIdx, edges_.size()));
        nodes[nodes.size() - 1].parent = pIdx;
        newSP.push_back(nodes.size() - 1);
        // update parent node pN
        for (int k = 0; k < cN.sp_chil.size(); ++k) {
          nodes[nodes.size() - 1].children.push_back(cN.sp_chil[k]);

          removeChild(nodes[pN.idx], cN.sp_chil[k]);
          if (flagIdx == false)
            removeChild(nodes[pIdx], cN.sp_chil[k]);
        }
        addChild(nodes[pIdx], nodes.size() - 1);
        debugPrint(logger_,
                   PDR,
                   "pdrev",
                   3,
                   "After cN: {}\nAfter pN: {}",
                   nodes[nodes.size() - 1],
                   nodes[pIdx]);
        pN = nodes[nodes.size() - 1];
      } else {
        if (nodes[idx].parent != pN.idx && idx != pN.parent
            && nodes[idx].parent != pN.parent) {
          logger_->warn(
              PDR, 120, "cNode ({}) != pNode ({})", nodes[idx], nodes[pN.idx]);

          for (int k = 0; k < newSP.size(); k++) {
            if (newSP[k] == idx) {
              removeChild(nodes[idx], pN.idx);
              addChild(nodes[nodes[pN.idx].parent], pN.idx);
              int dir1 = IdentLoc(pN.idx, idx);
              int dir2 = IdentLoc(nodes[idx].parent, idx);
              int dir3 = IdentLoc(pN.parent, idx);
              if (dir1 == dir2) {
                if ((nodes[idx].x < nodes[nodes[idx].parent].x
                     && nodes[pN.idx].x < nodes[nodes[idx].parent].x)
                    || (nodes[idx].x > nodes[nodes[idx].parent].x
                        && nodes[pN.idx].x > nodes[nodes[idx].parent].x)) {
                  debugPrint(logger_, PDR, "pdrev", 3, "same direction!!");

                  removeChild(nodes[idx], pN.idx);
                  removeChild(nodes[nodes[idx].parent], idx);

                  bool flag2 = true;
                  for (int l = 0; l < nodes[pN.idx].children.size(); ++l) {
                    if (nodes[pN.idx].children[l] == nodes[idx].parent) {
                      flag2 = false;
                      break;
                    }
                  }
                  if (flag2)
                    nodes[pN.idx].parent = nodes[idx].parent;
                  addChild(nodes[nodes[idx].parent], pN.idx);
                  nodes[idx].parent = pN.idx;
                  for (int l = 0; l < nodes[idx].children.size(); ++l) {
                    removeChild(nodes[pN.idx], nodes[idx].children[l]);
                  }
                  addChild(nodes[pN.idx], idx);
                  debugPrint(logger_, PDR, "pdrev", 3, "newSP: {}", nodes[idx]);
                }
              } else if (dir1 == dir3) {
                removeChild(nodes[idx], pN.idx);
                addChild(nodes[pN.parent], pN.idx);
                for (int k = 0; k < cN.sp_chil.size(); ++k) {
                  removeChild(nodes[pN.idx], cN.sp_chil[k]);
                }
              }
            }
          }
        }
        debugPrint(logger_,
                   PDR,
                   "pdrev",
                   3,
                   "After cN: {}\nAfter pN: {}",
                   nodes[idx],
                   nodes[pN.idx]);
        pN = nodes[idx];
      }
      if (j == e.STNodes.size() - 1) {
        nodes[child.idx].parent = pN.idx;
        edges_[child.idx].head = pN.idx;
        edges_[child.idx].best_shape = 5;
        for (int k = 0; k < cN.sp_chil.size(); ++k) {
          removeChild(nodes[child.idx], cN.sp_chil[k]);
        }
        addChild(nodes[pN.idx], child.idx);

        for (int k = 0; k < pN.children.size(); ++k) {
          removeChild(nodes[pN.children[k]], child.idx);
        }
      }
    }
  }

  FreeManhDist();
  UpdateManhDist();
  BuildDAG();
}

bool Graph::IsOnEdge(Node& tNode, int idx)
{
  Node& cNode = nodes[idx];
  Node& pNode = nodes[nodes[idx].parent];
  int maxX = max(cNode.x, pNode.x);
  int minX = min(cNode.x, pNode.x);
  int maxY = max(cNode.y, pNode.y);
  int minY = min(cNode.y, pNode.y);

  debugPrint(logger_, PDR, "pdrev", 3, "cNode: {}", cNode);
  debugPrint(logger_, PDR, "pdrev", 3, "pNode: {}", pNode);

  if (edges_[idx].best_shape == 0) {
    if (((pNode.x == tNode.x && (tNode.y <= maxY && tNode.y >= minY))
         || ((cNode.y == tNode.y) && (tNode.x <= maxX && tNode.x >= minX)))) {
      debugPrint(logger_, PDR, "pdrev", 3, "True");
      return true;
    }
  } else {
    if (((cNode.x == tNode.x && (tNode.y <= maxY && tNode.y >= minY))
         || ((pNode.y == tNode.y) && (tNode.x <= maxX && tNode.x >= minX)))) {
      debugPrint(logger_, PDR, "pdrev", 3, "True");
      return true;
    }
  }

  return false;
}

void Graph::BuildDAG()
{
  vector<bool> isVisited(nodes.size(), false);
  dag_.clear();
  // Build dag
  queue<int> q;
  q.push(0);
  while (!q.empty()) {
    int cIdx = q.front();
    if (isVisited[cIdx]) {
      int cParent = nodes[cIdx].parent;
      for (int i = 0; i < nodes.size(); ++i) {
        if (i != cParent) {
          removeChild(nodes[i], cIdx);
        }
      }
      addChild(nodes[cParent], cIdx);
      q.pop();
    } else {
      isVisited[cIdx] = true;
      dag_.push_back(cIdx);
      if (cIdx != 0) {
        nodes[cIdx].src_to_sink_dist
            = nodes[nodes[cIdx].parent].src_to_sink_dist
              + manh_dist_[cIdx][nodes[cIdx].parent];
      }
      q.pop();
      for (int i = 0; i < nodes[cIdx].children.size(); ++i) {
        q.push(nodes[cIdx].children[i]);
      }
    }
  }

  for (int i = 0; i < dag_.size(); ++i) {
    if (nodes[dag_[i]].src_to_sink_dist > max_pl_) {
      max_pl_ = nodes[dag_[i]].src_to_sink_dist;
    }
    UpdateMaxPLToChild(dag_[i]);
  }
}

float Graph::calc_tree_cost()
{
  float tree_cost = 0;
  int detour_cost = 0, wl = 0;
  for (int j = 0; j < num_terminals; ++j) {
    wl += abs(nodes[j].cost_edgeToP);
    detour_cost += nodes[j].detour_cost_node;
  }
  tree_cost = alpha2_ * m_ * (float) detour_cost + (1 - alpha2_) * (float) wl;
  return (tree_cost);
}

bool Graph::IsSubTree(int cIdx, int tIdx)
{
  queue<int> q;
  q.push(cIdx);
  while (!q.empty()) {
    int cIdx = q.front();
    q.pop();
    for (int i = 0; i < nodes[cIdx].children.size(); ++i) {
      q.push(nodes[cIdx].children[i]);
      if (tIdx == nodes[cIdx].children[i])
        return true;
    }
  }

  return false;
}

void Graph::GetSteiner(int cIdx, int nIdx, vector<Node>& STNodes)
{
  int pIdx = nodes[cIdx].parent;
  Node corner1 = GetCornerNode(cIdx);
  debugPrint(logger_, PDR, "pdrev", 3, "{} corner 1: {}", cIdx, corner1);
  if (IsOnEdge(corner1, nIdx)
      && (corner1.x != nodes[cIdx].x || corner1.y != nodes[cIdx].y)
      && (corner1.x != nodes[pIdx].x || corner1.y != nodes[pIdx].y)) {
    corner1.conn_to_parent = true;
    corner1.sp_chil.push_back(cIdx);
    corner1.sp_chil.push_back(nIdx);
    STNodes.push_back(corner1);
  }
  Node corner2 = GetCornerNode(nIdx);
  debugPrint(logger_, PDR, "pdrev", 3, "{} corner2: {}", nIdx, corner2);
  if (IsOnEdge(corner2, cIdx)
      && (corner1.x != corner2.x || corner1.y != corner2.y)
      && (corner2.x != nodes[cIdx].x || corner2.y != nodes[cIdx].y)
      && (corner2.x != nodes[pIdx].x || corner2.y != nodes[pIdx].y)) {
    corner2.conn_to_parent = true;
    corner2.sp_chil.push_back(cIdx);
    corner2.sp_chil.push_back(nIdx);
    STNodes.push_back(corner2);
  }
}

void Graph::GetSteinerNodes(int idx, vector<Node>& fSTNodes)
{
  debugPrint(logger_, PDR, "pdrev", 3, "cur fSTNode size: {}", fSTNodes.size());

  int cIdx = dag_[idx];
  vector<Node> STNodes;
  for (int i = idx + 1; i < dag_.size(); ++i) {
    int nIdx = dag_[i];
    debugPrint(logger_,
               PDR,
               "pdrev",
               3,
               "dag.size: {} i = {} nIdx: {}",
               dag_.size(),
               i,
               nIdx);
    if (IsParent(nodes[cIdx].parent, nIdx)) {
      GetSteiner(cIdx, nIdx, STNodes);
    }
  }

  for (int i = 0; i < fSTNodes.size(); ++i) {
    STNodes.push_back(fSTNodes[i]);
  }

  if (logger_->debugCheck(PDR, "pdrev", 3)) {
    for (int i = 0; i < STNodes.size(); ++i) {
      debugPrint(logger_,
                 PDR,
                 "pdrev",
                 3,
                 "Before dupRemoval STNodes: {}",
                 STNodes[i]);

      std::string childRpt = " Child: ";
      for (int j = 0; j < STNodes[i].sp_chil.size(); ++j) {
        childRpt = childRpt + std::to_string(STNodes[i].sp_chil[j]) + " ";
      }
      debugPrint(logger_, PDR, "pdrev", 3, "{}", childRpt);
    }
  }

  // post-processing
  DupRemoval(STNodes);

  if (logger_->debugCheck(PDR, "pdrev", 3)) {
    for (int i = 0; i < STNodes.size(); ++i) {
      debugPrint(
          logger_, PDR, "pdrev", 3, "After dupRemoval STNodes: {}", STNodes[i]);
      std::string childRpt = " Child: ";
      for (int j = 0; j < STNodes[i].sp_chil.size(); ++j) {
        childRpt = childRpt + std::to_string(STNodes[i].sp_chil[j]) + " ";
      }
      debugPrint(logger_, PDR, "pdrev", 3, "{}", childRpt);
    }
  }

  fSTNodes.clear();
  for (int i = 0; i < STNodes.size(); ++i) {
    fSTNodes.push_back(STNodes[i]);
  }
}

void Graph::DupRemoval(vector<Node>& STNodes)
{
  vector<Node> optSTNodes;
  for (int i = 0; i < STNodes.size(); ++i) {
    bool IsDup = false;
    int tIdx;
    for (int j = 0; j < optSTNodes.size(); ++j) {
      if (STNodes[i].x == optSTNodes[j].x && STNodes[i].y == optSTNodes[j].y) {
        tIdx = j;
        IsDup = true;
        break;
      }
    }
    if (IsDup) {
      vector<int> sp = optSTNodes[tIdx].sp_chil;
      for (int k1 = 0; k1 < STNodes[i].sp_chil.size(); ++k1) {
        bool IsDupN = false;
        for (int k2 = 0; k2 < sp.size(); ++k2) {
          if (STNodes[i].sp_chil[k1] == sp[k2]) {
            IsDupN = true;
            break;
          }
        }
        if (!IsDupN) {
          optSTNodes[tIdx].sp_chil.push_back(STNodes[i].sp_chil[k1]);
        }
      }
    } else {
      optSTNodes.push_back(STNodes[i]);
    }
  }
  STNodes.clear();
  for (int i = 0; i < optSTNodes.size(); ++i) {
    STNodes.push_back(optSTNodes[i]);
  }
}

Node Graph::GetCornerNode(int cIdx)
{
  Edge& e = edges_[cIdx];
  int shape = e.best_shape;

  if (shape == 0) {
    //
    return (Node(100000, nodes[e.head].x, nodes[e.tail].y));
  } else {
    return (Node(100000, nodes[e.tail].x, nodes[e.head].y));
  }
}

bool Graph::IsParent(int cIdx, int nIdx)
{
  int idx = nIdx;
  if (cIdx == 0)
    return true;

  while (idx != 0) {
    if (idx == cIdx)
      return true;
    else
      idx = nodes[idx].parent;
  }
  return false;
}

void Graph::PrintInfo()
{
  std::string dagRpt = "  Dag:";
  for (int i = 0; i < dag_.size(); ++i) {
    dagRpt = dagRpt + " " + std::to_string(i);
  }

  debugPrint(logger_, PDR, "pdrev", 3, "{}", dagRpt);

  std::string nnRpt = "  Nearest Neighbors:\n";
  for (int i = 0; i < nn_.size(); ++i) {
    nnRpt = nnRpt + "     " + std::to_string(i) + " -- ";
    for (int j = 0; j < nn_[i].size(); ++j) {
      nnRpt = nnRpt + std::to_string(nn_[i][j]) + " ";
    }
    nnRpt = nnRpt + "\n";
  }

  debugPrint(logger_, PDR, "pdrev", 3, "{}", nnRpt);

  debugPrint(logger_, PDR, "pdrev", 3, "  max_pl: {}", max_pl_);

  std::string dist = "  Manhattan distance:\n";
  for (int i = 0; i < manh_dist_.size(); ++i) {
    dist = dist + "    ";
    for (int j = 0; j < manh_dist_[i].size(); ++j) {
      dist = dist + std::to_string(manh_dist_[i][j]) + " ";
    }
  }

  debugPrint(logger_, PDR, "pdrev", 3, "{}", dist);

  debugPrint(logger_, PDR, "pdrev", 3, "  Node Info: ");
  for (int i = 0; i < dag_.size(); ++i) {
    debugPrint(logger_, PDR, "pdrev", 3, "    {}", nodes[dag_[i]]);
  }

  debugPrint(logger_, PDR, "pdrev", 3, "  Edge Info: ");
  for (int i = 0; i < dag_.size(); ++i) {
    debugPrint(logger_, PDR, "pdrev", 3, "    {}", edges_[dag_[i]]);
  }
}

bool Graph::IsSameDir(int cIdx, int nIdx)
{
  Node& cN = nodes[nIdx];
  int pIdx = nodes[nIdx].parent;
  int pId = 0;
  int cId = 0;
  for (int i = 0; i < cN.N.size(); ++i) {
    if (cN.N[i] == pIdx)
      pId = 1;
    if (cN.N[i] == cIdx)
      cId = 1;
  }
  for (int i = 0; i < cN.S.size(); ++i) {
    if (cN.S[i] == pIdx)
      pId = 2;
    if (cN.S[i] == cIdx)
      cId = 2;
  }
  for (int i = 0; i < cN.E.size(); ++i) {
    if (cN.E[i] == pIdx)
      pId = 3;
    if (cN.E[i] == cIdx)
      cId = 3;
  }
  for (int i = 0; i < cN.W.size(); ++i) {
    if (cN.W[i] == pIdx)
      pId = 4;
    if (cN.W[i] == cIdx)
      cId = 4;
  }
  if (cId != 0 && pId != 0 && cId == pId)
    return true;
  else
    return false;
}

#endif

////////////////////////////////////////////////////////////////

// Unused remnants of Guibas-Stolfi algorithm for computing nearest NE neighbors
// plagiarized from RMST-Pack

#ifdef Guibas_Stolfi

void Graph::NESW_NearestNeighbors(int left, int right, int oct)
{
  if (right == left + 1) {
    nn_[sorted_[left]][oct] = nn_[sorted_[left]][(oct + 4) % 8] = -1;
  } else {
    int mid = (left + right) / 2;

    NESW_NearestNeighbors(left, mid, oct);
    NESW_NearestNeighbors(mid, right, oct);
    NESW_Combine(left, mid, right, oct);

    if (logger_->debugCheck(PDR, "pdrev", 3)) {
      debugPrint(
          logger_, PDR, "pdrev", 3, "{} {} {} {}", oct, left, mid, right);
      std::string numTermRpt;
      for (int i = 0; i < num_terminals; ++i) {
        numTermRpt = numTermRpt + "  " + std::to_string(nn_[i][oct]);
      }
      debugPrint(logger_, PDR, "pdrev", 3, "{}", numTermRpt);
    }
  }
}

void Graph::NESW_Combine(int left, int mid, int right, int oct)
{
  int i, j, k, y2;
  int i1;
  int i2;
  int best_i2;   /* index of current best nearest-neighbor */
  int best_dist; /* distance to best nearest-neighbor      */
  int d;

  /*
     update north-east nearest neighbors accross the mid-line
   */
  i1 = left;
  i2 = mid;
  y2 = sheared_[sorted_[i2]].y;

  while ((i1 < mid) && (sheared_[sorted_[i1]].y >= y2)) {
    i1++;
  }

  if (i1 < mid) {
    best_i2 = i2;
    best_dist = dist(sheared_[sorted_[i1]], sheared_[sorted_[best_i2]]);
    i2++;

    while ((i1 < mid) && (i2 < right)) {
      if (sheared_[sorted_[i1]].y < sheared_[sorted_[i2]].y) {
        d = dist(sheared_[sorted_[i1]], sheared_[sorted_[i2]]);
        if (d < best_dist) {
          best_i2 = i2;
          best_dist = d;
        }
        i2++;
      } else {
        if ((nn_[sorted_[i1]][oct] == -1)
            || (best_dist < dist(sheared_[sorted_[i1]],
                                 sheared_[nn_[sorted_[i1]][oct]]))) {
          nn_[sorted_[i1]][oct] = sorted_[best_i2];
        }
        i1++;
        if (i1 < mid) {
          best_dist = dist(sheared_[sorted_[i1]], sheared_[sorted_[best_i2]]);
        }
      }
    }

    while (i1 < mid) {
      if ((nn_[sorted_[i1]][oct] == -1)
          || (dist(sheared_[sorted_[i1]], sheared_[sorted_[best_i2]])
              < dist(sheared_[sorted_[i1]], sheared_[nn_[sorted_[i1]][oct]]))) {
        nn_[sorted_[i1]][oct] = sorted_[best_i2];
      }
      i1++;
    }
  }

  /*
     repeat for south-west nearest neighbors
   */
  oct = (oct + 4) % 8;

  i1 = right - 1;
  i2 = mid - 1;
  y2 = sheared_[sorted_[i2]].y;

  while ((i1 >= mid) && (sheared_[sorted_[i1]].y <= y2)) {
    i1--;
  }

  if (i1 >= mid) {
    best_i2 = i2;
    best_dist = dist(sheared_[sorted_[i1]], sheared_[sorted_[best_i2]]);
    i2--;

    while ((i1 >= mid) && (i2 >= left)) {
      if (sheared_[sorted_[i1]].y > sheared_[sorted_[i2]].y) {
        d = dist(sheared_[sorted_[i1]], sheared_[sorted_[i2]]);
        if (d < best_dist) {
          best_i2 = i2;
          best_dist = d;
        }
        i2--;
      } else {
        if ((nn_[sorted_[i1]][oct] == -1)
            || (best_dist < dist(sheared_[sorted_[i1]],
                                 sheared_[nn_[sorted_[i1]][oct]]))) {
          nn_[sorted_[i1]][oct] = sorted_[best_i2];
        }
        i1--;
        if (i1 >= mid) {
          best_dist = dist(sheared_[sorted_[i1]], sheared_[sorted_[best_i2]]);
        }
      }
    }

    while (i1 >= mid) {
      if ((nn_[sorted_[i1]][oct] == -1)
          || (dist(sheared_[sorted_[i1]], sheared_[sorted_[best_i2]])
              < dist(sheared_[sorted_[i1]], sheared_[nn_[sorted_[i1]][oct]]))) {
        nn_[sorted_[i1]][oct] = sorted_[best_i2];
      }
      i1--;
    }
  }

  /*
     merge sorted[left..mid-1] with sorted[mid..right-1] by y-coordinate
   */

  i = left; /* first unprocessed element in left  list  */
  j = mid;  /* first unprocessed element in right list  */
  k = left; /* first free available slot in output list */

  while ((i < mid) && (j < right)) {
    if (sheared_[sorted_[i]].y >= sheared_[sorted_[j]].y) {
      aux_[k++] = sorted_[i++];
    } else {
      aux_[k++] = sorted_[j++];
    }
  }

  /*
     copy leftovers
   */
  while (i < mid) {
    aux_[k++] = sorted_[i++];
  }
  while (j < right) {
    aux_[k++] = sorted_[j++];
  }

  for (i = left; i < right; i++) {
    sorted_[i] = aux_[i];
  }
}

#endif

}  // namespace pdr
