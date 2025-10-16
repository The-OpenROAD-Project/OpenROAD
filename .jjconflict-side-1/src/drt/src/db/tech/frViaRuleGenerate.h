// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db/infra/frBox.h"
#include "frBaseTypes.h"
#include "odb/geom.h"

namespace drt {
class frViaRuleGenerate
{
 public:
  // constructors
  frViaRuleGenerate(const frString& nameIn) : name_(nameIn) {}
  // getters
  const frString& getName() const { return name_; }
  bool getDefault() const { return isDefault_; }
  const odb::Point& getLayer1Enc() const { return botEnc_; }
  const odb::Rect& getCutRect() const { return cutRect_; }
  const odb::Point& getCutSpacing() const { return cutSpacing_; }
  const odb::Point& getLayer2Enc() const { return topEnc_; }
  frLayerNum getLayer1Num() const { return botLayerNum_; }
  frLayerNum getLayer2Num() const { return topLayerNum_; }
  frLayerNum getCutLayerNum() const { return cutLayerNum_; }
  // setters
  void setDefault(bool in) { isDefault_ = in; }
  void setLayer1Enc(const odb::Point& in) { botEnc_ = in; }
  void setCutRect(const odb::Rect& in) { cutRect_ = in; }
  void setCutSpacing(const odb::Point& in) { cutSpacing_ = in; }
  void setLayer2Enc(const odb::Point& in) { topEnc_ = in; }
  void setLayer1Num(frLayerNum in) { botLayerNum_ = in; }
  void setCutLayerNum(frLayerNum in) { cutLayerNum_ = in; }
  void setLayer2Num(frLayerNum in) { topLayerNum_ = in; }

 private:
  frString name_;
  bool isDefault_{false};
  odb::Point botEnc_;
  odb::Rect cutRect_;
  odb::Point cutSpacing_;
  odb::Point topEnc_;
  frLayerNum botLayerNum_{0};
  frLayerNum cutLayerNum_{0};
  frLayerNum topLayerNum_{0};
};
}  // namespace drt
