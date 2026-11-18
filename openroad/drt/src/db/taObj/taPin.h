// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "db/taObj/taBlockObject.h"
#include "db/taObj/taFig.h"
#include "db/taObj/taShape.h"
#include "db/taObj/taVia.h"
#include "frBaseTypes.h"

namespace drt {
class frGuide;

class taPin : public taBlockObject
{
 public:
  // constructors
  taPin() = default;

  // getters
  int getNextIrouteDir() const { return nextIrouteDir_; }
  bool hasPinCoord() const { return hasPinCoord_; }
  int getPinCoord() const { return pinCoord_; }
  frGuide* getGuide() const { return guide_; }
  const std::vector<std::unique_ptr<taPinFig>>& getFigs() const
  {
    return pinFigs_;
  }
  frCost getCost() const { return cost_; }
  int getNumAssigned() const { return numAssigned_; }
  void setNextIrouteDir(int in) { nextIrouteDir_ = in; }
  void setPinCoord(frCoord in)
  {
    hasPinCoord_ = true;
    pinCoord_ = in;
  }
  void setGuide(frGuide* in) { guide_ = in; }
  void addPinFig(std::unique_ptr<taPinFig> in)
  {
    in->addToPin(this);
    pinFigs_.push_back(std::move(in));
  }
  void setCost(frCost in) { cost_ = in; }
  void addCost(frCost in) { cost_ += in; }
  void addNumAssigned() { numAssigned_++; }
  // others
  frBlockObjectEnum typeId() const override { return tacPin; }
  bool operator<(const taPin& b) const
  {
    if (this->cost_ != b.cost_) {
      return this->getCost() > b.getCost();
    }
    return this->getId() < b.getId();
  }

 protected:
  frGuide* guide_{nullptr};
  std::vector<std::unique_ptr<taPinFig>> pinFigs_;
  int nextIrouteDir_{0};  // for nbr global guides
  bool hasPinCoord_{false};
  frCoord pinCoord_{0};  // for local guides and pin guides
  frCost cost_{0};
  int numAssigned_{0};
};

struct taPinComp
{
  bool operator()(const taPin* lhs, const taPin* rhs) const
  {
    return *lhs < *rhs;
  }
};
}  // namespace drt
