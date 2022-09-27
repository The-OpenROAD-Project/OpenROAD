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

#ifndef _FR_MASTER_H_
#define _FR_MASTER_H_

#include <algorithm>

#include "db/obj/frBlockage.h"
#include "db/obj/frBoundary.h"
#include "db/obj/frMTerm.h"
#include "frBaseTypes.h"

namespace fr {
namespace io {
class Parser;
}
class frMaster : public frBlockObject
{
 public:
  // constructors
  frMaster(odb::dbMaster* master) : frBlockObject(), db_master_(master){};
  // getters
  odb::dbMaster* getDbMaster() const { return db_master_; }
  Rect getBBox() const
  {
    Rect box;
    if (getBoundaries().size()) {
      box = getBoundaries().begin()->getBBox();
    }
    frCoord llx = box.xMin();
    frCoord lly = box.yMin();
    frCoord urx = box.xMax();
    frCoord ury = box.yMax();
    for (auto& boundary : getBoundaries()) {
      Rect tmpBox = boundary.getBBox();
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
  Rect getDieBox() const
  {
    Rect dieBox;
    if (!getBoundaries().empty()) {
      dieBox = getBoundaries().begin()->getBBox();
    }
    frCoord llx = dieBox.xMin();
    frCoord lly = dieBox.yMin();
    frCoord urx = dieBox.xMax();
    frCoord ury = dieBox.yMax();
    for (auto& boundary : getBoundaries()) {
      Rect tmpBox = boundary.getBBox();
      llx = std::min(llx, tmpBox.xMin());
      lly = std::min(lly, tmpBox.yMin());
      urx = std::max(urx, tmpBox.xMax());
      ury = std::max(ury, tmpBox.yMax());
    }
    dieBox.init(llx, lly, urx, ury);
    return dieBox;
  }
  const std::vector<frBoundary> getBoundaries() const
  {
    frCoord originX;
    frCoord originY;
    db_master_->getOrigin(originX, originY);
    frCoord sizeX = db_master_->getWidth();
    frCoord sizeY = db_master_->getHeight();
    vector<frBoundary> bounds;
    frBoundary bound;
    vector<Point> points;
    points.push_back(Point(originX, originY));
    points.push_back(Point(sizeX, originY));
    points.push_back(Point(sizeX, sizeY));
    points.push_back(Point(originX, sizeY));
    bound.setPoints(points);
    bounds.push_back(bound);
    return bounds;
  }
  const std::vector<std::unique_ptr<frBlockage>>& getBlockages() const
  {
    return blockages_;
  }
  const frString getName() const { return db_master_->getName(); }
  const std::vector<std::unique_ptr<frMTerm>>& getTerms() const
  {
    return terms_;
  }
  frMTerm* getTerm(const std::string& in) const
  {
    for (const auto& term : terms_) {
      if (in == term->getName()) {
        return term.get();
      }
    }
    return nullptr;
  }
  dbMasterType getMasterType() { return db_master_->getType(); }

  // setters
  void addTerm(std::unique_ptr<frMTerm> in)
  {
    in->setIndexInOwner(terms_.size());
    in->setMaster(this);
    terms_.push_back(std::move(in));
  }
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
  // others
  frBlockObjectEnum typeId() const override { return frcMaster; }

 protected:
  std::vector<std::unique_ptr<frMTerm>> terms_;
  std::vector<std::unique_ptr<frBlockage>> blockages_;
  odb::dbMaster* db_master_;

  friend class io::Parser;
};
}  // namespace fr

#endif
