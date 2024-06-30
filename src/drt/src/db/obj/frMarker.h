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

#include <set>
#include <tuple>

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
