// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db/gcObj/gcBlockObject.h"
#include "db/infra/frBox.h"

namespace drt {
class gcFig : public gcBlockObject
{
};

class gcNet;
class gcConnFig : public gcFig
{
 public:
  // getters
  virtual bool hasNet() const = 0;
  virtual gcNet* getNet() const = 0;
  // setters
  virtual void addToNet(gcNet* in) = 0;
  virtual void removeFromNet() = 0;
  // others

  /* from gcFig
   * getBBox
   * move
   * overlaps
   */
};

class gcPin;
class gcPinFig : public gcConnFig
{
 public:
  // getters
  virtual bool hasPin() const = 0;
  virtual gcPin* getPin() const = 0;
  // setters
  virtual void addToPin(gcPin* in) = 0;
  virtual void removeFromPin() = 0;
  // others

  /* from gcConnFig
   * hasNet
   * getNet
   * addToNet
   * removedromNet
   */

  /* from gcFig
   * getBBox
   * move
   * overlaps
   */
};

}  // namespace drt
