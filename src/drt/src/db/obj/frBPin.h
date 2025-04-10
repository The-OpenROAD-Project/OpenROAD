// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <iostream>

#include "db/obj/frPin.h"

namespace drt {
class frBTerm;

class frBPin : public frPin
{
 public:
  frBPin() = default;
  frBPin(const frBPin&) = delete;
  frBPin& operator=(const frBPin&) = delete;

  // getters
  frBTerm* getTerm() const { return term_; }

  // setters
  void setTerm(frBTerm* in) { term_ = in; }
  // others
  frBlockObjectEnum typeId() const override { return frcBPin; }

 protected:
  frBTerm* term_{nullptr};
};
}  // namespace drt
