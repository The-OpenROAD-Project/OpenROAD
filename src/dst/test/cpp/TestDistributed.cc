// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2026, The OpenROAD Authors

#include <cstdint>
#include <exception>
#include <stdexcept>
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

// Allocate a free ephemeral TCP port by binding a throwaway acceptor to port 0
// and reading back the OS-assigned port. This replaces the previous hard-coded
// 1235/1236, which made the test collide with any leftover/concurrent process
// on the (shared) host and produce DST-0001/DST-0009 "Address already in use".
// A tiny TOCTOU window remains between closing this acceptor and the real bind;
// it is covered by wait_until_listening() + sendJob()'s connect retries.
uint16_t allocateFreePort(boost::asio::io_context& service)
{
  using boost::asio::ip::tcp;
  tcp::acceptor probe(service);
  probe.open(tcp::v4());
  probe.bind(tcp::endpoint(tcp::v4(), 0));
  const uint16_t port = probe.local_endpoint().port();
  probe.close();
  return port;
}

// Block (bounded) until something is accepting connections on ip:port. This
// closes the load-balancer startup race: runLoadBalancer() binds its acceptor
// on a detached thread, so the test must confirm "listening" before sending a
// job rather than relying on sendJob()'s fixed 250ms connect-retry budget.
bool waitUntilListening(const std::string& ip,
                        uint16_t port,
                        int timeout_ms = 5000)
{
  using boost::asio::ip::tcp;
  const int step_ms = 25;
  for (int waited = 0; waited <= timeout_ms; waited += step_ms) {
    boost::asio::io_context service;
    tcp::socket sock(service);
    boost::system::error_code ec;
    sock.connect(tcp::endpoint(boost::asio::ip::make_address(ip), port), ec);
    if (!ec) {
      sock.close();
      return true;
    }
    boost::this_thread::sleep_for(boost::chrono::milliseconds(step_ms));
  }
  return false;
}

}  // namespace

TEST(test_suite, test_distributed)
{
  try {
    utl::Logger* logger = new utl::Logger();
    Distributed* dist = new Distributed(logger);
    std::string local_ip = "127.0.0.1";

    // Dynamically allocated ports -- no fixed-port collisions on a shared host.
    boost::asio::io_context port_picker;
    const uint16_t worker_port = allocateFreePort(port_picker);
    uint16_t balancer_port = allocateFreePort(port_picker);
    // Ensure the two ports differ even in the unlikely event the OS hands back
    // the same just-freed port twice.
    while (balancer_port == worker_port) {
      balancer_port = allocateFreePort(port_picker);
    }

    dist->addCallBack(new HelperCallBack(dist));
    EXPECT_EQ(dist->getCallBacks().size(), 1);

    // runWorker(interactive=true) binds the worker acceptor synchronously on
    // this thread (only Worker::run() is detached), so the worker is listening
    // as soon as runWorker returns. Confirm anyway before sending.
    dist->addWorkerAddress(local_ip.c_str(), worker_port);
    dist->runWorker(local_ip.c_str(), worker_port, true);
    ASSERT_TRUE(waitUntilListening(local_ip, worker_port))
        << "worker never started listening on port " << worker_port;

    JobMessage msg(JobMessage::JobType::kRouting);
    JobMessage result;
    EXPECT_TRUE(dist->sendJob(msg, local_ip.c_str(), worker_port, result));
    EXPECT_EQ(result.getJobType(), JobMessage::JobType::kSuccess);

    // runLoadBalancer constructs the io_context AND the LoadBalancer (which
    // binds the acceptor) INSIDE this detached thread, so its "listening" state
    // is not synchronized with the test. Poll for readiness before sendJob.
    boost::thread t(boost::bind(&Distributed::runLoadBalancer,
                                dist,
                                local_ip.c_str(),
                                balancer_port,
                                ""));
    ASSERT_TRUE(waitUntilListening(local_ip, balancer_port))
        << "balancer never started listening on port " << balancer_port;

    result.setJobType(JobMessage::JobType::kNone);
    EXPECT_TRUE(dist->sendJob(msg, local_ip.c_str(), balancer_port, result));
    EXPECT_EQ(result.getJobType(), JobMessage::JobType::kSuccess);
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
