// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "db/grObj/grBlockObject.h"
#include "db/grObj/grFig.h"
#include "db/grObj/grNode.h"
#include "db/grObj/grPin.h"
#include "db/grObj/grShape.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frNode.h"
#include "frBaseTypes.h"
#include "odb/geom.h"

namespace drt {
class frNet;
class grNet : public grBlockObject
{
 public:
  // getters
  const std::vector<std::unique_ptr<grPin>>& getPins() const { return pins_; }
  std::vector<std::unique_ptr<grPin>>& getPins() { return pins_; }
  const std::vector<std::unique_ptr<grConnFig>>& getExtConnFigs() const
  {
    return extConnFigs_;
  }
  std::vector<std::unique_ptr<grConnFig>>& getExtConnFigs()
  {
    return extConnFigs_;
  }
  const std::vector<std::unique_ptr<grConnFig>>& getRouteConnFigs() const
  {
    return routeConnFigs_;
  }
  std::vector<std::unique_ptr<grConnFig>>& getRouteConnFigs()
  {
    return routeConnFigs_;
  }
  const std::list<std::unique_ptr<grNode>>& getNodes() const { return nodes_; }
  std::list<std::unique_ptr<grNode>>& getNodes() { return nodes_; }
  grNode* getRoot() { return root_; }
  frNet* getFrNet() const { return fNet_; }
  frNode* getFrRoot() { return frRoot_; }
  const std::vector<std::pair<grNode*, grNode*>>& getPinGCellNodePairs() const
  {
    return pinGCellNodePairs_;
  }
  std::vector<std::pair<grNode*, grNode*>>& getPinGCellNodePairs()
  {
    return pinGCellNodePairs_;
  }
  const frOrderedIdMap<grNode*, std::vector<grNode*>>& getGCell2PinNodes() const
  {
    return gcell2PinNodes_;
  }
  frOrderedIdMap<grNode*, std::vector<grNode*>>& getGCell2PinNodes()
  {
    return gcell2PinNodes_;
  }
  const std::vector<grNode*>& getPinGCellNodes() const
  {
    return pinGCellNodes_;
  }
  std::vector<grNode*>& getPinGCellNodes() { return pinGCellNodes_; }
  const std::vector<std::pair<frNode*, grNode*>>& getPinNodePairs() const
  {
    return pinNodePairs_;
  }
  std::vector<std::pair<frNode*, grNode*>>& getPinNodePairs()
  {
    return pinNodePairs_;
  }
  const frOrderedIdMap<grNode*, frNode*>& getGR2FrPinNode() const
  {
    return gr2FrPinNode_;
  }
  frOrderedIdMap<grNode*, frNode*>& getGR2FrPinNode() { return gr2FrPinNode_; }
  bool isModified() const { return modified_; }
  int getNumOverConGCells() const { return numOverConGCells_; }
  int getNumPinsIn() const { return numPinsIn_; }
  odb::Rect getPinBox() { return pinBox_; }
  bool isRipup() const { return ripup_; }
  int getNumReroutes() const { return numReroutes_; }
  bool isInQueue() const { return inQueue_; }
  bool isRouted() const { return routed_; }
  bool isTrivial() const { return trivial_; }

