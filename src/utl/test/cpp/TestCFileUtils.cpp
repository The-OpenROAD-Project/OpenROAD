// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "utl/CFileUtils.h"
#include "utl/Logger.h"
#include "utl/ScopedTemporaryFile.h"
#include "utl/prometheus/gauge.h"

namespace utl {
using ::testing::HasSubstr;

namespace {

// Helper function to make an HTTP request and return the response body.
std::string MakeHttpRequest(const std::string& host,
                            const std::string& port,
                            const std::string& target)
{
  boost::asio::io_context io_context;
  boost::asio::ip::tcp::resolver resolver(io_context);
  boost::asio::ip::tcp::socket socket(io_context);

  auto const results = resolver.resolve(host, port);
  boost::asio::connect(socket, results.begin(), results.end());

  // HTTP 1.1 request
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::get, target, /*version=*/11};
  req.set(boost::beast::http::field::host, host);
  req.set(boost::beast::http::field::user_agent, "BoostBeastTestClient");

  boost::beast::http::write(socket, req);

  boost::beast::flat_buffer buffer;
  boost::beast::http::response<boost::beast::http::string_body> res;
  boost::beast::http::read(socket, buffer, res);

  socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);

  return res.body();
}

}  // namespace

TEST(Utl, read_all_of_empty_file)
{
  Logger logger;
  ScopedTemporaryFile stf(&logger);
  std::string contents = GetContents(stf.file(), &logger);
  EXPECT_TRUE(contents.empty());
}

// Writes then reads 4B of data.
TEST(Utl, read_all_of_written_file_seek_required)
{
  Logger logger;
  ScopedTemporaryFile stf(&logger);

  const std::vector<uint8_t> kTestData = {0x01, 0x02, 0x03, 0x04};
  WriteAll(stf.file(), kTestData, &logger);

  std::string contents = GetContents(stf.file(), &logger);
  EXPECT_EQ(contents.size(), kTestData.size());
  for (size_t i = 0; i < contents.size(); ++i) {
    EXPECT_EQ(static_cast<uint8_t>(contents.at(i)), kTestData.at(i));
  }
}

// Writes then reads 1024B of data.
TEST(Utl, read_all_of_file_exactly_1024B)
{
  Logger logger;
  ScopedTemporaryFile stf(&logger);

  std::vector<uint8_t> test_data(1024);
  std::iota(test_data.begin(), test_data.end(), 0);

  WriteAll(stf.file(), test_data, &logger);

  std::string contents = GetContents(stf.file(), &logger);
  EXPECT_EQ(contents.size(), test_data.size());
  for (size_t i = 0; i < contents.size(); ++i) {
    EXPECT_EQ(static_cast<uint8_t>(contents.at(i)), test_data.at(i));
  }
}

// Writes then reads 1025B of data (whitebox test, we know internally the read
// buffer size is 1024B so this causes two chunks of read).
TEST(Utl, read_all_of_file_exactly_1025B)
{
  Logger logger;
  ScopedTemporaryFile stf(&logger);

  std::vector<uint8_t> test_data(1025);
  std::iota(test_data.begin(), test_data.end(), 0);

  WriteAll(stf.file(), test_data, &logger);

  std::string contents = GetContents(stf.file(), &logger);
  EXPECT_EQ(contents.size(), test_data.size());
  for (size_t i = 0; i < contents.size(); ++i) {
    EXPECT_EQ(static_cast<uint8_t>(contents.at(i)), test_data.at(i));
  }
}

// Add new tests for OutStreamHandler
TEST(Utl, stream_handler_write_and_read)
{
  const char* filename = "test_write_and_read.txt";
  const std::string kTestData = "\x1\x2\x3\x4";

  {
    OutStreamHandler sh(filename);
    std::ostream& os = sh.getStream();
    os.write(kTestData.c_str(), kTestData.size());
  }

  InStreamHandler ish(filename);
  std::string contents((std::istreambuf_iterator<char>(ish.getStream())),
                       std::istreambuf_iterator<char>());
  EXPECT_EQ(contents, kTestData);
  std::filesystem::remove(filename);
}

