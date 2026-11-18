// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once
#include "dst/Distributed.h"

namespace dst {
class JobMessage;
class JobCallBack
{
 public:
  virtual ~JobCallBack() = default;
  virtual void onRoutingJobReceived(JobMessage& msg, Socket& sock) = 0;
  virtual void onFrDesignUpdated(JobMessage& msg, Socket& sock) = 0;
  virtual void onPinAccessJobReceived(JobMessage& msg, Socket& sock) = 0;
  virtual void onGRDRInitJobReceived(JobMessage& msg, Socket& sock) = 0;
};
}  // namespace dst
