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
  void buildNearestNeighbors_single_node(int idx);
  void run_PD_brute_force(float alpha);
  void doSteiner_HoVW();
  void fix_max_dc();
  void find_max_dc_node(vector<float>& node_and_dc);
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
  void PDBU_new_NN(float alpha);
  void update_detourcosts_to_NNs(int j);
  void swap_and_update_tree(int min_node,
                            int nn_idx,
                            int distance,
                            int i_node);
  float calc_tree_cost();

  void heap_insert(int p, int key);
  int heap_delete_min();
  void heap_decrease_key(int p, float new_key);

  void get_children_of_node();
  void print_tree();
  float calc_tree_det_cost();
  int calc_tree_wl_pd();
  int calc_tree_pl();
  void updateMinDist();
  void NESW_NearestNeighbors(int left, int right, int oct);
  void NESW_Combine(int left, int mid, int right, int oct);
  bool make_unique(vector<Node>& vec);

  void BuildDAG();
  void UpdateManhDist();
  bool IsSubTree(int cIdx, int tIdx);
  void UpdateMaxPLToChild(int cIdx);
  void PrintInfo();
  void refineSteiner();
  void refineSteiner2();
  void UpdateSteinerNodes();
  void GetSteinerNodes(int idx, vector<Node>& fSTNodes);
  void GetSteiner(int cIdx, int nIdx, vector<Node>& STNodes);
  bool IsParent(int cIdx, int nIdx);
  bool IsOnEdge(Node& tNode, int idx);
  Node GetCornerNode(int cIdx);
  void DupRemoval(vector<Node>& STNodes);
  void removeChild(Node& pNode, int idx);
  void addChild(Node& pNode, int idx);
  void UpdateEdges(vector<Node>& STNodes);
  void UpdateAllEdgesNSEW();
  void UpdateNSEW(Edge& e);
  int IdentLoc(int cIdx, int tIdx);
  void SortAll(Node& n);
  void SortN(Node& n);
  void SortS(Node& n);
  void SortE(Node& n);
  void SortW(Node& n);
  int DeltaN(int idx, int rIdx, bool isRemove);
  int DeltaS(int idx, int rIdx, bool isRemove);
  int DeltaE(int idx, int rIdx, bool isRemove);
  int DeltaW(int idx, int rIdx, bool isRemove);
  int ComputeWL(int cIdx, int pIdx, bool isRemove, int eShape);
  void AddNode(int cIdx, int pIdx, int eShape);
  void SortCNodes(vector<Node>& cNodes, int cIdx, int pIdx, int eShape);
  void constructSteiner();
  int IsAdded(Node& cN);
  void FreeManhDist();
  void removeNeighbor(int pIdx, int cIdx);
  void removeN(Node& pN, int idx);
  void removeS(Node& pN, int idx);
  void removeE(Node& pN, int idx);
  void removeW(Node& pN, int idx);
  void replaceChild(Node& pNode, int idx, int tIdx);
  void replaceParent(Node& pNode, int idx, int tIdx);
  void RemoveUnneceSTNodes();
  void RemoveSTNodes();
  bool IsSameDir(int cIdx, int nIdx);

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

  vector<Node> nodes;
  int num_terminals;

private:
  bool nodeLessY(const int i, const int j);

  float alpha2;
  float alpha3;
  float alpha4;
  float beta;
  int distance;
  float M;

  vector<Edge> edges;
  vector<int> dag;
  int maxPL;  // max source to sink pathlength
  float maxPLRatio;
  vector<vector<int>> ManhDist;
  float PLmargin;
  int root_idx;

  // All of these should be local variables to buildNearestNeighborsForSPT -cherry 06/16/2021
  vector<int> urux;
  vector<int> urlx;
  vector<int> ulux;
  vector<int> ullx;
  vector<int> lrux;
  vector<int> lrlx;
  vector<int> llux;
  vector<int> lllx;

  vector<Node> sheared;
  vector<Node> hanan;
  // nearest neighbor in some undocumented sense -cherry 06/14/2021
  vector<vector<int>> nn;
  float avgNN;
  vector<vector<int>> nn_hanan;
  vector<int> sorted;
  vector<int> sorted_hanan;
  vector<int> aux;

  vector<vector<int>> tree_struct;
  vector<int> tree_struct_1darr;

  vector<int> heap_key;
  //   0 empty
  //  -1 visited (removed)
  vector<int> heap_idx;
  vector<int> heap_elt;
  int heap_size;

  Logger* logger_;
};

}  // namespace PD
