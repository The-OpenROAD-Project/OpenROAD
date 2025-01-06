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

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frGuide.h"
#include "db/obj/frNode.h"
#include "db/obj/frRPin.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "frBaseTypes.h"
#include "global.h"

namespace drt {
class frInstTerm;
class frTerm;
class frBTerm;
class frNonDefaultRule;

class frNet : public frBlockObject
{
 public:
  // constructors
  frNet(const frString& in, RouterConfiguration* router_cfg)
      : name_(in), router_cfg_(router_cfg)
  {
  }
  // getters
  const frString& getName() const { return name_; }
  const std::vector<frInstTerm*>& getInstTerms() const { return instTerms_; }
  const std::vector<frBTerm*>& getBTerms() const { return bterms_; }
  const std::list<std::unique_ptr<frShape>>& getShapes() const
  {
    return shapes_;
  }
  const std::list<std::unique_ptr<frVia>>& getVias() const { return vias_; }
  const std::list<std::unique_ptr<frShape>>& getPatchWires() const
  {
    return pwires_;
  }
  std::list<std::unique_ptr<grShape>>& getGRShapes() { return grShapes_; }
  const std::list<std::unique_ptr<grShape>>& getGRShapes() const
  {
    return grShapes_;
  }
  std::list<std::unique_ptr<grVia>>& getGRVias() { return grVias_; }
  const std::list<std::unique_ptr<grVia>>& getGRVias() const { return grVias_; }
  std::list<std::unique_ptr<frNode>>& getNodes() { return nodes_; }
  const std::list<std::unique_ptr<frNode>>& getNodes() const { return nodes_; }
  frNode* getRoot() { return root_; }
  frNode* getRootGCellNode() { return rootGCellNode_; }
  frNode* getFirstNonRPinNode() { return firstNonRPinNode_; }
  std::vector<std::unique_ptr<frRPin>>& getRPins() { return rpins_; }
  const std::vector<std::unique_ptr<frRPin>>& getRPins() const
  {
    return rpins_;
  }
  const std::vector<std::unique_ptr<frGuide>>& getGuides() const
  {
    return guides_;
  }
  bool isModified() const { return modified_; }
  bool isFake() const { return isFakeNet_; }
  frNonDefaultRule* getNondefaultRule() const { return ndr_; }
  bool hasInitialRouting() const { return hasInitialRouting_; }
  bool isFixed() const { return isFixed_; }
  bool hasGuides() const { return !guides_.empty(); }
  bool hasBTermsAboveTopLayer() const;
  // setters
  void addInstTerm(frInstTerm* in) { instTerms_.push_back(in); }
  void removeInstTerm(frInstTerm* in)
  {
    for (auto itr = instTerms_.begin(); itr != instTerms_.end(); itr++) {
      if (*itr == in) {
        instTerms_.erase(itr);
        return;
      }
    }
  }
  void addBTerm(frBTerm* in) { bterms_.push_back(in); }
  void setName(const frString& stringIn) { name_ = stringIn; }
  void addShape(std::unique_ptr<frShape> in)
  {
    in->addToNet(this);
    in->setIndexInOwner(all_pinfigs_.size());
    auto rptr = in.get();
    shapes_.push_back(std::move(in));
    rptr->setIter(--shapes_.end());
    all_pinfigs_.push_back(rptr);
  }
  void addVia(std::unique_ptr<frVia> in)
  {
    in->addToNet(this);
    in->setIndexInOwner(all_pinfigs_.size());
    auto rptr = in.get();
    vias_.push_back(std::move(in));
    rptr->setIter(--vias_.end());
    all_pinfigs_.push_back(rptr);
  }
  void addPatchWire(std::unique_ptr<frShape> in)
  {
    in->addToNet(this);
    in->setIndexInOwner(all_pinfigs_.size());
    auto rptr = in.get();
    pwires_.push_back(std::move(in));
    rptr->setIter(--pwires_.end());
    all_pinfigs_.push_back(rptr);
  }
  void addGRShape(std::unique_ptr<grShape>& in)
  {
    in->addToNet(this);
    auto rptr = in.get();
    grShapes_.push_back(std::move(in));
    rptr->setIter(--grShapes_.end());
  }
  void addGRVia(std::unique_ptr<grVia>& in)
  {
    in->addToNet(this);
    auto rptr = in.get();
    grVias_.push_back(std::move(in));
    rptr->setIter(--grVias_.end());
  }
  void addNode(std::unique_ptr<frNode>& in)
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
  void setRoot(frNode* in) { root_ = in; }
  void setRootGCellNode(frNode* in) { rootGCellNode_ = in; }
  void setFirstNonRPinNode(frNode* in) { firstNonRPinNode_ = in; }
  void addRPin(std::unique_ptr<frRPin>& in)
  {
    in->addToNet(this);
    rpins_.push_back(std::move(in));
  }
  void addGuide(std::unique_ptr<frGuide> in)
  {
    auto rptr = in.get();
    rptr->addToNet(this);
    in->setIndexInOwner(guides_.size());
    guides_.push_back(std::move(in));
  }
  void clearRPins() { rpins_.clear(); }
  void clearGuides() { guides_.clear(); }
  void clearOrigGuides() { orig_guides_.clear(); }
  void clearConns()
  {
    instTerms_.clear();
    bterms_.clear();
    nodes_.clear();
    root_ = nullptr;
    rootGCellNode_ = nullptr;
    firstNonRPinNode_ = nullptr;
  }
  void removeShape(frShape* in) { shapes_.erase(in->getIter()); }
  void removeVia(frVia* in) { vias_.erase(in->getIter()); }
  void removePatchWire(frShape* in) { pwires_.erase(in->getIter()); }
  void removeGRShape(grShape* in) { grShapes_.erase(in->getIter()); }
  void clearGRShapes() { grShapes_.clear(); }
  void removeGRVia(grVia* in) { grVias_.erase(in->getIter()); }
  void clearGRVias() { grVias_.clear(); }
  void removeNode(frNode* in) { nodes_.erase(in->getIter()); }
  void setModified(bool in) { modified_ = in; }
  void setIsFake(bool in) { isFakeNet_ = in; }
  void setHasInitialRouting(bool in) { hasInitialRouting_ = in; }
  void setFixed(bool in) { isFixed_ = in; }
  // others
  void clearRoutes()
  {
    shapes_.clear();
    vias_.clear();
    pwires_.clear();
  }
  dbSigType getType() const { return type_; }
  void setType(const dbSigType& in) { type_ = in; }
  frBlockObjectEnum typeId() const override { return frcNet; }
  void updateNondefaultRule(frNonDefaultRule* n)
  {
    ndr_ = n;
    updateAbsPriority();
  }
  bool hasNDR() const { return getNondefaultRule() != nullptr; }
  void setAbsPriorityLvl(int l) { absPriorityLvl = l; }
  int getAbsPriorityLvl() const { return absPriorityLvl; }
  bool isClock() const { return isClock_; }
  void updateIsClock(bool ic)
  {
    isClock_ = ic;
    updateAbsPriority();
  }
  void updateAbsPriority()
  {
    int max = absPriorityLvl;
    if (hasNDR()) {
      max = std::max(max, router_cfg_->NDR_NETS_ABS_PRIORITY);
    }
    if (isClock()) {
      max = std::max(max, router_cfg_->CLOCK_NETS_ABS_PRIORITY);
    }
    absPriorityLvl = max;
  }
  bool isSpecial() const { return isSpecial_; }
  void setIsSpecial(bool s) { isSpecial_ = s; }
  frPinFig* getPinFig(const int& id) { return all_pinfigs_[id]; }
  void setOrigGuides(const std::vector<frRect>& guides)
  {
    orig_guides_ = guides;
  }
  const std::vector<frRect>& getOrigGuides() const { return orig_guides_; }
  void setHasJumpers(bool has_jumpers) { has_jumpers_ = has_jumpers; }
  bool hasJumpers() { return has_jumpers_; }

