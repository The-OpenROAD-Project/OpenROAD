// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>

#include "db/grObj/grFig.h"
#include "db/infra/frSegStyle.h"

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
      : begin(in.begin),
        end(in.end),
        layer(in.layer),
        child(in.child),
        parent(in.parent),
        owner(in.owner)
  {
  }
  grPathSeg(const frPathSeg& in);
  // getters
  std::pair<Point, Point> getPoints() const { return {begin, end}; }

  // setters
  void setPoints(const Point& beginIn, const Point& endIn)
  {
    begin = beginIn;
    end = endIn;
  }
  // others
  frBlockObjectEnum typeId() const override { return grcPathSeg; }

  /* from grShape
   * setLayerNum
   * getLayerNum
   */
  void setLayerNum(frLayerNum numIn) override { layer = numIn; }
  frLayerNum getLayerNum() const override { return layer; }

  /* from grPinFig
   * hasPin
   * getPin
   * addToPin
   * removeFromPin
   */
  bool hasPin() const override
  {
    return (owner) && (owner->typeId() == grcPin);
  }

  grPin* getPin() const override { return reinterpret_cast<grPin*>(owner); }

  void addToPin(grPin* in) override
  {
    owner = reinterpret_cast<frBlockObject*>(in);
  }

  void removeFromPin() override { owner = nullptr; }

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
  bool hasNet() const override
  {
    return (owner) && (owner->typeId() == frcNet);
  }
  bool hasGrNet() const override
  {
    return (owner) && (owner->typeId() == grcNet);
  }
  frNet* getNet() const override { return reinterpret_cast<frNet*>(owner); }
  grNet* getGrNet() const override { return reinterpret_cast<grNet*>(owner); }
  frNode* getChild() const override { return reinterpret_cast<frNode*>(child); }
  frNode* getParent() const override
  {
    return reinterpret_cast<frNode*>(parent);
  }
  grNode* getGrChild() const override
  {
    return reinterpret_cast<grNode*>(child);
  }
  grNode* getGrParent() const override
  {
    return reinterpret_cast<grNode*>(parent);
  }

  void addToNet(frBlockObject* in) override { owner = in; }

  void removeFromNet() override { owner = nullptr; }

  void setChild(frBlockObject* in) override { child = in; }

  void setParent(frBlockObject* in) override { parent = in; }

  /* from grFig
   * getBBox
   */
  // needs to be updated
  Rect getBBox() const override
  {
    return Rect(begin.x(), begin.y(), end.x(), end.y());
  }

  void setIter(frListIter<std::unique_ptr<grShape>>& in) override { iter = in; }
  frListIter<std::unique_ptr<grShape>> getIter() const override { return iter; }

 protected:
  Point begin;
  Point end;
  frLayerNum layer{0};
  frBlockObject* child{nullptr};
  frBlockObject* parent{nullptr};
  frBlockObject* owner{nullptr};
  frListIter<std::unique_ptr<grShape>> iter;
};
}  // namespace drt