  // setters
  void addPin(std::unique_ptr<grPin>& in)
  {
    in->setNet(this);
    pins_.push_back(std::move(in));
  }
  void addRouteConnFig(std::unique_ptr<grConnFig>& in)
  {
    in->addToNet(this);
    routeConnFigs_.push_back(std::move(in));
  }
  void clearRouteConnFigs() { routeConnFigs_.clear(); }
  void addExtConnFig(std::unique_ptr<grConnFig>& in)
  {
    in->addToNet(this);
    extConnFigs_.push_back(std::move(in));
  }
  void clear()
  {
    // routeConnFigs.clear();
    modified_ = true;
    numOverConGCells_ = 0;
    routed_ = false;
  }
  void setFrNet(frNet* in) { fNet_ = in; }
  void setFrRoot(frNode* in) { frRoot_ = in; }
  void addPinGCellNodePair(std::pair<grNode*, grNode*>& in)
  {
    pinGCellNodePairs_.push_back(in);
  }
  void setPinGCellNodePairs(const std::vector<std::pair<grNode*, grNode*>>& in)
  {
    pinGCellNodePairs_ = in;
  }
  void setGCell2PinNodes(
      const frOrderedIdMap<grNode*, std::vector<grNode*>>& in)
  {
    gcell2PinNodes_ = in;
  }
  void setPinGCellNodes(const std::vector<grNode*>& in) { pinGCellNodes_ = in; }
  void setPinNodePairs(const std::vector<std::pair<frNode*, grNode*>>& in)
  {
    pinNodePairs_ = in;
  }
  void setGR2FrPinNode(const frOrderedIdMap<grNode*, frNode*>& in)
  {
    gr2FrPinNode_ = in;
  }
  void addNode(std::unique_ptr<grNode>& in)
  {
    in->addToNet(this);
    auto rptr = in.get();
    if (nodes_.empty()) {
      rptr->setId(0);
    } else {
      rptr->setId(nodes_.back()->getId() + 1);
    }
    nodes_.push_back(std::move(in));
    rptr->setIter(--nodes_.end());
  }
  void removeNode(grNode* in) { nodes_.erase(in->getIter()); }
  void setRoot(grNode* in) { root_ = in; }
  void setModified(bool in) { modified_ = in; }
  void setNumOverConGCell(int in) { numOverConGCells_ = in; }
  void setNumPinsIn(int in) { numPinsIn_ = in; }
  void setAllowRipup(bool in) { allowRipup_ = in; }
  void setPinBox(const odb::Rect& in) { pinBox_ = in; }
  void setRipup(bool in) { ripup_ = in; }
  void addNumReroutes() { numReroutes_++; }
  void resetNumReroutes() { numReroutes_ = 0; }
  void setInQueue(bool in) { inQueue_ = in; }
  void setRouted(bool in) { routed_ = in; }
  void setTrivial(bool in) { trivial_ = in; }
  void cleanup()
  {
    pins_.clear();
    pins_.shrink_to_fit();
    extConnFigs_.clear();
    extConnFigs_.shrink_to_fit();
    routeConnFigs_.clear();
    routeConnFigs_.shrink_to_fit();
    nodes_.clear();
    // fNetTerms_.clear();
  }

  // others
  frBlockObjectEnum typeId() const override { return grcNet; }

  bool operator<(const grNet& b) const
  {
    return (numOverConGCells_ == b.numOverConGCells_)
               ? (getId() < b.getId())
               : (numOverConGCells_ > b.numOverConGCells_);
  }

 protected:
  std::vector<std::unique_ptr<grPin>> pins_;
  std::vector<std::unique_ptr<grConnFig>> extConnFigs_;
  std::vector<std::unique_ptr<grConnFig>> routeConnFigs_;
  // std::vector<std::unique_ptr<grConnFig> > bestRouteConnFigs_;

  // pair of <pinNode, gcellNode> with first (0th) element always being root
  std::vector<std::pair<grNode*, grNode*>> pinGCellNodePairs_;
  frOrderedIdMap<grNode*, std::vector<grNode*>> gcell2PinNodes_;
  // unique, first (0th) element always being root
  std::vector<grNode*> pinGCellNodes_;
  std::vector<std::pair<frNode*, grNode*>> pinNodePairs_;
  frOrderedIdMap<grNode*, frNode*> gr2FrPinNode_;
  // std::set<frBlockObject*>                 fNetTerms_;
  std::list<std::unique_ptr<grNode>> nodes_;
  grNode* root_{nullptr};
  frNet* fNet_{nullptr};
  frNode* frRoot_{nullptr};  // subnet frRoot

  bool modified_{false};
  int numOverConGCells_{0};
  int numPinsIn_{0};
  bool allowRipup_{true};
  odb::Rect pinBox_;
  bool ripup_{false};

  int numReroutes_{0};
  bool inQueue_{false};
  bool routed_{false};
  bool trivial_{false};
};
}  // namespace drt
