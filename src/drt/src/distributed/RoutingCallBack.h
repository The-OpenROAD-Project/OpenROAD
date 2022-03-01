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
#include <omp.h>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <mutex>
#include <iostream>

#include "db/infra/frTime.h"
#include "distributed/RoutingJobDescription.h"
#include "dr/FlexDR.h"
#include "dst/Distributed.h"
#include "dst/JobCallBack.h"
#include "dst/JobMessage.h"
#include "ord/OpenRoad.hh"
#include "triton_route/TritonRoute.h"
#include "utl/Logger.h"

namespace asio = boost::asio;
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
    omp_set_num_threads(ord::OpenRoad::openRoad()->getThreadCount());
    // pool_ = std::make_unique<asio::thread_pool>(
    //     ord::OpenRoad::openRoad()->getThreadCount());
  }
  void onRoutingJobReceived(dst::JobMessage& msg, dst::socket& sock) override
  {
    if (msg.getJobType() != dst::JobMessage::ROUTING)
      return;
    RoutingJobDescription* desc
        = static_cast<RoutingJobDescription*>(msg.getJobDescription());
    if (desc->getDesignPath() != "") {
      if (design_path_ != desc->getDesignPath()) {
        frTime t;
        logger_->report("Design: {}", desc->getDesignPath());
        router_->updateDesign(desc->getDesignPath().c_str());
        design_path_ = desc->getDesignPath();
        t.print(logger_);
      }
    }
    if (desc->getGlobalsPath() != "") {
      if (globals_path_ != desc->getGlobalsPath()) {
        globals_path_ = desc->getGlobalsPath();
        router_->setSharedVolume(desc->getSharedDir());
        router_->updateGlobals(desc->getGlobalsPath().c_str());
      }
    }
    ;
    std::vector<std::string> resultWorkers(desc->getWorkers().size());
    #pragma omp parallel for schedule(dynamic)
    for(int i = 0; i < desc->getWorkers().size(); i++)
    {
      resultWorkers[i] = router_->runDRWorker(desc->getWorkers().at(i));
    }
    dst::JobMessage result(dst::JobMessage::ROUTING);
    auto uResultDesc = std::make_unique<RoutingJobDescription>();
    auto resultDesc = static_cast<RoutingJobDescription*>(uResultDesc.get());
    resultDesc->setWorkers(resultWorkers);
    result.setJobDescription(std::move(uResultDesc));
    dist_->sendResult(result, sock);
    sock.close();
  }
  void onRoutingResultReceived(dst::JobMessage& msg, dst::socket& sock)
  {

    if (msg.getJobType() != dst::JobMessage::ROUTING_RESULT)
      return;
    dst::JobMessage result;
    RoutingJobDescription* desc
        = static_cast<RoutingJobDescription*>(msg.getJobDescription());
    if (desc->getWorkerStr() != "") {
      router_->addWorkerResult(desc->getIdxInBatch(), desc->getWorkerStr());
      result.setJobType(dst::JobMessage::SUCCESS);
    } else {
      result.setJobType(dst::JobMessage::ERROR);
    }
    dist_->sendResult(result, sock);
    sock.close();
  }
  void onFrDesignUpdated(dst::JobMessage& msg, dst::socket& sock) override
  {
    if (msg.getJobType() != dst::JobMessage::UPDATE_DESIGN)
      return;
    dst::JobMessage result(dst::JobMessage::UPDATE_DESIGN);
    RoutingJobDescription* desc
        = static_cast<RoutingJobDescription*>(msg.getJobDescription());
    if (desc->getDesignPath() != "") {
      if (design_path_ != desc->getDesignPath()) {
        frTime t;
        logger_->report("Design: {}", desc->getDesignPath());
        router_->updateDesign(desc->getDesignPath().c_str());
        design_path_ = desc->getDesignPath();
        t.print(logger_);
      }
    }
    if (desc->getGlobalsPath() != "") {
      if (globals_path_ != desc->getGlobalsPath()) {
        globals_path_ = desc->getGlobalsPath();
        router_->setSharedVolume(desc->getSharedDir());
        router_->updateGlobals(desc->getGlobalsPath().c_str());
      }
    }
    dist_->sendResult(result, sock);
    sock.close();
  }

 private:
  triton_route::TritonRoute* router_;
  dst::Distributed* dist_;
  utl::Logger* logger_;
  std::string design_path_;
  std::string globals_path_;
  std::unique_ptr<asio::thread_pool> pool_;
};

}  // namespace fr
