// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "db/obj/frFig.h"

namespace drt {
class frConstraint;
class frMarker : public frFig
{
 public:
  // constructors
  frMarker() = default;
  frMarker(const frMarker& in)
      : constraint_(in.constraint_),
        bbox_(in.bbox_),
        layerNum_(in.layerNum_),
        srcs_(in.srcs_),
        vioHasDir_(in.vioHasDir_),
        vioIsH_(in.vioIsH_)
  {
  }
  // setters
  void setConstraint(frConstraint* constraintIn) { constraint_ = constraintIn; }

  void setBBox(const Rect& bboxIn) { bbox_ = bboxIn; }

  void setLayerNum(const frLayerNum& layerNumIn) { layerNum_ = layerNumIn; }

  void setHasDir(const bool& in) { vioHasDir_ = in; }

  void setIsH(const bool& in) { vioIsH_ = in; }

  void addSrc(frBlockObject* srcIn) { srcs_.insert(srcIn); }
  void addAggressor(frBlockObject* obj,
                    const std::tuple<frLayerNum, Rect, bool>& tupleIn)
  {
    aggressors_.emplace_back(obj, tupleIn);
  }
  void addVictim(frBlockObject* obj,
                 const std::tuple<frLayerNum, Rect, bool>& tupleIn)
  {
    victims_.emplace_back(obj, tupleIn);
  }
  // getters

  /* from frFig
   * getBBox
   * move, in .cpp
   * intersects in .cpp
   */

  Rect getBBox() const override { return bbox_; }
  frLayerNum getLayerNum() const { return layerNum_; }

  const std::set<frBlockObject*>& getSrcs() const { return srcs_; }

  void setSrcs(const std::set<frBlockObject*>& srcs) { srcs_ = srcs; }

  std::vector<std::pair<frBlockObject*, std::tuple<frLayerNum, Rect, bool>>>&
  getAggressors()
  {
    return aggressors_;
  }

  std::vector<std::pair<frBlockObject*, std::tuple<frLayerNum, Rect, bool>>>&
  getVictims()
  {
    return victims_;
  }

  frConstraint* getConstraint() const { return constraint_; }

  bool hasDir() const { return vioHasDir_; }

  bool isH() const { return vioIsH_; }

  void move(const dbTransform& xform) override {}

  bool intersects(const Rect& box) const override { return false; }

  // others
  frBlockObjectEnum typeId() const override { return frcMarker; }

  void setIter(frListIter<std::unique_ptr<frMarker>>& in) { iter_ = in; }
  frListIter<std::unique_ptr<frMarker>> getIter() const { return iter_; }
  void setIndexInOwner(const int& idx) { index_in_owner_ = idx; }
  int getIndexInOwner() const { return index_in_owner_; }

 private:
  frConstraint* constraint_{nullptr};
  Rect bbox_;
  frLayerNum layerNum_{0};
  std::set<frBlockObject*> srcs_;
  std::vector<std::pair<frBlockObject*, std::tuple<frLayerNum, Rect, bool>>>
      victims_;  // obj, isFixed
  std::vector<std::pair<frBlockObject*, std::tuple<frLayerNum, Rect, bool>>>
      aggressors_;  // obj, isFixed
  frListIter<std::unique_ptr<frMarker>> iter_;
  bool vioHasDir_{false};
  bool vioIsH_{false};
  int index_in_owner_{0};

  template <class Archive>
  void serialize(Archive& ar, unsigned int version);

  friend class boost::serialization::access;
};
}  // namespace drt
