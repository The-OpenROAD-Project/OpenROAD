#include <string>
#include <vector>

#include "HelperCallBack.h"
#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"
#include "boost/thread/thread.hpp"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "gtest/gtest.h"
#include "utl/Logger.h"

namespace dst {

TEST(test_suite, test_distributed)
{
  utl::Logger* logger = new utl::Logger();
  Distributed* dist = new Distributed(logger);
  std::string local_ip = "127.0.0.1";
  unsigned short worker_port = 1235;
  unsigned short balancer_port = 1236;

  // Test callbacks interface
  dist->addCallBack(new HelperCallBack(dist));
  EXPECT_EQ(dist->getCallBacks().size(), 1);

  // Adding worker address and running the worker.
  dist->addWorkerAddress(local_ip.c_str(), worker_port);
  dist->runWorker(local_ip.c_str(), worker_port, true);

  // Sending a job to worker to test if runWorker() correctly created a worker.
  // Note this test also tests sendJob().
  JobMessage msg(JobMessage::JobType::kRouting);
  JobMessage result;
  EXPECT_TRUE(dist->sendJob(msg, local_ip.c_str(), worker_port, result));
  EXPECT_EQ(result.getJobType(), JobMessage::JobType::kSuccess);

  // Running loadbalancer. Since now we know the worker is running correctly, we
  // test that runLoadBalancer() and addWorkerAddress() are working correctly
  // with the balancer is up and relaying messages to the worker.
  boost::thread t(boost::bind(&Distributed::runLoadBalancer,
                              dist,
                              local_ip.c_str(),
                              balancer_port,
                              ""));
  result.setJobType(JobMessage::JobType::kNone);
  EXPECT_TRUE(dist->sendJob(msg, local_ip.c_str(), balancer_port, result));
  EXPECT_EQ(result.getJobType(), JobMessage::JobType::kSuccess);
}

}  // namespace dst
