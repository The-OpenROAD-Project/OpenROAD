#pragma once
#include <stdio.h>

#include <boost/asio.hpp>
#include <mutex>

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
    globals_path_ = "";
  }
  void onRoutingJobReceived(dst::JobMessage& msg,
                            boost::asio::ip::tcp::socket& sock)
  {
    if (msg.type != dst::JobMessage::ROUTING)
      return;
    RoutingJobDescription* desc
        = static_cast<RoutingJobDescription*>(msg.desc.get());
    if (access(desc->path.c_str(), F_OK) == -1) {
      logger_->warn(utl::DRT, 605, "Worker file {} not found", desc->path);
      return;
    }
    if (globals_path_ != desc->globals_path) {
      mx_.lock();
      globals_path_ = desc->globals_path;
      router_->setSharedVolume(desc->shared_dir);
      router_->updateGlobals(desc->globals_path.c_str());
      mx_.unlock();
    }
    logger_->info(utl::DRT, 600, "running worker {}", desc->path);
    std::string resultPath = router_->runDRWorker(desc->path.c_str());
    logger_->info(utl::DRT, 603, "worker {} is done", resultPath);
    dst::JobMessage result(dst::JobMessage::ROUTING);
    result.desc = std::make_unique<RoutingJobDescription>(resultPath);
    dist_->sendResult(result, sock);
  }

 private:
  triton_route::TritonRoute* router_;
  dst::Distributed* dist_;
  utl::Logger* logger_;
  std::string globals_path_;
  std::mutex mx_;
};

}  // namespace fr