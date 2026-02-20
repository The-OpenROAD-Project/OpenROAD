// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frNet.h"
#include "db/obj/frShape.h"
#include "frBaseTypes.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {
class frTerm : public frBlockObject
{
 public:
  // getters
  virtual bool hasNet() const = 0;
  virtual frNet* getNet() const = 0;
  const frString& getName() const { return name_; }
  // setters
  void setType(const odb::dbSigType& in) { type_ = in; }
  odb::dbSigType getType() const { return type_; }
  void setDirection(const odb::dbIoType& in) { direction_ = in; }
  odb::dbIoType getDirection() const { return direction_; }
  // others
  void setIndexInOwner(int value) { index_in_owner_ = value; }
  int getIndexInOwner() { return index_in_owner_; }
  virtual frAccessPoint* getAccessPoint(frCoord x,
                                        frCoord y,
                                        frLayerNum lNum,
                                        int pinAccessIdx)
      = 0;
  bool hasAccessPoint(frCoord x, frCoord y, frLayerNum lNum, int pinAccessIdx)
  {
    return getAccessPoint(x, y, lNum, pinAccessIdx) != nullptr;
  }
  // fills outShapes with copies of the pinFigs
  virtual void getShapes(std::vector<frRect>& outShapes) const = 0;
  odb::Rect getBBox() const { return bbox_; }

 protected:
  // constructors
  frTerm(const frString& name) : name_(name) {}

  frString name_;        // A, B, Z, VSS, VDD
  frNet* net_{nullptr};  // set later, term in instTerm does not have net
  odb::dbSigType type_{odb::dbSigType::SIGNAL};
  odb::dbIoType direction_{odb::dbIoType::INPUT};
  int index_in_owner_{0};
  odb::Rect bbox_;
};

}  // namespace drt
