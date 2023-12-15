#define BOOST_TEST_MODULE TestBalancer

#include <boost/asio.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/thread/thread.hpp>
#include <string>

#include "HelperCallBack.h"
#include "LoadBalancer.h"
#include "dst/BroadcastJobDescription.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "utl/Logger.h"

using namespace dst;

BOOST_AUTO_TEST_SUITE(test_suite)

BOOST_AUTO_TEST_CASE(test_default)
{
  utl::Logger* logger = new utl::Logger();
  Distributed* dist = new Distributed(logger);
  std::string local_ip = "127.0.0.1";
  unsigned short balancer_port = 5555;
  unsigned short worker_port_1 = 5556;
  unsigned short worker_port_2 = 5557;
  unsigned short worker_port_3 = 5558;
  unsigned short worker_port_4 = 5559;
  asio::io_service io_service;
  LoadBalancer* balancer = new LoadBalancer(
      dist, io_service, logger, local_ip.c_str(), "", balancer_port);

  // Checking simple interface functions
  balancer->addWorker(local_ip, worker_port_1);
  balancer->addWorker(local_ip, worker_port_2);
  asio::ip::address address;
  unsigned short port;

  balancer->getNextWorker(address, port);
  BOOST_TEST(address.to_string() == local_ip);
  BOOST_TEST(port == worker_port_1);
  balancer->getNextWorker(address, port);
  BOOST_TEST(address.to_string() == local_ip);
  BOOST_TEST(port == worker_port_2);
  balancer->updateWorker(asio::ip::address::from_string(local_ip),
                         worker_port_2);
  balancer->getNextWorker(address, port);
  BOOST_TEST(address.to_string() == local_ip);
  BOOST_TEST(port == worker_port_2);

  // Checking if balancer is up and responding
  boost::thread t(boost::bind(&asio::io_service::run, &io_service));
  JobMessage msg(JobMessage::JobType::BALANCER);
  JobMessage result;
  BOOST_TEST(dist->sendJob(msg, local_ip.c_str(), balancer_port, result));
  BOOST_TEST(result.getJobType() == JobMessage::JobType::SUCCESS);

  // Checking if a balancer can relay a message to a worker and send the result
  // correctly. note we make worker 2, which is not running, the next
  // worker. That should be handled correctly by balancer.
  balancer->updateWorker(asio::ip::address::from_string(local_ip),
                         worker_port_2);
  dist->addCallBack(new HelperCallBack(dist));
  dist->runWorker(local_ip.c_str(), worker_port_1, true);
  msg.setJobType(JobMessage::JobType::ROUTING);
  result.setJobType(JobMessage::JobType::NONE);
  BOOST_TEST(dist->sendJob(msg, local_ip.c_str(), balancer_port, result));
  BOOST_TEST(result.getJobType() == JobMessage::JobType::SUCCESS);

  // Checking broadcast message relaying and handling.
  JobMessage broadcast_msg(JobMessage::JobType::ROUTING,
                           JobMessage::MessageType::BROADCAST);
  result.setJobType(JobMessage::JobType::NONE);
  balancer->addWorker(local_ip, worker_port_3);
  balancer->addWorker(local_ip, worker_port_4);
  BOOST_TEST(
      dist->sendJob(broadcast_msg, local_ip.c_str(), balancer_port, result));
  BOOST_TEST(
      result.getJobType()
      == JobMessage::JobType::ERROR);  // failed to send to more than 2 workers.
  BroadcastJobDescription* desc
      = static_cast<BroadcastJobDescription*>(result.getJobDescription());
  BOOST_TEST(desc->getWorkersCount()
             == 1);  // number of successful broadcasts was 1

  // Now worker 2, 3, and 4 shall have been removed and relaying messages to the
  // up workers shall work correctly.
  dist->runWorker(local_ip.c_str(),
                  worker_port_4,
                  true);  // make worker 4 running for this test
  // For worker 4 to be added correctly, it should have received the saved
  // broadcast messages. The next boost test confirms that.
  BOOST_TEST(balancer->addWorker(
      local_ip, worker_port_4));  // re-add it to the loadbalancer
  result.setJobType(JobMessage::JobType::NONE);
  BOOST_TEST(
      dist->sendJob(broadcast_msg, local_ip.c_str(), balancer_port, result));
  BOOST_TEST(result.getJobType()
             == JobMessage::JobType::SUCCESS);  // No more than 2 workers failed
                                                // to receive the broadcast
  desc = static_cast<BroadcastJobDescription*>(result.getJobDescription());
  BOOST_TEST(desc->getWorkersCount()
             == 2);  // number of successful broadcasts was 2

  // Since we have a histoty of broadcast messages, adding a not running worker
  // shall be invalid as it wouldn't have received the broadcast messages
  // history i.e have invalid state.
  BOOST_TEST(balancer->addWorker(local_ip, worker_port_2) == false);
}
BOOST_AUTO_TEST_SUITE_END()
