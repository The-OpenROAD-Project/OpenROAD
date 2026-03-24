// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db/grObj/grNet.h"
#include "db/grObj/grPin.h"
#include "db/grObj/grRef.h"
#include "db/obj/frNode.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {

class grVia : public grRef
{
 public:
  // constructor
  grVia() = default;
  grVia(const grVia& in)
      : grRef(in),
        origin_(in.origin_),
        viaDef_(in.viaDef_),
        child_(in.child_),
        parent_(in.parent_),
        owner_(in.owner_)
  {
  }

  // getters
  const frViaDef* getViaDef() const { return viaDef_; }

  // setters
  void setViaDef(const frViaDef* in) { viaDef_ = in; }

  // others
  frBlockObjectEnum typeId() const override { return grcVia; }

  /* from grRef
   * getOrient
   * setOrient
   * getOrigin
   * setOrigin
   * getTransform
   * setTransform
   */

  odb::dbOrientType getOrient() const override { return odb::dbOrientType(); }
  void setOrient(const odb::dbOrientType& in) override { ; }
  odb::Point getOrigin() const override { return origin_; }
  void setOrigin(const odb::Point& in) override { origin_ = in; }

  odb::dbTransform getTransform() const override
  {
    return odb::dbTransform(origin_);
  }
  void setTransform(const odb::dbTransform& in) override { ; }

  /* from gfrPinFig
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
  // if obj hasNet, then it is global GR net
  // if obj hasGrNet, then it is GR worker subnet
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
   * move
   * overlaps
   */

  odb::Rect getBBox() const override { return odb::Rect(origin_, origin_); }

  void setIter(frListIter<std::unique_ptr<grVia>>& in) { iter_ = in; }
  frListIter<std::unique_ptr<grVia>> getIter() const { return iter_; }

 protected:
  odb::Point origin_;
  const frViaDef* viaDef_{nullptr};
  frBlockObject* child_{nullptr};
  frBlockObject* parent_{nullptr};
  frBlockObject* owner_{nullptr};
  frListIter<std::unique_ptr<grVia>> iter_;
};
}  // namespace drt
