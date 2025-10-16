// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db/drObj/drBlockObject.h"
#include "db/infra/frBox.h"
#include "odb/geom.h"

namespace drt {
class drFig : public drBlockObject
{
 public:
  virtual odb::Rect getBBox() const = 0;

 protected:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<drBlockObject>(*this);
  }

  friend class boost::serialization::access;
};

class drNet;
class drConnFig : public drFig
{
 public:
  virtual bool hasNet() const = 0;
  virtual drNet* getNet() const = 0;

  virtual void addToNet(drNet* in) = 0;
  virtual void removeFromNet() = 0;

  /* drom drFig
   * getBBox
   * move
   * overlaps
   */

 protected:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<drFig>(*this);
  }

  friend class boost::serialization::access;
};

class drPin;
class drPinFig : public drConnFig
{
 public:
  virtual bool hasPin() const = 0;
  virtual drPin* getPin() const = 0;

  virtual void addToPin(drPin* in) = 0;
  virtual void removeFromPin() = 0;

  /* drom drConnFig
   * hasNet
   * getNet
   * addToNet
   * removedromNet
   */

  /* drom drFig
   * getBBox
   * move
   * overlaps
   */

 protected:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & boost::serialization::base_object<drConnFig>(*this);
  }

  friend class boost::serialization::access;
};

}  // namespace drt
