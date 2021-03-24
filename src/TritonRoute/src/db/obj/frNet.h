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

#ifndef _FR_NET_H_
#define _FR_NET_H_

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frGuide.h"
#include "db/obj/frNode.h"
#include "db/obj/frRPin.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "frBaseTypes.h"

namespace fr {
class frInstTerm;
class frTerm;
class frNonDefaultRule;

class frNet : public frBlockObject
{
 public:
  // constructors
  frNet(const frString& in)
      : frBlockObject(),
        name_(in),
        instTerms_(),
        terms_(),
        shapes_(),
        vias_(),
        pwires_(),
        grShapes_(),
        grVias_(),
        nodes_(),
        root_(nullptr),
        rootGCellNode_(nullptr),
        firstNonRPinNode_(nullptr),
        rpins_(),
        guides_(),
        type_(frNetEnum::frcNormalNet),
        modified_(false),
        isFakeNet_(false),
        ndr_(nullptr)
  {
  }
  // getters
  const frString& getName() const { return name_; }
  const std::vector<frInstTerm*>& getInstTerms() const { return instTerms_; }
  const std::vector<frTerm*>& getTerms() const { return terms_; }
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
  std::vector<std::unique_ptr<frGuide>>& getGuides() { return guides_; }
  const std::vector<std::unique_ptr<frGuide>>& getGuides() const
  {
    return guides_;
  }
  bool isModified() const { return modified_; }
  bool isFake() const { return isFakeNet_; }
  frNonDefaultRule* getNondefaultRule() const { return ndr_; }
  // setters
  void addInstTerm(frInstTerm* in) { instTerms_.push_back(in); }
  void addTerm(frTerm* in) { terms_.push_back(in); }
  void setName(const frString& stringIn) { name_ = stringIn; }
  void addShape(std::unique_ptr<frShape> in)
  {
    in->addToNet(this);
    auto rptr = in.get();
    shapes_.push_back(std::move(in));
    rptr->setIter(--shapes_.end());
  }
  void addVia(std::unique_ptr<frVia> in)
  {
    in->addToNet(this);
    auto rptr = in.get();
    vias_.push_back(std::move(in));
    rptr->setIter(--vias_.end());
  }
  void addPatchWire(std::unique_ptr<frShape> in)
  {
    in->addToNet(this);
    auto rptr = in.get();
    pwires_.push_back(std::move(in));
    rptr->setIter(--pwires_.end());
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
    guides_.push_back(std::move(in));
  }
  void clearGuides() { guides_.clear(); }
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
  // others
  frNetEnum getType() const { return type_; }
  void setType(frNetEnum in) { type_ = in; }
  virtual frBlockObjectEnum typeId() const override { return frcNet; }
  void setNondefaultRule(frNonDefaultRule* n) { ndr_ = n; }
  bool hasNDR() const { return getNondefaultRule() != nullptr; }

 protected:
  frString name_;
  std::vector<frInstTerm*> instTerms_;
  std::vector<frTerm*> terms_;  // terms is IO
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
  frNode* root_;
  frNode* rootGCellNode_;
  frNode* firstNonRPinNode_;
  std::vector<std::unique_ptr<frRPin>> rpins_;
  std::vector<std::unique_ptr<frGuide>> guides_;
  frNetEnum type_;
  bool modified_;
  bool isFakeNet_;  // indicate floating PG nets
  frNonDefaultRule* ndr_;
};
}  // namespace fr

#endif
