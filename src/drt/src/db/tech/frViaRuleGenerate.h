// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db/infra/frBox.h"

namespace drt {
class frViaRuleGenerate
{
 public:
  // constructors
  frViaRuleGenerate(const frString& nameIn) : name(nameIn) {}
  // getters
  const frString& getName() const { return name; }
  bool getDefault() const { return isDefault; }
  const Point& getLayer1Enc() const { return botEnc; }
  const Rect& getCutRect() const { return cutRect; }
  const Point& getCutSpacing() const { return cutSpacing; }
  const Point& getLayer2Enc() const { return topEnc; }
  frLayerNum getLayer1Num() const { return botLayerNum; }
  frLayerNum getLayer2Num() const { return topLayerNum; }
  frLayerNum getCutLayerNum() const { return cutLayerNum; }
  // setters
  void setDefault(bool in) { isDefault = in; }
  void setLayer1Enc(const Point& in) { botEnc = in; }
  void setCutRect(const Rect& in) { cutRect = in; }
  void setCutSpacing(const Point& in) { cutSpacing = in; }
  void setLayer2Enc(const Point& in) { topEnc = in; }
  void setLayer1Num(frLayerNum in) { botLayerNum = in; }
  void setCutLayerNum(frLayerNum in) { cutLayerNum = in; }
  void setLayer2Num(frLayerNum in) { topLayerNum = in; }

 private:
  frString name;
  bool isDefault{false};
  Point botEnc;
  Rect cutRect;
  Point cutSpacing;
  Point topEnc;
  frLayerNum botLayerNum{0};
  frLayerNum cutLayerNum{0};
  frLayerNum topLayerNum{0};
};
}  // namespace drt
