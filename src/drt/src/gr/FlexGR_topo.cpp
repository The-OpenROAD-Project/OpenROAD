// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frNode.h"
#include "frBaseTypes.h"
#include "gr/FlexGR.h"
#include "odb/geom.h"
#include "stt/SteinerTreeBuilder.h"

namespace drt {

// pinGCellNodes size always >= 2
void FlexGR::genSTTopology_FLUTE(std::vector<frNode*>& pinGCellNodes,
                                 std::vector<frNode*>& steinerNodes)
{
  auto root = pinGCellNodes[0];
  auto net = root->getNet();
  // prep for flute
  int degree = pinGCellNodes.size();
  std::vector<int> xs(degree);
  std::vector<int> ys(degree);
  for (int i = 0; i < (int) pinGCellNodes.size(); i++) {
    auto gcellNode = pinGCellNodes[i];
    odb::Point loc = gcellNode->getLoc();
    xs[i] = loc.x();
    ys[i] = loc.y();
  }
  // temporary to keep using flute here
  stt_builder_->setAlpha(0);
  auto fluteTree = stt_builder_->makeSteinerTree(xs, ys, 0);

  std::map<odb::Point, frNode*> pinGCell2Nodes, steinerGCell2Nodes;
  frOrderedIdMap<frNode*, frOrderedIdSet<frNode*>> adjacencyList;

  for (auto pinNode : pinGCellNodes) {
    pinGCell2Nodes[pinNode->getLoc()] = pinNode;
  }

  // iterate over branches, create new nodes and build connectivity
  for (int i = 0; i < degree * 2 - 2; i++) {
    auto& branch1 = fluteTree.branch[i];
    auto& branch2 = fluteTree.branch[branch1.n];

    odb::Point bp(branch1.x, branch1.y);
    odb::Point ep(branch2.x, branch2.y);

    if (bp == ep) {
      continue;
    }

    // colinear
    // if (bp.x() == ep.x() || bp.y() == ep.y()) {
    frNode* bpNode = nullptr;
    frNode* epNode = nullptr;

    if (pinGCell2Nodes.find(bp) == pinGCell2Nodes.end()) {
      if (steinerGCell2Nodes.find(bp) == steinerGCell2Nodes.end()) {
        // add steiner
        auto steinerNode = std::make_unique<frNode>();
        bpNode = steinerNode.get();
        steinerNode->setType(frNodeTypeEnum::frcSteiner);
        steinerNode->setLoc(bp);
        steinerNode->setLayerNum(2);
        steinerGCell2Nodes[bp] = steinerNode.get();
        steinerNodes.push_back(steinerNode.get());
        net->addNode(steinerNode);
      } else {
        bpNode = steinerGCell2Nodes[bp];
      }
    } else {
      bpNode = pinGCell2Nodes[bp];
    }
    if (pinGCell2Nodes.find(ep) == pinGCell2Nodes.end()) {
      if (steinerGCell2Nodes.find(ep) == steinerGCell2Nodes.end()) {
        // add steiner
        auto steinerNode = std::make_unique<frNode>();
        epNode = steinerNode.get();
        steinerNode->setType(frNodeTypeEnum::frcSteiner);
        steinerNode->setLoc(ep);
        steinerNode->setLayerNum(2);
        steinerGCell2Nodes[ep] = steinerNode.get();
        steinerNodes.push_back(steinerNode.get());
        net->addNode(steinerNode);
      } else {
        epNode = steinerGCell2Nodes[ep];
      }
    } else {
      epNode = pinGCell2Nodes[ep];
    }

    if (bpNode == nullptr || epNode == nullptr) {
      std::cout << "Error: bpNode or epNode is void\n";
    }

    adjacencyList[bpNode].insert(epNode);
    adjacencyList[epNode].insert(bpNode);
  }

  // reset nodes
  for (auto& [loc, node] : pinGCell2Nodes) {
    node->reset();
  }

  // build tree
  std::set<frNode*> visitedNodes;
  std::deque<frNode*> nodeQueue;

  nodeQueue.push_front(root);
  visitedNodes.insert(root);

  while (!nodeQueue.empty()) {
    auto currNode = nodeQueue.back();
    nodeQueue.pop_back();

    for (auto adjNode : adjacencyList[currNode]) {
      if (visitedNodes.find(adjNode) == visitedNodes.end()) {
        currNode->addChild(adjNode);
        adjNode->setParent(currNode);
        nodeQueue.push_front(adjNode);
        visitedNodes.insert(adjNode);
      }
    }
  }
}

void FlexGR::genMSTTopology(std::vector<frNode*>& nodes)
{
  genMSTTopology_PD(nodes);
}

void FlexGR::genMSTTopology_PD(std::vector<frNode*>& nodes, double alpha)
{
  // manhattan distance array
  std::vector<std::vector<int>> dists(nodes.size(),
                                      std::vector<int>(nodes.size(), INT_MAX));
  std::vector<bool> isVisited(nodes.size(), false);
  std::vector<int> pathLens(nodes.size(), INT_MAX);
  std::vector<int> parentIdx(nodes.size(), -1);
  std::vector<int> keys(nodes.size(), INT_MAX);

  // init dist array
  for (int i = 0; i < (int) nodes.size(); i++) {
    for (int j = i; j < (int) nodes.size(); j++) {
      auto node1 = nodes[i];
      auto node2 = nodes[j];
      odb::Point loc1 = node1->getLoc();
      odb::Point loc2 = node2->getLoc();
      int dist = abs(loc2.x() - loc1.x()) + abs(loc2.y() - loc1.y());
      dists[i][j] = dist;
      dists[j][i] = dist;
    }
  }

  pathLens[0] = 0;
  keys[0] = 0;
  parentIdx[0] = 0;

  // start pd
  for (int i = 0; i < (int) isVisited.size(); i++) {
    int minIdx = genMSTTopology_PD_minIdx(keys, isVisited);
    isVisited[minIdx] = true;
    pathLens[minIdx]
        = pathLens[parentIdx[minIdx]] + dists[parentIdx[minIdx]][minIdx];
    for (int idx = 0; idx < (int) nodes.size(); idx++) {
      if (isVisited[idx] == false
          && (pathLens[minIdx] * alpha + dists[minIdx][idx] < keys[idx])) {
        parentIdx[idx] = minIdx;
        keys[idx] = pathLens[minIdx] * alpha + dists[minIdx][idx];
      }
    }
  }

  // write back to nodes
  for (int i = 0; i < (int) nodes.size(); i++) {
    // only do for non-root (i.e., leaf) nodes
    if (parentIdx[i] != i) {
      auto parent = nodes[parentIdx[i]];
      auto child = nodes[i];
      child->setParent(parent);
      parent->addChild(child);
    }
  }
}

int FlexGR::genMSTTopology_PD_minIdx(const std::vector<int>& keys,
                                     const std::vector<bool>& isVisited)
{
  int min = INT_MAX;
  int minIdx = -1;

  for (int i = 0; i < (int) isVisited.size(); i++) {
    if (isVisited[i] == false && keys[i] < min) {
      min = keys[i];
      minIdx = i;
    }
  }
  return minIdx;
}

// root is 0
void FlexGR::genSTTopology_HVW(std::vector<frNode*>& nodes,
                               std::vector<frNode*>& steinerNodes)
{
  std::vector<unsigned> overlapL(nodes.size(), 0);
  std::vector<unsigned> overlapU(nodes.size(), 0);
  std::vector<unsigned> bestCombL(nodes.size(), 0);
  std::vector<unsigned> bestCombU(nodes.size(), 0);
  std::vector<bool> isU(nodes.size(), false);

  // recursively compute best overlap
  genSTTopology_HVW_compute(
      nodes[0], nodes, overlapL, overlapU, bestCombL, bestCombU);

  // recursively get L/U
  if (overlapL[0] >= overlapU[0]) {
    genSTTopology_HVW_commit(nodes[0],
                             false,
                             nodes,
                             /*overlapL, overlapU,*/ bestCombL,
                             bestCombU,
                             isU);
  } else {
    genSTTopology_HVW_commit(nodes[0],
                             true,
                             nodes,
                             /*overlapL, overlapU,*/ bestCombL,
                             bestCombU,
                             isU);
  }

  // additional steiner nodes needed for tree construction
  genSTTopology_build_tree(nodes, isU, steinerNodes);
}

void FlexGR::genSTTopology_HVW_compute(frNode* currNode,
                                       std::vector<frNode*>& nodes,
                                       std::vector<unsigned>& overlapL,
                                       std::vector<unsigned>& overlapU,
                                       std::vector<unsigned>& bestCombL,
                                       std::vector<unsigned>& bestCombU)
{
  if (currNode == nullptr) {
    return;
  }

  if (currNode->getChildren().empty()) {
    genSTTopology_HVW_compute(
        nullptr, nodes, overlapL, overlapU, bestCombL, bestCombU);
  } else {
    for (auto child : currNode->getChildren()) {
      genSTTopology_HVW_compute(
          child, nodes, overlapL, overlapU, bestCombL, bestCombU);
    }
  }

  unsigned numChild = currNode->getChildren().size();
  unsigned numComb = 1 << numChild;
  unsigned currOverlap = 0;
  unsigned currMaxOverlap = 0;
  unsigned bestComb = 0;
  int currNodeIdx = distance(nodes[0]->getIter(), currNode->getIter());
  // std::cout << "1. currNodeIdx = " << currNodeIdx << "\n";

  // curr overlapL = std::min(comb(children) + L)
  // bit 1 means using U, 0 means using L
  for (unsigned comb = 0; comb < numComb; comb++) {
    currOverlap = genSTTopology_HVW_levelOvlp(currNode, false, comb);
    unsigned combIdx = 0;
    for (auto child : currNode->getChildren()) {
      int nodeIdx = distance(nodes[0]->getIter(), child->getIter());
      if ((comb >> combIdx) & 1) {
        currOverlap += overlapU[nodeIdx];
      } else {
        currOverlap += overlapL[nodeIdx];
      }
      combIdx++;
    }
    if (currOverlap > currMaxOverlap) {
      currMaxOverlap = currOverlap;
      bestComb = comb;
    }
  }

  // commit for current level
  overlapL[currNodeIdx] = currMaxOverlap;
  bestCombL[currNodeIdx] = bestComb;

  currMaxOverlap = 0;
  // curr overlapU = std::min(comb(children) + R)
  for (unsigned comb = 0; comb < numComb; comb++) {
    currOverlap = genSTTopology_HVW_levelOvlp(currNode, true, comb);
    unsigned combIdx = 0;
    for (auto child : currNode->getChildren()) {
      int nodeIdx = distance(nodes[0]->getIter(), child->getIter());
      if ((comb >> combIdx) & 1) {
        currOverlap += overlapU[nodeIdx];
      } else {
        currOverlap += overlapL[nodeIdx];
      }
      combIdx++;
    }
    if (currOverlap > currMaxOverlap) {
      currMaxOverlap = currOverlap;
      bestComb = comb;
    }
  }
  // commit for current level
  overlapU[currNodeIdx] = currMaxOverlap;
  bestCombU[currNodeIdx] = bestComb;
}

unsigned FlexGR::genSTTopology_HVW_levelOvlp(frNode* currNode,
                                             bool isCurrU,
                                             unsigned comb)
{
  unsigned overlap = 0;
  std::map<frCoord, boost::icl::interval_map<frCoord, std::set<frNode*>>>
      horzIntvMaps;  // denoted by children node index
  std::map<frCoord, boost::icl::interval_map<frCoord, std::set<frNode*>>>
      vertIntvMaps;

  std::pair<frCoord, frCoord> horzIntv, vertIntv;
  odb::Point turnLoc;

  // connection to parent if exists
  auto parent = currNode->getParent();
  if (parent) {
    genSTTopology_HVW_levelOvlp_helper(
        parent, currNode, isCurrU, horzIntv, vertIntv, turnLoc);
    std::set<frNode*> currNodeSet;
    currNodeSet.insert(currNode);
    if (horzIntv.first != horzIntv.second) {
      horzIntvMaps[turnLoc.y()] += std::make_pair(
          boost::icl::interval<int>::closed(horzIntv.first, horzIntv.second),
          currNodeSet);
    }
    if (vertIntv.first != vertIntv.second) {
      vertIntvMaps[turnLoc.x()] += std::make_pair(
          boost::icl::interval<int>::closed(vertIntv.first, vertIntv.second),
          currNodeSet);
    }
  }

  // connections from children to current
  unsigned combIdx = 0;
  for (auto child : currNode->getChildren()) {
    bool isCurrU = (comb >> combIdx) & 1;
    genSTTopology_HVW_levelOvlp_helper(
        currNode, child, isCurrU, horzIntv, vertIntv, turnLoc);
    std::set<frNode*> childSet;
    childSet.insert(child);
    if (horzIntv.first != horzIntv.second) {
      horzIntvMaps[turnLoc.y()] += std::make_pair(
          boost::icl::interval<int>::closed(horzIntv.first, horzIntv.second),
          childSet);
    }
    if (vertIntv.first != vertIntv.second) {
      vertIntvMaps[turnLoc.x()] += std::make_pair(
          boost::icl::interval<int>::closed(vertIntv.first, vertIntv.second),
          childSet);
    }
    combIdx++;
  }

  // iterate over intvMaps to get overlaps
  for (auto& [coord, intvMap] : horzIntvMaps) {
    for (auto& [intv, idxs] : intvMap) {
      overlap += (intv.upper() - intv.lower()) * ((int) idxs.size() - 1);
    }
  }

  return overlap;
}

void FlexGR::genSTTopology_HVW_levelOvlp_helper(
    frNode* parent,
    frNode* child,
    bool isCurrU,
    std::pair<frCoord, frCoord>& horzIntv,
    std::pair<frCoord, frCoord>& vertIntv,
    odb::Point& turnLoc)
{
  odb::Point parentLoc = parent->getLoc();
  odb::Point childLoc = child->getLoc();

  if (isCurrU) {
    if ((childLoc.x() >= parentLoc.x() && childLoc.y() >= parentLoc.y())
        || (childLoc.x() <= parentLoc.x() && childLoc.y() <= parentLoc.y())) {
      turnLoc = odb::Point(std::min(childLoc.x(), parentLoc.x()),
                           std::max(childLoc.y(), parentLoc.y()));
    } else {
      turnLoc = odb::Point(std::max(childLoc.x(), parentLoc.x()),
                           std::max(childLoc.y(), parentLoc.y()));
    }
  } else {
    if ((childLoc.x() >= parentLoc.x() && childLoc.y() >= parentLoc.y())
        || (childLoc.x() <= parentLoc.x() && childLoc.y() <= parentLoc.y())) {
      turnLoc = odb::Point(std::max(childLoc.x(), parentLoc.x()),
                           std::min(childLoc.y(), parentLoc.y()));
    } else {
      turnLoc = odb::Point(std::min(childLoc.x(), parentLoc.x()),
                           std::min(childLoc.y(), parentLoc.y()));
    }
  }

  // set intvs
  // horz colinear
  if (childLoc.y() == parentLoc.y()) {
    horzIntv = std::make_pair(std::min(childLoc.x(), parentLoc.x()),
                              std::max(childLoc.x(), parentLoc.x()));
    vertIntv = std::make_pair(std::min(childLoc.y(), parentLoc.y()),
                              std::max(childLoc.y(), parentLoc.y()));
  } else if (childLoc.x() == parentLoc.x()) {
    horzIntv = std::make_pair(std::min(childLoc.x(), parentLoc.x()),
                              std::max(childLoc.x(), parentLoc.x()));
    vertIntv = std::make_pair(std::min(childLoc.y(), parentLoc.y()),
                              std::max(childLoc.y(), parentLoc.y()));
  } else {
    if (turnLoc.y() == childLoc.y()) {
      horzIntv = std::make_pair(std::min(turnLoc.x(), childLoc.x()),
                                std::max(turnLoc.x(), childLoc.x()));
      vertIntv = std::make_pair(std::min(turnLoc.y(), parentLoc.y()),
                                std::max(turnLoc.y(), parentLoc.y()));
    } else {
      horzIntv = std::make_pair(std::min(turnLoc.x(), parentLoc.x()),
                                std::max(turnLoc.x(), parentLoc.x()));
      vertIntv = std::make_pair(std::min(turnLoc.y(), childLoc.y()),
                                std::max(turnLoc.y(), childLoc.y()));
    }
  }
}

void FlexGR::genSTTopology_HVW_commit(frNode* currNode,
                                      bool isCurrU,
                                      std::vector<frNode*>& nodes,
                                      // std::vector<unsigned> &overlapL,
                                      // std::vector<unsigned> &overlapU,
                                      std::vector<unsigned>& bestCombL,
                                      std::vector<unsigned>& bestCombU,
                                      std::vector<bool>& isU)
{
  int currNodeIdx = distance(nodes[0]->getIter(), currNode->getIter());
  // std::cout << "2. currNodeIdx = " << currNodeIdx << "\n";
  isU[currNodeIdx] = isCurrU;

  unsigned comb = 0;
  if (isCurrU) {
    comb = bestCombU[currNodeIdx];
  } else {
    comb = bestCombL[currNodeIdx];
  }

  unsigned combIdx = 0;
  for (auto child : currNode->getChildren()) {
    bool isChildU = (comb >> combIdx) & 1;
    genSTTopology_HVW_commit(child,
                             isChildU,
                             nodes,
                             /*overlapL, overlapU,*/ bestCombL,
                             bestCombU,
                             isU);

    combIdx++;
  }
}

// build tree from L-shaped segment results
void FlexGR::genSTTopology_build_tree(std::vector<frNode*>& pinNodes,
                                      std::vector<bool>& isU,
                                      std::vector<frNode*>& steinerNodes)
{
  std::map<frCoord, boost::icl::interval_set<frCoord>> horzIntvs, vertIntvs;
  std::map<odb::Point, frNode*> pinGCell2Nodes, steinerGCell2Nodes;

  genSTTopology_build_tree_mergeSeg(pinNodes, isU, horzIntvs, vertIntvs);

  for (auto pinNode : pinNodes) {
    pinGCell2Nodes[pinNode->getLoc()] = pinNode;
  }

  // split seg and build tree
  genSTTopology_build_tree_splitSeg(pinNodes,
                                    pinGCell2Nodes,
                                    horzIntvs,
                                    vertIntvs,
                                    steinerGCell2Nodes,
                                    steinerNodes);

  // sanity check
  for (int i = 0; i < (int) pinNodes.size(); i++) {
    if (i != 0) {
      if (pinNodes[i]->getParent() == nullptr) {
        std::cout << "Error: non-root pin node does not have parent\n";
      }
    }
  }
  for (auto node : steinerNodes) {
    if (node->getParent() == nullptr) {
      std::cout << "Error: non-root steiner node does not have parent\n";
    }
  }
}

void FlexGR::genSTTopology_build_tree_mergeSeg(
    std::vector<frNode*>& pinNodes,
    std::vector<bool>& isU,
    std::map<frCoord, boost::icl::interval_set<frCoord>>& horzIntvs,
    std::map<frCoord, boost::icl::interval_set<frCoord>>& vertIntvs)
{
  odb::Point turnLoc;
  std::pair<frCoord, frCoord> horzIntv, vertIntv;
  for (int i = 1; i < (int) pinNodes.size(); i++) {
    genSTTopology_HVW_levelOvlp_helper(pinNodes[i],
                                       pinNodes[i]->getParent(),
                                       isU[i],
                                       horzIntv,
                                       vertIntv,
                                       turnLoc);
    if (horzIntv.first != horzIntv.second) {
      horzIntvs[turnLoc.y()].insert(
          boost::icl::interval<int>::closed(horzIntv.first, horzIntv.second));
    }
    if (vertIntv.first != vertIntv.second) {
      vertIntvs[turnLoc.x()].insert(
          boost::icl::interval<int>::closed(vertIntv.first, vertIntv.second));
    }
  }
}

void FlexGR::genSTTopology_build_tree_splitSeg(
    std::vector<frNode*>& pinNodes,
    std::map<odb::Point, frNode*>& pinGCell2Nodes,
    std::map<frCoord, boost::icl::interval_set<frCoord>>& horzIntvs,
    std::map<frCoord, boost::icl::interval_set<frCoord>>& vertIntvs,
    std::map<odb::Point, frNode*>& steinerGCell2Nodes,
    std::vector<frNode*>& steinerNodes)
{
  std::map<frCoord, std::set<std::pair<frCoord, frCoord>>> horzSegs, vertSegs;
  frOrderedIdMap<frNode*, frOrderedIdSet<frNode*>> adjacencyList;

  auto root = pinNodes[0];
  auto net = root->getNet();

  // horz first dimension is y coord, second dimension is x coord
  std::map<frCoord, std::set<frCoord>> horzPinHelper, vertPinHelper;
  // init
  for (const auto& [loc, node] : pinGCell2Nodes) {
    horzPinHelper[loc.y()].insert(loc.x());
    vertPinHelper[loc.x()].insert(loc.y());
  }

  // trackIdx == y coord
  for (auto& [trackIdx, currIntvs] : horzIntvs) {
    for (auto& intv : currIntvs) {
      std::set<frCoord> lineIdx;
      auto beginIdx = intv.lower();
      auto endIdx = intv.upper();
      // split by vertSeg
      for (auto it2 = vertIntvs.lower_bound(beginIdx);
           it2 != vertIntvs.end() && it2->first <= endIdx;
           it2++) {
        if (boost::icl::contains(it2->second, trackIdx)) {
          lineIdx.insert(it2->first);  // it2->first is intersection frCoord
        }
      }
      // split by pin
      if (horzPinHelper.find(trackIdx) != horzPinHelper.end()) {
        auto& pinHelperSet = horzPinHelper[trackIdx];
        for (auto it2 = pinHelperSet.lower_bound(beginIdx);
             it2 != pinHelperSet.end() && (*it2) <= endIdx;
             it2++) {
          lineIdx.insert(*it2);
        }
      }
      // add horz seg
      if (lineIdx.empty()) {
        std::cout
            << "Error: genSTTopology_build_tree_splitSeg lineIdx is empty\n";
        exit(1);
      } else if (lineIdx.size() == 1) {
        // std::cout << "Error: genSTTopology_build_tree_splitSeg lineIdx size
        // == 1\n"; exit(1);
      } else {
        auto prevIt = lineIdx.begin();
        for (auto currIt = (++(lineIdx.begin())); currIt != lineIdx.end();
             currIt++) {
          horzSegs[trackIdx].insert(std::make_pair(*prevIt, *currIt));
          prevIt = currIt;
        }
      }
    }
  }

  // trackIdx == x coord
  for (auto& [trackIdx, currIntvs] : vertIntvs) {
    for (auto& intv : currIntvs) {
      std::set<frCoord> lineIdx;
      auto beginIdx = intv.lower();
      auto endIdx = intv.upper();
      // split by horzSeg
      for (auto it2 = horzIntvs.lower_bound(beginIdx);
           it2 != horzIntvs.end() && it2->first <= endIdx;
           it2++) {
        if (boost::icl::contains(it2->second, trackIdx)) {
          lineIdx.insert(it2->first);
        }
      }
      // split by pin
      if (vertPinHelper.find(trackIdx) != vertPinHelper.end()) {
        auto& pinHelperSet = vertPinHelper[trackIdx];
        for (auto it2 = pinHelperSet.lower_bound(beginIdx);
             it2 != pinHelperSet.end() && (*it2) <= endIdx;
             it2++) {
          lineIdx.insert(*it2);
        }
      }
      // add vert seg
      if (lineIdx.empty()) {
        std::cout
            << "Error: genSTTopology_build_tree_splitSeg lineIdx is empty\n";
        exit(1);
      } else if (lineIdx.size() == 1) {
        // std::cout << "Error: genSTTopology_build_tree_splitSeg lineIdx size
        // == 1\n"; exit(1);
      } else {
        auto prevIt = lineIdx.begin();
        for (auto currIt = (++(lineIdx.begin())); currIt != lineIdx.end();
             currIt++) {
          vertSegs[trackIdx].insert(std::make_pair(*prevIt, *currIt));
          prevIt = currIt;
        }
      }
    }
  }

  // add steiner points
  // from horz segs
  for (auto& [trackIdx, intvs] : horzSegs) {
    for (auto& intv : intvs) {
      odb::Point bp(intv.first, trackIdx);
      odb::Point ep(intv.second, trackIdx);
      if (pinGCell2Nodes.find(bp) == pinGCell2Nodes.end()) {
        if (steinerGCell2Nodes.find(bp) == steinerGCell2Nodes.end()) {
          // add steiner
          auto steinerNode = std::make_unique<frNode>();
          steinerNode->setType(frNodeTypeEnum::frcSteiner);
          steinerNode->setLoc(bp);
          steinerNode->setLayerNum(2);
          steinerGCell2Nodes[bp] = steinerNode.get();
          steinerNodes.push_back(steinerNode.get());
          net->addNode(steinerNode);
        }
      }
      if (pinGCell2Nodes.find(ep) == pinGCell2Nodes.end()) {
        if (steinerGCell2Nodes.find(ep) == steinerGCell2Nodes.end()) {
          // add steiner
          auto steinerNode = std::make_unique<frNode>();
          steinerNode->setType(frNodeTypeEnum::frcSteiner);
          steinerNode->setLoc(ep);
          steinerNode->setLayerNum(2);
          steinerGCell2Nodes[ep] = steinerNode.get();
          steinerNodes.push_back(steinerNode.get());
          net->addNode(steinerNode);
        }
      }
    }
  }
  // from vert segs
  for (auto& [trackIdx, intvs] : vertSegs) {
    for (auto& intv : intvs) {
      odb::Point bp(trackIdx, intv.first);
      odb::Point ep(trackIdx, intv.second);
      if (pinGCell2Nodes.find(bp) == pinGCell2Nodes.end()) {
        if (steinerGCell2Nodes.find(bp) == steinerGCell2Nodes.end()) {
          // add steiner
          auto steinerNode = std::make_unique<frNode>();
          steinerNode->setType(frNodeTypeEnum::frcSteiner);
          steinerNode->setLoc(bp);
          steinerNode->setLayerNum(2);
          steinerGCell2Nodes[bp] = steinerNode.get();
          steinerNodes.push_back(steinerNode.get());
          net->addNode(steinerNode);
        }
      }
      if (pinGCell2Nodes.find(ep) == pinGCell2Nodes.end()) {
        if (steinerGCell2Nodes.find(ep) == steinerGCell2Nodes.end()) {
          // add steiner
          auto steinerNode = std::make_unique<frNode>();
          steinerNode->setType(frNodeTypeEnum::frcSteiner);
          steinerNode->setLoc(ep);
          steinerNode->setLayerNum(2);
          steinerGCell2Nodes[ep] = steinerNode.get();
          steinerNodes.push_back(steinerNode.get());
          net->addNode(steinerNode);
        }
      }
    }
  }

  // populate adjacency list
  // from horz segs
  for (auto& [trackIdx, intvs] : horzSegs) {
    for (auto& intv : intvs) {
      odb::Point bp(intv.first, trackIdx);
      odb::Point ep(intv.second, trackIdx);
      frNode* bpNode = nullptr;
      frNode* epNode = nullptr;
      if (pinGCell2Nodes.find(bp) != pinGCell2Nodes.end()) {
        bpNode = pinGCell2Nodes[bp];
      } else if (steinerGCell2Nodes.find(bp) != steinerGCell2Nodes.end()) {
        bpNode = steinerGCell2Nodes[bp];
      } else {
        std::cout << "Error: node not found\n";
      }
      if (pinGCell2Nodes.find(ep) != pinGCell2Nodes.end()) {
        epNode = pinGCell2Nodes[ep];
      } else if (steinerGCell2Nodes.find(ep) != steinerGCell2Nodes.end()) {
        epNode = steinerGCell2Nodes[ep];
      } else {
        std::cout << "Error: node not found\n";
      }

      if (bpNode == nullptr || epNode == nullptr) {
        std::cout << "Error: node is nullptr";
      }

      adjacencyList[bpNode].insert(epNode);
      adjacencyList[epNode].insert(bpNode);
    }
  }
  // from vert segs
  for (auto& [trackIdx, intvs] : vertSegs) {
    for (auto& intv : intvs) {
      odb::Point bp(trackIdx, intv.first);
      odb::Point ep(trackIdx, intv.second);
      frNode* bpNode = nullptr;
      frNode* epNode = nullptr;
      if (pinGCell2Nodes.find(bp) != pinGCell2Nodes.end()) {
        bpNode = pinGCell2Nodes[bp];
      } else if (steinerGCell2Nodes.find(bp) != steinerGCell2Nodes.end()) {
        bpNode = steinerGCell2Nodes[bp];
      } else {
        std::cout << "Error: node not found\n";
      }
      if (pinGCell2Nodes.find(ep) != pinGCell2Nodes.end()) {
        epNode = pinGCell2Nodes[ep];
      } else if (steinerGCell2Nodes.find(ep) != steinerGCell2Nodes.end()) {
        epNode = steinerGCell2Nodes[ep];
      } else {
        std::cout << "Error: node not found\n";
      }

      if (bpNode == nullptr || epNode == nullptr) {
        std::cout << "Error: node is nullptr";
      }

      adjacencyList[bpNode].insert(epNode);
      adjacencyList[epNode].insert(bpNode);
    }
  }

  // reset nodes
  for (auto& [loc, node] : pinGCell2Nodes) {
    node->reset();
  }

  frOrderedIdSet<frNode*> visitedNodes;
  std::deque<frNode*> nodeQueue;

  nodeQueue.push_front(root);
  visitedNodes.insert(root);

  while (!nodeQueue.empty()) {
    auto currNode = nodeQueue.back();
    nodeQueue.pop_back();

    for (auto adjNode : adjacencyList[currNode]) {
      if (visitedNodes.find(adjNode) == visitedNodes.end()) {
        currNode->addChild(adjNode);
        adjNode->setParent(currNode);
        nodeQueue.push_front(adjNode);
        visitedNodes.insert(adjNode);
      }
    }
  }
}

}  // namespace drt
