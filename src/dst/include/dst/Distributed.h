// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "boost/asio.hpp"

namespace utl {
class Logger;
}

namespace dst {

namespace asio = boost::asio;
using asio::ip::tcp;
using Socket = asio::basic_stream_socket<tcp>;

class JobMessage;
class JobCallBack;
class Worker;

class Distributed
{
 public:
  Distributed(utl::Logger* logger);
  ~Distributed();
  void runWorker(const char* ip, unsigned short port, bool interactive);
  void runLoadBalancer(const char* ip,
                       unsigned short port,
                       const char* workers_domain);
  void addWorkerAddress(const char* address, unsigned short port);
  bool sendJob(JobMessage& msg,
               const char* ip,
               unsigned short port,
               JobMessage& result);
  bool sendJobMultiResult(JobMessage& msg,
                          const char* ip,
                          unsigned short port,
                          JobMessage& result);
  bool sendResult(JobMessage& msg, Socket& sock);
  void addCallBack(JobCallBack* cb);
  const std::vector<JobCallBack*>& getCallBacks() const { return callbacks_; }

 private:
  struct EndPoint
  {
    std::string ip;
    unsigned short port;
    EndPoint(const std::string& ip_in, unsigned short port_in)
        : ip(ip_in), port(port_in)
    {
    }
  };
  utl::Logger* logger_;
  std::vector<EndPoint> end_points_;
  std::vector<JobCallBack*> callbacks_;
  std::vector<std::unique_ptr<Worker>> workers_;
};
}  // namespace dst
