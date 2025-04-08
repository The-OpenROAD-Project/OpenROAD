// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>

#include "WorkerConnection.h"

namespace utl {
class Logger;
}

namespace dst {
class Distributed;
class Worker
{
 public:
  // constructor for accepting connection from client
  Worker(Distributed* dist,
         utl::Logger* logger,
         const char* ip,
         unsigned short port);
  void run();
  ~Worker();

 private:
  asio::io_context service_;
  tcp::acceptor acceptor_;
  Distributed* dist_;
  utl::Logger* logger_;
  void start_accept();
  void handle_accept(const boost::shared_ptr<WorkerConnection>& connection,
                     const boost::system::error_code& err);
  friend class WorkerConnection;
};
}  // namespace dst
