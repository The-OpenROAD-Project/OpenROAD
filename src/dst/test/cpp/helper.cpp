
#include "dst/Distributed.h"
#include "dst/JobCallBack.h"
#include "dst/JobMessage.h"
#include "utl/Logger.h"

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

 private:
  dst::Distributed* dist_;
};