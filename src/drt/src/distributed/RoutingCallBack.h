/* Authors: Osama */
/*
 * Copyright (c) 2021, The Regents of the University of California
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

#pragma once
#include <stdio.h>

#include <mutex>

#include "db/infra/frTime.h"
#include "distributed/RoutingJobDescription.h"
#include "dr/FlexDR.h"
#include "dst/Distributed.h"
#include "dst/JobCallBack.h"
#include "dst/JobMessage.h"
#include "triton_route/TritonRoute.h"
#include "utl/Logger.h"
namespace odb {
class dbDatabase;
}
namespace fr {

class RoutingCallBack : public dst::JobCallBack
{
 public:
  RoutingCallBack(triton_route::TritonRoute* router,
                  dst::Distributed* dist,
                  utl::Logger* logger)
      : router_(router), dist_(dist), logger_(logger)
  {
  }
  void onRoutingJobReceived(dst::JobMessage& msg, dst::socket& sock) override
  {
    
    if (msg.getJobType() != dst::JobMessage::ROUTING)
      return;
    dst::JobMessage result(dst::JobMessage::ROUTING);
    RoutingJobDescription* desc
        = static_cast<RoutingJobDescription*>(msg.getJobDescription());
    if (desc->getDesignPath() != "") {
      std::lock_guard<std::mutex> lock(mx_);
      if (design_path_ != desc->getDesignPath()) {
        router_->updateDesign(desc->getDesignPath().c_str());
        design_path_ = desc->getDesignPath();
        time_.print(logger_);
      }
    }
    if (desc->getGlobalsPath() != "") {
      std::lock_guard<std::mutex> lock(mx_);
      if (globals_path_ != desc->getGlobalsPath()) {
        globals_path_ = desc->getGlobalsPath();
        router_->setSharedVolume(desc->getSharedDir());
        router_->updateGlobals(desc->getGlobalsPath().c_str());
      }
    }
    if (desc->getWorkerPath() != "") {
      if (access(desc->getWorkerPath().c_str(), F_OK) == -1) {
        logger_->warn(
            utl::DRT, 605, "Worker file {} not found", desc->getWorkerPath());
        return;
      }
      std::string resultPath
          = router_->runDRWorker(desc->getWorkerPath().c_str());
      auto uResultDesc = std::make_unique<RoutingJobDescription>();
      auto resultDesc = static_cast<RoutingJobDescription*>(uResultDesc.get());
      resultDesc->setWorkerPath(resultPath);
      result.setJobDescription(std::move(uResultDesc));
    }
    dist_->sendResult(result, sock);
  }

 private:
  triton_route::TritonRoute* router_;
  frTime time_;
  dst::Distributed* dist_;
  utl::Logger* logger_;
  std::string design_path_;
  std::string globals_path_;
  std::mutex mx_;
};

}  // namespace fr