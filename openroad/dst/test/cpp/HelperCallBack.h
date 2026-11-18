// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2026, The OpenROAD Authors

#include "dst/Distributed.h"
#include "dst/JobCallBack.h"
#include "dst/JobMessage.h"

namespace dst {

class HelperCallBack : public dst::JobCallBack
{
 public:
  HelperCallBack(dst::Distributed* dist) : dist_(dist) {}
  void onRoutingJobReceived(dst::JobMessage& msg, dst::Socket& sock) override
  {
    JobMessage reply_msg;
    if (msg.getJobType() == JobMessage::JobType::kRouting) {
      reply_msg.setJobType(JobMessage::JobType::kSuccess);
    } else {
      reply_msg.setJobType(JobMessage::JobType::kError);
    }
    dist_->sendResult(reply_msg, sock);
  }

  void onFrDesignUpdated(dst::JobMessage& msg, dst::Socket& sock) override {}
  void onPinAccessJobReceived(dst::JobMessage& msg, dst::Socket& sock) override
  {
  }
  void onGRDRInitJobReceived(dst::JobMessage& msg, dst::Socket& sock) override
  {
  }

 private:
  dst::Distributed* dist_;
};

}  // namespace dst