TEST(Utl, stream_handler_write_and_read_gzip)
{
  const char* filename = "test_write_and_read.txt.gz";
  const std::string kTestData = "\x1\x2\x3\x4";

  {
    OutStreamHandler sh(filename);
    std::ostream& os = sh.getStream();
    os.write(kTestData.c_str(), kTestData.size());
  }

  InStreamHandler ish(filename);
  std::string contents((std::istreambuf_iterator<char>(ish.getStream())),
                       std::istreambuf_iterator<char>());
  EXPECT_EQ(contents, kTestData);
  std::filesystem::remove(filename);
}

TEST(Utl, stream_handler_temp_file_handling)
{
  const char* filename = "test_temp_file_handling.txt";
  std::string tmp_filename = std::string(filename) + ".1";

  // Check that the temp file is created
  {
    OutStreamHandler sh(filename);
    EXPECT_TRUE(std::filesystem::exists(tmp_filename));
  }

  // Check that the temp file is renamed to the original filename
  EXPECT_TRUE(!std::filesystem::exists(tmp_filename));
  EXPECT_TRUE(std::filesystem::exists(filename));
  std::filesystem::remove(filename);
}

TEST(Utl, stream_handler_exception_handling)
{
  const char* filename = "test_exception_handling.txt";

  // Ensure the temporary file is handled correctly if an exception occurs
  try {
    OutStreamHandler sh(filename);
    throw std::runtime_error("Simulated exception");
  } catch (...) {
    std::string tmp_filename = std::string(filename) + ".1";
    // Ensure temporary file is cleaned up
    EXPECT_TRUE(!std::filesystem::exists(tmp_filename));
    // Original file should exist
    EXPECT_TRUE(std::filesystem::exists(filename));
    std::filesystem::remove(filename);
  }
}

// Add new tests for FileHandler
TEST(Utl, file_handler_write_and_read)
{
  const char* filename = "test_write_and_read_file.txt";
  const std::string kTestData = "\x1\x2\x3\x4";
  {
    FileHandler fh(filename, true);  // binary mode
    FILE* file = fh.getFile();
    fwrite(kTestData.c_str(), sizeof(uint8_t), kTestData.size(), file);
  }

  std::ifstream is(filename, std::ios_base::binary);
  std::string contents((std::istreambuf_iterator<char>(is)),
                       std::istreambuf_iterator<char>());
  EXPECT_EQ(contents, kTestData);
  std::filesystem::remove(filename);
}

TEST(Utl, file_handler_temp_file_handling)
{
  const char* filename = "test_temp_file_handling_file.txt";
  std::string tmp_filename = std::string(filename) + ".1";

  // Check that the temp file is created
  {
    FileHandler fh(filename, true);  // binary mode
    EXPECT_TRUE(std::filesystem::exists(tmp_filename));
  }

  // Check that the temp file is renamed to the original filename
  EXPECT_TRUE(!std::filesystem::exists(tmp_filename));
  EXPECT_TRUE(std::filesystem::exists(filename));
  std::filesystem::remove(filename);
}

TEST(Utl, file_handler_exception_handling)
{
  const char* filename = "test_exception_handling_file.txt";

  // Ensure the temporary file is handled correctly if an exception occurs
  try {
    FileHandler fh(filename, true);  // binary mode
    throw std::runtime_error("Simulated exception");
  } catch (...) {
    std::string tmp_filename = std::string(filename) + ".1";
    // Ensure temporary file is cleaned up
    EXPECT_TRUE(!std::filesystem::exists(tmp_filename));
    // Original file should exist
    EXPECT_TRUE(std::filesystem::exists(filename));
    std::filesystem::remove(filename);
  }
}

TEST(Utl, metrics_server_responds_with_basic_metric)
{
  Logger logger;
  logger.startPrometheusEndpoint(0);
  std::shared_ptr<PrometheusRegistry> registry = logger.getRegistry();
  auto& test_gauge_family = BuildGauge()
                                .Name("test_gauge")
                                .Help("A test gauge for testing")
                                .Register(*registry);
  auto& test_gauge = test_gauge_family.Add({});
  test_gauge.Set(10101);

  std::time_t t = std::time(nullptr);
  while (true) {
    // Timeout after 10 seconds
    if ((std::time(nullptr) - t) > 10) {
      EXPECT_LT((std::time(nullptr) - t), 10);
    }

    if (logger.isPrometheusServerReadyToServe()) {
      break;
    }
  }

  uint16_t port = logger.getPrometheusPort();
  std::string response
      = MakeHttpRequest("localhost", fmt::format("{}", port), "/metrics");
  EXPECT_THAT(response, HasSubstr("10101"));
}

}  // namespace utl
