#include <string>

#include "HelperCallBack.h"
#include "Worker.h"
#include "boost/asio.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/bind/bind.hpp"
#include "boost/system/system_error.hpp"
#include "boost/thread/thread.hpp"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "gtest/gtest.h"
#include "utl/Logger.h"

namespace dst {

TEST(test_suite, test_worker)
{
  utl::Logger* logger = new utl::Logger();
  Distributed* dist = new Distributed(logger);
  std::string local_ip = "127.0.0.1";
  unsigned short port = 1234;

  Worker* worker = new Worker(dist, logger, local_ip.c_str(), port);
  boost::thread t(boost::bind(&Worker::run, worker));

  // Checking if the worker is up and calling callbacks correctly
  dist->addCallBack(new HelperCallBack(dist));
  JobMessage msg(JobMessage::JobType::kRouting);
  JobMessage result;
  EXPECT_TRUE(dist->sendJob(msg, local_ip.c_str(), port, result));
  EXPECT_EQ(result.getJobType(), JobMessage::JobType::kSuccess);
}

}  // namespace dst
