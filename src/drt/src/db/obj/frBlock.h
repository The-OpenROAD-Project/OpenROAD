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

#include <algorithm>
#include <type_traits>

#include "db/obj/frBTerm.h"
#include "db/obj/frBlockage.h"
#include "db/obj/frBoundary.h"
#include "db/obj/frGCellPattern.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frMarker.h"
#include "db/obj/frNet.h"
#include "db/obj/frTrackPattern.h"
#include "frBaseTypes.h"

namespace drt {
namespace io {
class Parser;
}
class frGuide;
class frBlock : public frBlockObject
{
 public:
  // constructors
  frBlock(const frString& name) : name_(name) {}
  // getters
  frUInt4 getDBUPerUU() const { return dbUnit_; }
  Rect getBBox() const
  {
    Rect box;
    if (!boundaries_.empty()) {
      box = boundaries_.begin()->getBBox();
    }
    frCoord llx = box.xMin();
    frCoord lly = box.yMin();
    frCoord urx = box.xMax();
    frCoord ury = box.yMax();
    for (auto& boundary : boundaries_) {
      Rect tmpBox = boundary.getBBox();
      llx = llx < tmpBox.xMin() ? llx : tmpBox.xMin();
      lly = lly < tmpBox.yMin() ? lly : tmpBox.yMin();
      urx = urx > tmpBox.xMax() ? urx : tmpBox.xMax();
      ury = ury > tmpBox.yMax() ? ury : tmpBox.yMax();
    }
    for (auto& inst : getInsts()) {
      Rect tmpBox = inst->getBBox();
      llx = llx < tmpBox.xMin() ? llx : tmpBox.xMin();
      lly = lly < tmpBox.yMin() ? lly : tmpBox.yMin();
      urx = urx > tmpBox.xMax() ? urx : tmpBox.xMax();
      ury = ury > tmpBox.yMax() ? ury : tmpBox.yMax();
    }
    for (auto& term : getTerms()) {
      for (auto& pin : term->getPins()) {
        for (auto& fig : pin->getFigs()) {
          Rect tmpBox = fig->getBBox();
          llx = llx < tmpBox.xMin() ? llx : tmpBox.xMin();
          lly = lly < tmpBox.yMin() ? lly : tmpBox.yMin();
          urx = urx > tmpBox.xMax() ? urx : tmpBox.xMax();
          ury = ury > tmpBox.yMax() ? ury : tmpBox.yMax();
        }
      }
    }
    return Rect(llx, lly, urx, ury);
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
  frInst* findInst(const std::string& name) const
  {
    if (name2inst_.find(name) != name2inst_.end()) {
      return name2inst_.at(name);
    }
    return nullptr;
  }
  frNet* getNet(int id) const
  {
    if (id >= nets_.size()) {
      return nullptr;
    }
    return nets_[id].get();
  }
  frNet* getSNet(int id) const
  {
    if (id >= snets_.size()) {
      return nullptr;
    }
    return snets_[id].get();
  }
  const std::vector<std::unique_ptr<frNet>>& getNets() const { return nets_; }
  frNet* findNet(const std::string& name) const
  {
    if (name2net_.find(name) != name2net_.end()) {
      return name2net_.at(name);
    }
    if (name2snet_.find(name) != name2snet_.end()) {
      return name2snet_.at(name);
    }
    return nullptr;
  }
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
  // isHorizontal means vertical tracks
  std::vector<frTrackPattern*> getTrackPatterns(frCoord layerNum,
                                                bool isHorizontal) const
  {
    std::vector<frTrackPattern*> tps;
    for (auto& t : trackPatterns_.at(layerNum)) {
      if (t->isHorizontal() == isHorizontal) {
        tps.push_back(t.get());
      }
    }
    return tps;
  }
  const std::vector<std::unique_ptr<frTrackPattern>>& getTrackPatterns(
      frLayerNum lNum) const
  {
    return trackPatterns_.at(lNum);
  }
  const std::vector<std::unique_ptr<frBTerm>>& getTerms() const
  {
    return terms_;
  }
  frBTerm* getTerm(const std::string& in) const
  {
    auto it = name2term_.find(in);
    if (it == name2term_.end()) {
      return nullptr;
    }
    return it->second;
  }
  frInst* getInst(const std::string& in) const
  {
    auto it = name2inst_.find(in);
    if (it == name2inst_.end()) {
      return nullptr;
    }
    return it->second;
  }
  frCoord getGCellSizeHorizontal()
  {
    return getGCellPatterns()[0].getSpacing();
  }
  frCoord getGCellSizeVertical() { return getGCellPatterns()[1].getSpacing(); }
  // idx must be legal
  Rect getGCellBox(const Point& idx1) const
  {
    Point idx(idx1);
    Rect dieBox = getDieBox();
    auto& gp = getGCellPatterns();
    auto& xgp = gp[0];
    auto& ygp = gp[1];
    if (idx.x() <= 0) {
      idx = {0, idx.y()};
    }
    if (idx.y() <= 0) {
      idx = {idx.x(), 0};
    }
    if (idx.x() >= (int) xgp.getCount() - 1) {
      idx = {(int) xgp.getCount() - 1, idx.y()};
    }
    if (idx.y() >= (int) ygp.getCount() - 1) {
      idx = {idx.x(), (int) ygp.getCount() - 1};
    }
    frCoord xl = (frCoord) xgp.getSpacing() * idx.x() + xgp.getStartCoord();
    frCoord yl = (frCoord) ygp.getSpacing() * idx.y() + ygp.getStartCoord();
    frCoord xh
        = (frCoord) xgp.getSpacing() * (idx.x() + 1) + xgp.getStartCoord();
    frCoord yh
        = (frCoord) ygp.getSpacing() * (idx.y() + 1) + ygp.getStartCoord();
    if (idx.x() <= 0) {
      xl = dieBox.xMin();
    }
    if (idx.y() <= 0) {
      yl = dieBox.yMin();
    }
    if (idx.x() >= (int) xgp.getCount() - 1) {
      xh = dieBox.xMax();
    }
    if (idx.y() >= (int) ygp.getCount() - 1) {
      yh = dieBox.yMax();
    }
    return Rect(xl, yl, xh, yh);
  }
  Point getGCellCenter(const Point& idx) const
  {
    Rect dieBox = getDieBox();
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
      xl = dieBox.xMin();
    }
    if (idx.y() == 0) {
      yl = dieBox.yMin();
    }
    if (idx.x() == (int) xgp.getCount() - 1) {
      xh = dieBox.xMax();
    }
    if (idx.y() == (int) ygp.getCount() - 1) {
      yh = dieBox.yMax();
    }
    return Point((xl + xh) / 2, (yl + yh) / 2);
  }
  Point getGCellIdx(const Point& pt) const
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
    return Point(idxX, idxY);
  }
  bool isValidGCellIdx(const Point& pt) const
  {
    const auto& gp = getGCellPatterns();
    const auto& xgp = gp[0];
    const auto& ygp = gp[1];
    return pt.x() >= 0 && pt.x() < xgp.getCount() && pt.y() >= 0
           && pt.y() < ygp.getCount();
  }
  const frList<std::unique_ptr<frMarker>>& getMarkers() const
  {
    return markers_;
  }
  frMarker* getMarker(const int& id) { return all_markers_[id]; }
  int getNumMarkers() const { return markers_.size(); }
  frNet* getFakeVSSNet() { return fakeSNets_[0].get(); }
  frNet* getFakeVDDNet() { return fakeSNets_[1].get(); }

