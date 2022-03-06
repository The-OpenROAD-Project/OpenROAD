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
#include <boost/bind/bind.hpp>
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
      : router_(router), dist_(dist), logger_(logger), remaining_(0)
  {
    omp_set_num_threads(ord::OpenRoad::openRoad()->getThreadCount());
    routing_pool_ = std::make_unique<asio::thread_pool>(1);
    results_pool_ = std::make_unique<asio::thread_pool>(1);
  }
  void onRoutingJobReceived(dst::JobMessage& msg, dst::socket& sock) override
  {
    if (msg.getJobType() != dst::JobMessage::ROUTING)
      return;
    dst::JobMessage reply(dst::JobMessage::SUCCESS);
    RoutingJobDescription* desc
        = static_cast<RoutingJobDescription*>(msg.getJobDescription());
    remaining_ += desc->getWorkers().size();
    asio::post(*routing_pool_, boost::bind(&RoutingCallBack::runWorkers, this, desc->getWorkers()));
    dist_->sendResult(reply, sock);
    sock.close();
  }
  void onRoutingResultReceived(dst::JobMessage& msg, dst::socket& sock)
  {

    if (msg.getJobType() != dst::JobMessage::ROUTING_RESULT)
      return;
    dst::JobMessage result;
    RoutingJobDescription* desc
        = static_cast<RoutingJobDescription*>(msg.getJobDescription());
    router_->addWorkerResults(desc->getWorkers());
    result.setJobType(dst::JobMessage::SUCCESS);
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
    leader_ip_ = desc->getReplyIp();
    leader_port_ = desc->getReplyPort();
    dist_->sendResult(result, sock);
    sock.close();
  }
  void runWorkers(const std::vector<std::pair<int, std::string>>& workers)
  {
    #pragma omp parallel for schedule(dynamic)
    for(int i = 0; i < workers.size(); i++)
    {
      std::pair<int, std::string> result = { workers.at(i).first, router_->runDRWorker(workers.at(i).second) };
      asio::post(*results_pool_, boost::bind(&RoutingCallBack::addResult, this, result));
    }
  }
  void addResult(std::pair<int, std::string> result)
  {
    results_.push_back(result);
    remaining_--;
    if(results_.size() == 32 || remaining_ == 0)
    {
      dst::JobMessage result(dst::JobMessage::ROUTING_RESULT), tmp;
      auto uResultDesc = std::make_unique<RoutingJobDescription>();
      auto resultDesc = static_cast<RoutingJobDescription*>(uResultDesc.get());
      resultDesc->setWorkers(results_);
      result.setJobDescription(std::move(uResultDesc));
      dist_->sendJob(result, leader_ip_.c_str(), leader_port_, tmp);
      results_.clear();
    } 
  }

 private:
  triton_route::TritonRoute* router_;
  dst::Distributed* dist_;
  utl::Logger* logger_;
  std::string design_path_;
  std::string globals_path_;
  std::unique_ptr<asio::thread_pool> routing_pool_;
  std::unique_ptr<asio::thread_pool> results_pool_;
  std::string leader_ip_;
  unsigned short leader_port_;
  std::vector<std::pair<int, std::string>> results_;
  unsigned int remaining_;
};

}  // namespace fr
