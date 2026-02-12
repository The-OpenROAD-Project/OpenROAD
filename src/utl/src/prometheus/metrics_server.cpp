// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "utl/prometheus/metrics_server.h"

#include <chrono>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include "utl/Logger.h"
#include "utl/prometheus/text_serializer.h"

namespace {

std::string SnapshotPrometheusMetrics(utl::PrometheusRegistry* registry)
{
  if (registry) {
    std::stringstream stringstream;
    utl::TextSerializer::Serialize(stringstream, registry->Collect());
    return stringstream.str();
  }

  return "# Registry uninitialized";
}

boost::beast::http::response<boost::beast::http::string_body> HandleRequest(
    boost::beast::http::request<boost::beast::http::string_body>& request,
    boost::asio::ip::tcp::socket& socket,
    utl::PrometheusRegistry* registry)
{
  namespace http = boost::beast::http;

  // Prepare the response message
  http::response<http::string_body> response;
  response.version(request.version());
  response.set(http::field::server, "OpenROAD <3");
  response.set(http::field::content_type, "text/plain");
  if (request.target() == "/metrics") {
    response.result(http::status::ok);
    response.body() = SnapshotPrometheusMetrics(registry);
  } else {
    response.result(http::status::not_found);
    response.body() = "Not Found";
  }
  response.content_length(response.body().size());
  response.prepare_payload();

  return response;
}
}  // namespace

namespace utl {

PrometheusMetricsServer::~PrometheusMetricsServer()
{
  shutdown_ = true;

  // Make a dummy connection to unblock the accept().
  if (is_ready_) {  // Only connect if the server was actually started.
    try {
      boost::asio::io_context
          io_context;  // Use a separate io_context for the connection.
      boost::asio::ip::tcp::socket socket(io_context);
      boost::asio::ip::tcp::endpoint endpoint(
          boost::asio::ip::make_address("127.0.0.1"), port_);
      socket.connect(endpoint);          // This will unblock the accept().
    } catch (const std::exception& e) {  // NOLINT(bugprone-empty-catch)
                                         /*Do nothing, we're dying*/
    }
  }
  worker_thread_.join();
}

void PrometheusMetricsServer::RunServer()
{
  using tcp = boost::asio::ip::tcp;
  namespace http = boost::beast::http;

  boost::asio::io_context io_context;
  boost::asio::ip::tcp::acceptor acceptor(io_context,
                                          {boost::asio::ip::tcp::v4(), port_});
  boost::system::error_code ec;  // Create error_code outside the loop.

  // Set the port in case of user passing 0, which lets the OS choose
  // the port.
  port_ = acceptor.local_endpoint().port();
  is_ready_ = true;

  logger_.load()->info(utl::UTL,
                       104,
                       "Starting Prometheus collection endpoint: "
                       "http://localhost:{}/metrics",
                       port_);

  while (!shutdown_) {
    tcp::socket socket(io_context);
    acceptor.accept(socket, ec);

    if (ec) {
      logger_.load()->warn(
          utl::UTL, 105, "Metrics server accept error: {}", ec.message());
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;  // Skip to the next iteration
    }

    // Read the HTTP request
    boost::beast::flat_buffer buffer;
    http::request<http::string_body> request;
    boost::beast::http::read(socket, buffer, request, ec);

    if (ec) {
      logger_.load()->warn(
          utl::UTL, 106, "Metrics server read error: {}", ec.message());
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;  // Skip to the next iteration
    }

    // Handle the request
    boost::beast::http::response<boost::beast::http::string_body> response
        = HandleRequest(request, socket, registry_ptr_.get());

    // Send the response to the client
    boost::beast::http::write(socket, response, ec);

    // Close the socket
    if (socket.is_open()) {
      socket.shutdown(tcp::socket::shutdown_send, ec);
      if (ec) {
        logger_.load()->warn(utl::UTL, 107, "Shutdown error: {}", ec.message());
      }
      socket.close(ec);
      if (ec) {
        logger_.load()->warn(
            utl::UTL, 108, "Socket close error: {}", ec.message());
      }
    }
  }
}

void PrometheusMetricsServer::WorkerFunction()
{
  while (!shutdown_) {
    try {
      RunServer();
    } catch (const std::exception& e) {
      logger_.load()->warn(
          utl::UTL, 103, "Prometheus Server Exception: {}", e.what());
    }
  }
}

}  // namespace utl
