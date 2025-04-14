// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frShape.h"
#include "frBaseTypes.h"

namespace drt {
class frTerm;

class frPin : public frBlockObject
{
 public:
  frPin(const frPin& in) = delete;
  frPin& operator=(const frPin&) = delete;

  // getters
  const std::vector<std::unique_ptr<frPinFig>>& getFigs() const
  {
    return pinFigs_;
  }

  int getNumPinAccess() const { return aps_.size(); }
  bool hasPinAccess() const { return !aps_.empty(); }
  frPinAccess* getPinAccess(int idx) const { return aps_[idx].get(); }

  // setters
  void addPinFig(std::unique_ptr<frPinFig> in)
  {
    in->addToPin(this);
    pinFigs_.push_back(std::move(in));
  }
  void addPinAccess(std::unique_ptr<frPinAccess> in)
  {
    in->setId(aps_.size());
    in->setPin(this);
    aps_.push_back(std::move(in));
  }
  void setPinAccess(int idx, std::unique_ptr<frPinAccess> in)
  {
    in->setId(idx);
    in->setPin(this);
    aps_[idx] = std::move(in);
  }
  void clearPinAccess() { aps_.clear(); }

 protected:
  frPin() = default;

  std::vector<std::unique_ptr<frPinFig>> pinFigs_;
  std::vector<std::unique_ptr<frPinAccess>> aps_;
};

}  // namespace drt
