/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <memory>

#include "db/grObj/grBlockObject.h"
#include "db/grObj/grNode.h"
#include "db/grObj/grPin.h"
#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"

namespace drt {
class frNet;
class grNet : public grBlockObject
{
 public:
  // getters
  const std::vector<std::unique_ptr<grPin>>& getPins() const { return pins; }
  std::vector<std::unique_ptr<grPin>>& getPins() { return pins; }
  const std::vector<std::unique_ptr<grConnFig>>& getExtConnFigs() const
  {
    return extConnFigs;
  }
  std::vector<std::unique_ptr<grConnFig>>& getExtConnFigs()
  {
    return extConnFigs;
  }
  const std::vector<std::unique_ptr<grConnFig>>& getRouteConnFigs() const
  {
    return routeConnFigs;
  }
  std::vector<std::unique_ptr<grConnFig>>& getRouteConnFigs()
  {
    return routeConnFigs;
  }
  const std::list<std::unique_ptr<grNode>>& getNodes() const { return nodes; }
  std::list<std::unique_ptr<grNode>>& getNodes() { return nodes; }
  grNode* getRoot() { return root; }
  frNet* getFrNet() const { return fNet; }
  frNode* getFrRoot() { return frRoot; }
  const std::vector<std::pair<grNode*, grNode*>>& getPinGCellNodePairs() const
  {
    return pinGCellNodePairs;
  }
  std::vector<std::pair<grNode*, grNode*>>& getPinGCellNodePairs()
  {
    return pinGCellNodePairs;
  }
  const std::map<grNode*, std::vector<grNode*>, frBlockObjectComp>&
  getGCell2PinNodes() const
  {
    return gcell2PinNodes;
  }
  std::map<grNode*, std::vector<grNode*>, frBlockObjectComp>&
  getGCell2PinNodes()
  {
    return gcell2PinNodes;
  }
  const std::vector<grNode*>& getPinGCellNodes() const { return pinGCellNodes; }
  std::vector<grNode*>& getPinGCellNodes() { return pinGCellNodes; }
  const std::vector<std::pair<frNode*, grNode*>>& getPinNodePairs() const
  {
    return pinNodePairs;
  }
  std::vector<std::pair<frNode*, grNode*>>& getPinNodePairs()
  {
    return pinNodePairs;
  }
  const std::map<grNode*, frNode*, frBlockObjectComp>& getGR2FrPinNode() const
  {
    return gr2FrPinNode;
  }
  std::map<grNode*, frNode*, frBlockObjectComp>& getGR2FrPinNode()
  {
    return gr2FrPinNode;
  }
  bool isModified() const { return modified; }
  int getNumOverConGCells() const { return numOverConGCells; }
  int getNumPinsIn() const { return numPinsIn; }
  Rect getPinBox() { return pinBox; }
  bool isRipup() const { return ripup; }
  int getNumReroutes() const { return numReroutes; }
  bool isInQueue() const { return inQueue; }
  bool isRouted() const { return routed; }
  bool isTrivial() const { return trivial; }

