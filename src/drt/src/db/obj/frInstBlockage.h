// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>

#include "db/obj/frBlockObject.h"
#include "db/obj/frBlockage.h"
#include "frBaseTypes.h"

namespace drt {
class frInst;
class frInstBlockage : public frBlockObject
{
 public:
  // constructors
  frInstBlockage(frInst* inst, frBlockage* blockage)
      : inst_(inst), blockage_(blockage)
  {
  }
  // getters
  frInst* getInst() const { return inst_; }
  frBlockage* getBlockage() const { return blockage_; }
  // setters
  // others
  frBlockObjectEnum typeId() const override { return frcInstBlockage; }
  void setIndexeInOwner(int in) { index_in_owner_ = in; }
  int getIndexInOwner() const { return index_in_owner_; }

 private:
  // Place this first so it is adjacent to "int id_" inherited from
  // frBlockObject, saving 8 bytes.
  int index_in_owner_{0};
  frInst* inst_;
  frBlockage* blockage_;
};
}  // namespace drt