  // setters
  void setDBUPerUU(frUInt4 uIn) { dbUnit_ = uIn; }
  void addTerm(std::unique_ptr<frBTerm> in)
  {
    in->setIndexInOwner(terms_.size());
    in->setId(upcoming_term_id_++);
    name2term_[in->getName()] = in.get();
    terms_.push_back(std::move(in));
  }
  void addInst(std::unique_ptr<frInst> in)
  {
    in->setId(upcoming_inst_id_++);
    for (auto& iterm : in->getInstTerms()) {
      iterm->setId(upcoming_term_id_++);
    }
    for (auto& iblk : in->getInstBlockages()) {
      iblk->setId(upcoming_blkg_id_++);
    }
    name2inst_[in->getName()] = in.get();
    insts_.push_back(std::move(in));
  }
  void removeInst(frInst* inst)
  {
    for (const auto& iterm : inst->getInstTerms()) {
      auto net = iterm->getNet();
      if (net != nullptr) {
        net->removeInstTerm(iterm.get());
      }
    }
    name2inst_.erase(inst->getName());
    inst->setToBeDeleted(true);
  }
  void removeDeletedInsts()
  {
    insts_.erase(std::remove_if(insts_.begin(),
                                insts_.end(),
                                [](const std::unique_ptr<frInst>& inst) {
                                  return inst->isToBeDeleted();
                                }),
                 insts_.end());
    int id = 0;
    for (const auto& inst : insts_) {
      inst->setId(id++);
    }
  }
  void addNet(std::unique_ptr<frNet> in)
  {
    in->setId(nets_.size());
    name2net_[in->getName()] = in.get();
    nets_.push_back(std::move(in));
  }
  void addSNet(std::unique_ptr<frNet> in)
  {
    in->setId(snets_.size());
    name2snet_[in->getName()] = in.get();
    snets_.push_back(std::move(in));
  }
  const Rect& getDieBox() const { return dieBox_; }
  void setBoundaries(const std::vector<frBoundary>& in)
  {
    boundaries_ = in;
    if (!boundaries_.empty()) {
      dieBox_ = boundaries_.begin()->getBBox();
    }
    frCoord llx = dieBox_.xMin();
    frCoord lly = dieBox_.yMin();
    frCoord urx = dieBox_.xMax();
    frCoord ury = dieBox_.yMax();
    for (auto& boundary : boundaries_) {
      Rect tmpBox = boundary.getBBox();
      llx = std::min(llx, tmpBox.xMin());
      lly = std::min(lly, tmpBox.yMin());
      urx = std::max(urx, tmpBox.xMax());
      ury = std::max(ury, tmpBox.yMax());
    }
    dieBox_.init(llx, lly, urx, ury);
  }
  void setBlockages(std::vector<std::unique_ptr<frBlockage>>& in)
  {
    for (auto& blk : in) {
      blk->setIndexInOwner(blockages_.size());
      blockages_.push_back(std::move(blk));
    }
  }
  void addBlockage(std::unique_ptr<frBlockage> in)
  {
    in->setIndexInOwner(blockages_.size());
    in->setId(upcoming_blkg_id_++);
    blockages_.push_back(std::move(in));
  }
  void setGCellPatterns(const std::vector<frGCellPattern>& gpIn)
  {
    gCellPatterns_ = gpIn;
  }
  void addMarker(std::unique_ptr<frMarker> in)
  {
    auto rptr = in.get();
    markers_.push_back(std::move(in));
    rptr->setIter(--(markers_.end()));
    rptr->setIndexInOwner(all_markers_.size());
    all_markers_.push_back(rptr);
  }
  void removeMarker(frMarker* in) { markers_.erase(in->getIter()); }
  void addFakeSNet(std::unique_ptr<frNet> in)
  {
    fakeSNets_.push_back(std::move(in));
  }
  // others
  frBlockObjectEnum typeId() const override { return frcBlock; }