  // setters
  void addPin(std::unique_ptr<grPin>& in)
  {
    in->setNet(this);
    pins.push_back(std::move(in));
  }
  void addRouteConnFig(std::unique_ptr<grConnFig>& in)
  {
    in->addToNet(this);
    routeConnFigs.push_back(std::move(in));
  }
  void clearRouteConnFigs() { routeConnFigs.clear(); }
  void addExtConnFig(std::unique_ptr<grConnFig>& in)
  {
    in->addToNet(this);
    extConnFigs.push_back(std::move(in));
  }
  void clear()
  {
    // routeConnFigs.clear();
    modified = true;
    numOverConGCells = 0;
    routed = false;
  }
  void setFrNet(frNet* in) { fNet = in; }
  void setFrRoot(frNode* in) { frRoot = in; }
  void addPinGCellNodePair(std::pair<grNode*, grNode*>& in)
  {
    pinGCellNodePairs.push_back(in);
  }
  void setPinGCellNodePairs(const std::vector<std::pair<grNode*, grNode*>>& in)
  {
    pinGCellNodePairs = in;
  }
  void setGCell2PinNodes(
      const std::map<grNode*, std::vector<grNode*>, frBlockObjectComp>& in)
  {
    gcell2PinNodes = in;
  }
  void setPinGCellNodes(const std::vector<grNode*>& in) { pinGCellNodes = in; }
  void setPinNodePairs(const std::vector<std::pair<frNode*, grNode*>>& in)
  {
    pinNodePairs = in;
  }
  void setGR2FrPinNode(const std::map<grNode*, frNode*, frBlockObjectComp>& in)
  {
    gr2FrPinNode = in;
  }
  void addNode(std::unique_ptr<grNode>& in)
  {
    in->addToNet(this);
    auto rptr = in.get();
    if (nodes.empty()) {
      rptr->setId(0);
    } else {
      rptr->setId(nodes.back()->getId() + 1);
    }
    nodes.push_back(std::move(in));
    rptr->setIter(--nodes.end());
  }
  void removeNode(grNode* in) { nodes.erase(in->getIter()); }
  void setRoot(grNode* in) { root = in; }
  void setModified(bool in) { modified = in; }
  void setNumOverConGCell(int in) { numOverConGCells = in; }
  void setNumPinsIn(int in) { numPinsIn = in; }
  void setAllowRipup(bool in) { allowRipup = in; }
  void setPinBox(const Rect& in) { pinBox = in; }
  void setRipup(bool in) { ripup = in; }
  void addNumReroutes() { numReroutes++; }
  void resetNumReroutes() { numReroutes = 0; }
  void setInQueue(bool in) { inQueue = in; }
  void setRouted(bool in) { routed = in; }
  void setTrivial(bool in) { trivial = in; }
  void cleanup()
  {
    pins.clear();
    pins.shrink_to_fit();
    extConnFigs.clear();
    extConnFigs.shrink_to_fit();
    routeConnFigs.clear();
    routeConnFigs.shrink_to_fit();
    nodes.clear();
    // fNetTerms.clear();
  }

  // others
  frBlockObjectEnum typeId() const override { return grcNet; }

  bool operator<(const grNet& b) const
  {
    return (numOverConGCells == b.numOverConGCells)
               ? (getId() < b.getId())
               : (numOverConGCells > b.numOverConGCells);
  }

 protected:
  std::vector<std::unique_ptr<grPin>> pins;
  std::vector<std::unique_ptr<grConnFig>> extConnFigs;
  std::vector<std::unique_ptr<grConnFig>> routeConnFigs;
  // std::vector<std::unique_ptr<grConnFig> > bestRouteConnFigs;

  // pair of <pinNode, gcellNode> with first (0th) element always being root
  std::vector<std::pair<grNode*, grNode*>> pinGCellNodePairs;
  std::map<grNode*, std::vector<grNode*>, frBlockObjectComp> gcell2PinNodes;
  // unique, first (0th) element always being root
  std::vector<grNode*> pinGCellNodes;
  std::vector<std::pair<frNode*, grNode*>> pinNodePairs;
  std::map<grNode*, frNode*, frBlockObjectComp> gr2FrPinNode;
  // std::set<frBlockObject*>                 fNetTerms;
  std::list<std::unique_ptr<grNode>> nodes;
  grNode* root{nullptr};
  frNet* fNet{nullptr};
  frNode* frRoot{nullptr};  // subnet frRoot

  bool modified{false};
  int numOverConGCells{0};
  int numPinsIn{0};
  bool allowRipup{true};
  Rect pinBox;
  bool ripup{false};

  int numReroutes{0};
  bool inQueue{false};
  bool routed{false};
  bool trivial{false};
};
}  // namespace drt
