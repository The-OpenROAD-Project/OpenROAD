#define BOOST_TEST_MODULE TestWorker

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/system_error.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/thread/thread.hpp>
#include <string>

#include "HelperCallBack.h"
#include "Worker.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "utl/Logger.h"

using namespace dst;

BOOST_AUTO_TEST_SUITE(test_suite)

BOOST_AUTO_TEST_CASE(test_default)
{
  Distributed* dist = new Distributed();
  utl::Logger* logger = new utl::Logger();
  std::string local_ip = "127.0.0.1";
  unsigned short port = 1234;

  Worker* worker = new Worker(dist, logger, local_ip.c_str(), port);
  boost::thread t(boost::bind(&Worker::run, worker));

  // Checking if the worker is up and calling callbacks correctly
  dist->addCallBack(new HelperCallBack(dist));
  JobMessage msg(JobMessage::JobType::ROUTING);
  JobMessage result;
  BOOST_TEST(dist->sendJob(msg, local_ip.c_str(), port, result));
  BOOST_TEST(result.getJobType() == JobMessage::JobType::SUCCESS);
}

BOOST_AUTO_TEST_SUITE_END()
