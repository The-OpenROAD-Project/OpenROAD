#include <string>

#include "HelperCallBack.h"
#include "LoadBalancer.h"
#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"
#include "boost/thread/thread.hpp"
#include "dst/BroadcastJobDescription.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "gtest/gtest.h"
#include "utl/Logger.h"

namespace dst {

TEST(test_suite, test_balancer)
{
  utl::Logger* logger = new utl::Logger();
  Distributed* dist = new Distributed(logger);
  std::string local_ip = "127.0.0.1";
  unsigned short balancer_port = 5555;
  unsigned short worker_port_1 = 5556;
  unsigned short worker_port_2 = 5557;
  unsigned short worker_port_3 = 5558;
  unsigned short worker_port_4 = 5559;
  asio::io_context service;
  LoadBalancer* balancer = new LoadBalancer(
      dist, service, logger, local_ip.c_str(), "", balancer_port);

  // Checking simple interface functions
  balancer->addWorker(local_ip, worker_port_1);
  balancer->addWorker(local_ip, worker_port_2);
  asio::ip::address address;
  unsigned short port;

  balancer->getNextWorker(address, port);
  EXPECT_EQ(address.to_string(), local_ip);
  EXPECT_EQ(port, worker_port_1);
  balancer->getNextWorker(address, port);
  EXPECT_EQ(address.to_string(), local_ip);
  EXPECT_EQ(port, worker_port_2);
  balancer->updateWorker(asio::ip::make_address(local_ip), worker_port_2);
  balancer->getNextWorker(address, port);
  EXPECT_EQ(address.to_string(), local_ip);
  EXPECT_EQ(port, worker_port_2);

  // Checking if balancer is up and responding
  boost::thread t(boost::bind(&asio::io_context::run, &service));
  JobMessage msg(JobMessage::JobType::kBalancer);
  JobMessage result;
  EXPECT_TRUE(dist->sendJob(msg, local_ip.c_str(), balancer_port, result));
  EXPECT_EQ(result.getJobType(), JobMessage::JobType::kSuccess);

  // Checking if a balancer can relay a message to a worker and send the result
  // correctly. note we make worker 2, which is not running, the next
  // worker. That should be handled correctly by balancer.
  balancer->updateWorker(asio::ip::make_address(local_ip), worker_port_2);
  dist->addCallBack(new HelperCallBack(dist));
  dist->runWorker(local_ip.c_str(), worker_port_1, true);
  msg.setJobType(JobMessage::JobType::kRouting);
  result.setJobType(JobMessage::JobType::kNone);
  EXPECT_TRUE(dist->sendJob(msg, local_ip.c_str(), balancer_port, result));
  EXPECT_EQ(result.getJobType(), JobMessage::JobType::kSuccess);

  // Checking broadcast message relaying and handling.
  JobMessage broadcast_msg(JobMessage::JobType::kRouting,
                           JobMessage::MessageType::kBroadcast);
  result.setJobType(JobMessage::JobType::kNone);
  balancer->addWorker(local_ip, worker_port_3);
  balancer->addWorker(local_ip, worker_port_4);
  EXPECT_TRUE(
      dist->sendJob(broadcast_msg, local_ip.c_str(), balancer_port, result));
  EXPECT_EQ(result.getJobType(),
            JobMessage::JobType::kError);  // failed to send to more than 2
                                           // workers.
  BroadcastJobDescription* desc
      = static_cast<BroadcastJobDescription*>(result.getJobDescription());
  // number of successful broadcasts was 1
  EXPECT_EQ(desc->getWorkersCount(), 1);

  // Now worker 2, 3, and 4 shall have been removed and relaying messages to the
  // up workers shall work correctly.
  dist->runWorker(local_ip.c_str(),
                  worker_port_4,
                  true);  // make worker 4 running for this test
  // For worker 4 to be added correctly, it should have received the saved
  // broadcast messages. The next expect confirms that.
  EXPECT_TRUE(balancer->addWorker(
      local_ip, worker_port_4));  // re-add it to the loadbalancer
  result.setJobType(JobMessage::JobType::kNone);
  EXPECT_TRUE(
      dist->sendJob(broadcast_msg, local_ip.c_str(), balancer_port, result));
  EXPECT_EQ(result.getJobType(),
            JobMessage::JobType::kSuccess);  // No more than 2 workers failed
                                             // to receive the broadcast
  desc = static_cast<BroadcastJobDescription*>(result.getJobDescription());
  // number of successful broadcasts was 2
  EXPECT_EQ(desc->getWorkersCount(), 2);

  // Since we have a histoty of broadcast messages, adding a not running worker
  // shall be invalid as it wouldn't have received the broadcast messages
  // history i.e have invalid state.
  EXPECT_FALSE(balancer->addWorker(local_ip, worker_port_2));
}

}  // namespace dst
