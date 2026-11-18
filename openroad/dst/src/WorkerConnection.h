// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once
#include <cstddef>

#include "boost/asio.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "boost/make_shared.hpp"
#include "dst/JobMessage.h"

namespace utl {
class Logger;
}

namespace dst {

namespace asio = boost::asio;
using asio::ip::tcp;

class Distributed;
class Worker;

class WorkerConnection : public boost::enable_shared_from_this<WorkerConnection>
{
 public:
  WorkerConnection(asio::io_context& service,
                   Distributed* dist,
                   utl::Logger* logger,
                   Worker* worker);
  tcp::socket& socket();
  void start();
  void handle_read(boost::system::error_code const& err,
                   size_t bytes_transferred);
  Worker* getWorker() const { return worker_; }

 private:
  tcp::socket sock_;
  Distributed* dist_;
  asio::streambuf in_packet_;
  utl::Logger* logger_;
  JobMessage msg_;
  Worker* worker_;
};
}  // namespace dst