 protected:
  frString name_;
  RouterConfiguration* router_cfg_;
  std::vector<frInstTerm*> instTerms_;
  std::vector<frBTerm*> bterms_;
  // dr
  std::list<std::unique_ptr<frShape>> shapes_;
  std::list<std::unique_ptr<frVia>> vias_;
  std::list<std::unique_ptr<frShape>> pwires_;
  // gr
  std::list<std::unique_ptr<grShape>> grShapes_;
  std::list<std::unique_ptr<grVia>> grVias_;
  //
  std::list<std::unique_ptr<frNode>>
      nodes_;  // the nodes at the beginning of the list correspond to rpins
               // there is no guarantee that first element is root
  frNode* root_{nullptr};
  frNode* rootGCellNode_{nullptr};
  frNode* firstNonRPinNode_{nullptr};
  std::vector<std::unique_ptr<frRPin>> rpins_;
  std::vector<std::unique_ptr<frGuide>> guides_;
  std::vector<frRect> orig_guides_;
  dbSigType type_{dbSigType::SIGNAL};
  bool modified_{false};
  bool isFakeNet_{false};  // indicate floating PG nets
  frNonDefaultRule* ndr_{nullptr};
  int absPriorityLvl{0};  // absolute priority level: will be checked in net
                          // ordering before other criteria
  bool isClock_{false};
  bool isSpecial_{false};
  bool hasInitialRouting_{false};
  bool isFixed_{false};

  // Flag to mark when a frNet has a jumper, which is a special route guide used
  // to prevent antenna violations
  bool has_jumpers_{false};
  std::vector<frPinFig*> all_pinfigs_;
};
}  // namespace drt
