// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>

#include "db/obj/frBPin.h"
#include "frBaseTypes.h"

namespace drt {
class frBlockage : public frBlockObject
{
 public:
  // getters
  frBPin* getPin() const { return pin_.get(); }
  frCoord getDesignRuleWidth() const { return design_rule_width_; }
  // setters
  void setPin(std::unique_ptr<frBPin> in) { pin_ = std::move(in); }
  void setDesignRuleWidth(frCoord width) { design_rule_width_ = width; }
  // others
  frBlockObjectEnum typeId() const override { return frcBlockage; }
  void setIndexInOwner(int in) { index_in_owner_ = in; }
  int getIndexInOwner() const { return index_in_owner_; }

 private:
  std::unique_ptr<frBPin> pin_;
  frCoord design_rule_width_{-1};
  int index_in_owner_{0};
};
}  // namespace drt
