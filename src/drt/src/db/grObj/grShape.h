// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>

#include "db/grObj/grFig.h"
#include "db/infra/frSegStyle.h"
#include "frBaseTypes.h"
#include "odb/geom.h"

namespace drt {
class frNet;
class grPin;
class frPathSeg;
class grShape : public grPinFig
{
 public:
  // setters
  virtual void setLayerNum(frLayerNum tmpLayerNum) = 0;
  // getters
  virtual frLayerNum getLayerNum() const = 0;
  // others

  /* from grPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */

  /* from grConnFig
   * hasNet
   * getNet
   * getChild
   * getParent
   * getGrChild
   * getGrParent
   * addToNet
   * removeFromNet
   * setChild
   * setParent
   * setChild
   * setParent
   */

  /* from grFig
   * getBBox
   * move
   * overlaps
   */

  virtual void setIter(frListIter<std::unique_ptr<grShape>>& in) = 0;
  virtual frListIter<std::unique_ptr<grShape>> getIter() const = 0;
};

class grPathSeg : public grShape
{
 public:
  // constructors
  grPathSeg() = default;
  grPathSeg(const grPathSeg& in)
      : begin_(in.begin_),
        end_(in.end_),
        layer_(in.layer_),
        child_(in.child_),
        parent_(in.parent_),
        owner_(in.owner_)
  {
  }
  grPathSeg(const frPathSeg& in);
  // getters
  std::pair<odb::Point, odb::Point> getPoints() const { return {begin_, end_}; }

  // setters
  void setPoints(const odb::Point& begin, const odb::Point& end)
  {
    begin_ = begin;
    end_ = end;
  }
  // others
  frBlockObjectEnum typeId() const override { return grcPathSeg; }

  /* from grShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer_ = numIn; }
  frLayerNum getLayerNum() const override { return layer_; }

  /* from grPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override { return owner_ && owner_->typeId() == grcPin; }

  grPin* getPin() const override { return reinterpret_cast<grPin*>(owner_); }

  void addToPin(grPin* in) override
  {
    owner_ = reinterpret_cast<frBlockObject*>(in);
  }

  void removeFromPin() override { owner_ = nullptr; }

  /* from grConnFig
   * hasNet
   * getNet
   * hasGrNet
   * getGrNet
   * getChild
   * getParent
   * getGrChild
   * getGrParent
   * addToNet
   * removeFromNet
   * setChild
   * setParent
   */
  bool hasNet() const override { return owner_ && owner_->typeId() == frcNet; }
  bool hasGrNet() const override
  {
    return owner_ && owner_->typeId() == grcNet;
  }
  frNet* getNet() const override { return reinterpret_cast<frNet*>(owner_); }
  grNet* getGrNet() const override { return reinterpret_cast<grNet*>(owner_); }
  frNode* getChild() const override
  {
    return reinterpret_cast<frNode*>(child_);
  }
  frNode* getParent() const override
  {
    return reinterpret_cast<frNode*>(parent_);
  }
  grNode* getGrChild() const override
  {
    return reinterpret_cast<grNode*>(child_);
  }
  grNode* getGrParent() const override
  {
    return reinterpret_cast<grNode*>(parent_);
  }

  void addToNet(frBlockObject* in) override { owner_ = in; }

  void removeFromNet() override { owner_ = nullptr; }

  void setChild(frBlockObject* in) override { child_ = in; }

  void setParent(frBlockObject* in) override { parent_ = in; }

  /* from grFig
   * getBBox
   */
  // needs to be updated
  odb::Rect getBBox() const override
  {
    return odb::Rect(begin_.x(), begin_.y(), end_.x(), end_.y());
  }

  void setIter(frListIter<std::unique_ptr<grShape>>& in) override
  {
    iter_ = in;
  }
  frListIter<std::unique_ptr<grShape>> getIter() const override
  {
    return iter_;
  }

 protected:
  odb::Point begin_;
  odb::Point end_;
  frLayerNum layer_{0};
  frBlockObject* child_{nullptr};
  frBlockObject* parent_{nullptr};
  frBlockObject* owner_{nullptr};
  frListIter<std::unique_ptr<grShape>> iter_;
};
}  // namespace drt
