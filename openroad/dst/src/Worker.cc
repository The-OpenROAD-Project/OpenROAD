// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "Worker.h"

#include "WorkerConnection.h"
#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"
#include "boost/thread/thread.hpp"
#include "utl/Logger.h"

namespace dst {

namespace ip = asio::ip;

void Worker::start_accept()
{
  auto connection
      = boost::make_shared<WorkerConnection>(service_, dist_, logger_, this);
  acceptor_.async_accept(
      connection->socket(),
      boost::bind(
          &Worker::handle_accept, this, connection, asio::placeholders::error));
}

Worker::Worker(Distributed* dist,
               utl::Logger* logger,
               const char* ip,
               unsigned short port)
    : acceptor_(service_, tcp::endpoint(ip::make_address(ip), port)),
      dist_(dist),
      logger_(logger)
{
  start_accept();
}

Worker::~Worker()
{
  service_.stop();
}
void Worker::run()
{
  service_.run();
}

void Worker::handle_accept(
    const boost::shared_ptr<WorkerConnection>& connection,
    const boost::system::error_code& err)
{
  if (!err) {
    connection->start();
  }
  start_accept();
}
}  // namespace dst
