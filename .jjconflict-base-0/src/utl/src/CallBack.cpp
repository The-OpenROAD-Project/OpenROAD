// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "utl/CallBack.h"

#include "utl/CallBackHandler.h"

namespace utl {

CallBack::~CallBack()
{
  removeOwner();
}

void CallBack::setOwner(CallBackHandler* owner)
{
  owner_ = owner;
  owner_->addCallBack(this);
}

void CallBack::removeOwner()
{
  if (owner_) {
    owner_->removeCallBack(this);
    owner_ = nullptr;
  }
}

}  // namespace utl