/* Authors: Mahfouz-z */
/*
 * Copyright (c) 2022, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dst/Distributed.h"
#include "dst/JobCallBack.h"
#include "dst/JobMessage.h"

using namespace dst;

class HelperCallBack : public dst::JobCallBack
{
 public:
  HelperCallBack(dst::Distributed* dist) : dist_(dist) {}
  void onRoutingJobReceived(dst::JobMessage& msg, dst::socket& sock) override
  {
    JobMessage replyMsg;
    if (msg.getJobType() == JobMessage::JobType::ROUTING)
      replyMsg.setJobType(JobMessage::JobType::SUCCESS);
    else
      replyMsg.setJobType(JobMessage::JobType::ERROR);
    dist_->sendResult(replyMsg, sock);
  }

  void onFrDesignUpdated(dst::JobMessage& msg, dst::socket& sock) override {}
  void onPinAccessJobReceived(dst::JobMessage& msg, dst::socket& sock) override
  {
  }
  void onGRDRInitJobReceived(dst::JobMessage& msg, dst::socket& sock) override
  {
  }

 private:
  dst::Distributed* dist_;
};
