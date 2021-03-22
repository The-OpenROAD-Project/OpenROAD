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

#ifndef _FR_BLOCK_H_
#define _FR_BLOCK_H_

#include <algorithm>

#include "db/obj/frBlockage.h"
#include "db/obj/frBoundary.h"
#include "db/obj/frGCellPattern.h"
#include "db/obj/frInst.h"
#include "db/obj/frMarker.h"
#include "db/obj/frNet.h"
#include "db/obj/frTerm.h"
#include "db/obj/frTrackPattern.h"
#include "frBaseTypes.h"

namespace fr {
namespace io {
class Parser;
}
class frGuide;
class frBlock : public frBlockObject
{
 public:
  // constructors
  frBlock(const frString& name)
      : frBlockObject(),
        name_(name),
        dbUnit_(0) /*, manufacturingGrid_(0)*/,
        macroClass_(MacroClassEnum::UNKNOWN){};
  // getters
  frUInt4 getDBUPerUU() const { return dbUnit_; }
  void getBBox(frBox& boxIn) const
  {
    if (boundaries_.size()) {
      boundaries_.begin()->getBBox(boxIn);
    }
    frCoord llx = boxIn.left();
    frCoord lly = boxIn.bottom();
    frCoord urx = boxIn.right();
    frCoord ury = boxIn.top();
    frBox tmpBox;
    for (auto& boundary : boundaries_) {
      boundary.getBBox(tmpBox);
      llx = llx < tmpBox.left() ? llx : tmpBox.left();
      lly = lly < tmpBox.bottom() ? lly : tmpBox.bottom();
      urx = urx > tmpBox.right() ? urx : tmpBox.right();
      ury = ury > tmpBox.top() ? ury : tmpBox.top();
    }
    for (auto& inst : getInsts()) {
      inst->getBBox(tmpBox);
      llx = llx < tmpBox.left() ? llx : tmpBox.left();
      lly = lly < tmpBox.bottom() ? lly : tmpBox.bottom();
      urx = urx > tmpBox.right() ? urx : tmpBox.right();
      ury = ury > tmpBox.top() ? ury : tmpBox.top();
    }
    for (auto& term : getTerms()) {
      for (auto& pin : term->getPins()) {
        for (auto& fig : pin->getFigs()) {
          fig->getBBox(tmpBox);
          llx = llx < tmpBox.left() ? llx : tmpBox.left();
          lly = lly < tmpBox.bottom() ? lly : tmpBox.bottom();
          urx = urx > tmpBox.right() ? urx : tmpBox.right();
          ury = ury > tmpBox.top() ? ury : tmpBox.top();
        }
      }
    }
    boxIn.set(llx, lly, urx, ury);
  }
  void getBoundaryBBox(frBox& boxIn) const
  {
    if (boundaries_.size()) {
      boundaries_.begin()->getBBox(boxIn);
    }
    frCoord llx = boxIn.left();
    frCoord lly = boxIn.bottom();
    frCoord urx = boxIn.right();
    frCoord ury = boxIn.top();
    frBox tmpBox;
    for (auto& boundary : boundaries_) {
      boundary.getBBox(tmpBox);
      llx = llx < tmpBox.left() ? llx : tmpBox.left();
      lly = lly < tmpBox.bottom() ? lly : tmpBox.bottom();
      urx = urx > tmpBox.right() ? urx : tmpBox.right();
      ury = ury > tmpBox.top() ? ury : tmpBox.top();
    }
    boxIn.set(llx, lly, urx, ury);
  }
  const std::vector<frBoundary>& getBoundaries() const { return boundaries_; }
  const std::vector<std::unique_ptr<frBlockage>>& getBlockages() const
  {
    return blockages_;
  }
  const std::vector<frGCellPattern>& getGCellPatterns() const
  {
    return gCellPatterns_;
  }
  std::vector<frGuide*> getGuides() const
  {
    std::vector<frGuide*> sol;
    for (auto& net : getNets()) {
      for (auto& guide : net->getGuides()) {
        sol.push_back(guide.get());
      }
    }
    return sol;
  }
  const frString& getName() const { return name_; }
  const std::vector<std::unique_ptr<frInst>>& getInsts() const
  {
    return insts_;
  }
  const std::vector<std::unique_ptr<frNet>>& getNets() const { return nets_; }
  const std::vector<std::unique_ptr<frNet>>& getSNets() const { return snets_; }
  std::vector<frTrackPattern*> getTrackPatterns() const
  {
    std::vector<frTrackPattern*> sol;
    for (auto& m : trackPatterns_) {
      for (auto& n : m) {
        sol.push_back(n.get());
      }
    }
    return sol;
  }
  const std::vector<std::unique_ptr<frTrackPattern>>& getTrackPatterns(
      frLayerNum lNum) const
  {
    return trackPatterns_.at(lNum);
  }
  const std::vector<std::unique_ptr<frTerm>>& getTerms() const
  {
    return terms_;
  }
  frTerm* getTerm(const std::string& in) const
  {
    auto it = name2term_.find(in);
    if (it == name2term_.end()) {
      return nullptr;
    } else {
      return it->second;
    }
  }
  // idx must be legal
  void getGCellBox(const frPoint& idx1, frBox& box) const
  {
    frPoint idx(idx1);
    frBox dieBox;
    getBoundaryBBox(dieBox);
    auto& gp = getGCellPatterns();
    auto& xgp = gp[0];
    auto& ygp = gp[1];
    if (idx.x() <= 0) {
      idx.set(0, idx.y());
    }
    if (idx.y() <= 0) {
      idx.set(idx.x(), 0);
    }
    if (idx.x() >= (int) xgp.getCount() - 1) {
      idx.set((int) xgp.getCount() - 1, idx.y());
    }
    if (idx.y() >= (int) ygp.getCount() - 1) {
      idx.set(idx.x(), (int) ygp.getCount() - 1);
    }
    frCoord xl = (frCoord) xgp.getSpacing() * idx.x() + xgp.getStartCoord();
    frCoord yl = (frCoord) ygp.getSpacing() * idx.y() + ygp.getStartCoord();
    frCoord xh
        = (frCoord) xgp.getSpacing() * (idx.x() + 1) + xgp.getStartCoord();
    frCoord yh
        = (frCoord) ygp.getSpacing() * (idx.y() + 1) + ygp.getStartCoord();
    if (idx.x() <= 0) {
      xl = dieBox.left();
    }
    if (idx.y() <= 0) {
      yl = dieBox.bottom();
    }
    if (idx.x() >= (int) xgp.getCount() - 1) {
      xh = dieBox.right();
    }
    if (idx.y() >= (int) ygp.getCount() - 1) {
      yh = dieBox.top();
    }
    box.set(xl, yl, xh, yh);
  }
  void getGCellCenter(const frPoint& idx, frPoint& pt) const
  {
    frBox dieBox;
    getBoundaryBBox(dieBox);
    auto& gp = getGCellPatterns();
    auto& xgp = gp[0];
    auto& ygp = gp[1];
    frCoord xl = (frCoord) xgp.getSpacing() * idx.x() + xgp.getStartCoord();
    frCoord yl = (frCoord) ygp.getSpacing() * idx.y() + ygp.getStartCoord();
    frCoord xh
        = (frCoord) xgp.getSpacing() * (idx.x() + 1) + xgp.getStartCoord();
    frCoord yh
        = (frCoord) ygp.getSpacing() * (idx.y() + 1) + ygp.getStartCoord();
    if (idx.x() == 0) {
      xl = dieBox.left();
    }
    if (idx.y() == 0) {
      yl = dieBox.bottom();
    }
    if (idx.x() == (int) xgp.getCount() - 1) {
      xh = dieBox.right();
    }
    if (idx.y() == (int) ygp.getCount() - 1) {
      yh = dieBox.top();
    }
    pt.set((xl + xh) / 2, (yl + yh) / 2);
  }
  void getGCellIdx(const frPoint& pt, frPoint& idx) const
  {
    auto& gp = getGCellPatterns();
    auto& xgp = gp[0];
    auto& ygp = gp[1];
    frCoord idxX = (pt.x() - xgp.getStartCoord()) / (frCoord) xgp.getSpacing();
    frCoord idxY = (pt.y() - ygp.getStartCoord()) / (frCoord) ygp.getSpacing();
    if (idxX < 0) {
      idxX = 0;
    }
    if (idxY < 0) {
      idxY = 0;
    }
    if (idxX >= (int) xgp.getCount()) {
      idxX = (int) xgp.getCount() - 1;
    }
    if (idxY >= (int) ygp.getCount()) {
      idxY = (int) ygp.getCount() - 1;
    }
    idx.set(idxX, idxY);
  }
  MacroClassEnum getMacroClass() { return macroClass_; }
  const frList<std::unique_ptr<frMarker>>& getMarkers() const
  {
    return markers_;
  }
  int getNumMarkers() const { return markers_.size(); }
  frNet* getFakeVSSNet() { return fakeSNets_[0].get(); }
  frNet* getFakeVDDNet() { return fakeSNets_[1].get(); }

