// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db/infra/frBox.h"
#include "frBaseTypes.h"

namespace drt {
class frViaRuleGenerate
{
 public:
  // constructors
  frViaRuleGenerate(const frString& nameIn) : name_(nameIn) {}
  // getters
  const frString& getName() const { return name_; }
  bool getDefault() const { return isDefault_; }
  const Point& getLayer1Enc() const { return botEnc_; }
  const Rect& getCutRect() const { return cutRect_; }
  const Point& getCutSpacing() const { return cutSpacing_; }
  const Point& getLayer2Enc() const { return topEnc_; }
  frLayerNum getLayer1Num() const { return botLayerNum_; }
  frLayerNum getLayer2Num() const { return topLayerNum_; }
  frLayerNum getCutLayerNum() const { return cutLayerNum_; }
  // setters
  void setDefault(bool in) { isDefault_ = in; }
  void setLayer1Enc(const Point& in) { botEnc_ = in; }
  void setCutRect(const Rect& in) { cutRect_ = in; }
  void setCutSpacing(const Point& in) { cutSpacing_ = in; }
  void setLayer2Enc(const Point& in) { topEnc_ = in; }
  void setLayer1Num(frLayerNum in) { botLayerNum_ = in; }
  void setCutLayerNum(frLayerNum in) { cutLayerNum_ = in; }
  void setLayer2Num(frLayerNum in) { topLayerNum_ = in; }

 private:
  frString name_;
  bool isDefault_{false};
  Point botEnc_;
  Rect cutRect_;
  Point cutSpacing_;
  Point topEnc_;
  frLayerNum botLayerNum_{0};
  frLayerNum cutLayerNum_{0};
  frLayerNum topLayerNum_{0};
};
}  // namespace drt
