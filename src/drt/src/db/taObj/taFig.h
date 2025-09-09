// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db/infra/frBox.h"
#include "db/taObj/taBlockObject.h"

namespace drt {
class taFig : public taBlockObject
{
 public:
  virtual Rect getBBox() const = 0;
  virtual void move(const dbTransform& xform) = 0;
  virtual bool overlaps(const Rect& box) const = 0;
};

class frNet;
class taConnFig : public taFig
{
 public:
  // getters
  virtual bool hasNet() const = 0;

  // setters
  virtual void addToNet(frNet* in) = 0;
  virtual void removeFromNet() = 0;
  // others

  /* from frFig
   * getBBox
   * move
   * overlaps
   */
};

class taPin;
class taPinFig : public taConnFig
{
 public:
  // getters
  virtual bool hasPin() const = 0;
  virtual taPin* getPin() const = 0;
  frNet* getNet() const { return net_; }
  // setters
  virtual void addToPin(taPin* in) = 0;
  virtual void removeFromPin() = 0;
  // others

  void setNet(frNet* n) { net_ = n; }
  /* from frConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */

  /* from frFig
   * getBBox
   * move
   * overlaps
   */
 protected:
  frNet* net_ = nullptr;
};

}  // namespace drt
