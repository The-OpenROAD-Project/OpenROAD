// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db/infra/frBox.h"
#include "db/obj/frBlockObject.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"

namespace drt {
class frFig : public frBlockObject
{
 public:
  virtual odb::Rect getBBox() const = 0;
  virtual void move(const odb::dbTransform& xform) = 0;
  virtual bool intersects(const odb::Rect& box) const = 0;

 protected:
  frFig() = default;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frBlockObject>(*this);
  }

  friend class boost::serialization::access;
};

class frNet;
class frConnFig : public frFig
{
 public:
  virtual bool hasNet() const = 0;
  virtual frNet* getNet() const = 0;
  virtual void addToNet(frNet* in) = 0;
  virtual void removeFromNet() = 0;

 protected:
  frConnFig() = default;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frFig>(*this);
  }

  friend class boost::serialization::access;
};

class frPin;
class frBPin;
class frPinFig : public frConnFig
{
 public:
  virtual bool hasPin() const = 0;
  virtual frPin* getPin() const = 0;
  virtual void addToPin(frPin* in) = 0;
  virtual void addToPin(frBPin* in) = 0;
  virtual void removeFromPin() = 0;

 protected:
  frPinFig() = default;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<frConnFig>(*this);
  }

  friend class boost::serialization::access;
};

}  // namespace drt
