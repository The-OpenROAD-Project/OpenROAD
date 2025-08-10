// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "utl/CallBackHandler.h"

namespace utl {

void CallBackHandler::addCallBack(CallBack* callback)
{
  callbacks_.insert(callback);
}

void CallBackHandler::removeCallBack(CallBack* callback)
{
  callbacks_.erase(callback);
}

void CallBackHandler::triggerOnPinAccessUpdateRequired()
{
  for (CallBack* callback : callbacks_) {
    if (callback != nullptr) {
      callback->onPinAccessUpdateRequired();
    }
  }
}

}  // namespace utl