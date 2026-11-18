#include <exception>
#include <stdexcept>
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
  return message.starts_with("DST-0001");
}

bool isSocketEnvironmentErrorMessage(const std::string& message)
{
  return message.find("Operation not permitted") != std::string::npos
         || message.find("Permission denied") != std::string::npos
         || message.find("DST-0001") != std::string::npos;
}

}  // namespace

TEST(test_suite, test_worker)
{
  try {
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
