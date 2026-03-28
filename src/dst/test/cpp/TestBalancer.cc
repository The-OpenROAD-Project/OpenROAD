#include <cstdint>
#include <exception>
#include <stdexcept>
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

namespace {

bool isSocketPermissionError(const boost::system::system_error& error)
{
  return error.code()
             == boost::system::errc::make_error_code(
                 boost::system::errc::operation_not_permitted)
         || error.code()
                == boost::system::errc::make_error_code(
                    boost::system::errc::permission_denied);
}

bool isSocketStartupRuntimeError(const std::runtime_error& error)
{
  const std::string message = error.what();
  return message.starts_with("DST-0001") || message.starts_with("DST-0009");
}

bool isSocketEnvironmentErrorMessage(const std::string& message)
{
  return message.find("Operation not permitted") != std::string::npos
         || message.find("Permission denied") != std::string::npos
         || message.find("DST-0001") != std::string::npos
         || message.find("DST-0009") != std::string::npos;
}

}  // namespace

TEST(test_suite, test_balancer)
{
  try {
    utl::Logger* logger = new utl::Logger();
    Distributed* dist = new Distributed(logger);
    std::string local_ip = "127.0.0.1";
    uint16_t balancer_port = 5555;
    uint16_t worker_port_1 = 5556;
    uint16_t worker_port_2 = 5557;
    uint16_t worker_port_3 = 5558;
    uint16_t worker_port_4 = 5559;
    asio::io_context service;
    LoadBalancer* balancer = new LoadBalancer(
        dist, service, logger, local_ip.c_str(), "", balancer_port);

    // Checking simple interface functions
    balancer->addWorker(local_ip, worker_port_1);
    balancer->addWorker(local_ip, worker_port_2);
    asio::ip::address address;
    uint16_t port;

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

    // Checking if a balancer can relay a message to a worker and send the
    // result correctly. note we make worker 2, which is not running, the next
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
    EXPECT_EQ(result.getJobType(), JobMessage::JobType::kError);
    BroadcastJobDescription* desc
        = static_cast<BroadcastJobDescription*>(result.getJobDescription());
    EXPECT_EQ(desc->getWorkersCount(), 1);

    dist->runWorker(local_ip.c_str(), worker_port_4, true);
    EXPECT_TRUE(balancer->addWorker(local_ip, worker_port_4));
    result.setJobType(JobMessage::JobType::kNone);
    EXPECT_TRUE(
        dist->sendJob(broadcast_msg, local_ip.c_str(), balancer_port, result));
    EXPECT_EQ(result.getJobType(), JobMessage::JobType::kSuccess);
    desc = static_cast<BroadcastJobDescription*>(result.getJobDescription());
    EXPECT_EQ(desc->getWorkersCount(), 2);

    EXPECT_FALSE(balancer->addWorker(local_ip, worker_port_2));
  } catch (const boost::system::system_error& error) {
    if (isSocketPermissionError(error)) {
      GTEST_SKIP() << error.what();
    }
    throw;
  } catch (const std::runtime_error& error) {
    if (isSocketStartupRuntimeError(error)) {
      GTEST_SKIP() << error.what();
    }
    throw;
  } catch (const std::exception& error) {
    if (isSocketEnvironmentErrorMessage(error.what())) {
      GTEST_SKIP() << error.what();
    }
    throw;
  }
}

}  // namespace dst