 private:
  frString name_;
  frUInt4 dbUnit_{0};

  std::map<std::string, frInst*> name2inst_;
  std::vector<std::unique_ptr<frInst>> insts_;

  std::map<std::string, frBTerm*> name2term_;
  std::vector<std::unique_ptr<frBTerm>> terms_;

  std::map<std::string, frNet*> name2net_;
  std::vector<std::unique_ptr<frNet>> nets_;

  std::map<std::string, frNet*> name2snet_;
  std::vector<std::unique_ptr<frNet>> snets_;

  std::vector<std::unique_ptr<frBlockage>> blockages_;

  std::vector<frBoundary> boundaries_;
  std::vector<std::vector<std::unique_ptr<frTrackPattern>>> trackPatterns_;
  std::vector<frGCellPattern> gCellPatterns_;

  frList<std::unique_ptr<frMarker>> markers_;
  std::vector<frMarker*> all_markers_;

  std::vector<std::unique_ptr<frNet>>
      fakeSNets_;  // 0 is floating VSS, 1 is floating VDD
  Rect dieBox_;

  frUInt4 upcoming_inst_id_{0};
  frUInt4 upcoming_term_id_{0};
  frUInt4 upcoming_blkg_id_{0};
  friend class io::Parser;
};

}  // namespace drt
