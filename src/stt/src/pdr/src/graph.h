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

#pragma once

#include <vector>

#include "pdrevII.h"
#include "edge.h"
#include "node.h"

namespace utl {
class Logger;
}  // namespace utl

namespace pdr {

using std::ofstream;
using std::vector;
using utl::Logger;

class Graph
{
public:
  Graph(vector<int>& x,
        vector<int>& y,
        int root_index,
        Logger* logger);
  void buildNearestNeighborsForSPT();
  void run_PD_brute_force(float alpha);
  void doSteiner_HoVW();

  // used by PdRev::translateTree
  void replaceParent(Node& pNode, int idx, int tIdx);
  void addChild(Node& pNode, int idx);
  void replaceChild(Node& pNode, int idx, int tIdx);
  void removeChild(Node& pNode, int idx);
  int calc_tree_wl_pd();

  vector<Node> nodes;
  int num_terminals;
  int root_idx_;

private:
  int calc_overlap(vector<vector<Node>>& set_of_nodes);
  int calc_ov_x_or_y(vector<Node>& sorted, Node curr_node, char tag);
  void get_overlap_lshape(vector<Node>& set_of_nodes, int index);
  void generate_permutations(vector<vector<int>> lists,
                             vector<vector<int>>& result,
                             int depth,
                             vector<int> current);
  void update_edgecosts_to_parent(int child, int par);
  void update_node_detcost_Kt(int j);
  void get_level_in_tree();

  void heap_insert(int p, float key);
  int heap_delete_min();
  void heap_decrease_key(int p, float new_key);

  void get_children_of_node();
  void print_tree();
  float calc_tree_det_cost();
  int calc_tree_pl();
  void updateMinDist();
  bool make_unique(vector<Node>& vec);

  void UpdateManhDist();
  void UpdateMaxPLToChild(int cIdx);
  int IdentLoc(int cIdx, int tIdx);
  int IsAdded(Node& cN);
  void FreeManhDist();
  void RemoveUnneceSTNodes();

  // Aux functions
  void intersection(const std::vector<std::pair<double, double>> l1,
                    const std::vector<std::pair<double, double>> l2,
                    std::vector<std::pair<double, double>>& out);
  double length(std::vector<std::pair<double, double>> l);
  bool segmentIntersection(std::pair<double, double> A,
                           std::pair<double, double> B,
                           std::pair<double, double> C,
                           std::pair<double, double> D,
                           std::pair<double, double>& out);
  bool nodeLessY(const int i, const int j);

  vector<Edge> edges_;
  vector<vector<int>> manh_dist_;

  // nearest neighbor in some undocumented sense -cherry 06/14/2021
  vector<vector<int>> nn_;
  vector<int> sorted_;

  // Children of each level in the greph [level][child]
  vector<vector<int>> tree_struct_;

  vector<float> heap_key_;
  //   0 empty
  //  -1 visited (removed)
  vector<int> heap_idx_;
  vector<int> heap_elt_;
  int heap_size_;

  Logger* logger_;

#ifdef PDREVII
  // Segregated PDrevII code
public:
  void PDBU_new_NN(float alpha);
  void fix_max_dc();

private:
  void buildNearestNeighbors_single_node(int idx);
  void refineSteiner();
  void refineSteiner2();
  bool IsOnEdge(Node& tNode, int idx);
  void update_detourcosts_to_NNs(int j);
  void swap_and_update_tree(int min_node,
                            int nn_idx,
                            int distance,
                            int i_node);
  int ComputeWL(int cIdx, int pIdx, bool isRemove, int eShape);
  void UpdateSteinerNodes();
  void UpdateAllEdgesNSEW();
  void UpdateNSEW(Edge& e);
  void SortAll(Node& n);
  void SortN(Node& n);
  void SortS(Node& n);
  void SortE(Node& n);
  void SortW(Node& n);
  int DeltaN(int idx, int rIdx, bool isRemove);
  int DeltaS(int idx, int rIdx, bool isRemove);
  int DeltaE(int idx, int rIdx, bool isRemove);
  int DeltaW(int idx, int rIdx, bool isRemove);
  void AddNode(int cIdx, int pIdx, int eShape);
  void SortCNodes(vector<Node>& cNodes, int cIdx, int pIdx, int eShape);
  void UpdateEdges(vector<Node>& STNodes);
  void constructSteiner();
  float calc_tree_cost();
  void BuildDAG();
  bool IsSubTree(int cIdx, int tIdx);
  void GetSteinerNodes(int idx, vector<Node>& fSTNodes);
  void GetSteiner(int cIdx, int nIdx, vector<Node>& STNodes);
  void removeNeighbor(int pIdx, int cIdx);
  void removeN(Node& pN, int idx);
  void removeS(Node& pN, int idx);
  void removeE(Node& pN, int idx);
  void removeW(Node& pN, int idx);
  void DupRemoval(vector<Node>& STNodes);
  Node GetCornerNode(int cIdx);
  bool IsParent(int cIdx, int nIdx);
  void PrintInfo();
  bool IsSameDir(int cIdx, int nIdx);

  float m_;
  float alpha2_;
  float alpha3_;
  float alpha4_;
  float pl_margin_;
  float beta_;
  int distance_;

  vector<int> dag_;
  int max_pl_;  // max source to sink pathlength
  vector<int> aux_;
  vector<int> tree_struct_1darr_;
#endif

#ifdef Guibas_Stolfi
  void NESW_NearestNeighbors(int left, int right, int oct);
  void NESW_Combine(int left, int mid, int right, int oct);

  vector<Node> sheared_;
#endif

};

}  // namespace PD
