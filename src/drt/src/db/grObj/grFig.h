// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db/grObj/grBlockObject.h"
#include "db/infra/frBox.h"

namespace drt {

class grFig : public grBlockObject
{
 public:
  virtual Rect getBBox() const = 0;
};

class frNet;
class grNet;
class frNode;
class grNode;
class grConnFig : public grFig
{
 public:
  // getters
  virtual bool hasNet() const = 0;
  virtual frNet* getNet() const = 0;
  virtual bool hasGrNet() const = 0;
  virtual grNet* getGrNet() const = 0;
  virtual frNode* getChild() const = 0;
  virtual frNode* getParent() const = 0;
  virtual grNode* getGrChild() const = 0;
  virtual grNode* getGrParent() const = 0;
  // setters
  virtual void addToNet(frBlockObject* in) = 0;
  virtual void removeFromNet() = 0;
  virtual void setChild(frBlockObject* in) = 0;
  virtual void setParent(frBlockObject* in) = 0;
  // others

  /* from frFig
   * getBBox
   * move
   * overlaps
   */
 protected:
};

class grPin;
class grPinFig : public grConnFig
{
 public:
  // getters
  virtual bool hasPin() const = 0;
  virtual grPin* getPin() const = 0;
  // setters
  virtual void addToPin(grPin* in) = 0;
  virtual void removeFromPin() = 0;
  // others

  /* from grConnFig
   * hasNet
   * getNet
   * addToNet
   * removeFromNet
   */

  /* from grFig
   * getBBox
   * move
   * overlaps
   */
};

}  // namespace drt
