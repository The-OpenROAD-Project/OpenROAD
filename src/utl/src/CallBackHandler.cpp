// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "utl/CallBackHandler.h"

#include "utl/CallBack.h"
#include "utl/Logger.h"

namespace utl {

CallBackHandler::CallBackHandler(utl::Logger* logger) : logger_(logger)
{
}

void CallBackHandler::addCallBack(CallBack* callback)
{
  if (callback == nullptr) {
    logger_->error(utl::UTL, 200, "Registering null callback is not allowed");
  }
  callbacks_.insert(callback);
}

void CallBackHandler::removeCallBack(CallBack* callback)
{
  callbacks_.erase(callback);
}

void CallBackHandler::triggerOnPinAccessUpdateRequired()
{
  for (CallBack* callback : callbacks_) {
    callback->onPinAccessUpdateRequired();
  }
}

void CallBackHandler::triggerOnEstimateParasiticsRequired()
{
  for (CallBack* callback : callbacks_) {
    callback->onEstimateParasiticsRequired();
  }
}

}  // namespace utl