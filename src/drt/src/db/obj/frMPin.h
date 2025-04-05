// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <iostream>

#include "db/obj/frPin.h"

namespace drt {
class frMTerm;

class frMPin : public frPin
{
 public:
  // constructors
  frMPin() = default;
  frMPin(const frMPin& in) = delete;
  frMPin& operator=(const frMPin&) = delete;

  // getters
  frMTerm* getTerm() const { return term_; }

  // setters
  // cannot have setterm, must be available when creating
  void setTerm(frMTerm* in) { term_ = in; }
  // others
  frBlockObjectEnum typeId() const override { return frcMPin; }

 protected:
  frMTerm* term_{nullptr};
};
}  // namespace drt