  // setters
  void setDBUPerUU(frUInt4 uIn) { dbUnit_ = uIn; }
  void addTerm(std::unique_ptr<frTerm> in)
  {
    in->setOrderId(terms_.size());
    in->setBlock(this);
    name2term_[in->getName()] = in.get();
    terms_.push_back(std::move(in));
  }
  void addInst(std::unique_ptr<frInst> in)
  {
    name2inst_[in->getName()] = in.get();
    insts_.push_back(std::move(in));
  }
  void addNet(std::unique_ptr<frNet> in)
  {
    name2net_[in->getName()] = in.get();
    nets_.push_back(std::move(in));
  }
  void addSNet(std::unique_ptr<frNet> in)
  {
    name2snet_[in->getName()] = in.get();
    snets_.push_back(std::move(in));
  }
  void setBoundaries(const std::vector<frBoundary> in) { boundaries_ = in; }
  void setBlockages(std::vector<std::unique_ptr<frBlockage>>& in)
  {
    for (auto& blk : in) {
      blockages_.push_back(std::move(blk));
    }
  }
  void addBlockage(std::unique_ptr<frBlockage> in)
  {
    blockages_.push_back(std::move(in));
  }
  void setGCellPatterns(const std::vector<frGCellPattern>& gpIn)
  {
    gCellPatterns_ = gpIn;
  }
  void setMacroClass(const MacroClassEnum& in) { macroClass_ = in; }
  void addMarker(std::unique_ptr<frMarker> in)
  {
    auto rptr = in.get();
    markers_.push_back(std::move(in));
    rptr->setIter(--(markers_.end()));
  }
  void removeMarker(frMarker* in) { markers_.erase(in->getIter()); }
  void addFakeSNet(std::unique_ptr<frNet> in)
  {
    fakeSNets_.push_back(std::move(in));
  }
  // others
  frBlockObjectEnum typeId() const override { return frcBlock; }
  friend class io::Parser;

 protected:
  frString name_;
  frUInt4 dbUnit_;

  MacroClassEnum macroClass_;

  std::map<std::string, frInst*> name2inst_;
  std::vector<std::unique_ptr<frInst>> insts_;

  std::map<std::string, frTerm*> name2term_;
  std::vector<std::unique_ptr<frTerm>> terms_;

  std::map<std::string, frNet*> name2net_;
  std::vector<std::unique_ptr<frNet>> nets_;

  std::map<std::string, frNet*> name2snet_;
  std::vector<std::unique_ptr<frNet>> snets_;

  std::vector<std::unique_ptr<frBlockage>> blockages_;

  std::vector<frBoundary> boundaries_;
  std::vector<std::vector<std::unique_ptr<frTrackPattern>>> trackPatterns_;
  std::vector<frGCellPattern> gCellPatterns_;

  frList<std::unique_ptr<frMarker>> markers_;

  std::vector<std::unique_ptr<frNet>>
      fakeSNets_;  // 0 is floating VSS, 1 is floating VDD
};
}  // namespace fr

#endif
