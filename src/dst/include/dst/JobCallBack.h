// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once
#include <dst/Distributed.h>

namespace dst {
class JobMessage;
class JobCallBack
{
 public:
  virtual void onRoutingJobReceived(JobMessage& msg, socket& sock) = 0;
  virtual void onFrDesignUpdated(JobMessage& msg, socket& sock) = 0;
  virtual void onPinAccessJobReceived(JobMessage& msg, socket& sock) = 0;
  virtual void onGRDRInitJobReceived(JobMessage& msg, socket& sock) = 0;
  virtual ~JobCallBack() {}
};
}  // namespace dst
