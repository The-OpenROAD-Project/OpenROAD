// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "db/obj/frBlockage.h"
#include "db/obj/frBoundary.h"
#include "db/obj/frMTerm.h"
#include "frBaseTypes.h"

namespace drt {
namespace io {
class Parser;
}
class frMaster : public frBlockObject
{
 public:
  // constructors
  frMaster(const frString& name) : name_(name) {}
  // getters
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
  Rect getDieBox() const { return dieBox_; }
  const std::vector<frBoundary>& getBoundaries() const { return boundaries_; }
  const std::vector<std::unique_ptr<frBlockage>>& getBlockages() const
  {
    return blockages_;
  }
  const frString& getName() const { return name_; }
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
  dbMasterType getMasterType() { return masterType_; }

  // setters
  void addTerm(std::unique_ptr<frMTerm> in)
  {
    in->setIndexInOwner(terms_.size());
    in->setMaster(this);
    terms_.push_back(std::move(in));
  }
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
      blockages_.push_back(std::move(blk));
    }
  }
  void addBlockage(std::unique_ptr<frBlockage> in)
  {
    blockages_.push_back(std::move(in));
  }
  void setMasterType(const dbMasterType& in) { masterType_ = in; }
  // others
  frBlockObjectEnum typeId() const override { return frcMaster; }

 protected:
  std::vector<std::unique_ptr<frMTerm>> terms_;
  std::vector<std::unique_ptr<frBlockage>> blockages_;
  std::vector<frBoundary> boundaries_;
  Rect dieBox_;
  frString name_;
  dbMasterType masterType_{dbMasterType::CORE};

  friend class io::Parser;
};
}  // namespace drt
